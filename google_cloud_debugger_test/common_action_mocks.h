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

// This headers contains common actions that tests can use.
namespace google_cloud_debugger_test {

// SetArgPointee does not allow casting so we have to write our own action.
ACTION_P2(SetArg2ToWcharArray, wchar_array, len) {
  memcpy((LPWSTR)arg2, wchar_array, len * sizeof(WCHAR));
}

// SetArgPointee does not allow casting so we have to write our own action.
ACTION_P(SetArg0ToInt32Value, value) { *static_cast<uint32_t*>(arg0) = value; }

ACTION_P(SetArg0ToByteValue, value) { *static_cast<uint8_t*>(arg0) = value; }

}  // namespace google_cloud_debugger_test

#endif  //  COMMON_ACTION_MOCKS_H_
