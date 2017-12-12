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
#include <vector>

#include "ccomptr.h"
#include "common_action_mocks.h"
#include "debugger_callback.h"
#include "i_cor_debug_mocks.h"
#include "i_eval_coordinator_mock.h"
#include "i_metadata_import_mock.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using google_cloud_debugger::CComPtr;
using google_cloud_debugger::ConvertStringToWCharPtr;
using google_cloud_debugger::DebuggerCallback;
using std::string;
using std::vector;

namespace google_cloud_debugger_test {

// Test Fixture for DebuggerCallback.
// Contains various ICorDebug mock objects needed.
class DebuggerCallbackTest : public ::testing::Test {
 protected:
  // Sets up mock calls for QueryInterface calls.
  virtual void SetUp() {
    callback = new DebuggerCallback("pipe-name");

    ON_CALL(debug_breakpoint_mock_, QueryInterface(_, _))
        .WillByDefault(
            DoAll(SetArgPointee<1>(&debug_breakpoint_mock_), Return(S_OK)));

    ON_CALL(metadata_import_, QueryInterface(_, _))
        .WillByDefault(
            DoAll(SetArgPointee<1>(&metadata_import_), Return(S_OK)));

    ON_CALL(debug_thread_mock_, QueryInterface(__uuidof(ICorDebugThread3), _))
        .WillByDefault(
            DoAll(SetArgPointee<1>(&debug_thread_3_mock_), Return(S_OK)));
  }

  // Sets up mock calls for Breakpoint() callback function.
  virtual void SetUpBreakpoint() {
    EXPECT_CALL(app_domain_mock_, Continue(FALSE))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(S_OK));

    EXPECT_CALL(debug_breakpoint_mock_, GetFunction(_))
        .Times(1)
        .WillRepeatedly(
            DoAll(SetArgPointee<0>(&debug_function_), Return(S_OK)));

    EXPECT_CALL(debug_function_, GetToken(_))
        .Times(1)
        .WillRepeatedly(Return(S_OK));

    EXPECT_CALL(debug_breakpoint_mock_, GetOffset(_))
        .Times(1)
        .WillRepeatedly(Return(S_OK));

    EXPECT_CALL(debug_function_, GetModule(_))
        .Times(1)
        .WillRepeatedly(DoAll(SetArgPointee<0>(&debug_module_), Return(S_OK)));

    EXPECT_CALL(debug_module_, GetMetaDataInterface(_, _))
        .Times(1)
        .WillRepeatedly(
            DoAll(SetArgPointee<1>(&metadata_import_), Return(S_OK)));
  }

  // The debugger callback object being tested.
  CComPtr<DebuggerCallback> callback;

  // The app domain object used by callback functions.
  ICorDebugAppDomainMock app_domain_mock_;

  // Debug thread used by callback functions.
  ICorDebugThreadMock debug_thread_mock_;

  // ICorDebugThread3 extracted out from debug_thread_mock_.
  ICorDebugThread3Mock debug_thread_3_mock_;

  // Breakpoint used by callback functions.
  ICorDebugFunctionBreakpointMock debug_breakpoint_mock_;

  // Function that the breakpoint above is in.
  ICorDebugFunctionMock debug_function_;

  // Module that the function above is in.
  ICorDebugModuleMock debug_module_;

  // MetaDataImport from the module above.
  IMetaDataImportMock metadata_import_;
};

// Tests Breakpoint callback function of DbgArray.
TEST_F(DebuggerCallbackTest, Breakpoint) {
  HRESULT hr = callback->Initialize();
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  SetUpBreakpoint();
  hr = callback->Breakpoint(&app_domain_mock_, &debug_thread_mock_,
                           &debug_breakpoint_mock_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
}

// Tests Breakpoint callback function of DbgArray
// when there is an error.
TEST_F(DebuggerCallbackTest, BreakpointError) {
  HRESULT hr = callback->Initialize();
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  // Makes ICorDebugFunctionBreakpoint returns error when GetFunction is called.
  EXPECT_CALL(debug_breakpoint_mock_, GetFunction(_))
      .WillRepeatedly(Return(CORDBG_E_FUNCTION_NOT_IL));

  // Even if there are error, Continue should still be called.
  EXPECT_CALL(app_domain_mock_, Continue(FALSE))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S_OK));

  hr = callback->Breakpoint(&app_domain_mock_, &debug_thread_mock_,
                           &debug_breakpoint_mock_);
  EXPECT_EQ(hr, CORDBG_E_FUNCTION_NOT_IL);
}

}  // namespace google_cloud_debugger_test
