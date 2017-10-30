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

#include "document_index.h"

#include <assert.h>
#include <algorithm>
#include <iostream>

#include "custom_binary_reader.h"
#include "i_portable_pdb_file.h"

using std::cerr;
using std::max;
using std::min;
using std::string;
using std::vector;

namespace google_cloud_debugger_portable_pdb {

bool DocumentIndex::Initialize(const IPortablePdbFile &pdb, int doc_index) {
  if (doc_index == 0) {
    cerr << "Document index has to be larger than 0.";
    return false;
  }

  const vector<DocumentRow> &document_table = pdb.GetDocumentTable();
  if (document_table.size() <= doc_index) {
    cerr << "Document index " << std::to_string(doc_index)
         << " is larger than the Document Table size.";
    return false;
  }

  const DocumentRow &doc_row = document_table[doc_index];

  if (!pdb.GetDocumentName(doc_row.name, &file_path_)) {
    cerr << "Failed to get document name for file " << file_path_ << std::endl;
    return false;
  }

  // See:
  // https://github.com/dotnet/corefx/blob/master/src/System.Reflection.Metadata/specs/PortablePdb-Metadata.md#document-table-0x30
  // for the various GUIDs.
  string language_guid;
  if (!pdb.GetHeapGuid(doc_row.language, &language_guid)) {
    cerr << "Failed to get language GUID.";
    return false;
  }

  source_language_ = GetLanguageName(language_guid);

  string hash_guid;
  if (!pdb.GetHeapGuid(doc_row.hash_algorithm, &hash_guid)) {
    cerr << "Failed to get hash GUID.";
    return false;
  }

  hash_algorithm_ = GetHashAlgorithmName(hash_guid);

  if (!pdb.GetHash(doc_row.hash, &hash_)) {
    cerr << "Failed to get heap blob stream.";
    return false;
  }

  // We rely on the 1:1 mapping between the Method and MethodDebugInfo tables.
  vector<MethodDebugInformationRow> method_debug_info_rows =
      pdb.GetMethodDebugInfoTable();
  size_t num_of_methods = method_debug_info_rows.size();
  methods_.reserve(num_of_methods);

  for (size_t method_def = 1; method_def < num_of_methods; ++method_def) {
    MethodDebugInformationRow debug_info_row =
        method_debug_info_rows[method_def];
    // Pedantically we are ignoring methods that span multiple files.
    if (debug_info_row.document != doc_index) {
      continue;
    }

    MethodInfo method;
    if (!ParseMethod(&method, pdb, debug_info_row, method_def, doc_index)) {
      cerr << "Failed to parse the method " << std::to_string(method_def)
           << " in document " << std::to_string(doc_index);
      return false;
    }

    methods_.push_back(std::move(method));
  }

  return true;
}

bool DocumentIndex::ParseMethod(MethodInfo *method, const IPortablePdbFile &pdb,
                                const MethodDebugInformationRow &debug_info_row,
                                uint32_t method_def, uint32_t doc_index) {
  assert(method != nullptr);

  method->method_def = method_def;
  method->first_line = UINT32_MAX;
  method->last_line = 0;

  MethodSequencePointInformation sequence_point_info;
  if (!pdb.GetMethodSeqInfo(doc_index, debug_info_row.sequence_points,
                            &sequence_point_info)) {
    cerr << "Failed to get Sequence Point Info from MethodDebugInfo row.";
    return false;
  }

  uint32_t il_offset = 0;
  method->sequence_points.reserve(sequence_point_info.records.size());

  for (const auto &seq_point_record : sequence_point_info.records) {
    if (IsDocumentChange(seq_point_record)) {
      return false;
    }

    il_offset += seq_point_record.il_delta;

    SequencePoint seq_point;
    seq_point.is_hidden =
        IsHidden(seq_point_record) || IsDocumentChange(seq_point_record);
    seq_point.start_line = seq_point_record.start_line;
    seq_point.end_line = seq_point_record.end_line;
    seq_point.start_col = seq_point_record.start_col;
    seq_point.end_col = seq_point_record.end_col;
    seq_point.il_offset = il_offset;

    if (!seq_point.is_hidden) {
      method->first_line = min(seq_point_record.start_line, method->first_line);
      method->last_line = max(seq_point_record.end_line, method->last_line);
    }

    method->sequence_points.push_back(std::move(seq_point));
  }

  bool first_scope = true;
  const vector<LocalScopeRow> &local_scope_table = pdb.GetLocalScopeTable();
  const vector<LocalVariableRow> &local_variable_table =
      pdb.GetLocalVariableTable();
  const vector<LocalConstantRow> &local_constant_table =
      pdb.GetLocalConstantTable();
  for (size_t index = 1; index < local_scope_table.size(); ++index) {
    const LocalScopeRow &local_scope_row = local_scope_table[index];
    if (local_scope_row.method_def != method_def) {
      continue;
    }

    Scope local_scope;
    if (!ParseScope(&local_scope, pdb, local_scope_row, local_scope_table,
                    local_variable_table, local_constant_table, method_def,
                    index)) {
      cerr << "Failed to parse local scope at index " << std::to_string(index);
    }

    method->local_scope.push_back(std::move(local_scope));
  }

  return true;
}

bool DocumentIndex::ParseScope(
    Scope *local_scope, const IPortablePdbFile &pdb,
    const LocalScopeRow &local_scope_row,
    const std::vector<LocalScopeRow> &local_scope_table,
    const std::vector<LocalVariableRow> &local_variable_table,
    const std::vector<LocalConstantRow> &local_constant_table,
    uint32_t method_def, uint32_t scope_index) {
  if (scope_index >= local_scope_table.size()) {
    cerr << "Scope index is out of range for Local Scope table.";
    return false;
  }

  local_scope->local_var_row_start_index = local_scope_row.variable_list;
  local_scope->local_var_row_end_index = local_variable_table.size();
  local_scope->local_const_row_start_index = local_scope_row.constant_list;
  local_scope->local_const_row_end_index = local_constant_table.size();
  local_scope->start_offset = local_scope_row.start_offset;
  local_scope->length = local_scope_row.length;
  local_scope->index = scope_index;

  if (scope_index + 1 < local_scope_table.size()) {
    // The run of local variables owned by this scope continues to the
    // smaller of :
    //  - The last row of the LocalVariable table
    //  - The next run of LocalVariables, found by inspecting the
    //  VariableList of the next row in this LocalScope table.
    // Note that the next scope does not have to have the same method!
    const LocalScopeRow &next_scope_row = local_scope_table[scope_index + 1];
    local_scope->local_var_row_end_index =
        min(local_scope->local_var_row_end_index, next_scope_row.variable_list);
    local_scope->local_const_row_end_index = min(
        local_scope->local_const_row_end_index, next_scope_row.constant_list);
  }

  if (local_scope->local_var_row_end_index <
          local_scope->local_var_row_start_index ||
      local_scope->local_var_row_end_index > local_variable_table.size()) {
    cerr << "Local variable row indices are out of range.";
    return false;
  }

  for (size_t var_idx = local_scope->local_var_row_start_index;
       var_idx < local_scope->local_var_row_end_index; ++var_idx) {
    const LocalVariableRow &local_variable_row = local_variable_table[var_idx];
    LocalVariableInfo new_variable;
    new_variable.debugger_hidden =
        (local_variable_row.attributes == kDebuggerHidden);
    new_variable.slot = local_variable_row.index;
    if (!pdb.GetHeapString(local_variable_row.name, &new_variable.name)) {
      return false;
    }

    local_scope->local_variables.push_back(std::move(new_variable));
  }

  if ((local_scope->local_const_row_end_index <
       local_scope->local_const_row_start_index) ||
      (local_scope->local_const_row_end_index > local_constant_table.size())) {
    cerr << "Local variable row indices are out of range.";
    return false;
  }

  // Local constants.
  for (size_t const_idx = local_scope->local_const_row_start_index;
       const_idx < local_scope->local_const_row_end_index; ++const_idx) {
    const LocalConstantRow &local_constant_row =
        local_constant_table[const_idx];
    LocalConstantInfo new_const;
    if (!pdb.GetHeapString(local_constant_row.name, &new_const.name)) {
      return false;
    }

    local_scope->local_constants.push_back(std::move(new_const));
  }

  return true;
}

}  // namespace google_cloud_debugger_portable_pdb
