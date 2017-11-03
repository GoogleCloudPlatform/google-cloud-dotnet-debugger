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

#include "metadata_headers.h"

#include <assert.h>

#include "custom_binary_reader.h"

namespace google_cloud_debugger_portable_pdb {

using std::array;
using std::string;
using std::vector;

bool ParseFrom(CustomBinaryStream *binary_reader,
               MetadataRootHeader *root_header) {
  assert(binary_reader != nullptr);
  assert(root_header != nullptr);

  if (!binary_reader->ReadUInt32(&root_header->signature)) {
    return false;
  }

  if (!binary_reader->ReadUInt16(&root_header->major_version) ||
      root_header->major_version != 1) {
    return false;
  }

  if (!binary_reader->ReadUInt16(&root_header->minor_version) ||
      root_header->minor_version != 1) {
    return false;
  }

  if (!binary_reader->ReadUInt32(&root_header->reserved) ||
      root_header->reserved != 0) {
    return false;
  }

  if (!binary_reader->ReadUInt32(&root_header->version_string_length) &&
      root_header->version_string_length > 0) {
    return false;
  }

  vector<uint8_t> version_string_bytes(root_header->version_string_length, 0);

  uint32_t bytes_read = 0;
  if (!binary_reader->ReadBytes(version_string_bytes.data(),
                                root_header->version_string_length,
                                &bytes_read)) {
    return false;
  }

  string temp_vers_string(version_string_bytes.begin(),
                          version_string_bytes.end());
  root_header->version_string = temp_vers_string;

  // We have to advance to the next 4 byte boundary.
  uint32_t bytes_to_skipped = 4 - (root_header->version_string_length % 4);
  if (bytes_to_skipped != 4 &&
      !binary_reader->SeekFromCurrent(bytes_to_skipped)) {
    return false;
  }

  if (!binary_reader->ReadUInt16(&root_header->flags)) {
    return false;
  }

  if (!binary_reader->ReadUInt16(&root_header->number_streams)) {
    return false;
  }

  return true;
}

bool ParseFrom(CustomBinaryStream *binary_reader, StreamHeader *stream_header) {
  assert(binary_reader != nullptr);
  assert(stream_header != nullptr);

  if (!binary_reader->ReadUInt32(&stream_header->offset)) {
    return false;
  }

  if (!binary_reader->ReadUInt32(&stream_header->size)) {
    return false;
  }

  vector<uint8_t> header_name;
  uint8_t bytes_read = 0;
  uint8_t character;
  while (binary_reader->ReadByte(&character)) {
    ++bytes_read;
    if (character == 0) {
      break;
    }

    header_name.push_back(character);
    // Name cannot be longer than 32 characters.
    if (bytes_read > 32) {
      return false;
    }
  }

  // Pad until 4 boundary.
  uint32_t bytes_to_skipped = 4 - (bytes_read % 4);
  if (bytes_to_skipped % 4 != 0 &&
      !binary_reader->SeekFromCurrent(bytes_to_skipped)) {
    return false;
  }

  string name_string(header_name.begin(), header_name.end());
  stream_header->name = name_string;

  return true;
}

bool ParseFrom(CustomBinaryStream *binary_reader,
               PortablePdbMetadataSectionHeader *pdb_metadata_header) {
  assert(binary_reader != nullptr);
  assert(pdb_metadata_header != nullptr);

  uint32_t bytes_read = 0;
  if (!binary_reader->ReadBytes(pdb_metadata_header->pdb_id.data(),
                                pdb_metadata_header->pdb_id.size(),
                                &bytes_read)) {
    return false;
  }

  if (!binary_reader->ReadUInt32(&pdb_metadata_header->entry_point)) {
    return false;
  }

  array<uint8_t, 8> system_tables_bits;
  if (!binary_reader->ReadBytes(system_tables_bits.data(),
                                system_tables_bits.size(), &bytes_read)) {
    return false;
  }

  uint32_t num_referenced_type_system_tables =
      CountAndSetBits<64, array<uint8_t, 8>>(
          &pdb_metadata_header->referenced_type_system_tables,
          system_tables_bits);

  pdb_metadata_header->type_system_table_rows.resize(
      num_referenced_type_system_tables);
  for (size_t i = 0; i < num_referenced_type_system_tables; ++i) {
    if (!binary_reader->ReadUInt32(
            &pdb_metadata_header->type_system_table_rows[i])) {
      return false;
    }
  }

  return true;
}

bool ParseFrom(CustomBinaryStream *binary_reader,
               CompressedMetadataTableHeader *table_header) {
  assert(binary_reader != nullptr);
  assert(table_header != nullptr);

  if (!binary_reader->ReadUInt32(&table_header->reserved) &&
      table_header->reserved != 0) {
    return false;
  }

  if (!binary_reader->ReadByte(&table_header->major_version) &&
      table_header->major_version != 2) {
    return false;
  }

  if (!binary_reader->ReadByte(&table_header->minor_version) &&
      table_header->minor_version != 0) {
    return false;
  }

  if (!binary_reader->ReadByte(&table_header->heap_sizes)) {
    return false;
  }

  if (!binary_reader->ReadByte(&table_header->reserved_2) &&
      table_header->reserved_2 != 1) {
    return false;
  }

  array<uint8_t, 8> bits_array;
  uint32_t bytes_read;
  if (!binary_reader->ReadBytes(bits_array.data(), bits_array.size(),
                                &bytes_read)) {
    return false;
  }
  uint32_t num_valids = CountAndSetBits<64, array<uint8_t, 8>>(
      &table_header->valid_mask, bits_array);

  if (!binary_reader->ReadBytes(bits_array.data(), bits_array.size(),
                                &bytes_read)) {
    return false;
  }
  uint32_t num_sorted = CountAndSetBits<64, array<uint8_t, 8>>(
      &table_header->sorted_mask, bits_array);

  table_header->num_rows.resize(num_valids);
  for (size_t i = 0; i < num_valids; ++i) {
    if (!binary_reader->ReadUInt32(&table_header->num_rows[i])) {
      return false;
    }
  }

  return true;
}

}  // namespace google_cloud_debugger_portable_pdb
