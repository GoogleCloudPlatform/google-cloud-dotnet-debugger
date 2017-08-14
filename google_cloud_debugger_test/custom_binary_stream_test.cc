#include <custombinaryreader.h>
#include <gtest/gtest.h>

namespace google_cloud_debugger_test {

TEST(BinaryReader, ReadCompressedInts) {
  uint8_t test_data[] = {// Unsigned tests.
                         0x03, 0x7F, 0x80, 0x80, 0xAE, 0x57, 0xBF, 0xFF, 0xC0,
                         0x00, 0x40, 0x00, 0xDF, 0xFF, 0xFF, 0xFF,

                         // Signed tests.
                         0x06, 0x7B, 0x80, 0x80, 0x01, 0xC0, 0x00, 0x40, 0x00,
                         0x80, 0x01, 0xDF, 0xFF, 0xFF, 0xFE, 0xC0, 0x00, 0x00,
                         0x01};

  google_cloud_debugger_portable_pdb::CustomBinaryStream binary_stream;
  uint32_t unsigned_int;
  // Should fail and don't throw error if no stream is available.
  EXPECT_FALSE(binary_stream.ReadCompressedUInt32(&unsigned_int));

  std::vector<uint8_t> test_data_vector(
      test_data, test_data + sizeof test_data / sizeof test_data[0]);
  EXPECT_TRUE(binary_stream.ConsumeVector(test_data_vector));

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

  int32_t signed_int;

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
  uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

  google_cloud_debugger_portable_pdb::CustomBinaryStream binary_stream;

  std::vector<uint8_t> test_data_vector(
      test_data, test_data + sizeof test_data / sizeof test_data[0]);
  EXPECT_TRUE(binary_stream.ConsumeVector(test_data_vector));

  // Checks that peeking works.
  uint8_t peek_byte;

  EXPECT_EQ(binary_stream.GetRemainingStreamLength(), 8);
  EXPECT_TRUE(binary_stream.Peek(&peek_byte));
  EXPECT_EQ(peek_byte, 0x01);

  // Checks that peeking does not change the underlying stream.
  EXPECT_EQ(binary_stream.GetRemainingStreamLength(), 8);
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

  EXPECT_EQ(binary_stream.GetRemainingStreamLength(), 2);
}

}  // namespace google_cloud_debugger_test
