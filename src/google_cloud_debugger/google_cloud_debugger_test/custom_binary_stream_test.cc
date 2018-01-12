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

#include <gtest/gtest.h>

#include <array>
#include <string>

#include "custom_binary_reader.h"

using std::array;
using std::string;
using std::stringstream;
using std::unique_ptr;

namespace google_cloud_debugger_test {

// Sets up a string stream based on char array stream_data.
unique_ptr<stringstream> SetUpStream(char *stream_data, uint32_t stream_size) {
  unique_ptr<stringstream> test_stream =
      unique_ptr<stringstream>(new (std::nothrow) stringstream());
  EXPECT_TRUE(test_stream != nullptr);
  for (size_t i = 0; i < stream_size; ++i) {
    *test_stream << stream_data[i];
  }
  return test_stream;
}

TEST(BinaryReader, ReadCompressedUnsignedInts) {
  char test_data[] = {// Unsigned tests.
                      0x03, 0x7F, 0x80, 0x80, 0xAE, 0x57, 0xBF, 0xFF,
                      0xC0, 0x00, 0x40, 0x00, 0xDF, 0xFF, 0xFF, 0xFF};
  unique_ptr<stringstream> test_stream =
      SetUpStream(test_data, sizeof(test_data));
  google_cloud_debugger_portable_pdb::CustomBinaryStream binary_stream;
  uint32_t unsigned_int;

  EXPECT_TRUE(binary_stream.ConsumeStream(test_stream.release()));

  // Checks that we can read compressed unsigned int.
  EXPECT_TRUE(binary_stream.ReadCompressedUInt32(&unsigned_int));
  EXPECT_EQ(unsigned_int, 0x3);

  EXPECT_TRUE(binary_stream.ReadCompressedUInt32(&unsigned_int));
  EXPECT_EQ(unsigned_int, 0x7F);

  EXPECT_TRUE(binary_stream.ReadCompressedUInt32(&unsigned_int));
  EXPECT_EQ(unsigned_int, 0x80);

  EXPECT_TRUE(binary_stream.ReadCompressedUInt32(&unsigned_int));
  EXPECT_EQ(unsigned_int, 0x2E57);

  EXPECT_TRUE(binary_stream.ReadCompressedUInt32(&unsigned_int));
  EXPECT_EQ(unsigned_int, 0x3FFF);

  EXPECT_TRUE(binary_stream.ReadCompressedUInt32(&unsigned_int));
  EXPECT_EQ(unsigned_int, 0x4000);

  EXPECT_TRUE(binary_stream.ReadCompressedUInt32(&unsigned_int));
  EXPECT_EQ(unsigned_int, 0x1FFFFFFF);
}

TEST(BinaryReader, ReadCompressedInts) {
  char test_data[] = {0x06, 0x7B, 0x80, 0x80, 0x01, 0xC0, 0x00,
                      0x40, 0x00, 0x80, 0x01, 0xDF, 0xFF, 0xFF,
                      0xFE, 0xC0, 0x00, 0x00, 0x01};
  unique_ptr<stringstream> test_stream =
      SetUpStream(test_data, sizeof(test_data));
  google_cloud_debugger_portable_pdb::CustomBinaryStream binary_stream;
  int32_t signed_int;

  EXPECT_TRUE(binary_stream.ConsumeStream(test_stream.release()));

  // Checks that we can read compressed signed int.
  EXPECT_TRUE(binary_stream.ReadCompressSignedInt32(&signed_int));
  EXPECT_EQ(signed_int, 3);

  EXPECT_TRUE(binary_stream.ReadCompressSignedInt32(&signed_int));
  EXPECT_EQ(signed_int, -3);

  EXPECT_TRUE(binary_stream.ReadCompressSignedInt32(&signed_int));
  EXPECT_EQ(signed_int, 64);

  EXPECT_TRUE(binary_stream.ReadCompressSignedInt32(&signed_int));
  EXPECT_EQ(signed_int, -64);

  EXPECT_TRUE(binary_stream.ReadCompressSignedInt32(&signed_int));
  EXPECT_EQ(signed_int, 8192);

  EXPECT_TRUE(binary_stream.ReadCompressSignedInt32(&signed_int));
  EXPECT_EQ(signed_int, -8192);

  EXPECT_TRUE(binary_stream.ReadCompressSignedInt32(&signed_int));
  EXPECT_EQ(signed_int, 268435455);

  EXPECT_TRUE(binary_stream.ReadCompressSignedInt32(&signed_int));
  EXPECT_EQ(signed_int, -268435456);
}

TEST(BinaryReader, BasicTests) {
  char test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  unique_ptr<stringstream> test_stream =
      SetUpStream(test_data, sizeof(test_data));
  google_cloud_debugger_portable_pdb::CustomBinaryStream binary_stream;

  EXPECT_TRUE(binary_stream.ConsumeStream(test_stream.release()));

  // Checks that peeking works.
  uint8_t peek_byte;

  EXPECT_TRUE(binary_stream.Peek(&peek_byte));
  EXPECT_EQ(peek_byte, 0x01);

  // Checks that peeking does not change the underlying stream.
  EXPECT_TRUE(binary_stream.Peek(&peek_byte));
  EXPECT_EQ(peek_byte, 0x01);

  // Checks that HasNext works.
  EXPECT_TRUE(binary_stream.HasNext());

  // Checks that SeekFromOrigin should not work.
  EXPECT_FALSE(binary_stream.SeekFromOrigin(10));
  EXPECT_TRUE(binary_stream.SeekFromOrigin(2));

  EXPECT_TRUE(binary_stream.Peek(&peek_byte));
  EXPECT_EQ(peek_byte, 0x03);

  // Checks that SeekFromCurrent works.
  EXPECT_FALSE(binary_stream.SeekFromCurrent(10));
  EXPECT_TRUE(binary_stream.SeekFromCurrent(2));

  EXPECT_TRUE(binary_stream.Peek(&peek_byte));
  EXPECT_EQ(peek_byte, 0x05);

  // Checks SetStreamLength works.
  EXPECT_FALSE(binary_stream.SetStreamLength(10));
  EXPECT_TRUE(binary_stream.SetStreamLength(2));
}

// Tests that GetString function of CustomBinaryReader works.
TEST(BinaryReader, GetStringTest) {
  char test_data[] = {'a', 'b', 'c', 0, 'd', 'e', 'f', 0};
  unique_ptr<stringstream> test_stream =
      SetUpStream(test_data, sizeof(test_data));
  google_cloud_debugger_portable_pdb::CustomBinaryStream binary_stream;

  EXPECT_TRUE(binary_stream.ConsumeStream(test_stream.release()));

  // Gets the string abc by using offset 0.
  std::string first_string;

  EXPECT_TRUE(binary_stream.GetString(&first_string, 0));
  EXPECT_EQ(first_string, "abc");

  // Gets the string def by using offset 4.
  std::string second_string;

  EXPECT_TRUE(binary_stream.GetString(&second_string, 4));
  EXPECT_EQ(second_string, "def");
}

}  // namespace google_cloud_debugger_test
