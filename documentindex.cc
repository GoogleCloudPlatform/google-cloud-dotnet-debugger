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

#include "documentindex.h"

#include <algorithm>

#include "custombinaryreader.h"
#include "metadatatables.h"
#include "portablepdbfile.h"

namespace google_cloud_debugger_portable_pdb {
using std::min;
using std::max;
using std::vector;

bool DocumentIndex::Initialize(PortablePdbFile *pdb, int doc_index) {
  if (doc_index == 0) {
    return false;
  }

  vector<DocumentRow> document_table = pdb->GetDocumentTable();
  DocumentRow doc_row = document_table[doc_index];

  if (!pdb->GetDocumentName(doc_row.name, &file_path_)) {
    return false;
  }

  source_language_ =
    GetLanguageName(pdb->GetHeapGuid(doc_row.language));
  hash_algorithm_ =
    GetHashAlgorithmName(pdb->GetHeapGuid(doc_row.hash_algorithm));

  CustomBinaryStream binary_stream;

  if (!pdb->GetHeapBlobStream(doc_row.hash, &binary_stream)) {
    return false;
  }

  size_t stream_len = binary_stream.GetRemainingStreamLength();
  hash_.resize(stream_len);
  uint32_t bytes_read = 0;
  if (!binary_stream.ReadBytes(hash_.data(), hash_.size(), &bytes_read)) {
    return false;
  }

  // We rely on the 1:1 mapping between the Method and MethodDebugInfo tables.
  vector<MethodDebugInformationRow> method_debug_info_rows =
    pdb->GetMethodDebugInfoTable();
  size_t num_of_methods = method_debug_info_rows.size();
  methods_.reserve(num_of_methods);

  for (size_t method_def = 1; method_def < num_of_methods; ++method_def) {
    MethodDebugInformationRow debug_info_row =
      method_debug_info_rows[method_def];
    if (debug_info_row.document != doc_index) {
      // Pedantically we are ignoring methods that span multiple files.
      continue;
    }

    Method method;
    method.method_def = method_def;
    method.first_line = UINT32_MAX;
    method.last_line = 0;

    CustomBinaryStream sequence_point_blob_stream;
    if (!pdb->GetHeapBlobStream(debug_info_row.sequence_points,
      &sequence_point_blob_stream)) {
      return false;
    }

    MethodSequencePointInformation sequence_point_info;
    if (!ParseFrom(doc_index, &sequence_point_blob_stream, &sequence_point_info)) {
      return false;
    }

    uint32_t il_offset = 0;
    method.sequence_points.reserve(sequence_point_info.records.size());

    for (auto &&seq_point_record: sequence_point_info.records) {
      if (IsDocumentChange(seq_point_record)) {
        return false;
      }

      il_offset += seq_point_record.il_delta;

      SequencePoint seq_point;
      seq_point.is_hidden = IsHidden(seq_point_record) ||
        IsDocumentChange(seq_point_record);
      seq_point.start_line = seq_point_record.start_line;
      seq_point.end_line = seq_point_record.end_line;
      seq_point.start_col = seq_point_record.start_col;
      seq_point.end_col = seq_point_record.end_col;
      seq_point.il_offset = il_offset;

      if (!seq_point.is_hidden) {
        method.first_line =
          min(seq_point_record.start_line, method.first_line);
        method.last_line =
          max(seq_point_record.end_line, method.last_line);
      }

      method.sequence_points.push_back(std::move(seq_point));
    }

    bool first_scope = true;
    vector<LocalScopeRow> local_scope_table = pdb->GetLocalScopeTable();
    vector<LocalVariableRow> local_variable_table =
      pdb->GetLocalVariableTable();
    vector<LocalConstantRow> local_constant_table =
      pdb->GetLocalConstantTable();
    for (size_t index = 1; index < local_scope_table.size(); ++index) {
      LocalScopeRow local_scope_row = local_scope_table[index];
      if (local_scope_row.method_def != method_def) continue;

      Scope local_scope;
      local_scope.local_var_row_start_index = local_scope_row.variable_list;
      local_scope.local_var_row_end_index = local_variable_table.size();
      local_scope.local_const_row_start_index = local_scope_row.constant_list;
      local_scope.local_const_row_end_index = local_constant_table.size();
      local_scope.start_offset = local_scope_row.start_offset;
      local_scope.length = local_scope_row.length;
      local_scope.index = index;

      if (index + 1 < local_scope_table.size()) {
        // The run of local variables owned by this scope continues to the
        // smaller of :
        //  - The last row of the LocalVariable table
        //  - The next run of LocalVariables, found by inspecting the
        //  VariableList of the next row in this LocalScope table.
        // Note that the next scope does not have to have the same method!
        LocalScopeRow next_scope_row = local_scope_table[index + 1];
        local_scope.local_var_row_end_index =
          min(local_scope.local_var_row_end_index,
            next_scope_row.variable_list);
        local_scope.local_const_row_end_index =
          min(local_scope.local_const_row_end_index,
            next_scope_row.constant_list);
      }

      for (size_t var_idx = local_scope.local_var_row_start_index;
        var_idx < local_scope.local_var_row_end_index; ++var_idx) {
        LocalVariableRow local_variable_row = local_variable_table[var_idx];
        Variable new_variable;
        new_variable.debugger_hidden = (local_variable_row.attributes ==
          kDebuggerHidden);
        new_variable.slot = local_variable_row.index;
        new_variable.name = pdb->GetHeapString(local_variable_row.name);

        local_scope.local_variables.push_back(std::move(new_variable));
      }

      // Local constants.
      for (size_t const_idx = local_scope.local_const_row_start_index;
        const_idx < local_scope.local_const_row_end_index; ++const_idx) {
        LocalConstantRow local_constant_row = local_constant_table[const_idx];
        Constant new_const;
        new_const.name = pdb->GetHeapString(local_constant_row.name);

        local_scope.local_constants.push_back(std::move(new_const));
      }

      method.local_scope.push_back(std::move(local_scope));
    }

    methods_.push_back(std::move(method));
  }

  return true;
}

}  // namespace google_cloud_debugger_portable_pdb
