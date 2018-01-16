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

#include "common_action_mocks.h"

#include "cordebug.h"

using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::_;
using google_cloud_debugger::VariableWrapper;
using std::vector;

namespace google_cloud_debugger_test {

// This function sets up mock calls so that generic_value
// will represent an int object with value value.
void SetUpMockGenericValue(
    ICorDebugGenericValueMock *generic_value, int32_t value) {
  // The generic value is used to create a DbgObject so GetType is called.
  EXPECT_CALL(*generic_value, GetType(_))
      .WillRepeatedly(DoAll(SetArgPointee<0>(CorElementType::ELEMENT_TYPE_I4),
                            Return(S_OK)));

  ON_CALL(*generic_value, QueryInterface(_, _))
      .WillByDefault(Return(E_NOINTERFACE));

  ON_CALL(*generic_value, QueryInterface(__uuidof(ICorDebugGenericValue), _))
      .WillByDefault(DoAll(SetArgPointee<1>(generic_value), Return(S_OK)));

  // When GetValue is called from generic value, returns value as the value.
  EXPECT_CALL(*generic_value, GetValue(_))
      .WillRepeatedly(DoAll(SetArg0ToInt32Value(value), Return(S_OK)));
}

void PopulateTypeAndValue(vector<VariableWrapper> &variable_wrappers) {
  for (auto &wrapper : variable_wrappers) {
    EXPECT_EQ(wrapper.PopulateType(), S_OK);
    EXPECT_EQ(wrapper.PopulateValue(), S_OK);
  }
}

}  // namespace google_cloud_debugger_test
