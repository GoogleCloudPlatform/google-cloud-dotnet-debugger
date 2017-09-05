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

#include "cordebugvalue_mocks.h"

namespace google_cloud_debugger_test {

using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::_;
using google::cloud::diagnostics::debug::Variable;
using google_cloud_debugger::DbgString;
using std::string;

// Tests Initialize function of DbgString.
TEST(DbgStringTest, Initialize) {
  DbgString dbg_string(nullptr);

  ICorDebugStringValueMock string_value_mock;
  EXPECT_CALL(string_value_mock, QueryInterface(_, _))
      .WillRepeatedly(
          DoAll(SetArgPointee<1>(&string_value_mock), Return(S_OK)));

  dbg_string.Initialize(&string_value_mock, FALSE);
  EXPECT_FALSE(dbg_string.GetIsNull());

  dbg_string.Initialize(&string_value_mock, TRUE);
  EXPECT_TRUE(dbg_string.GetIsNull());
}

// Tests error cases for Initialize function of DbgString.
TEST(DbgStringTest, InitializeError) {
  DbgString dbg_string(nullptr);

  // If ICorDebugStringValue is null, E_INVALIDARG is returned.
  dbg_string.Initialize(nullptr, FALSE);
  EXPECT_EQ(dbg_string.GetInitializeHr(), E_INVALIDARG);

  ICorDebugStringValueMock string_value_mock;
  EXPECT_CALL(string_value_mock, QueryInterface(_, _))
      .WillRepeatedly(Return(E_NOTIMPL));

  dbg_string.Initialize(&string_value_mock, FALSE);
  EXPECT_EQ(dbg_string.GetInitializeHr(), E_NOTIMPL);
}

// Tests that PopulateType function always return System.String.
TEST(DbgStringTest, PopulateType) {
  static string string_type = "System.String";
  DbgString dbg_string(nullptr);

  Variable variable;
  EXPECT_EQ(dbg_string.PopulateType(&variable), S_OK);
  string string_result = variable.type();
  EXPECT_EQ(variable.type(), string_type);
}

// Tests error cases for PopulateType.
TEST(DbgStringTest, PopulateTypeError) {
  DbgString dbg_string(nullptr);
  EXPECT_EQ(dbg_string.PopulateType(nullptr), E_INVALIDARG);
}

TEST(DbgStringTest, GetString) {
  DbgString dbg_string(nullptr);

  static string test_string_value = "This is a test string";

// On Linux, PAL_STDCPP_COMPAT header is used. We have to use
// different string types because WCHAR defined on Linux is
// different than WCHAR defined on Windows.
#ifdef PAL_STDCPP_COMPAT
  WCHAR wchar_string[22] = u"This is a test string";
#else
  WCHAR wchar_string[22] = L"This is a test string";
#endif

  uint32_t string_size = test_string_value.size();
  ICorDebugStringValueMock string_value_mock;
  EXPECT_CALL(string_value_mock, GetLength(_))
      .Times(1)
      .WillRepeatedly(DoAll(SetArgPointee<0>(string_size), Return(S_OK)));

  EXPECT_CALL(string_value_mock, GetString(string_size + 1, _, _))
      .Times(1)
      .WillRepeatedly(DoAll(
          SetArrayArgument<2>(wchar_string, wchar_string + string_size + 1),
          Return(S_OK)));

  std::string returned_string;
  dbg_string.GetString(&string_value_mock, &returned_string);

  EXPECT_EQ(returned_string, test_string_value);
}

// Tests error cases for GetString.
TEST(DbgStringTest, GetStringError) {
  DbgString dbg_string(nullptr);

  static string test_string_value = "This is a test string";

// On Linux, PAL_STDCPP_COMPAT header is used. We have to use
// different string types because WCHAR defined on Linux is
// different than WCHAR defined on Windows.
#ifdef PAL_STDCPP_COMPAT
  WCHAR wchar_string[22] = u"This is a test string";
#else
  WCHAR wchar_string[22] = L"This is a test string";
#endif

  uint32_t string_size = test_string_value.size();
  ICorDebugStringValueMock string_value_mock;
  std::string returned_string;

  // Test null cases.
  EXPECT_EQ(dbg_string.GetString(nullptr, nullptr), E_INVALIDARG);
  EXPECT_EQ(dbg_string.GetString(&string_value_mock, nullptr), E_INVALIDARG);
  EXPECT_EQ(dbg_string.GetString(nullptr, &returned_string), E_INVALIDARG);

  {
    // Makes GetLength return an error.
    EXPECT_CALL(string_value_mock, GetLength(_))
        .Times(1)
        .WillRepeatedly(Return(E_ACCESSDENIED));
    EXPECT_EQ(dbg_string.GetString(&string_value_mock, &returned_string),
              E_ACCESSDENIED);
    EXPECT_TRUE(returned_string.empty());
  }

  {
    // Makes ICorDebugString's GetString method return an error.
    EXPECT_CALL(string_value_mock, GetLength(_))
        .Times(1)
        .WillRepeatedly(DoAll(SetArgPointee<0>(string_size), Return(S_OK)));
    EXPECT_CALL(string_value_mock, GetString(string_size + 1, _, _))
        .Times(1)
        .WillRepeatedly(Return(E_ABORT));
    EXPECT_EQ(dbg_string.GetString(&string_value_mock, &returned_string),
              E_ABORT);
    EXPECT_TRUE(returned_string.empty());
  }
}

}  // namespace google_cloud_debugger_test
