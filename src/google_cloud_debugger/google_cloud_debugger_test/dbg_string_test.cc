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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

#include "class_names.h"
#include "common_action_mocks.h"
#include "cor_debug_helper.h"
#include "i_cor_debug_mocks.h"

using google::cloud::diagnostics::debug::Variable;
using google_cloud_debugger::ConvertStringToWCharPtr;
using google_cloud_debugger::CorDebugHelper;
using google_cloud_debugger::DbgString;
using google_cloud_debugger::ICorDebugHelper;
using std::string;
using std::vector;
using ::testing::_;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;

namespace google_cloud_debugger_test {

// Test Fixture for DbgString.
class DbgStringTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    debug_helper_ = std::shared_ptr<ICorDebugHelper>(new CorDebugHelper());
  }

  // Sets up various mock objects for DbgString class.
  void SetUpString() {
    // If queried for ICorDebugHeapValue2, returns heap_value.
    // This happens when the Initialize function tries to create a strong
    // handle of the string.
    ON_CALL(string_value_, QueryInterface(__uuidof(ICorDebugHeapValue2), _))
        .WillByDefault(DoAll(SetArgPointee<1>(&heap_value_), Return(S_OK)));

    // Makes heap_value returns handle_value if CreateHandle is called.
    ON_CALL(heap_value_, CreateHandle(_, _))
        .WillByDefault(DoAll(SetArgPointee<1>(&handle_value_), Return(S_OK)));

    // The handle should dereference to the string value.
    ON_CALL(handle_value_, Dereference(_))
        .WillByDefault(DoAll(SetArgPointee<0>(&string_value_), Return(S_OK)));

    // If queried for ICorDebugStringValue, returns string_value_.
    ON_CALL(string_value_, QueryInterface(__uuidof(ICorDebugStringValue), _))
        .WillByDefault(DoAll(SetArgPointee<1>(&string_value_), Return(S_OK)));
  }

  // ICorDebugValue that represents the string.
  ICorDebugStringValueMock string_value_;

  // Heap and handle value created for the string.
  ICorDebugHeapValue2Mock heap_value_;
  ICorDebugHandleValueMock handle_value_;

  // ICorDebugHelper used for DbgString constructor.
  std::shared_ptr<ICorDebugHelper> debug_helper_;
};

// Tests Initialize function of DbgString.
TEST_F(DbgStringTest, Initialize) {
  DbgString dbg_string(nullptr, debug_helper_);

  SetUpString();

  dbg_string.Initialize(&string_value_, FALSE);
  EXPECT_FALSE(dbg_string.GetIsNull());

  dbg_string.Initialize(&string_value_, TRUE);
  EXPECT_TRUE(dbg_string.GetIsNull());
}

// Tests error cases for Initialize function of DbgString.
TEST_F(DbgStringTest, InitializeError) {
  DbgString dbg_string(nullptr, debug_helper_);

  // If ICorDebugStringValue is null, E_INVALIDARG is returned.
  dbg_string.Initialize(nullptr, FALSE);
  EXPECT_EQ(dbg_string.GetInitializeHr(), E_INVALIDARG);

  EXPECT_CALL(string_value_, QueryInterface(_, _))
      .Times(1)
      .WillRepeatedly(Return(E_NOTIMPL));

  dbg_string.Initialize(&string_value_, FALSE);
  EXPECT_EQ(dbg_string.GetInitializeHr(), E_NOTIMPL);
}

// Tests that PopulateType function always return System.String.
TEST_F(DbgStringTest, PopulateType) {
  DbgString dbg_string(nullptr, debug_helper_);

  Variable variable;
  EXPECT_EQ(dbg_string.PopulateType(&variable), S_OK);
  string string_result = variable.type();
  EXPECT_EQ(variable.type(), google_cloud_debugger::kStringClassName);
}

// Tests error cases for PopulateType.
TEST_F(DbgStringTest, PopulateTypeError) {
  DbgString dbg_string(nullptr, debug_helper_);
  EXPECT_EQ(dbg_string.PopulateType(nullptr), E_INVALIDARG);
}

TEST_F(DbgStringTest, GetString) {
  static const string test_string_value = "This is a test string";

  vector<WCHAR> wchar_string = ConvertStringToWCharPtr(test_string_value);

  uint32_t string_size = wchar_string.size();
  EXPECT_CALL(string_value_, GetLength(_))
      .Times(1)
      .WillRepeatedly(DoAll(SetArgPointee<0>(string_size - 1), Return(S_OK)));

  EXPECT_CALL(string_value_, GetString(string_size, _, _))
      .Times(1)
      .WillRepeatedly(
          DoAll(SetArrayArgument<2>(wchar_string.data(),
                                    wchar_string.data() + string_size),
                Return(S_OK)));

  DbgString dbg_string(nullptr, debug_helper_);
  SetUpString();
  dbg_string.Initialize(&string_value_, FALSE);

  std::string returned_string;
  EXPECT_EQ(DbgString::GetString(&dbg_string, &returned_string), S_OK);

  EXPECT_EQ(returned_string, test_string_value);
}

// Tests error cases for GetString.
TEST_F(DbgStringTest, GetStringError) {
  static const string test_string_value = "This is a test string";
  uint32_t string_size = test_string_value.size();

  std::string returned_string;

  SetUpString();
  DbgString dbg_string(nullptr, debug_helper_);
  dbg_string.Initialize(&string_value_, FALSE);

  // Test null cases.
  EXPECT_EQ(dbg_string.GetString(nullptr, nullptr), E_INVALIDARG);
  EXPECT_EQ(dbg_string.GetString(&dbg_string, nullptr), E_INVALIDARG);
  EXPECT_EQ(dbg_string.GetString(nullptr, &returned_string), E_INVALIDARG);

  {
    // Makes GetLength return an error.
    EXPECT_CALL(string_value_, GetLength(_))
        .Times(1)
        .WillRepeatedly(Return(E_ACCESSDENIED));
    EXPECT_EQ(DbgString::GetString(&dbg_string, &returned_string),
              E_ACCESSDENIED);
    EXPECT_TRUE(returned_string.empty());
  }

  {
    // Makes ICorDebugString's GetString method return an error.
    EXPECT_CALL(string_value_, GetLength(_))
        .Times(1)
        .WillRepeatedly(DoAll(SetArgPointee<0>(string_size), Return(S_OK)));
    EXPECT_CALL(string_value_, GetString(string_size + 1, _, _))
        .Times(1)
        .WillRepeatedly(Return(E_ABORT));
    EXPECT_EQ(DbgString::GetString(&dbg_string, &returned_string), E_ABORT);
    EXPECT_TRUE(returned_string.empty());
  }
}

}  // namespace google_cloud_debugger_test
