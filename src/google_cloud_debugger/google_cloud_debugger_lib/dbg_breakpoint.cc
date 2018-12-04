// Copyright 2017 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "dbg_breakpoint.h"

#include <algorithm>
#include <cctype>
#include <queue>

#include "compiler_helpers.h"
#include "dbg_class_property.h"
#include "document_index.h"
#include "expression_evaluator.h"
#include "expression_util.h"
#include "i_dbg_stack_frame.h"
#include "i_eval_coordinator.h"
#include "i_portable_pdb_file.h"
#include "i_stack_frame_collection.h"
#include "variable_wrapper.h"

using google::cloud::diagnostics::debug::Breakpoint;
using google::cloud::diagnostics::debug::SourceLocation;
using google::cloud::diagnostics::debug::Variable;
using google_cloud_debugger_portable_pdb::DocumentIndex;
using google_cloud_debugger_portable_pdb::LocalConstantRow;
using google_cloud_debugger_portable_pdb::LocalScopeRow;
using google_cloud_debugger_portable_pdb::LocalVariableRow;
using google::cloud::diagnostics::debug::Breakpoint_LogLevel;
using std::string;
using std::unique_ptr;
using std::vector;

namespace google_cloud_debugger {

std::int32_t DbgBreakpoint::current_max_collection_size_ =
    DbgBreakpoint::kMaximumCollectionSize;

void DbgBreakpoint::Initialize(const DbgBreakpoint &other) {
  Initialize(other.file_path_, other.id_, other.line_, other.column_,
             other.log_point_, other.log_message_format_, other.log_level_,
             other.condition_, other.expressions_);
}

void DbgBreakpoint::Initialize(const string &file_path, const string &id,
                               uint32_t line, uint32_t column,
                               const bool &log_point,
                               const std::string &log_message_format,
                               const Breakpoint_LogLevel &log_level,
                               const std::string &condition,
                               const std::vector<std::string> &expressions) {
  file_path_ = file_path;
  std::transform(
      file_path_.begin(), file_path_.end(), file_path_.begin(),
      [](unsigned char c) -> unsigned char { return std::tolower(c); });
  file_path_segments_ = SplitFilePath(file_path_);

  id_ = id;
  log_point_ = log_point;
  line_ = line;
  column_ = column;
  condition_ = condition;
  expressions_ = expressions;
  log_message_format_ = log_message_format;
  log_level_ = log_level;
}

HRESULT DbgBreakpoint::GetCorDebugBreakpoint(
    ICorDebugBreakpoint **debug_breakpoint) const {
  if (!debug_breakpoint) {
    return E_INVALIDARG;
  }

  if (debug_breakpoint_) {
    *debug_breakpoint = debug_breakpoint_;
    debug_breakpoint_->AddRef();
    return S_OK;
  }

  return E_FAIL;
}

bool DbgBreakpoint::TrySetBreakpoint(
    google_cloud_debugger_portable_pdb::IPortablePdbFile *pdb_file) {
  if (!pdb_file) {
    return false;
  }

  int32_t current_doc_index_index = -1;
  int32_t best_match_doc_index = -1;
  int32_t longest_match = -1;

  // Loop through all the documents and try to find the one that
  // best matches the breakpoint's file name.
  for (auto &&document_index : pdb_file->GetDocumentIndexTable()) {
    ++current_doc_index_index;
    string document_name = document_index->GetFilePath();
    // Normalize path separators. The PDB may use either Unix or Windows-style
    // paths, but the Cloud Debugger only uses Unix.
    std::replace(document_name.begin(), document_name.end(), '\\', '/');
    std::transform(
        document_name.begin(), document_name.end(), document_name.begin(),
        [](unsigned char c) -> unsigned char { return std::tolower(c); });

    std::vector<std::string> document_name_segments = SplitFilePath(document_name);

    // Loop and see how many segments matches.
    int segments_matches = 0;
    int document_segment_idx = 0;
    for (auto &&segment : file_path_segments_) {
      if (document_segment_idx == document_name_segments.size()) {
        break;
      }

      if (segment.compare(document_name_segments[document_segment_idx]) == 0) {
        segments_matches += 1;
      }

      document_segment_idx += 1;
    }

    if (segments_matches > 0 && segments_matches > longest_match) {
      best_match_doc_index = current_doc_index_index;
      longest_match = segments_matches;
    }
  }

  if (best_match_doc_index == -1) {
    return false;
  }

  auto &&best_document_index =
      pdb_file->GetDocumentIndexTable()[best_match_doc_index];
  // Try to find the best matched method.
  // This is because the breakpoint can be inside method A but if
  // method A is defined inside method B then we should use method A
  // to get the local variables instead of method B. An example is a
  // delegate function that is defined inside a normal function.
  bool found_breakpoint = false;
  uint32_t best_matched_method_first_line = 0;
  for (auto &&method : best_document_index->GetMethods()) {
    if (method.first_line > line_ || method.last_line < line_) {
      continue;
    }

    // If this method's first line is greater than the previous one,
    // this means that this method is inside it.
    if (method.first_line > best_matched_method_first_line) {
      // If this is false, it means no sequence points in the method
      // corresponds to this breakpoint.
      if (TrySetBreakpointInMethod(method)) {
        best_matched_method_first_line = method.first_line;
        found_breakpoint = true;
      }
    }
  }

  return found_breakpoint;
}

HRESULT DbgBreakpoint::EvaluateExpressions(IDbgStackFrame *stack_frame,
                                           IEvalCoordinator *eval_coordinator,
                                           IDbgObjectFactory *obj_factory) {
  for (auto &expression : expressions_) {
    CompiledExpression compiled_expression = CompileExpression(expression);
    if (compiled_expression.evaluator == nullptr) {
      WriteError("Failed to compile expression: " + expression);
      return E_FAIL;
    }

    // When we call compiled_expression.evaluator->Evaluate below,
    // this may affect variables in the frame.
    // Because of that, we gets a fresh active frame for each iteration.
    CComPtr<ICorDebugILFrame> active_frame;
    HRESULT hr = eval_coordinator->GetActiveDebugFrame(&active_frame);
    if (FAILED(hr)) {
      WriteError("Failed to evaluate expression: " + expression + ".");
      return hr;
    }

    hr = compiled_expression.evaluator->Compile(stack_frame, active_frame,
                                                GetErrorStream());
    if (FAILED(hr)) {
      WriteError("Failed to evaluate expression: " + expression + ".");
      return hr;
    }

    std::shared_ptr<DbgObject> expression_obj;
    hr = compiled_expression.evaluator->Evaluate(
        &expression_obj, eval_coordinator, obj_factory, GetErrorStream());
    if (FAILED(hr)) {
      WriteError("Failed to evaluate expression: " + expression + ".");
      return hr;
    }

    expressions_map_[expression] = expression_obj;
  }

  return S_OK;
}

HRESULT DbgBreakpoint::EvaluateCondition(IDbgStackFrame *stack_frame,
                                         IEvalCoordinator *eval_coordinator,
                                         IDbgObjectFactory *obj_factory) {
  if (condition_.empty()) {
    evaluated_condition_ = true;
    return S_OK;
  }

  CompiledExpression compiled_expression = CompileExpression(condition_);
  if (compiled_expression.evaluator == nullptr) {
    // TODO(quoct): Get the error from CompileExpression.
    return E_FAIL;
  }

  CComPtr<ICorDebugILFrame> active_frame;
  HRESULT hr = eval_coordinator->GetActiveDebugFrame(&active_frame);
  if (FAILED(hr)) {
    return hr;
  }

  hr = compiled_expression.evaluator->Compile(stack_frame, active_frame,
                                              GetErrorStream());
  if (FAILED(hr)) {
    return hr;
  }

  const TypeSignature &type_sig =
      compiled_expression.evaluator->GetStaticType();
  if (type_sig.cor_type != CorElementType::ELEMENT_TYPE_BOOLEAN) {
    WriteError("Condition of the breakpoint must be of type boolean.");
    return E_FAIL;
  }

  std::shared_ptr<DbgObject> condition_result;
  hr = compiled_expression.evaluator->Evaluate(
      &condition_result, eval_coordinator, obj_factory, GetErrorStream());
  if (FAILED(hr)) {
    return hr;
  }

  return NumericCompilerHelper::ExtractPrimitiveValue<bool>(
      condition_result.get(), &evaluated_condition_);
}

HRESULT DbgBreakpoint::PopulateBreakpoint(Breakpoint *breakpoint,
                                          IStackFrameCollection *stack_frames,
                                          IEvalCoordinator *eval_coordinator) {
  if (!breakpoint) {
    std::cerr << "Breakpoint proto is null";
    return E_INVALIDARG;
  }

  breakpoint->set_id(id_);
  breakpoint->set_log_point(log_point_);
  breakpoint->set_log_message_format(log_message_format_);
  breakpoint->set_log_level(log_level_);

  if (!stack_frames) {
    std::cerr << "Stack frame collection is null.";
    return E_INVALIDARG;
  }

  if (!eval_coordinator) {
    std::cerr << "Eval coordinator is null.";
    return E_INVALIDARG;
  }

  SourceLocation *location = breakpoint->mutable_location();
  if (!location) {
    std::cerr << "Mutable location returns null.";
    return E_FAIL;
  }

  location->set_line(line_);
  location->set_path(file_path_);

  eval_coordinator->WaitForReadySignal();

  if (!expressions_map_.empty()) {
    HRESULT hr = PopulateExpression(breakpoint, eval_coordinator);
    if (FAILED(hr)) {
      return hr;
    }
  }

  return stack_frames->PopulateStackFrames(breakpoint, eval_coordinator);
}

HRESULT DbgBreakpoint::PopulateExpression(Breakpoint *breakpoint,
                                          IEvalCoordinator *eval_coordinator) {
  std::queue<VariableWrapper> bfs_queue;

  for (auto &&kvp : expressions_map_) {
    Variable *expression_proto = breakpoint->add_evaluated_expressions();

    expression_proto->set_name(kvp.first);
    const std::shared_ptr<DbgObject> &expression_value = kvp.second;
    if (!expression_value) {
      continue;
    }

    bfs_queue.push(VariableWrapper(expression_proto, expression_value));
  }

  if (bfs_queue.size() != 0) {
    current_max_collection_size_ = kMaximumCollectionExpressionSize;
    HRESULT hr = VariableWrapper::PerformBFS(
        &bfs_queue,
        [breakpoint]() {
          return breakpoint->ByteSize() > DbgBreakpoint::kMaximumBreakpointSize;
        },
        eval_coordinator);
    current_max_collection_size_ = kMaximumCollectionSize;
    return hr;
  }

  return S_OK;
}

bool DbgBreakpoint::TrySetBreakpointInMethod(
    const google_cloud_debugger_portable_pdb::MethodInfo &method) {
  const auto &find_seq = std::find_if(
      method.sequence_points.begin(), method.sequence_points.end(),
      [&](const google_cloud_debugger_portable_pdb::SequencePoint &seq) {
        return !seq.is_hidden && seq.start_line >= line_;
      });

  if (find_seq == method.sequence_points.end()) {
    return false;
  }

  il_offset_ = find_seq->il_offset;
  line_ = find_seq->start_line;
  method_def_ = method.method_def;
  return true;
}

std::vector<std::string> DbgBreakpoint::SplitFilePath(const std::string &path) {
  std::vector<std::string> result;

  static const std::string delimiter = "/";
  size_t delimiter_position = path.find(delimiter);
  size_t start_offset = 0;

  while (delimiter_position != std::string::npos) {
    result.push_back(path.substr(start_offset, delimiter_position - start_offset));
    start_offset = delimiter_position + delimiter.length();
    delimiter_position = path.find(delimiter, start_offset);
  }

  result.push_back(path.substr(start_offset, delimiter_position));
  std::reverse(result.begin(), result.end());
  return result;
}

}  // namespace google_cloud_debugger
