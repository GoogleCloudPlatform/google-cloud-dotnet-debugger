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

#ifndef METADATA_HEADERS_H_
#define METADATA_HEADERS_H_

#include <array>
#include <bitset>
#include <cstdint>
#include <string>
#include <vector>

namespace google_cloud_debugger_portable_pdb {

class CustomBinaryStream;

// The following data structures are defined in the ECMA-335 Partition II
// metadata standard.
// https://www.ecma-international.org/publications/standards/Ecma-335.htm

// Header for metadata files in physical storage.
// II.24.2.1 Metadata root
struct MetadataRootHeader {
  // Magic signature for physical metadata.
  std::uint32_t signature = 0;

  // Major version, 1 (ignore on read).
  std::uint16_t major_version = 0;

  // Minor version, 1 (ignore on read).
  std::uint16_t minor_version = 0;

  // Reserved, always 0.
  std::uint32_t reserved = 0;

  // Number of bytes allocated to hold version string (including
  // null terminator), call this x.
  //
  // Call the length of the string (including the terminator) m (we
  // require m <= 255); the length x is m rounded up to a multiple
  // of four.
  std::uint32_t version_string_length = 0;

  // UTF8-encoded null-terminated version string of length m (from above).
  std::string version_string;

  // Reserved, always 0.
  std::uint16_t flags = 0;

  // The number of metadata streams.
  std::uint16_t number_streams = 0;
};

// Header for an individual data stream of a Metadata file.
// II.24.2.2 Stream header
struct StreamHeader {
  // Memory offset to start of this stream from start of the metadata root.
  std::uint32_t offset = 0;

  // Size of this stream in bytes, shall be a multiple of 4.
  std::uint32_t size = 0;

  // Name of the stream as null-terminated variable length array of ASCII
  // characters, padded to the next 4-byte boundary with \0 characters.
  // The name is limited to 32 characters.
  std::string name;
};

// Header for a PortablePDB metadata section. (The #Pdb stream.)
// https://github.com/dotnet/corefx/blob/master/src/System.Reflection.Metadata/specs/PortablePdb-Metadata.md
struct PortablePdbMetadataSectionHeader {
  // A 20-byte sequence uniquely representing the debugging metadata blob
  // content.
  std::array<uint8_t, 20> pdb_id;

  // Entry point MethodDef token, or 0 if not applicable.The same value as
  // stored in CLI header of the PE file. See ECMA-335-II 15.4.1.2.
  std::uint32_t entry_point = 0;

  // Bit vector of referenced type system metadata tables, let n be the number
  // of bits that are 1.
  std::bitset<64> referenced_type_system_tables;

  // Array of n 4-byte unsigned integers indicating the number of rows for each
  // referenced type system metadata table.
  std::vector<uint32_t> type_system_table_rows;
};

// Header for the compressed metadata table stream. (The #~ stream.)
// II.24.2.6 #~ stream
struct CompressedMetadataTableHeader {
  // Reserved, always 0.
  std::uint32_t reserved = 0;

  // Major version of the table schema (1 for v1.0 and v1.1; 2 for v2.0).
  std::uint8_t major_version = 0;

  // Minor version of the table schema (0 for all versions).
  std::uint8_t minor_version = 0;

  // Bit vector for heap sizes.
  //
  // Binary flags indicate the offset sizes to be used within the heaps.
  // A 4-byte unsigned int offset is indicated by 0x01 for a string heap.
  // 0x02 for a GUID heap, and 0x04 for a blob heap.
  //
  // If a flag is not set, the respective heap offset is a 2-byte unsigned
  // integer. A "#-" (uncompressed metadata stream) can also have special
  // flags set: flag 0x20, indiciating that the stream only contains only
  // changes made during an edit-and-continue session, and flag 0x80,
  // indicating the metadata might contain items marked as delete.
  std::uint8_t heap_sizes = 0;

  // Reserved, always 1.
  std::uint8_t reserved_2 = 0;

  // (8 bytes) Bit vector of present tables, each bit representing one table. (1
  // if present).
  std::bitset<64> valid_mask;

  // (8 bytes) Bit vector of sorted tables. Each bit representing a respectivce
  // table (1 if present).
  std::bitset<64> sorted_mask;

  // Number of rows in each table marked 1 in the valid_mask_ bit vector.
  std::vector<uint32_t> num_rows;
};

// Extracts a MetadataRootHeader from a binary stream.
bool ParseFrom(CustomBinaryStream *binary_reader, MetadataRootHeader *result);

// Extracts a StreamHeader from a binary stream.
bool ParseFrom(CustomBinaryStream *binary_reader, StreamHeader *stream_header);

// Extracts a PortablePdbMetadataSectionHeader from a binary stream.
bool ParseFrom(CustomBinaryStream *binary_reader,
               PortablePdbMetadataSectionHeader *result);

// Extracts a CompressedMetadataTableHeader from a binary stream.
bool ParseFrom(CustomBinaryStream *binary_reader,
               CompressedMetadataTableHeader *table_header);

// Helper function to set the bitset destination based on the uint8_t
// array source. The function returns the number of bits that are set.
// TSource must have operator [] and size() method.
template <size_t size, typename TSource>
std::uint32_t CountAndSetBits(std::bitset<size> *destination,
                              const TSource &source) {
  if (!destination) {
    return 0;
  }

  std::uint32_t results = 0;
  std::size_t index = 0;

  for (size_t i = 0; i < size / 8; ++i) {
    if (i == source.size()) {
      break;
    }

    std::uint8_t current_byte = source[i];
    std::uint8_t mask = 1;
    for (size_t j = 0; j < 8; ++j) {
      (*destination)[index] = current_byte & mask;

      if ((*destination)[index]) {
        ++results;
      }

      current_byte >>= 1;
      ++index;
    }
  }

  return results;
}

}  // namespace google_cloud_debugger_portable_pdb

#endif
