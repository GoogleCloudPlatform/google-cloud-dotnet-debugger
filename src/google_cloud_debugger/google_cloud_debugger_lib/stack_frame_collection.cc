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

#include "stack_frame_collection.h"

#include <algorithm>
#include <iostream>
#include <string>

#include "dbg_breakpoint.h"
#include "expression_util.h"
#include "i_cor_debug_helper.h"
#include "i_dbg_object_factory.h"
#include "i_eval_coordinator.h"

using google::cloud::diagnostics::debug::Breakpoint;
using google::cloud::diagnostics::debug::SourceLocation;
using google::cloud::diagnostics::debug::StackFrame;
using google_cloud_debugger_portable_pdb::LocalConstantInfo;
using google_cloud_debugger_portable_pdb::LocalVariableInfo;
using google_cloud_debugger_portable_pdb::SequencePoint;
using std::cerr;
using std::cout;
using std::max;
using std::string;
using std::vector;

namespace google_cloud_debugger {
StackFrameCollection::StackFrameCollection(
    std::shared_ptr<ICorDebugHelper> debug_helper,
    std::shared_ptr<IDbgObjectFactory> obj_factory)
    : debug_helper_(debug_helper),
      obj_factory_(obj_factory) {
}

HRESULT StackFrameCollection::ProcessBreakpoint(
    const vector<
        std::shared_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
        &pdb_files,
    DbgBreakpoint *breakpoint, IEvalCoordinator *eval_coordinator) {
  if (!breakpoint) {
    std::cerr << "DbgBreakpoint is null.";
    return E_INVALIDARG;
  }

  if (!eval_coordinator) {
    std::cerr << "Eval coordinator is null.";
    return E_INVALIDARG;
  }

  HRESULT hr;

  // If there are conditions or expressions, handle them first.
  // TODO(quoct): Add expressions handling.
  const std::string &breakpoint_condition = breakpoint->GetCondition();
  if (!breakpoint_condition.empty()) {
    hr = EvaluateBreakpointCondition(breakpoint, eval_coordinator, pdb_files);
    if (FAILED(hr)) {
      breakpoint->WriteError("Failed to evaluate breakpoint condition "
          + breakpoint_condition);
      return hr;
    }

    if (!breakpoint->GetEvaluatedCondition()) {
      return S_FALSE;
    }
  }

  return WalkStackAndProcessStackFrame(eval_coordinator, pdb_files);
}

HRESULT StackFrameCollection::PopulateStackFrames(
    Breakpoint *breakpoint, IEvalCoordinator *eval_coordinator) {
  if (!breakpoint) {
    std::cerr << "Null breakpoint.";
    return E_INVALIDARG;
  }

  if (!eval_coordinator) {
    std::cerr << "Null eval coordinator.";
    return E_INVALIDARG;
  }

  HRESULT hr = S_OK;
  eval_coordinator->WaitForReadySignal();

  // Gives the first frame half available kb in the breakpoint.
  int frame_max_size = (kMaximumBreakpointSize - breakpoint->ByteSize()) / 2;
  int processed_il_frames_so_far = 0;

  for (auto &&dbg_stack_frame : stack_frames_) {
    // If this is the last processed IL frame, just gives it the rest
    // of the size available.
    if (processed_il_frames_so_far == number_of_processed_il_frames_ - 1) {
      frame_max_size = kMaximumBreakpointSize - breakpoint->ByteSize();
    }

    StackFrame *frame = breakpoint->add_stack_frames();
    // If dbg_stack_frame is an empty stack frame, just says it's undebuggable.
    if (dbg_stack_frame->IsEmpty()) {
      frame->set_method_name("Undebuggable code.");
      continue;
    }

    frame->set_method_name(dbg_stack_frame->GetShortModuleName() + "!" +
                           dbg_stack_frame->GetClass() + "." +
                           dbg_stack_frame->GetMethod());
    SourceLocation *frame_location = frame->mutable_location();
    if (!frame_location) {
      std::cerr << "Mutable location returns null.";
      continue;
    }

    frame_location->set_line(dbg_stack_frame->GetLineNumber());
    frame_location->set_path(dbg_stack_frame->GetFile());

    hr = dbg_stack_frame->PopulateStackFrame(frame, frame_max_size,
                                             eval_coordinator);
    if (FAILED(hr)) {
      return hr;
    }

    if (dbg_stack_frame->IsProcessedIlFrame()) {
      ++processed_il_frames_so_far;
    }

    if (breakpoint->ByteSize() > kMaximumBreakpointSize) {
      break;
    }

    // Updates frame_max_size to half of whatever is left.
    frame_max_size = (kMaximumBreakpointSize - breakpoint->ByteSize()) / 2;
  }

  return S_OK;
}

HRESULT StackFrameCollection::PopulateLocalVarsAndMethodArgs(
    mdMethodDef target_function_token, DbgStackFrame *dbg_stack_frame,
    ICorDebugILFrame *il_frame, IMetaDataImport *metadata_import,
    google_cloud_debugger_portable_pdb::IPortablePdbFile *pdb_file) {
  if (!dbg_stack_frame || !il_frame || !pdb_file) {
    return E_INVALIDARG;
  }

  HRESULT hr;
  // Retrieves the IP offset in the function that corresponds to this stack
  // frame.
  ULONG32 ip_offset;
  CorDebugMappingResult mapping_result;
  hr = il_frame->GetIP(&ip_offset, &mapping_result);
  if (FAILED(hr)) {
    cerr << "Failed to get instruction pointer offset from ICorDebugFrame.";
    return hr;
  }

  // Can't show this stack frame as the mapping is not good.
  if (mapping_result == CorDebugMappingResult::MAPPING_NO_INFO ||
      mapping_result == CorDebugMappingResult::MAPPING_UNMAPPED_ADDRESS) {
    return S_FALSE;
  }

  // Loops through all methods in all the documents of the pdb file to find
  // a MethodInfo object that corresponds with the method at this breakpoint.
  for (auto &&document_index : pdb_file->GetDocumentIndexTable()) {
    for (auto &&method : document_index->GetMethods()) {
      PCCOR_SIGNATURE current_method_signature = 0;
      ULONG current_method_virtual_addr = 0;
      mdTypeDef type_def = 0;
      ULONG method_name_length = 0;
      DWORD flags1 = 0;
      ULONG signature_blob;
      DWORD flags2 = 0;

      hr = metadata_import->GetMethodProps(
          method.method_def, &type_def, nullptr, 0, &method_name_length,
          &flags1, &current_method_signature, &signature_blob,
          &current_method_virtual_addr, &flags2);
      if (FAILED(hr)) {
        cerr << "Failed to extract method info from method "
             << method.method_def;
        return hr;
      }

      // Checks that the virtual address of this method matches the one of the
      // stack frame.
      if (current_method_virtual_addr !=
          dbg_stack_frame->GetFuncVirtualAddr()) {
        continue;
      }

      // Sets the file path since we know we are in the correct function.
      dbg_stack_frame->SetFile(document_index->GetFilePath());

      long matching_sequence_point_position = -1;

      // We find the first non-hidden sequence point that is just larger than
      // the ip offset.
      for (long index = 0; index < method.sequence_points.size(); index++) {
        if (!method.sequence_points[index].is_hidden &&
            method.sequence_points[index].il_offset <= ip_offset) {
          matching_sequence_point_position =
              max(matching_sequence_point_position, index);
        }
      }

      // If we find the matching sequence point, populates the list of local
      // variables in dbg_stack_frame from the local variable's vector of the
      // matching sequence point.
      if (matching_sequence_point_position != -1) {
        SequencePoint sequence_point =
            method.sequence_points[matching_sequence_point_position];

        dbg_stack_frame->SetLineNumber(sequence_point.start_line);
        vector<LocalVariableInfo> local_variables;
        for (auto &&local_scope : method.local_scope) {
          if (local_scope.start_offset > sequence_point.il_offset ||
              local_scope.start_offset + local_scope.length <
                  sequence_point.il_offset) {
            continue;
          }

          local_variables.insert(local_variables.end(),
                                 local_scope.local_variables.begin(),
                                 local_scope.local_variables.end());
        }

        hr = dbg_stack_frame->Initialize(
            il_frame, local_variables, target_function_token, metadata_import);
      }

      return S_OK;
    }
  }

  return S_OK;
}

HRESULT StackFrameCollection::PopulateModuleClassAndFunctionName(
    DbgStackFrame *dbg_stack_frame, mdMethodDef function_token,
    IMetaDataImport *metadata_import) {
  if (!dbg_stack_frame || !metadata_import) {
    return E_INVALIDARG;
  }

  HRESULT hr;
  mdTypeDef type_def = 0;
  ULONG method_name_length = 0;
  DWORD flags1 = 0;
  ULONG signature_blob = 0;
  ULONG target_method_virtual_addr = 0;
  DWORD flags2 = 0;
  PCCOR_SIGNATURE target_method_signature = 0;

  // Retrieves the length of the name of the method that this stack frame
  // is at.
  hr = metadata_import->GetMethodProps(
      function_token, &type_def, nullptr, 0, &method_name_length, &flags1,
      &target_method_signature, &signature_blob, &target_method_virtual_addr,
      &flags2);

  if (FAILED(hr)) {
    cerr << "Failed to get length of name of method for stack frame.";
    return hr;
  }

  std::vector<WCHAR> method_name(method_name_length, 0);

  // Retrieves the name of the method that this stack frame is at.
  hr = metadata_import->GetMethodProps(
      function_token, &type_def, method_name.data(), method_name.size(),
      &method_name_length, &flags1, &target_method_signature, &signature_blob,
      &target_method_virtual_addr, &flags2);
  if (FAILED(hr)) {
    cerr << "Failed to get name of method for stack frame.";
    return hr;
  }

  // Retrieves the class name.
  // TODO(quoct): Cache the class name in MethodInfo method.
  mdToken extends_token;
  DWORD class_flags;
  ULONG class_name_length;
  hr = metadata_import->GetTypeDefProps(
      type_def, nullptr, 0, &class_name_length, &class_flags, &extends_token);
  if (FAILED(hr)) {
    cerr << "Failed to get length of name of class type for stack frame.";
    return hr;
  }

  std::vector<WCHAR> class_name(class_name_length, 0);
  hr = metadata_import->GetTypeDefProps(type_def, class_name.data(),
                                        class_name.size(), &class_name_length,
                                        &class_flags, &extends_token);
  if (FAILED(hr)) {
    cerr << "Failed to get name of class type for stack frame.";
    return hr;
  }

  // Even if we cannot get variables, we should still report
  // method and class name of this frame.
  dbg_stack_frame->SetMethod(method_name);
  dbg_stack_frame->SetClass(class_name);
  dbg_stack_frame->SetFuncVirtualAddr(target_method_virtual_addr);

  return S_OK;
}

HRESULT StackFrameCollection::WalkStackAndProcessStackFrame(
    IEvalCoordinator *eval_coordinator,
    const std::vector<
        std::shared_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
        &parsed_pdb_files) {
  if (stack_walked) {
    return S_OK;
  }

  CComPtr<ICorDebugStackWalk> debug_stack_walk;
  CComPtr<ICorDebugFrame> frame;
  int il_frame_parsed_so_far = 0;
  int frame_parsed_so_far = 0;
  HRESULT hr = eval_coordinator->CreateStackWalk(&debug_stack_walk);
  if (FAILED(hr)) {
    cerr << "Failed to create stack walk.";
    return hr;
  }

  // Skips the first stack if it is already processed.
  if (first_stack_) {
    stack_frames_.push_back(first_stack_);
    ++frame_parsed_so_far;
    if (first_stack_->IsProcessedIlFrame()) {
      ++il_frame_parsed_so_far;
    }
    hr = debug_stack_walk->Next();
  }

  // Walks through the stack and populates stack_frames_ vector.
  while (SUCCEEDED(hr)) {
    // Don't parse too many stack frames.
    if (frame_parsed_so_far >= kMaximumStackFrames) {
      stack_walked = true;
      return S_OK;
    }

    hr = debug_stack_walk->GetFrame(&frame);
    // No more stacks.
    if (hr == S_FALSE) {
      stack_walked = true;
      return S_OK;
    }

    if (FAILED(hr)) {
      cerr << "Failed to get active frame.";
      return hr;
    }

    // Do not process too many IL frames to minimize breakpoint size.
    bool process_il_frame =
        il_frame_parsed_so_far < kMaximumStackFramesWithVariables;

    std::shared_ptr<DbgStackFrame> stack_frame(
        new DbgStackFrame(debug_helper_, obj_factory_));
    hr = PopulateDbgStackFrameHelper(parsed_pdb_files, frame, stack_frame.get(),
                                     process_il_frame);
    if (FAILED(hr)) {
      cerr << "Failed to process stack frame.";
      return hr;
    }

    ++frame_parsed_so_far;
    if (stack_frame->IsProcessedIlFrame()) {
      ++il_frame_parsed_so_far;
    }

    stack_frames_.push_back(std::move(stack_frame));
    hr = debug_stack_walk->Next();
  }

  if (FAILED(hr)) {
    cerr << "Failed to get stack frame's information.";
  }

  stack_walked = true;
  return hr;
}

HRESULT StackFrameCollection::EvaluateBreakpointCondition(
    DbgBreakpoint *breakpoint, IEvalCoordinator *eval_coordinator,
    const std::vector<
        std::shared_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
        &parsed_pdb_files) {
  CComPtr<ICorDebugThread> debug_thread;
  HRESULT hr = eval_coordinator->GetActiveDebugThread(&debug_thread);
  if (FAILED(hr)) {
    std::cerr << "Failed to get active thread.";
    return hr;
  }

  CComPtr<ICorDebugFrame> debug_frame;
  hr = debug_thread->GetActiveFrame(&debug_frame);
  if (FAILED(hr)) {
    std::cerr << "Failed to get active frame.";
    return hr;
  }

  if (!first_stack_) {
    first_stack_ = std::shared_ptr<DbgStackFrame>(
        new DbgStackFrame(debug_helper_, obj_factory_));
    hr = PopulateDbgStackFrameHelper(parsed_pdb_files, debug_frame,
                                     first_stack_.get(), true);
    if (FAILED(hr)) {
      std::cerr << "Failed to process stack frame.";
      return hr;
    }
  }

  if (first_stack_->IsEmpty() || !first_stack_->IsProcessedIlFrame()) {
    std::cerr << "Conditional breakpoint and expressions are not "
              << "supported on non-IL frame.";
    return E_NOTIMPL;
  }

  return breakpoint->EvaluateCondition(first_stack_.get(), eval_coordinator,
                                       obj_factory_.get());
}

HRESULT StackFrameCollection::PopulateDbgStackFrameHelper(
    const std::vector<
        std::shared_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
        &parsed_pdb_files,
    ICorDebugFrame *debug_frame, DbgStackFrame *stack_frame,
    bool process_il_frame) {
  // Gets ICorDebugFunction that corresponds to the function at this frame.
  // We delay the logic to query the IL frame until we have to get the
  // variables and method arguments.
  CComPtr<ICorDebugFunction> frame_function;
  HRESULT hr = debug_frame->GetFunction(&frame_function);
  // This means the debug function is not available (mostly because of
  // native code) so we skip to the next frame.
  if (hr == CORDBG_E_CODE_NOT_AVAILABLE) {
    // Adds an empty stack frame.
    stack_frame->SetEmpty(true);
    return S_OK;
  }

  if (FAILED(hr)) {
    cerr << "Failed to get ICorDebugFunction from IL Frame.";
    return hr;
  }

  // Gets the token of the function above.
  mdMethodDef target_function_token;
  hr = frame_function->GetToken(&target_function_token);
  if (FAILED(hr)) {
    cerr << "Failed to extract token from debug function.";
    return hr;
  }

  // Gets the ICorDebugModule of the module at this frame.
  CComPtr<ICorDebugModule> frame_module;
  hr = frame_function->GetModule(&frame_module);
  if (FAILED(hr)) {
    cerr << "Failed to get ICorDebugModule from ICorDebugFunction.";
    return hr;
  }

  vector<WCHAR> module_name;
  hr = debug_helper_->GetModuleNameFromICorDebugModule(frame_module,
                                                       &module_name, &cerr);
  if (FAILED(hr)) {
    return hr;
  }

  stack_frame->SetModuleName(module_name);
  string target_module_name = stack_frame->GetModule();

  CComPtr<IMetaDataImport> metadata_import;
  hr = debug_helper_->GetMetadataImportFromICorDebugModule(
      frame_module, &metadata_import, &cerr);
  if (FAILED(hr)) {
    return hr;
  }

  // Populates the module, class and function name of this stack frame
  // so we can report this even if we don't have local variables or
  // method arguments.
  hr = PopulateModuleClassAndFunctionName(stack_frame, target_function_token,
                                          metadata_import);
  if (FAILED(hr)) {
    return hr;
  }

  if (!process_il_frame) {
    return S_OK;
  }

  CComPtr<ICorDebugILFrame> il_frame;
  hr = debug_frame->QueryInterface(__uuidof(ICorDebugILFrame),
                                   reinterpret_cast<void **>(&il_frame));
  // If this is a non-IL frame, we cannot get local variables
  // and method arguments so simply skip that step.
  if (hr == E_NOINTERFACE) {
    return S_OK;
  }

  if (FAILED(hr)) {
    cerr << "Failed to get ILFrame";
    return hr;
  }

  for (auto &&pdb_file : parsed_pdb_files) {
    // Gets the PDB file that has the same name as the module.
    CComPtr<ICorDebugModule> pdb_debug_module;
    // TODO(quoct): Possible performance improvement by caching the pdb_file
    // based on token.
    string pdb_module_name = pdb_file->GetModuleName();
    if (pdb_module_name.compare(target_module_name) != 0) {
      continue;
    }

    // Tries to populate local variables and method arguments of this frame.
    hr = PopulateLocalVarsAndMethodArgs(target_function_token, stack_frame,
                                        il_frame, metadata_import,
                                        pdb_file.get());
    if (FAILED(hr)) {
      cerr << "Failed to populate stack frame information.";
      return hr;
    }

    return S_OK;
  }

  return S_FALSE;
}

}  //  namespace google_cloud_debugger
