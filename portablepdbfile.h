// Copyright 2017 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#ifndef PORTABLE_PDB_H_
#define PORTABLE_PDB_H_

#include <cstdint>
#include <string>
#include <vector>
#include "custombinaryreader.h"
#include "documentindex.h"
#include "metadataheaders.h"
#include "metadatatables.h"
#include "cor.h"
#include "cordebug.h"
#include "ccomptr.h"

namespace google_cloud_debugger_portable_pdb {
// PortablePDB file. Wraps all the gory details of PE headers and metadata
// compression.
//
// The file format is very information dense, and we expand all of the
// compressed metadata into arrays which are exposed as read only lists. If load
// time and/or memory pressure become concerns, an alternative implemenation
// could lock the file and generate the necessary structures on-demand.
class PortablePdbFile {
 public:
  // Finds the stream header with a given name. Returns false if not found.
  bool GetStream(const std::string &name, StreamHeader *stream_header) const;

  // Parses the strings heap.
  bool InitializeStringsHeap();

  // Get string from the heap at index index.
  std::string GetHeapString(uint32_t index) const;

  // Parse the pdb file.
  bool ParsePdbFile(const std::string &file_path);

  // Parses the Blobs heap.
  bool InitializeBlobHeap();

  // Get data from the blob heap.
  bool GetHeapBlobStream(uint32_t index, CustomBinaryStream *binary_stream);

  // Returns the name of a document using the provided blob heap index.
  // The exact conversion from a blob to document name is in the Portable PDB
  // spec.
  std::string GetDocumentName(uint32_t index);

  // Parses the GUID heap.
  bool InitializeGuidHeap();

  // Gets GUID based on index;
  std::string GetHeapGuid(uint32_t index) const;

  // Parses the PDB metadata section's header.
  bool ParsePortablePdbStream();

  // Parses the compressed metadata tables stream.
  bool ParseCompressedMetadataTableStream();

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
  const std::vector<MethodDebugInformationRow>
    &GetMethodDebugInfoTable() const {
    return method_debug_info_table_;
  }

  // Returns the local constant table.
  const std::vector<LocalConstantRow> &GetLocalConstantTable() const {
    return local_constant_table_;
  }

  // Returns the document index table.
  const std::vector<DocumentIndex> &GetDocumentIndexTable() const {
    return document_indices_;
  }

  // Sets the name of the module of this PDB.
  void SetModuleName(const std::string &module_name) {
    module_name_ = module_name;
  }

  // Gets the name of the module of this PDB.
  std::string GetModuleName() const { return module_name_; }

  // Sets the ICorDebugModule of the module of this PDB.
  HRESULT SetDebugModule(ICorDebugModule *debug_module);

  // Gets the ICorDebugModule of the module of this PDB.
  HRESULT GetDebugModule(ICorDebugModule **debug_module);

  // Gets the MetadataImport of the module of this PDB.
  HRESULT GetMetaDataImport(IMetaDataImport **metadata_import);

 private:
  // Name of the module that corresponds to this pdb.
  std::string module_name_;

  // Binary Stream contents of the PE file.
  CustomBinaryStream pdb_file_binary_stream_;

  // Not all PDB-specific metadata tables implemented/exposed.
  MetadataRootHeader root_header_;
  std::vector<StreamHeader> stream_headers_;

  // Header for the PDB metadata section.
  PortablePdbMetadataSectionHeader pdb_metadata_header_;

  // Header for the compressed metadata table stream.
  CompressedMetadataTableHeader metadata_table_header_;

  // Byte array of the Strings heap.
  std::vector<uint8_t> string_heap_data_;

  // Byte array of the Blob heap.
  std::vector<uint8_t> blob_heap_data_;

  // Byte array of the GUID heap.
  std::vector<std::string> guids_heap_data_;

  // PDB-specific metadata tables. All are 1-indexed, and contain a zeroed out
  // entry at index 0.
  std::vector<DocumentRow> document_table_;
  std::vector<MethodDebugInformationRow> method_debug_info_table_;
  std::vector<LocalScopeRow> local_scope_table_;
  std::vector<LocalVariableRow> local_variable_table_;
  std::vector<LocalConstantRow> local_constant_table_;

  // Vector of all document indices inside this pdb.
  std::vector<DocumentIndex> document_indices_;

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
};

}  // namespace google_cloud_debugger_portable_pdb

#endif
