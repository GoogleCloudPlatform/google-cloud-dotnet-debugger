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

#include "common_action_mocks.h"
#include "i_cordebug_mocks.h"
#include "dbgarray.h"

using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::_;
using google::cloud::diagnostics::debug::Variable;
using google_cloud_debugger::DbgArray;
using std::string;

namespace google_cloud_debugger_test {

// Tests Initialize function of DbgArray.
TEST(DbgArrayTest, Initialize) {
  // The array_type we passed in will be used
  // to determine the type of the element of the array.
  ICorDebugTypeMock array_type;
  ICorDebugTypeMock array_element_type;

  EXPECT_CALL(array_type, GetFirstTypeParameter(_))
    .Times(1)
    .WillRepeatedly(DoAll(SetArgPointee<0>(&array_element_type), Return(S_OK)));

  EXPECT_CALL(array_element_type, GetType(_))
    .WillRepeatedly(DoAll(SetArgPointee<0>(CorElementType::ELEMENT_TYPE_I4), Return(S_OK)));

  ICorDebugArrayValueMock array_value;

  // By default, sets array_value to the second argument
  // whenever QueryInterface is called.
  ON_CALL(array_value, QueryInterface(_, _))
      .WillByDefault(
          DoAll(SetArgPointee<1>(&array_value), Return(S_OK)));

  // If queried for ICorDebugHeapValue2, returns heap_value.
  // This happens when the Initialize function tries to create a strong
  // handle of the array.
  ICorDebugHeapValue2Mock heap_value;
  ON_CALL(array_value, QueryInterface(__uuidof(ICorDebugHeapValue2), _))
    .WillByDefault(DoAll(SetArgPointee<1>(&heap_value), Return(S_OK)));

  // Makes heap_value returns handle_value if CreateHandle is called.
  ICorDebugHandleValueMock handle_value;
  EXPECT_CALL(heap_value, CreateHandle(_, _))
      .Times(1)
      .WillRepeatedly(
          DoAll(SetArgPointee<1>(&handle_value), Return(S_OK)));

  // Initialize function should issue call to get dimensions
  // and ranks of the array.
  EXPECT_CALL(array_value, GetDimensions(_, _))
    .Times(1)
    .WillRepeatedly(Return(S_OK));

  EXPECT_CALL(array_type, GetRank(_))
    .Times(1)
    .WillRepeatedly(DoAll(SetArgPointee<0>(1), Return(S_OK)));

  // Have to make sure that the mock value survives after
  // the destructor of DbgArray is called, otherwise, DbgArray
  // may try to delete value that is obselete.
  {
    DbgArray dbgarray(&array_type, 0);
    dbgarray.Initialize(&array_value, FALSE);
    HRESULT hr = dbgarray.GetInitializeHr();
    EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
  }
}

// Tests error cases for Initialize function of DbgArray.
TEST(DbgArrayTest, InitializeError) {  
  ICorDebugArrayValueMock array_value;

  // Null type.
  {
    DbgArray dbgarray(nullptr, 0);
    dbgarray.Initialize(&array_value, FALSE);
    EXPECT_EQ(dbgarray.GetInitializeHr(), E_INVALIDARG);
  }

  // The array_type we passed in will be used
  // to determine the type of the element of the array.
  ICorDebugTypeMock array_type;
  ICorDebugTypeMock array_element_type;

  // Makes GetFirstTypeParameter returns error.
  {
    DbgArray dbgarray(&array_type, 0);
    EXPECT_CALL(array_type, GetFirstTypeParameter(_))
      .Times(1)
      .WillRepeatedly(Return(CORDBG_E_CONTEXT_UNVAILABLE));
    dbgarray.Initialize(&array_value, FALSE);
    EXPECT_EQ(dbgarray.GetInitializeHr(), CORDBG_E_CONTEXT_UNVAILABLE);
  }

  EXPECT_CALL(array_type, GetFirstTypeParameter(_))
    .WillRepeatedly(DoAll(SetArgPointee<0>(&array_element_type), Return(S_OK)));

  // Returns failure when querying the element type of the array.
  {
    DbgArray dbgarray(&array_type, 0);
    EXPECT_CALL(array_element_type, GetType(_))
      .WillRepeatedly(Return(E_ACCESSDENIED));
    dbgarray.Initialize(&array_value, FALSE);
    EXPECT_EQ(dbgarray.GetInitializeHr(), E_ACCESSDENIED);
  }

  EXPECT_CALL(array_element_type, GetType(_))
    .WillRepeatedly(DoAll(SetArgPointee<0>(CorElementType::ELEMENT_TYPE_I4), Return(S_OK)));

  // Makes GetRank returns error.
  {
    DbgArray dbgarray(&array_type, 0);
    EXPECT_CALL(array_type, GetRank(_))
      .WillRepeatedly(DoAll(SetArgPointee<0>(1), Return(COR_E_SAFEARRAYRANKMISMATCH)));
    dbgarray.Initialize(&array_value, FALSE);
    EXPECT_EQ(dbgarray.GetInitializeHr(), COR_E_SAFEARRAYRANKMISMATCH);
  }

  EXPECT_CALL(array_type, GetRank(_))
    .WillRepeatedly(DoAll(SetArgPointee<0>(1), Return(S_OK)));

  // By default, sets array_value to the second argument
  // whenever QueryInterface is called.
  ON_CALL(array_value, QueryInterface(_, _))
      .WillByDefault(
          DoAll(SetArgPointee<1>(&array_value), Return(S_OK)));

  // Returns error when trying to create a handle for the array.
  {
    DbgArray dbgarray(&array_type, 0);
    ON_CALL(array_value, QueryInterface(__uuidof(ICorDebugHeapValue2), _))
      .WillByDefault(Return(E_NOINTERFACE));
    dbgarray.Initialize(&array_value, FALSE);
    EXPECT_EQ(dbgarray.GetInitializeHr(), E_NOINTERFACE);
  }

  ICorDebugHeapValue2Mock heap_value;
  ON_CALL(array_value, QueryInterface(__uuidof(ICorDebugHeapValue2), _))
    .WillByDefault(DoAll(SetArgPointee<1>(&heap_value), Return(S_OK)));

  // Returns error when trying to create a handle for the array.
  {
    DbgArray dbgarray(&array_type, 0);
    EXPECT_CALL(heap_value, CreateHandle(_, _))
        .Times(1)
        .WillRepeatedly(Return(CORDBG_E_BAD_REFERENCE_VALUE));
    dbgarray.Initialize(&array_value, FALSE);
    EXPECT_EQ(dbgarray.GetInitializeHr(), CORDBG_E_BAD_REFERENCE_VALUE);
  }

  // Makes heap_value returns handle_value if CreateHandle is called.
  ICorDebugHandleValueMock handle_value;
  EXPECT_CALL(heap_value, CreateHandle(_, _))
      .WillRepeatedly(
          DoAll(SetArgPointee<1>(&handle_value), Return(S_OK)));

  // Makes GetDimensions returns incorrect value.
  {
    EXPECT_CALL(array_value, GetDimensions(_, _))
      .Times(1)
      .WillRepeatedly(Return(CORDBG_S_BAD_START_SEQUENCE_POINT));
    DbgArray dbgarray(&array_type, 0);
    dbgarray.Initialize(&array_value, FALSE);
    EXPECT_EQ(dbgarray.GetInitializeHr(), CORDBG_S_BAD_START_SEQUENCE_POINT);
  }
}

}  // namespace google_cloud_debugger_test
