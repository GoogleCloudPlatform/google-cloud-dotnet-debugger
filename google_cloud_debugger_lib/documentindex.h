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

#ifndef DOCUMENT_INDEX_H_
#define DOCUMENT_INDEX_H_

#include <cstdint>
#include <string>
#include <vector>

#include "metadatatables.h"

namespace google_cloud_debugger_portable_pdb {
class IPortablePdbFile;

// Struct that represents a sequence point in a method.
// Unlike SequencePointRecord struct, this struct has
// the absoluate IL Offset.
struct SequencePoint {
  // IL Offset of this sequence point.
  std::uint32_t il_offset = 0;

  // Start line of this sequence point.
  std::uint32_t start_line = 0;

  // Start column of this sequence point.
  std::uint32_t start_col = 0;

  // End line of this sequence point.
  std::uint32_t end_line = 0;

  // End column of this sequence point.
  std::uint32_t end_col = 0;

  // True if this is a hidden or document change sequence point.
  bool is_hidden = false;
};

// Struct that represents a local variable in a method.
struct LocalVariableInfo {
  // The slot (index) of the variable in the method.
  std::uint16_t slot = 0;

  // Name of the variable.
  std::string name;

  // True if the variable should be hidden from the debugger.
  bool debugger_hidden = false;
};

// Struct that represents constant in a method.
struct LocalConstantInfo {
  // TODO(quoct): Parse constant type and value information.
  std::string name;
};

// Struct that represents the local scope of a method.
// Each local scope will be mapped bijectively to a number of
// rows in the LocalVariable table of the PDB. That means
// each row in the LocalVariable table is owned by only 1 scope.
struct Scope {
  // Index of this scope in the LocalScope table.
  std::uint32_t index = 0;

  // The start row of the local variables owned by this scope.
  std::uint32_t local_var_row_start_index = 0;

  // The end row of the local variables owned by this scope.
  std::uint32_t local_var_row_end_index = 0;

  // The start row of the local constant owned by this scope.
  std::uint32_t local_const_row_start_index = 0;

  // The end row of the local constant owned by this scope.
  std::uint32_t local_const_row_end_index = 0;

  // Start IL offset of this scope.
  std::uint32_t start_offset = 0;

  // The length of this scope (in terms of IL offset).
  std::uint32_t length = 0;

  // Local variables owned by this scope.
  std::vector<LocalVariableInfo> local_variables;

  // Local constants owned by this scope.
  std::vector<LocalConstantInfo> local_constants;
};

// Struct that represents a method in a document.
struct MethodInfo {
  // MethodDef for this method.
  std::uint32_t method_def = 0;

  // First line of this method.
  std::uint32_t first_line = 0;

  // Last line of this method.
  std::uint32_t last_line = 0;

  // Vector of sequence points of this method.
  std::vector<SequencePoint> sequence_points;

  // Vector of local scopes of this method.
  std::vector<Scope> local_scope;
};

// Index for a single source file described in a Portable PDB. Essentially a
// user-friendly copy of all the data encoded in the PDB's metadata table.
//
// A document index is initialized by calling the Initialize method.
class IDocumentIndex {
 public:
  // Destructor.
  virtual ~IDocumentIndex() = default;

  // Initialize this document index to the document at index doc_index
  // in the DocumentTable of the Portable PDB file pdb.
  virtual bool Initialize(const IPortablePdbFile &pdb, int doc_index) = 0;

  // Returns the file path of this document.
  virtual const std::string &GetFilePath() const = 0;

  // Returns all the methods in this document.
  virtual const std::vector<MethodInfo> &GetMethods() const = 0;
};

// Implementation of IDocumentIndex interface.
class DocumentIndex : public IDocumentIndex {
 public:
  // Initialize this document index to the document at index doc_index
  // in the DocumentTable of the Portable PDB file pdb.
  bool Initialize(const IPortablePdbFile &pdb, int doc_index);

  // Returns the file path of this document.
  const std::string &GetFilePath() const { return file_path_; }

  // Returns all the methods in this document.
  const std::vector<MethodInfo> &GetMethods() const { return methods_; }

 private:
  // Populate a method object that corresponds to MethodDebugInformationRow
  // debug_info_row. This function assumes that the method only spans
  // 1 document.
  bool ParseMethod(MethodInfo *method, const IPortablePdbFile &pdb,
                   const MethodDebugInformationRow &debug_info_row,
                   std::uint32_t method_def, std::uint32_t doc_index);

  // Returns a Scope object that corresponds with LocalScopeRow
  // local_scope_row. The Scope object will have its variable
  // and constant tables filled up with variables and constants that
  // belong to the scope.
  bool ParseScope(Scope *scope, const IPortablePdbFile &pdb,
                  const LocalScopeRow &local_scope_row,
                  const std::vector<LocalScopeRow> &local_scope_table,
                  const std::vector<LocalVariableRow> &local_variable_table,
                  const std::vector<LocalConstantRow> &local_constant_table,
                  std::uint32_t method_def, std::uint32_t scope_index);

  // The file path of this document.
  std::string file_path_;

  // The source language of this document.
  std::string source_language_;

  // The hash algorithm of this document.
  std::string hash_algorithm_;

  // The hash of this document.
  std::vector<uint8_t> hash_;

  // The methods of this document.
  std::vector<MethodInfo> methods_;
};

}  // namespace google_cloud_debugger_portable_pdb

#endif  // DOCUMENT_INDEX_H_
