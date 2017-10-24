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

#include "ccomptr.h"
#include "common_action_mocks.h"
#include "debuggercallback.h"
#include "i_cordebug_mocks.h"
#include "i_evalcoordinator_mock.h"
#include "i_metadataimport_mock.h"

using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::_;
using ::testing::AtLeast;
using google::cloud::diagnostics::debug::Variable;
using google_cloud_debugger::CComPtr;
using std::string;

namespace google_cloud_debugger_test {

// Test Fixture for DbgArray.
// Contains various ICorDebug mock objects needed.
class DebuggerCallbackTest : public ::testing::Test {
 protected:
  virtual void SetUp() {}

  virtual void SetUpBreakpoint() {
    EXPECT_CALL(app_domain_mock_, Continue(FALSE))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S_OK));

    ON_CALL(debug_breakpoint_mock_, QueryInterface(_, _))
      .WillByDefault(DoAll(
        SetArgPointee<1>(&debug_breakpoint_mock_),
        Return(S_OK)));

    EXPECT_CALL(debug_breakpoint_mock_, GetFunction(_))
      .Times(1)
      .WillRepeatedly(DoAll(
        SetArgPointee<0>(&debug_function_),
        Return(S_OK)));

    EXPECT_CALL(debug_breakpoint_mock_, GetFunction(_))
      .Times(1)
      .WillRepeatedly(DoAll(
        SetArgPointee<0>(&debug_function_),
        Return(S_OK)));

    EXPECT_CALL(debug_function_, GetToken(_))
      .Times(1)
      .WillRepeatedly(DoAll(
        SetArgPointee<0>(function_token_),
        Return(S_OK)));

    EXPECT_CALL(debug_breakpoint_mock_, GetOffset(_))
      .Times(1)
      .WillRepeatedly(DoAll(
        SetArgPointee<0>(function_offset_),
        Return(S_OK)));

    EXPECT_CALL(debug_function_, GetModule(_))
      .Times(1)
      .WillRepeatedly(DoAll(
        SetArgPointee<0>(&debug_module_),
        Return(S_OK)));

    EXPECT_CALL(debug_module_, GetMetaDataInterface(_, _))
      .Times(1)
      .WillRepeatedly(DoAll(
        SetArgPointee<1>(&metadata_import_),
        Return(S_OK)));

    ON_CALL(metadata_import_, QueryInterface(_, _))
      .WillByDefault(DoAll(
        SetArgPointee<1>(&metadata_import_),
        Return(S_OK)));    
  }

  ICorDebugAppDomainMock app_domain_mock_;

  ICorDebugThreadMock debug_thread_mock_;

  ICorDebugFunctionBreakpointMock debug_breakpoint_mock_;

  ICorDebugFunctionMock debug_function_;

  ICorDebugModuleMock debug_module_;

  IMetaDataImportMock metadata_import_;

  mdMethodDef function_token_ = 100;

  ULONG32 function_offset_ = 10;
};

// Tests Initialize function of DbgArray.
TEST_F(DebuggerCallbackTest, Initialize) {
  
}

}  // namespace google_cloud_debugger_test
