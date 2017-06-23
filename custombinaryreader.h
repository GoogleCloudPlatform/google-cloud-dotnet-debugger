// Copyright 2017 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#ifndef CUSTOM_BINARY_READER_H_
#define CUSTOM_BINARY_READER_H_

#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "metadatatables.h"

namespace google_cloud_debugger_portable_pdb {
  struct CompressedMetadataTableHeader;

  enum Heap : uint8_t { StringsHeap = 0x01, GuidsHeap = 0x02, BlobsHeap = 0x04 };

  // Class that consumes a file or a uint8_t vector and produces a
  // binary stream. This stream is used to read byte, integers,
  // compressed integers and table index.
  class CustomBinaryStream {
  public:
    // Consumes a file and exposes the file content as a binary stream.
    bool ConsumeFile(std::string file);

    // Consumes a uint8_t vector and exposes it as a binary stream.
    bool ConsumeVector(std::vector<uint8_t> *byte_vector);

    // Returns true if there is a next byte in the stream.
    bool HasNext();

    // Returns the next byte without advancing the stream.
    bool Peak(uint8_t *result);

    // Sets the stream position to position from the current position.
    bool SeekFromCurrent(uint32_t position);

    // Sets the stream position to position from the original position.
    bool SeekFromOrigin(uint32_t position);

    // Sets where the stream will end. This should be less than the current end_.
    bool SetStreamLength(uint32_t length);

    // Returns the number of bytes remaining in the stream.
    size_t GetStreamLength() { return end_ - iterator_; };

    // Reads the next byte in the stream.
    bool ReadByte(uint8_t *result);

    // Tries to read bytes_to_read in the stream and stores result in result.
    // Returns false if the number of bytes read are less than that.
    bool ReadBytes(uint8_t *result, uint32_t bytes_to_read);

    // Reads the next UInt16 from the stream.
    bool ReadUInt16(uint16_t *result);

    // Reads the next UInt32 from the stream.
    bool ReadUInt32(uint32_t *result);

    // Reads an unsigned integer using the encoding mechanism described in the
    // ECMA spec. See II.23.2 "Blobs and signatures".
    bool ReadCompressedUInt32(uint32_t *result);

    // Reads a signed int using the encoding mechanism described in the ECMA spec.
    // See II.23.2 "Blobs and signatures".
    //
    //
    // Why the long SignedInt name, rather than "CompressedUInt" and
    // "CompressedInt"? Because 90% of all reads from compressed values are for
    // UInts and it is too easy to confuse the two. Only call this method if you
    // are positive you are reading a signed value.
    bool ReadCompressSignedInt32(int32_t *result);

    // Reads a heap table index according to II.24.2.6 "#~ stream" under schema.
    bool ReadTableIndex(Heap heap, uint8_t heap_size, uint32_t *table_index);

    // Reads the size of a metadata table index according to II.24.2.6 "#~ stream"
    // under schema.
    bool ReadTableIndex(MetadataTable table,
      const CompressedMetadataTableHeader &metadata_header,
      uint32_t *table_index);

    // Returns the current stream position.
    std::vector<uint8_t>::iterator Current() { return iterator_; }
    
    // Returns the end position of the stream.
    std::vector<uint8_t>::iterator End() { return end_; }

    // Returns the beginning position of the stream.
    std::vector<uint8_t>::iterator Begin() {
      return begin_;
    }

  private:
    // The binary content of the file (if this stream s from a file).
    std::vector<uint8_t> file_content_;

    // The current stream position.
    std::vector<uint8_t>::iterator iterator_;

    // The end position of the stream.
    std::vector<uint8_t>::iterator end_;

    // The start position of the stream.
    std::vector<uint8_t>::iterator begin_;
  };

}  // namespace google_cloud_debugger_portable_pdb

#endif
