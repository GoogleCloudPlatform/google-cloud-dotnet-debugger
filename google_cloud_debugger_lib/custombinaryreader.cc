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

#include "custombinaryreader.h"

#include <iterator>
#include <vector>

#include "metadataheaders.h"

using std::ifstream;
using std::ios;
using std::string;
using std::vector;
using std::unique_ptr;
using std::streampos;

const int one = 1;
#define big_endian() ((*(char *)&one) == 0)

namespace google_cloud_debugger_portable_pdb {

const std::uint32_t kCompressedIntOneByteMask = 0x80;
const std::uint32_t kCompressedIntTwoByteMask = 0xC0;
const std::uint32_t kCompressedIntFourByteMask = 0xE0;

const std::uint32_t kCompressedUIntTwoByteUncompressMask = 0x3FFF;
const std::uint32_t kCompressedUIntFourByteUncompressMask = 0x1FFFFFFF;

const std::uint32_t kCompressedSignedIntOneByteUncompressMask = 0xFFFFFFC0;
const std::uint32_t kCompressedSignedIntTwoByteUncompressMask = 0xFFFFE000;
const std::uint32_t kCompressedSignedIntFourByteUncompressMask = 0xF0000000;

bool CustomBinaryStream::ConsumeStream(std::istream *stream) {
  if (!stream) {
    return false;
  }

  stream_.reset(stream);

  if (stream_->good()) {
    stream_->unsetf(std::ios::skipws);
    stream_->seekg(0, stream_->end);
    end_ = stream_->tellg();
    stream_->seekg(0, stream_->beg);
    begin_ = stream_->tellg();

    return true;
  }
}

bool CustomBinaryStream::ConsumeFile(const string &file) {
  unique_ptr<std::ifstream> file_stream = unique_ptr<std::ifstream>(
    new (std::nothrow) ifstream(file, ios::in | ios::binary | ios::ate));
  if (!file_stream && !file_stream->is_open()) {
    return false;
  }

  return ConsumeStream(file_stream.release());
}

bool CustomBinaryStream::ReadBytes(uint8_t *result, uint32_t bytes_to_read,
                                   uint32_t *bytes_read) {
  if (!stream_ || end_ - stream_->tellg() < bytes_to_read) {
    return false;
  }

  stream_->read(reinterpret_cast<char *>(result), bytes_to_read);
  *bytes_read = stream_->gcount();
  return *bytes_read == bytes_to_read;
}

bool CustomBinaryStream::HasNext() const {
  if (!stream_) {
    return false;
  }

  return stream_->tellg() < end_;
}

bool CustomBinaryStream::Peek(uint8_t *result) const {
  if (!stream_ || !HasNext()) {
    return false;
  }

  *result = stream_->peek();
  return true;
}

bool CustomBinaryStream::SeekFromCurrent(uint32_t index) {
  // Have to take into account the end_ based on the stream
  // length that we set.
  if (!stream_ || end_ - stream_->tellg() < index) {
    return false;
  }

  stream_->seekg(index, stream_->cur);
  if (stream_->fail()) {
    stream_->clear();
    return false;
  }

  return true;
}

bool CustomBinaryStream::SeekFromOrigin(uint32_t position) {
  if (!stream_) {
    return false;
  }

  stream_->seekg(position, stream_->beg);
  if (stream_->fail()) {
    stream_->clear();
    return false;
  }
  return true;
}

bool CustomBinaryStream::SetStreamLength(uint32_t length) {
  if (!stream_ || end_ - stream_->tellg() < length) {
    return false;
  }

  end_ = stream_->tellg() + static_cast<std::streamoff>(length);
  return true;
}

void CustomBinaryStream::ResetStreamLength() {
  if (!stream_) {
    return;
  }

  streampos cur_pos = stream_->tellg();
  stream_->seekg(0, stream_->end);
  end_ = stream_->tellg();
  stream_->seekg(cur_pos, stream_->beg);
}

bool CustomBinaryStream::GetString(std::string *result, std::uint32_t offset) {
  result->clear();
  if (!stream_) {
    return false;
  }

  // Makes a copy of the current position so we can restores the stream.
  streampos previous_pos = stream_->tellg();
  stream_->seekg(offset, stream_->beg);
  if (stream_->fail()) {
    stream_->clear();
    stream_->seekg(previous_pos);
    return false;
  }

  // Read until we encounter a null character
  // or the end of the file is reached.
  while (stream_->tellg() != end_) {
    char read_char;
    stream_->read(&read_char, 1);
    if (stream_->fail()) {
      stream_->clear();
      stream_->seekg(previous_pos);
      return false;
    }

    if (read_char != 0) {
      *result += read_char;
    }
    else {
      break;
    }
  }

  stream_->seekg(previous_pos);
  return true;
}

bool CustomBinaryStream::ReadByte(uint8_t *result) {
  uint32_t bytes_read = 0;
  return ReadBytes(result, 1, &bytes_read);
}

bool CustomBinaryStream::ReadUInt16(uint16_t *result) {
  uint32_t bytes_read = 0;
  vector<uint8_t> buffer(2, 0);
  if (!ReadBytes(buffer.data(), 2, &bytes_read)) {
    return false;
  }

  if (big_endian()) {
    *result = ((buffer[0] << 8) & 0xFF00) | buffer[1];
  } else {
    *result = ((buffer[1] << 8) & 0xFF00) | buffer[0];
  }

  return true;
}

bool CustomBinaryStream::ReadUInt32(uint32_t *result) {
  uint32_t bytes_read = 0;
  vector<uint8_t> buffer(4, 0);
  if (!ReadBytes(buffer.data(), 4, &bytes_read)) {
    return false;
  }

  if (big_endian()) {
    *result =
        ((buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3]);
  } else {
    *result =
        ((buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0]);
  }

  return true;
}

bool CustomBinaryStream::ReadCompressedUInt32(uint32_t *uncompress_int) {
  uint8_t first_byte;
  if (!ReadByte(&first_byte)) {
    return false;
  }

  // If the first bit is a 0, return the value. Range 0 - 0x7F.
  if ((first_byte & kCompressedIntOneByteMask) == 0) {
    *uncompress_int = first_byte;
    return true;
  }

  uint8_t second_byte;
  if (!ReadByte(&second_byte)) {
    return false;
  }

  // If the first two bits are "10", return the first two bytes.
  // Mask it with 0b11000000 (0xC0) and confirm the result is 0b10000000 (0x80).
  // Result should be in the range 0x80 - 0x3FFF.
  if ((first_byte & kCompressedIntTwoByteMask) == kCompressedIntOneByteMask) {
    *uncompress_int = ((first_byte << 8) | second_byte) &
                      kCompressedUIntTwoByteUncompressMask;
    return true;
  }

  uint8_t third_byte;
  uint8_t fourth_byte;
  if (!ReadByte(&third_byte) || !ReadByte(&fourth_byte)) {
    return false;
  }

  // If the first three bits are "110", return the first four bytes.
  // Mask it with 0b11100000 (0xE0) and confirm the result is 0b11000000 (0xC0).
  // Result should be in the range 0x4000 - 0x1FFFFFFF.
  if ((first_byte & kCompressedIntFourByteMask) == kCompressedIntTwoByteMask) {
    *uncompress_int = ((first_byte << 24) | (second_byte << 16) |
                       (third_byte << 8) | fourth_byte) &
                      kCompressedUIntFourByteUncompressMask;
    return true;
  }

  return false;
}

bool CustomBinaryStream::ReadCompressSignedInt32(int32_t *uncompressed_int) {
  // A simpler explanation is in "Expert .NET 2.0 IL Assembler". To encode a
  // signed integer value:
  // 1. Take the absolute value of the integer and shift it left by 1 bit.
  // 2. Set the least significant bit equal to the sign (MSB) of the original
  // value.
  // 3. Apply the regular ComporessedInt method.
  // Reversing is straight forward.
  uint8_t first_byte;
  if (!Peek(&first_byte)) {
    return false;
  }

  uint32_t raw_bytes;
  if (!ReadCompressedUInt32(&raw_bytes)) {
    return false;
  }

  int32_t result = (int32_t)raw_bytes;
  // Bits are rotated by 1 so 2 complement bit is at the end.
  bool negative = ((result & 0x1) != 0);
  result >>= 1;

  if (negative) {
    // To apply two's complement we merge the bits in based on the width.
    // 1 byte uses 6 bits, 2 byte values use 14 bits, and 4 byte values use 28
    // bits. Get the width by checking the first byte again.
    if ((first_byte & kCompressedIntOneByteMask) == 0) {
      result |= kCompressedSignedIntOneByteUncompressMask;
    } else if ((first_byte & kCompressedIntTwoByteMask) ==
               kCompressedIntOneByteMask) {
      result |= kCompressedSignedIntTwoByteUncompressMask;
    } else if ((first_byte & kCompressedIntFourByteMask) ==
               kCompressedIntTwoByteMask) {
      result |= kCompressedSignedIntFourByteUncompressMask;
    } else {
      return false;
    }
  }

  *uncompressed_int = result;
  return true;
}

bool CustomBinaryStream::ReadTableIndex(Heap heap, uint8_t heap_size,
                                        uint32_t *table_index) {
  // The Heap enum also encodes the bit mask into the heapSize value.
  if ((heap & heap_size) != 0x0) {
    return ReadUInt32(table_index);
  }

  uint16_t temp;
  if (!ReadUInt16(&temp)) {
    return false;
  }

  *table_index = temp;
  return true;
}

bool CustomBinaryStream::ReadTableIndex(
    MetadataTable table, const CompressedMetadataTableHeader &metadata_header,
    uint32_t *table_index) {
  if (!metadata_header.valid_mask[static_cast<int>(table)]) {
    // WARNING: If you are reading a table index into a metadata table that
    // isn't present, something is wrong.
    //
    // In practice, this happens when you only load the PDB metadata tables and
    // not the primary assembly's too. Since the PDB doesn't contain the rest of
    // the metadata. If the table happens to contain more than 2^16 entries, we
    // will read the wrong number of bytes and TERRIBLE THINGS will happen since
    // all future reads will be corrupt.
    //
    // BUG: Read assembly metadata (headers at least) in addition to PDB
    // metadata. For now we assume everything is less than 2^16.
    uint16_t temp;
    if (!ReadUInt16(&temp)) {
      return false;
    }

    *table_index = temp;
    return true;
  }

  uint32_t present_table_index = 0;
  for (size_t index = 0; index < static_cast<int>(table); ++index) {
    if (metadata_header.valid_mask[index]) {
      ++present_table_index;
    }
  }

  uint32_t rows_present = metadata_header.num_rows[present_table_index];
  // If the table has less than 2^16 rows then it is stored using 2 bytes.
  // Otherwise, 4 bytes.
  if (rows_present < 0x10000) {  // 2^16
    uint16_t temp;
    if (!ReadUInt16(&temp)) {
      return false;
    }

    *table_index = temp;
    return true;
  }

  return ReadUInt32(table_index);
}
}  // namespace google_cloud_debugger_portable_pdb
