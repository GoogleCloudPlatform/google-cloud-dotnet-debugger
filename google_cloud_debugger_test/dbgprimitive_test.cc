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
using ::testing::DoAll;
using google::cloud::diagnostics::debug::Variable;
using google_cloud_debugger::DbgPrimitive;
using std::string;

// SetArgPointee does not allow casting so we have to write our own action.
ACTION_P(SetArg0ToInt32Value, value) {
  *static_cast<uint32_t*>(arg0) = value;
}

ACTION_P(SetArg0ToByteValue, value) {
  *static_cast<uint8_t*>(arg0) = value;
}

// Tests Initialize function of DbgPrimitive.
TEST(DbgPrimitiveTest, Initialize) {
  CorDebugTypeMock debug_type;
  DbgPrimitive<uint32_t> dbg_primitive(&debug_type);

  // Get type should be called during initialize.
  // Mocks and sets the type to U4 (UInt32).
  EXPECT_CALL(debug_type, GetType(_))
    .WillRepeatedly(
      DoAll(SetArgPointee<0>(CorElementType::ELEMENT_TYPE_U4), Return(S_OK)));

  DebugGenericValueMock generic_value_mock;
  EXPECT_CALL(generic_value_mock, QueryInterface(_, _))
      .WillRepeatedly(
          DoAll(SetArgPointee<1>(&generic_value_mock), Return(S_OK)));

  dbg_primitive.Initialize(&generic_value_mock, FALSE);
}

// Tests that PopulateType function of DbgPrimitive returns correct type.
TEST(DbgPrimitiveTest, PopulateType) {
  CorDebugTypeMock debug_type;
  DbgPrimitive<uint64_t> dbg_primitive(&debug_type);

  // Get type should be called during initialize.
  // Mocks and sets the type to U8 (UInt64).
  EXPECT_CALL(debug_type, GetType(_))
    .WillRepeatedly(
      DoAll(SetArgPointee<0>(CorElementType::ELEMENT_TYPE_U8), Return(S_OK)));

  DebugGenericValueMock generic_value_mock;
  EXPECT_CALL(generic_value_mock, QueryInterface(_, _))
      .WillRepeatedly(
          DoAll(SetArgPointee<1>(&generic_value_mock), Return(S_OK)));

  dbg_primitive.Initialize(&generic_value_mock, FALSE);

  Variable variable;
  EXPECT_EQ(dbg_primitive.PopulateType(&variable), S_OK);
  EXPECT_EQ(variable.type(), "System.UInt64");
}

// Tests that PopulateValue function of DbgPrimitive returns correct value.
TEST(DbgPrimitiveTest, PopulateValue) {
  CorDebugTypeMock debug_type;
  DbgPrimitive<int32_t> dbg_primitive(&debug_type);

  // Get type should be called during initialize.
  // Mocks and sets the type to I4 (Int32).
  EXPECT_CALL(debug_type, GetType(_))
    .WillRepeatedly(
      DoAll(SetArgPointee<0>(CorElementType::ELEMENT_TYPE_I4), Return(S_OK)));

  DebugGenericValueMock generic_value_mock;
  EXPECT_CALL(generic_value_mock, QueryInterface(_, _))
      .WillRepeatedly(
          DoAll(SetArgPointee<1>(&generic_value_mock), Return(S_OK)));

  int32_t int32_value = 10;
  EXPECT_CALL(generic_value_mock, GetValue(_))
    .WillRepeatedly(DoAll(SetArg0ToInt32Value(int32_value), Return(S_OK)));

  dbg_primitive.Initialize(&generic_value_mock, FALSE);

  Variable variable;
  EXPECT_EQ(dbg_primitive.PopulateValue(&variable), S_OK);
  EXPECT_EQ(variable.value(), std::to_string(int32_value));
}

// Tests that SetValue function of DbgPrimitive works.
TEST(DbgPrimitiveTest, SetValue) {
  CorDebugTypeMock debug_type;
  DbgPrimitive<uint8_t> dbg_primitive(&debug_type);

  // Get type should be called during initialize.
  // Mocks and sets the type to Byte (U1).
  EXPECT_CALL(debug_type, GetType(_))
    .WillRepeatedly(
      DoAll(SetArgPointee<0>(CorElementType::ELEMENT_TYPE_U1), Return(S_OK)));

  dbg_primitive.Initialize(nullptr, FALSE);

  // Now sets the value.
  DebugGenericValueMock generic_value_mock;
  EXPECT_CALL(generic_value_mock, QueryInterface(_, _))
      .WillRepeatedly(
          DoAll(SetArgPointee<1>(&generic_value_mock), Return(S_OK)));

  uint8_t byte_value = 45;
  EXPECT_CALL(generic_value_mock, GetValue(_))
    .WillRepeatedly(DoAll(SetArg0ToByteValue(byte_value), Return(S_OK)));

  EXPECT_EQ(dbg_primitive.SetValue(&generic_value_mock), S_OK);

  // Value should now be 45 since we call SetValue.
  Variable variable;
  EXPECT_EQ(dbg_primitive.PopulateValue(&variable), S_OK);
  EXPECT_EQ(variable.value(), std::to_string(byte_value));

  // The variable type should still be correct even if we did not give an
  // ICorDebugValue to Initialize function.
  EXPECT_EQ(dbg_primitive.PopulateType(&variable), S_OK);
  EXPECT_EQ(variable.type(), "System.Byte");
}

}  // namespace google_cloud_debugger_test
