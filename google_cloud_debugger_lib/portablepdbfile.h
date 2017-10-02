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

#ifndef PORTABLE_PDB_H_
#define PORTABLE_PDB_H_

#include <cstdint>
#include <string>
#include <vector>

#include "i_portablepdbfile.h"

namespace google_cloud_debugger_portable_pdb {

// PortablePDB file. Wraps all the gory details of PE headers and metadata
// compression.
//
// The file format is very information dense, and we expand all of the
// compressed metadata into arrays which are exposed as read only vectors.
// If load time and/or memory pressure become concerns, an alternative
// implemenation could lock the file and generate the necessary structures
// on-demand.
//
// To use this class, create a PortablePdbFile object and calls
// InitializeFromFile with the path to the pdb file.
class PortablePdbFile : public IPortablePdbFile {
 public:
  // Parse the pdb file. This method initializes the class.
  bool InitializeFromFile(const std::string &file_path);

  // Finds the stream header with a given name. Returns false if not found.
  // name is the name of the stream header.
  // stream_header is the stream header that has name name.
  bool GetStream(const std::string &name, StreamHeader *stream_header) const;

  // Get string from the heap at index index.
  bool GetHeapString(std::uint32_t index, std::string *result) const;

  // Retrieves the name of a document using the provided blob heap index.
  // The exact conversion from a blob to document name is in the Portable PDB
  // spec. Returns true if succeeds.
  bool GetDocumentName(std::uint32_t index, std::string *doc_name) const;

  // Gets GUID based on index;
  bool GetHeapGuid(std::uint32_t index, std::string *guid) const;

  // Gets the hash based on index.
  bool GetHash(std::uint32_t index, std::vector<std::uint8_t> *hash) const;

  // Gets the method sequence information based on index.
  bool GetMethodSeqInfo(
      std::uint32_t doc_index, std::uint32_t sequence_index,
      MethodSequencePointInformation *sequence_point_info) const;

  // Returns the document table.
  const std::vector<DocumentRow> &GetDocumentTable() const {
    return document_table_;
  }

  // Returns the local scope table.
  const std::vector<LocalScopeRow> &GetLocalScopeTable() const {
    return local_scope_table_;
  }

  // Returns the local variable table.
  const std::vector<LocalVariableRow> &GetLocalVariableTable() const {
    return local_variable_table_;
  }

  // Returns the method debug info table.
  const std::vector<MethodDebugInformationRow> &GetMethodDebugInfoTable()
      const {
    return method_debug_info_table_;
  }

  // Returns the local constant table.
  const std::vector<LocalConstantRow> &GetLocalConstantTable() const {
    return local_constant_table_;
  }

  // Returns the document index table.
  const std::vector<std::unique_ptr<IDocumentIndex>> &GetDocumentIndexTable()
      const {
    return document_indices_;
  }

  // Sets the name of the module of this PDB.
  void SetModuleName(const std::string &module_name) {
    module_name_ = module_name;
  }

  // Gets the name of the module of this PDB.
  const std::string &GetModuleName() const { return module_name_; }

  // Sets the ICorDebugModule of the module of this PDB.
  HRESULT SetDebugModule(ICorDebugModule *debug_module);

  // Gets the ICorDebugModule of the module of this PDB.
  HRESULT GetDebugModule(ICorDebugModule **debug_module) const;

  // Gets the MetadataImport of the module of this PDB.
  HRESULT GetMetaDataImport(IMetaDataImport **metadata_import) const;

 private:
  // Name of the module that corresponds to this pdb.
  std::string module_name_;

  // Binary Stream contents of the PE file.
  mutable CustomBinaryStream pdb_file_binary_stream_;

  // Not all PDB-specific metadata tables implemented/exposed.
  MetadataRootHeader root_header_;
  std::vector<StreamHeader> stream_headers_;

  StreamHeader string_heap_header_;

  StreamHeader blob_heap_header_;

  StreamHeader guid_heap_header_;

  // Header for the PDB metadata section.
  PortablePdbMetadataSectionHeader pdb_metadata_header_;

  // Header for the compressed metadata table stream.
  CompressedMetadataTableHeader metadata_table_header_;

  // PDB-specific metadata tables. All are 1-indexed, and contain a zeroed out
  // entry at index 0.
  std::vector<DocumentRow> document_table_;
  std::vector<MethodDebugInformationRow> method_debug_info_table_;
  std::vector<LocalScopeRow> local_scope_table_;
  std::vector<LocalVariableRow> local_variable_table_;
  std::vector<LocalConstantRow> local_constant_table_;

  // Vectors that contains all the strings in the Strings Heap.
  // This vector is used to cache the strings.
  mutable std::vector<std::string> heap_strings_;

  // Vector of all document indices inside this pdb.
  std::vector<std::unique_ptr<IDocumentIndex>> document_indices_;

  // The ICorDebugModule of the module of this PDB.
  google_cloud_debugger::CComPtr<ICorDebugModule> debug_module_;

  // The IMetaDataImport of the module of this PDB.
  google_cloud_debugger::CComPtr<IMetaDataImport> metadata_import_;

  // Template function to parse row for a specific metadata table.
  template <typename TableRow>
  bool ParseMetadataTableRow(uint32_t rows_in_table,
                             CustomBinaryStream *binary_stream,
                             std::vector<TableRow> *table) {
    if (!binary_stream || !table) {
      return false;
    }

    // Metadata row indicies are all 1-indexed. So always leave a 0th row that
    // is empty.
    table->resize(rows_in_table + 1);
    for (size_t i = 0; i < rows_in_table; i++) {
      TableRow row;
      if (!ParseFrom(binary_stream, metadata_table_header_, &row)) {
        return false;
      }
      (*table)[i + 1] = std::move(row);
    }

    return true;
  }

  // Parses the Blobs heap.
  bool InitializeBlobHeap();

  // Parses the GUID heap.
  bool InitializeGuidHeap();

  // Parses the strings heap.
  bool InitializeStringsHeap();

  // Parses the PDB metadata section's header.
  bool ParsePortablePdbStream();

  // Parses the compressed metadata tables stream.
  bool ParseCompressedMetadataTableStream();
};

}  // namespace google_cloud_debugger_portable_pdb

#endif
