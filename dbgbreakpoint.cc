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

#include "dbgbreakpoint.h"

#include <algorithm>
#include <cctype>

using google_cloud_debugger_portable_pdb::DocumentIndex;
using google_cloud_debugger_portable_pdb::LocalConstantRow;
using google_cloud_debugger_portable_pdb::LocalScopeRow;
using google_cloud_debugger_portable_pdb::LocalVariableRow;
using std::string;
using std::vector;

namespace google_cloud_debugger {

void DbgBreakpoint::Initialize(const DbgBreakpoint &other) {
  set_ = other.set_;
  line_ = other.line_;
  column_ = other.column_;
  file_name_ = other.file_name_;
  id_ = other.id_;
  il_offset_ = other.il_offset_;
  method_def_ = other.method_def_;
  method_token_ = other.method_token_;
  method_name_ = other.method_name_;
  local_variables_ = other.local_variables_;
  local_constants_ = other.local_constants_;
  debug_breakpoint_ = other.debug_breakpoint_;
}

void DbgBreakpoint::Initialize(const string &file_name, const string &id,
                               uint32_t line, uint32_t column) {
  file_name_ = file_name;
  id_ = id;
  for (size_t i = 0; i < file_name_.size(); ++i) {
    file_name_[i] = tolower(file_name[i]);
  }
  line_ = line;
  column_ = column;
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
    const google_cloud_debugger_portable_pdb::PortablePdbFile &pdb_file) {
  bool found_breakpoint = false;
  uint32_t current_doc_index_index = -1;
  uint32_t best_match_index = current_doc_index_index;
  size_t best_file_name_location_matched = UINT32_MAX;
  // Loop through all the document and try to find the one that
  // best matches the breakpoint's file name.
  for (auto &&document_index : pdb_file.GetDocumentIndexTable()) {
    ++current_doc_index_index;
    string document_name = document_index.GetFilePath();
    // Normalize path separators. The PDB may use either Unix or Windows-style
    // paths, but the Cloud Debugger only uses Unix.
    std::replace(document_name.begin(), document_name.end(), '\\', '/');
    if (document_name.size() < file_name_.size()) {
      continue;
    }

    for (size_t i = 0; i < document_name.size(); ++i) {
      document_name[i] = tolower(document_name[i]);
    }

    size_t file_name_location = document_name.rfind(file_name_);
    // Has to matched from the end.
    if (file_name_location + file_name_.size() != document_name.size()) {
      continue;
    }

    // Try to find the best match.
    if (file_name_location < best_file_name_location_matched) {
      best_file_name_location_matched = file_name_location;
      best_match_index = current_doc_index_index;
    }
  }

  if (best_match_index == -1) {
    return false;
  }

  vector<google_cloud_debugger_portable_pdb::DocumentIndex> docs =
      pdb_file.GetDocumentIndexTable();
  google_cloud_debugger_portable_pdb::DocumentIndex matched_doc =
      docs[best_match_index];

  // Loop through the methods in the document to see which one the
  // breakpoint falls into.
  for (auto &&method : matched_doc.GetMethods()) {
    if (method.first_line <= line_ && method.last_line >= line_) {
      for (auto &&sequence_point : method.sequence_points) {
        if (sequence_point.start_line <= line_ &&
            sequence_point.end_line >= line_) {
          il_offset_ = sequence_point.il_offset;
          method_def_ = method.method_def;

          // Loop through all scopes of method to retrieve local variables and
          // constants. We are only interested in scopes that encompasses this
          // sequence point.
          for (auto &&local_scope : method.local_scope) {
            if (local_scope.start_offset > il_offset_ ||
                local_scope.start_offset + local_scope.length < il_offset_) {
              continue;
            }

            local_variables_.insert(local_variables_.end(),
                                    local_scope.local_variables.begin(),
                                    local_scope.local_variables.end());

            local_constants_.insert(local_constants_.end(),
                                    local_scope.local_constants.begin(),
                                    local_scope.local_constants.end());
          }

          found_breakpoint = true;
          break;
        }
      }

      if (found_breakpoint) {
        break;
      }
    }
  }

  set_ = found_breakpoint;
  return found_breakpoint;
}

bool EqualIgnoreCase(const std::string &first_string,
                     const std::string &second_string) {
  if (first_string.length() != second_string.length()) {
    return false;
  }

  for (size_t i = 0; i < first_string.length(); ++i) {
    if (tolower(first_string[i]) != tolower(second_string[i])) {
      return false;
    }
  }

  return true;
}

}  // namespace google_cloud_debugger
