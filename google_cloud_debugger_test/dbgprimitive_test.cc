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

using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::_;
using google::cloud::diagnostics::debug::Variable;
using google_cloud_debugger::DbgPrimitive;
using std::string;

// SetArgPointee does not allow casting so we have to write our own action.
ACTION_P(SetArg0ToInt32Value, value) { *static_cast<uint32_t*>(arg0) = value; }

ACTION_P(SetArg0ToByteValue, value) { *static_cast<uint8_t*>(arg0) = value; }

// Tests Initialize function of DbgPrimitive.
TEST(DbgPrimitiveTest, Initialize) {
  ICorDebugTypeMock debug_type;
  DbgPrimitive<uint32_t> dbg_primitive(&debug_type);

  // GetType should be called during initialize.
  // Mocks and sets the type to U4 (UInt32).
  EXPECT_CALL(debug_type, GetType(_))
      .WillRepeatedly(DoAll(SetArgPointee<0>(CorElementType::ELEMENT_TYPE_U4),
                            Return(S_OK)));

  ICorDebugGenericValueMock generic_value_mock;
  EXPECT_CALL(generic_value_mock, QueryInterface(_, _))
      .WillRepeatedly(
          DoAll(SetArgPointee<1>(&generic_value_mock), Return(S_OK)));

  dbg_primitive.Initialize(&generic_value_mock, FALSE);
}

// Tests error cases for the Initialize function of DbgPrimitive.
TEST(DbgPrimitiveTest, InitializeError) {
  ICorDebugTypeMock debug_type;
  DbgPrimitive<uint32_t> dbg_primitive(&debug_type);
  ICorDebugGenericValueMock generic_value_mock;

  {
    // GetType should be called during initialize.
    // Mocks and returns an error.
    EXPECT_CALL(debug_type, GetType(_))
        .WillRepeatedly(Return(CORDBG_E_BAD_REFERENCE_VALUE));
    dbg_primitive.Initialize(&generic_value_mock, FALSE);
    EXPECT_EQ(dbg_primitive.GetInitializeHr(), CORDBG_E_BAD_REFERENCE_VALUE);
  }

  {
    // GetType should be called during initialize.
    // Mocks and sets the type to U4 (UInt32).
    EXPECT_CALL(debug_type, GetType(_))
        .WillRepeatedly(DoAll(SetArgPointee<0>(CorElementType::ELEMENT_TYPE_U4),
                              Return(S_OK)));

    ICorDebugGenericValueMock generic_value_mock;
    {
      // Mocks and returns an error for QueryInterface
      EXPECT_CALL(generic_value_mock, QueryInterface(_, _))
          .WillRepeatedly(
              DoAll(SetArgPointee<1>(&generic_value_mock), Return(E_NOTIMPL)));

      dbg_primitive.Initialize(&generic_value_mock, FALSE);
      EXPECT_EQ(dbg_primitive.GetInitializeHr(), E_NOTIMPL);
    }

    {
      EXPECT_CALL(generic_value_mock, QueryInterface(_, _))
          .WillRepeatedly(
              DoAll(SetArgPointee<1>(&generic_value_mock), Return(S_OK)));
      // Mocks and returns error for GetValue.
      EXPECT_CALL(generic_value_mock, GetValue(_))
          .WillRepeatedly(Return(CORDBG_S_FUNC_EVAL_HAS_NO_RESULT));

      dbg_primitive.Initialize(&generic_value_mock, FALSE);
      EXPECT_EQ(dbg_primitive.GetInitializeHr(),
                CORDBG_S_FUNC_EVAL_HAS_NO_RESULT);
    }
  }
}

// Tests that PopulateType function of DbgPrimitive returns correct type.
TEST(DbgPrimitiveTest, PopulateType) {
  ICorDebugTypeMock debug_type;
  DbgPrimitive<uint64_t> dbg_primitive(&debug_type);

  // GetType should be called during initialize.
  // Mocks and sets the type to U8 (UInt64).
  EXPECT_CALL(debug_type, GetType(_))
      .WillRepeatedly(DoAll(SetArgPointee<0>(CorElementType::ELEMENT_TYPE_U8),
                            Return(S_OK)));

  ICorDebugGenericValueMock generic_value_mock;
  EXPECT_CALL(generic_value_mock, QueryInterface(_, _))
      .WillRepeatedly(
          DoAll(SetArgPointee<1>(&generic_value_mock), Return(S_OK)));

  dbg_primitive.Initialize(&generic_value_mock, FALSE);

  Variable variable;
  EXPECT_EQ(dbg_primitive.PopulateType(&variable), S_OK);
  EXPECT_EQ(variable.type(), "System.UInt64");
}

// Tests error cases for PopulateType function of DbgPrimitive.
TEST(DbgPrimitiveTest, PopulateTypeError) {
  DbgPrimitive<uint64_t> dbg_primitive(nullptr);

  // Errors out for null pointer.
  EXPECT_EQ(dbg_primitive.PopulateType(nullptr), E_INVALIDARG);
}

// Tests that PopulateValue function of DbgPrimitive returns correct value.
TEST(DbgPrimitiveTest, PopulateValue) {
  ICorDebugTypeMock debug_type;
  DbgPrimitive<int32_t> dbg_primitive(&debug_type);

  // Get type should be called during initialize.
  // Mocks and sets the type to I4 (Int32).
  EXPECT_CALL(debug_type, GetType(_))
      .WillRepeatedly(DoAll(SetArgPointee<0>(CorElementType::ELEMENT_TYPE_I4),
                            Return(S_OK)));

  ICorDebugGenericValueMock generic_value_mock;
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

// Tests error cases for PopulateValue function of DbgPrimitive.
TEST(DbgPrimitiveTest, PopulateValueError) {
  ICorDebugTypeMock debug_type;
  DbgPrimitive<uint32_t> dbg_primitive(&debug_type);
  ICorDebugGenericValueMock generic_value_mock;
  Variable variable;

  // Returns E_INVALIDARG for null pointer.
  EXPECT_EQ(dbg_primitive.PopulateValue(nullptr), E_INVALIDARG);

  // GetType should be called during initialize.
  // Mocks and returns an error.
  EXPECT_CALL(debug_type, GetType(_))
      .WillRepeatedly(Return(CORDBG_E_BAD_REFERENCE_VALUE));
  dbg_primitive.Initialize(&generic_value_mock, FALSE);

  // GetValue should returns the same error as the one from Initialize.
  EXPECT_EQ(dbg_primitive.PopulateValue(&variable),
            CORDBG_E_BAD_REFERENCE_VALUE);
}

// Tests that SetValue function of DbgPrimitive works.
TEST(DbgPrimitiveTest, SetValue) {
  ICorDebugTypeMock debug_type;
  DbgPrimitive<uint8_t> dbg_primitive(&debug_type);

  // Get type should be called during initialize.
  // Mocks and sets the type to Byte (U1).
  EXPECT_CALL(debug_type, GetType(_))
      .WillRepeatedly(DoAll(SetArgPointee<0>(CorElementType::ELEMENT_TYPE_U1),
                            Return(S_OK)));

  dbg_primitive.Initialize(nullptr, FALSE);

  // Now sets the value.
  ICorDebugGenericValueMock generic_value_mock;
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

// Tests error cases for SetValue function of DbgPrimitive.
TEST(DbgPrimitiveTest, SetValueError) {
  ICorDebugTypeMock debug_type;
  DbgPrimitive<uint8_t> dbg_primitive(&debug_type);

  // Get type should be called during initialize.
  // Mocks and sets the type to Byte (U1).
  EXPECT_CALL(debug_type, GetType(_))
      .WillRepeatedly(DoAll(SetArgPointee<0>(CorElementType::ELEMENT_TYPE_U1),
                            Return(S_OK)));

  dbg_primitive.Initialize(nullptr, FALSE);

  ICorDebugGenericValueMock generic_value_mock;
  Variable variable;

  EXPECT_EQ(dbg_primitive.SetValue(nullptr), E_INVALIDARG);

  {
    // Mocks and returns error for QueryInterface.
    EXPECT_CALL(generic_value_mock, QueryInterface(_, _))
        .WillRepeatedly(Return(E_NOTIMPL));

    EXPECT_EQ(dbg_primitive.SetValue(&generic_value_mock), E_NOTIMPL);
  }

  {
    // Mocks and returns error for GetValue (but not QueryInterface).
    EXPECT_CALL(generic_value_mock, QueryInterface(_, _))
        .WillRepeatedly(
            DoAll(SetArgPointee<1>(&generic_value_mock), Return(S_OK)));

    EXPECT_CALL(generic_value_mock, GetValue(_))
        .WillRepeatedly(Return(CORDBG_E_MODULE_NOT_LOADED));

    EXPECT_EQ(dbg_primitive.SetValue(&generic_value_mock),
              CORDBG_E_MODULE_NOT_LOADED);
  }
}

}  // namespace google_cloud_debugger_test
