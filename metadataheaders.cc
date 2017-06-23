// Copyright 2017 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#include "metadataheaders.h"

#include "custombinaryreader.h"

namespace google_cloud_debugger_portable_pdb {

  using std::vector;
  using std::string;
  using std::array;

  bool MetadataRootHeader::ParseFrom(CustomBinaryStream *binary_reader,
    MetadataRootHeader *root_header) {
    if (!binary_reader && !root_header) {
      return false;
    }

    if (!binary_reader->ReadUInt32(&root_header->signature_)) {
      return false;
    }

    if (!binary_reader->ReadUInt16(&root_header->major_version_) ||
      root_header->major_version_ != 1) {
      return false;
    }

    if (!binary_reader->ReadUInt16(&root_header->minor_version_) ||
      root_header->minor_version_ != 1) {
      return false;
    }

    if (!binary_reader->ReadUInt32(&root_header->reserved_) ||
      root_header->reserved_ != 0) {
      return false;
    }

    if (!binary_reader->ReadUInt32(&root_header->version_string_length_) &&
      root_header->version_string_length_ > 0) {
      return false;
    }

    vector<uint8_t> version_string_bytes(root_header->version_string_length_, 0);

    if (!binary_reader->ReadBytes(version_string_bytes.data(),
      root_header->version_string_length_)) {
      return false;
    }

    string version_string(version_string_bytes.begin(),
      version_string_bytes.end());
    root_header->version_string_ = version_string;

    // TODO(quoct): check this.
    // Not sure why these values are being set. Removing them.
    // header.version = header.version.Trim(new char[] { '\0' });

    // We have to advance to the next 4 byte boundary.
    uint32_t bytes_to_read = 4 - (root_header->version_string_length_ % 4);
    if (bytes_to_read != 4) {
      vector<uint8_t> result(bytes_to_read, 0);
      if (!binary_reader->ReadBytes(result.data(), bytes_to_read)) {
        return false;
      }
    }

    if (!binary_reader->ReadUInt16(&root_header->flags_)) {
      return false;
    }

    if (!binary_reader->ReadUInt16(&root_header->number_streams_)) {
      return false;
    }

    return true;
  }

  bool StreamHeader::ParseFrom(CustomBinaryStream *binary_reader,
    StreamHeader *stream_header) {
    if (!binary_reader && !stream_header) {
      return false;
    }

    if (!binary_reader->ReadUInt32(&stream_header->offset_)) {
      return false;
    }

    if (!binary_reader->ReadUInt32(&stream_header->size_)) {
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
    while (bytes_read % 4 != 0) {
      if (!binary_reader->ReadByte(&character)) {
        return false;
      }
      ++bytes_read;
    }

    string name_string(header_name.begin(), header_name.end());
    stream_header->name_ = name_string;

    return true;
  }

  bool PortablePdbMetadataSectionHeader::ParseFrom(
    CustomBinaryStream *binary_reader,
    PortablePdbMetadataSectionHeader *pdb_metadata_header) {
    if (!binary_reader && !pdb_metadata_header) {
      return false;
    }

    if (!binary_reader->ReadBytes(pdb_metadata_header->pdb_id_.data(),
      pdb_metadata_header->pdb_id_.size())) {
      return false;
    }

    if (!binary_reader->ReadUInt32(&pdb_metadata_header->entry_point_)) {
      return false;
    }

    array<uint8_t, 8> system_tables_bits;
    if (!binary_reader->ReadBytes(system_tables_bits.data(),
      system_tables_bits.size())) {
      return false;
    }

    uint32_t num_referenced_type_system_tables =
      CountAndSetBits<64>(pdb_metadata_header->referenced_type_system_tables_,
        system_tables_bits.data());

    pdb_metadata_header->type_system_table_rows_.resize(
      num_referenced_type_system_tables);
    for (size_t i = 0; i < num_referenced_type_system_tables; ++i) {
      if (!binary_reader->ReadUInt32(
        &pdb_metadata_header->type_system_table_rows_[i])) {
        return false;
      }
    }

    return true;
  }

  bool CompressedMetadataTableHeader::ParseFrom(
    CustomBinaryStream *binary_reader,
    CompressedMetadataTableHeader *table_header) {
    if (!binary_reader && !table_header) {
      return false;
    }

    if (!binary_reader->ReadUInt32(&table_header->reserved_) &&
      table_header->reserved_ != 0) {
      return false;
    }

    if (!binary_reader->ReadByte(&table_header->major_version_) &&
      table_header->major_version_ != 2) {
      return false;
    }

    if (!binary_reader->ReadByte(&table_header->minor_version_) &&
      table_header->minor_version_ != 0) {
      return false;
    }

    if (!binary_reader->ReadByte(&table_header->heap_sizes)) {
      return false;
    }

    if (!binary_reader->ReadByte(&table_header->reserved_2_) &&
      table_header->reserved_2_ != 1) {
      return false;
    }

    array<uint8_t, 8> bits_array;
    if (!binary_reader->ReadBytes(bits_array.data(), bits_array.size())) {
      return false;
    }
    uint32_t num_valids =
      CountAndSetBits<64>(table_header->valid_mask_, bits_array.data());

    if (!binary_reader->ReadBytes(bits_array.data(), bits_array.size())) {
      return false;
    }
    uint32_t num_sorted =
      CountAndSetBits<64>(table_header->sorted_mask_, bits_array.data());

    table_header->num_rows_.resize(num_valids);
    for (size_t i = 0; i < num_valids; ++i) {
      if (!binary_reader->ReadUInt32(&table_header->num_rows_[i])) {
        return false;
      }
    }

    return true;
  }

}  // namespace google_cloud_debugger_portable_pdb
