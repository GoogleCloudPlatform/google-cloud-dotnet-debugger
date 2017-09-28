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

#include "portablepdbfile.h"

#include <assert.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <memory>

#include "custombinaryreader.h"
#include "metadataheaders.h"
#include "metadatatables.h"

using google_cloud_debugger::CComPtr;
using std::array;
using std::string;
using std::streampos;
using std::unique_ptr;
using std::vector;

namespace google_cloud_debugger_portable_pdb {

bool PortablePdbFile::GetStream(const string &name,
                                StreamHeader *stream_header) const {
  assert(stream_header != nullptr);

  const auto &result =
      std::find_if(stream_headers_.begin(), stream_headers_.end(),
                   [&name](const StreamHeader &stream) {
                     return name.compare(stream.name) == 0;
                   });

  if (result == stream_headers_.end()) {
    return false;
  }

  *stream_header = *result;

  return true;
}

bool PortablePdbFile::InitializeStringsHeap() {
  static const string kStringsHeapName = "#Strings";
  return GetStream(kStringsHeapName, &string_heap_header_);
}

bool PortablePdbFile::GetHeapString(uint32_t index, string *heap_string) const {
  return pdb_file_binary_stream_.GetString(heap_string, string_heap_header_.offset);
}

bool PortablePdbFile::InitializeFromFile(const string &file_path) {
  if (!pdb_file_binary_stream_.ConsumeFile(file_path)) {
    return false;
  }

  if (!ParseFrom(&pdb_file_binary_stream_, &root_header_)) {
    return false;
  }

  stream_headers_.reserve(root_header_.number_streams);
  for (size_t i = 0; i < root_header_.number_streams; ++i) {
    StreamHeader stream_header;
    if (!ParseFrom(&pdb_file_binary_stream_, &stream_header)) {
      return false;
    }

    stream_headers_.push_back(std::move(stream_header));
  }

  if (!InitializeBlobHeap() || !InitializeStringsHeap() ||
      !InitializeGuidHeap()) {
    return false;
  }

  if (!ParseCompressedMetadataTableStream() || !ParsePortablePdbStream()) {
    return false;
  }

  if (document_table_.size() > 1) {
    document_indices_.reserve(document_table_.size() - 1);
    for (size_t i = 1; i < document_table_.size(); ++i) {
      unique_ptr<DocumentIndex> document_index(new (std::nothrow)
                                                   DocumentIndex());
      if (!document_index || !document_index->Initialize(*this, i)) {
        return false;
      }
      document_indices_.push_back(std::move(document_index));
    }
  }

  return true;
}

bool PortablePdbFile::InitializeBlobHeap() {
  static const string kBlobHeapName = "#Blob";
  return GetStream(kBlobHeapName, &blob_heap_header_);
}

bool PortablePdbFile::GetDocumentName(uint32_t index, string *doc_name) const {
  if (index == 0) {
    return false;
  }
  
  streampos current_pos = pdb_file_binary_stream_.current();
  pdb_file_binary_stream_.SeekFromOrigin(blob_heap_header_.offset + index);
  uint32_t data_length;
  if (!pdb_file_binary_stream_.ReadCompressedUInt32(&data_length)) {
    return false;
  }

  pdb_file_binary_stream_.SetStreamLength(data_length);

  uint8_t separator;
  if (!pdb_file_binary_stream_.ReadByte(&separator)) {
    return false;
  }

  string result;
  while (pdb_file_binary_stream_.HasNext()) {
    uint32_t part_index;
    if (!pdb_file_binary_stream_.ReadCompressedUInt32(&part_index)) {
      return false;
    }

    // 0 means empty string.
    if (part_index != 0) {
      pdb_file_binary_stream_.SeekFromOrigin(blob_heap_header_.offset + part_index);
      uint32_t part_data_length;
      if (!pdb_file_binary_stream_.ReadCompressedUInt32(&part_data_length)) {
        return false;
      }

      uint32_t bytes_read;
      vector<uint8_t> part_string(part_data_length, 0);
      if (!pdb_file_binary_stream_.ReadBytes(part_string.data(), part_data_length, &bytes_read)) {
        return false;
      }

      result.append(part_string.begin(), part_string.end());
    }

    result += separator;
  }

  result.pop_back();
  *doc_name = result;

  return true;
}

bool PortablePdbFile::GetHeapGuid(uint32_t index, string *guid) const {
  // GUID are 16 bytes. Index is 1-based so we have to minus 1.
  uint32_t offset = (index - 1) * 16;

  if (!pdb_file_binary_stream_.SeekFromOrigin(guid_heap_header_.offset + offset)) {
    return false;
  }

  guid->clear();
  guid->resize(16);
  vector<uint8_t> bytes_read(16, 0);
  uint32_t num_bytes_read;

  return !pdb_file_binary_stream_.ReadBytes(bytes_read.data(), bytes_read.size(),
    &num_bytes_read);
}

bool PortablePdbFile::GetHash(uint32_t index, vector<uint8_t> *hash) const {
  if (!pdb_file_binary_stream_.SeekFromOrigin(blob_heap_header_.offset + index)) {
    return false;
  }

  uint32_t data_length;
  if (!pdb_file_binary_stream_.ReadCompressedUInt32(&data_length)) {
    return false;
  }

  hash->resize(data_length);
  uint32_t bytes_read = 0;
  if (!pdb_file_binary_stream_.ReadBytes(hash->data(), hash->size(), &bytes_read)) {
    std::cerr << "Failed to read the hash from the heap blob stream.";
    return false;
  }

  return true;
}

bool PortablePdbFile::GetMethodSeqInfo(uint32_t doc_index, uint32_t sequence_index, MethodSequencePointInformation *sequence_point_info) const {
  if (!pdb_file_binary_stream_.SeekFromOrigin(guid_heap_header_.offset + sequence_index)) {
    return false;
  }

  return ParseFrom(doc_index, &pdb_file_binary_stream_, sequence_point_info);
}

bool PortablePdbFile::InitializeGuidHeap() {
  static const string kGuidHeapName = "#GUID";

  return GetStream(kGuidHeapName, &guid_heap_header_);
}

bool PortablePdbFile::ParsePortablePdbStream() {
  StreamHeader pdb_stream_header;
  if (!GetStream("#Pdb", &pdb_stream_header)) {
    return false;
  }

  if (!pdb_file_binary_stream_.SeekFromOrigin(pdb_stream_header.offset) ||
      !pdb_file_binary_stream_.SetStreamLength(pdb_stream_header.size)) {
    return false;
  }

  return ParseFrom(&pdb_file_binary_stream_, &pdb_metadata_header_);
}

bool PortablePdbFile::ParseCompressedMetadataTableStream() {
  static const string kCompressedStreamName = "#~";
  // NOTE: The sizes of references to type system tables are determined using
  // the algorithm described in ECMA -335-II Chapter 24.2.6, except their
  // respective row counts are found in TypeSystemTableRows field of the #Pdb
  // stream.
  StreamHeader compressed_stream_header;
  if (!GetStream(kCompressedStreamName, &compressed_stream_header)) {
    return false;
  }
  
  if (!pdb_file_binary_stream_.SeekFromOrigin(
          compressed_stream_header.offset) ||
      !pdb_file_binary_stream_.SetStreamLength(compressed_stream_header.size)) {
    return false;
  }

  if (!ParseFrom(&pdb_file_binary_stream_, &metadata_table_header_)) {
    return false;
  }

  // Build a mapping of metadata table to the number of rows it contains.
  array<uint32_t, MetadataTable::MaxValue> rows_per_table;
  size_t referenced_table = 0;
  for (size_t table_index = 0; table_index < rows_per_table.size();
       ++table_index) {
    if (metadata_table_header_.valid_mask[table_index]) {
      rows_per_table[table_index] =
          metadata_table_header_.num_rows[referenced_table];
      ++referenced_table;
    } else {
      rows_per_table[table_index] = 0;
    }
  }

  // Confirm the PDB only contains PDB-related metadata tables.
  for (size_t i = 0; i < rows_per_table[MetadataTable::Document]; i++) {
    if (rows_per_table[i] != 0) {
      return false;
    }
  }

  if (!ParseMetadataTableRow<DocumentRow>(
          rows_per_table[MetadataTable::Document], &pdb_file_binary_stream_,
          &document_table_)) {
    return false;
  }

  if (!ParseMetadataTableRow<MethodDebugInformationRow>(
          rows_per_table[MetadataTable::MethodDebugInformation],
          &pdb_file_binary_stream_, &method_debug_info_table_)) {
    return false;
  }

  if (!ParseMetadataTableRow<LocalScopeRow>(
          rows_per_table[MetadataTable::LocalScope], &pdb_file_binary_stream_,
          &local_scope_table_)) {
    return false;
  }

  if (!ParseMetadataTableRow<LocalVariableRow>(
          rows_per_table[MetadataTable::LocalVariable],
          &pdb_file_binary_stream_, &local_variable_table_)) {
    return false;
  }

  if (!ParseMetadataTableRow<LocalConstantRow>(
          rows_per_table[MetadataTable::LocalConstant],
          &pdb_file_binary_stream_, &local_constant_table_)) {
    return false;
  }

  return true;
}

HRESULT PortablePdbFile::SetDebugModule(ICorDebugModule *debug_module) {
  if (!debug_module) {
    return E_INVALIDARG;
  }

  HRESULT hr;
  CComPtr<IUnknown> temp_import;
  hr = debug_module->GetMetaDataInterface(IID_IMetaDataImport, &temp_import);
  if (FAILED(hr)) {
    return hr;
  }

  hr = temp_import->QueryInterface(
      IID_IMetaDataImport, reinterpret_cast<void **>(&metadata_import_));
  if (FAILED(hr)) {
    return hr;
  }

  debug_module_ = debug_module;
  return S_OK;
}

HRESULT PortablePdbFile::GetDebugModule(ICorDebugModule **debug_module) const {
  if (!debug_module) {
    return E_INVALIDARG;
  }

  if (debug_module_) {
    *debug_module = debug_module_;
    debug_module_->AddRef();
    return S_OK;
  }
  return E_FAIL;
}

HRESULT PortablePdbFile::GetMetaDataImport(
    IMetaDataImport **metadata_import) const {
  if (!metadata_import) {
    return E_INVALIDARG;
  }

  if (metadata_import_) {
    (*metadata_import) = metadata_import_;
    metadata_import_->AddRef();
    return S_OK;
  }

  return E_FAIL;
}

}  // namespace google_cloud_debugger_portable_pdb
