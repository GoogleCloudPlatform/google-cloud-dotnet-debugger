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

#ifndef COMMON_ACTION_MOCKS_H_
#define COMMON_ACTION_MOCKS_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstdint>

#include "i_cordebug_mocks.h"

// This headers contains common actions that tests can use.
namespace google_cloud_debugger_test {

// On Linux, PAL_STDCPP_COMPAT header is used. We have to use
// different string types because WCHAR defined on Linux is
// different than WCHAR defined on Windows.
#ifdef PAL_STDCPP_COMPAT
#define WCHAR_STRING(string) u#string;
#else
#define WCHAR_STRING(string) L#string;
#endif

// SetArgPointee does not allow casting so we have to write our own action.
ACTION_P2(SetArg2ToWcharArray, wchar_array, len) {
  memcpy(const_cast<LPWSTR>(arg2), wchar_array, len * sizeof(WCHAR));
}

// SetArgPointee does not allow casting so we have to write our own action.
ACTION_P(SetArg0ToInt32Value, value) { *static_cast<uint32_t *>(arg0) = value; }

ACTION_P(SetArg0ToByteValue, value) { *static_cast<uint8_t *>(arg0) = value; }

// This action will pop the last string in string_vector
// and make arg0 points to that string.
ACTION_P(ReadFromStringVectorToArg0, string_vector) {
  std::vector<std::string> *string_vector_ptr =
      static_cast<std::vector<std::string> *>(string_vector);
  if (!string_vector_ptr->empty()) {
    *static_cast<std::string *>(arg0) = string_vector_ptr->back();
    string_vector_ptr->pop_back();
  }
}

void SetUpMockGenericValue(ICorDebugGenericValueMock *generic_value,
                           std::int32_t value);

}  // namespace google_cloud_debugger_test

#endif  //  COMMON_ACTION_MOCKS_H_
