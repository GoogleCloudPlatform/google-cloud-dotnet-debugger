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

#include "custombinaryreader.h"
#include "metadataheaders.h"
#include "metadatatables.h"

namespace google_cloud_debugger_portable_pdb {

using google_cloud_debugger::CComPtr;
using std::array;
using std::string;
using std::vector;

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

  StreamHeader string_heap;
  if (!GetStream(kStringsHeapName, &string_heap)) {
    return false;
  }

  string_heap_data_.insert(
      string_heap_data_.begin(),
      pdb_file_binary_stream_.begin() + string_heap.offset,
      pdb_file_binary_stream_.begin() + string_heap.offset + string_heap.size);

  heap_strings_.resize(string_heap.size + 1);

  return true;
}

const string &PortablePdbFile::GetHeapString(uint32_t index) const {
  if (heap_strings_[index].empty()) {
    size_t str_len = 0;
    while (string_heap_data_[index + str_len] != 0) {
      ++str_len;
    }

    heap_strings_[index] = string(string_heap_data_.begin() + index,
                                  string_heap_data_.begin() + index + str_len);
  }

  return heap_strings_[index];
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
      DocumentIndex document_index;
      if (!document_index.Initialize(*this, i)) {
        return false;
      }
      document_indices_.push_back(std::move(document_index));
    }
  }

  return true;
}

bool PortablePdbFile::InitializeBlobHeap() {
  static const string kBlobHeapName = "#Blob";

  StreamHeader blob_heap;
  if (!GetStream(kBlobHeapName, &blob_heap)) {
    return false;
  }

  blob_heap_data_.insert(
      blob_heap_data_.begin(),
      pdb_file_binary_stream_.begin() + blob_heap.offset,
      pdb_file_binary_stream_.begin() + blob_heap.offset + blob_heap.size);

  return true;
}

bool PortablePdbFile::GetHeapBlobStream(
    uint32_t index, CustomBinaryStream *binary_stream) const {
  assert(binary_stream != nullptr);

  binary_stream->ConsumeVector(blob_heap_data_);
  if (!binary_stream->SeekFromCurrent(index)) {
    return false;
  }

  uint32_t data_length;
  if (!binary_stream->ReadCompressedUInt32(&data_length)) {
    return false;
  }

  if (!binary_stream->SetStreamLength(data_length)) {
    return false;
  }

  return true;
}

bool PortablePdbFile::GetDocumentName(uint32_t index, string *doc_name) const {
  if (index == 0) {
    return false;
  }

  CustomBinaryStream binary_stream;
  if (!GetHeapBlobStream(index, &binary_stream)) {
    return false;
  }

  uint8_t separator;
  if (!binary_stream.ReadByte(&separator)) {
    return false;
  }

  string result;
  while (binary_stream.HasNext()) {
    uint32_t part_index;
    if (!binary_stream.ReadCompressedUInt32(&part_index)) {
      return false;
    }

    // 0 means empty string.
    if (part_index != 0) {
      CustomBinaryStream part_binary_stream;
      if (GetHeapBlobStream(part_index, &part_binary_stream)) {
        result.append(part_binary_stream.Current(), part_binary_stream.End());
      }
    }

    result += separator;
  }

  result.pop_back();
  *doc_name = result;

  return true;
}

bool PortablePdbFile::InitializeGuidHeap() {
  static const string kGuidHeapName = "#GUID";

  // GUIDs, like most metadata tables, are 1-indexed. So insert a placeholder
  // for slot 0.
  guids_heap_data_.push_back(string());

  StreamHeader guid_stream_header;
  if (!GetStream(kGuidHeapName, &guid_stream_header)) {
    return false;
  }

  if (!pdb_file_binary_stream_.SeekFromOrigin(guid_stream_header.offset) ||
      !pdb_file_binary_stream_.SetStreamLength(guid_stream_header.size)) {
    return false;
  }

  while (pdb_file_binary_stream_.HasNext()) {
    vector<uint8_t> bytes_read(16, 0);
    uint32_t num_bytes_read = 0;

    if (!pdb_file_binary_stream_.ReadBytes(bytes_read.data(), bytes_read.size(),
                                           &num_bytes_read)) {
      return false;
    }

    guids_heap_data_.push_back(string(bytes_read.begin(), bytes_read.end()));
  }

  return true;
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
