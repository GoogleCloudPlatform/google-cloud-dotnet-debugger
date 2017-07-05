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

namespace google_cloud_debugger_portable_pdb {
class PortablePdbFile;

// Index for a single source file described in a Portable PDB. Essentially a
// user-friendly copy of all the data encoded in the PDB's metadata table.
class DocumentIndex {
 public:
  bool Initialize(PortablePdbFile *pdb, int doc_index);

  // Struct that represents a sequence point in a method.
  // Unlike SequencePointRecord struct, this struct has
  // the absoluate IL Offset.
  struct SequencePoint {
    // IL Offset of this sequence point.
    std::uint32_t il_offset;

    // Start line of this sequence point.
    std::uint32_t start_line;

    // Start column of this sequence point.
    std::uint32_t start_col;

    // End line of this sequence point.
    std::uint32_t end_line;

    // End column of this sequence point.
    std::uint32_t end_col;

    // True if this is a hidden or document change sequence point.
    bool is_hidden;
  };

  // Struct that represents a local variable in a method.
  struct Variable {
    // The slot (index) of the variable in the method.
    std::uint16_t slot;

    // Name of the variable.
    std::string name;

    // True if the variable should be hidden from the debugger.
    bool debugger_hidden;
  };

  // Struct that represents constant.
  struct Constant {
    // TODO(chrsmith): Parse constant type and value information.
    std::string name;
  };

  // Struct that represents the local scope of a method.
  // Each local scope will be mapped bijectively to a number of
  // rows in the LocalVariable table of the PDB. That means
  // each row in the LocalVariable table is owned by only 1 scope.
  struct Scope {
    // Index of this scope in the LocalScope table.
    std::uint32_t index;

    // The start row of the local variables owned by this scope.
    std::uint32_t local_var_row_start_index;

    // The end row of the local variables owned by this scope.
    std::uint32_t local_var_row_end_index;

    // The start row of the local constant owned by this scope.
    std::uint32_t local_const_row_start_index;

    // The end row of the local constant owned by this scope.
    std::uint32_t local_const_row_end_index;

    // Start IL offset of this scope.
    std::uint32_t start_offset;

    // The length of this scope (in terms of IL offset).
    std::uint32_t length;

    // Local variables owned by this scope.
    std::vector<Variable> local_variables;

    // Local constants owned by this scope.
    std::vector<Constant> local_constants;
  };

  // Struct that represents a method in a document.
  struct Method {
    // MethodDef for this method.
    std::uint32_t method_def;

    // First line of this method.
    std::uint32_t first_line;

    // Last line of this method.
    std::uint32_t last_line;

    // Vector of sequence points of this method.
    std::vector<SequencePoint> sequence_points;

    // Vector of local scopes of this method.
    std::vector<Scope> local_scope;
  };

  // Returns the file path of this document.
  std::string GetFilePath() const { return file_path_; }

  // Returns all the methods in this document.
  std::vector<Method> &GetMethods() { return methods_; }

 private:
  // The file path of this document.
  std::string file_path_;

  // The source language of this document.
  std::string source_language_;

  // The hash algorithm of this document.
  std::string hash_algorithm_;

  // The hash of this document.
  std::vector<uint8_t> hash_;

  // The methods of this document.
  std::vector<Method> methods_;
};

}  // namespace google_cloud_debugger_portable_pdb

#endif  // DOCUMENT_INDEX_H_
