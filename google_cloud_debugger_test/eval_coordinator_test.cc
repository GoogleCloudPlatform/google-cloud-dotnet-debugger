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

#include <gtest/gtest.h>
#include <algorithm>
#include <memory>
#include <string>
#include <chrono>

#include "debugger_callback.h"
#include "i_breakpoint_collection_mock.h"
#include "ccomptr.h"
#include "eval_coordinator.h"
#include "common_action_mocks.h"

using ::testing::DoAll;
using ::testing::Return;
using ::testing::_;
using google::cloud::diagnostics::debug::Breakpoint;
using google_cloud_debugger::EvalCoordinator;
using google_cloud_debugger::CComPtr;
using std::chrono::high_resolution_clock;
using std::chrono::minutes;
using std::chrono::seconds;
using std::string;
using std::unique_ptr;
using std::vector;

namespace google_cloud_debugger_test {

// Test fixture for EvalCoordinator tests.
class EvalCoordinatorTest : public ::testing::Test {
 protected:
  virtual void SetUp() {}

  ICorDebugThreadMock debug_thread_;

  EvalCoordinator eval_coordinator;

  IBreakpointCollectionMock breakpoint_collection_;

  google_cloud_debugger::DbgBreakpoint breakpoint_;

  vector<unique_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>> pdb_files_;

  // The ICorDebugEval being evaluated.
  ICorDebugEvalMock eval_;

  ICorDebugStackWalkMock debug_stack_walk_;

  // The result that eval_ returns.
  CComPtr<ICorDebugValue> eval_result_;

  // BOOL that indicates whether the eval throws error or not.
  BOOL exception_thrown;
};

// Tests that WaitForEval works.
TEST_F(EvalCoordinatorTest, TestWaitForEval) {
  EXPECT_CALL(eval_, GetResult(_)).Times(1);
  HRESULT hr = eval_coordinator.WaitForEval(&exception_thrown, &eval_, &eval_result_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
}

// Tests that WaitForEval returns error if GetResult
// call of ICorDebugEval object returns error.
TEST_F(EvalCoordinatorTest, TestWaitForEvalError) {
  EvalCoordinator eval_coordinator;

  EXPECT_CALL(eval_, GetResult(_)).Times(1).WillOnce(Return(CORDBG_E_BAD_REFERENCE_VALUE));
  HRESULT hr = eval_coordinator.WaitForEval(&exception_thrown, &eval_, &eval_result_);
  EXPECT_EQ(hr, CORDBG_E_BAD_REFERENCE_VALUE);
}

// Tests that WaitForEval will time out after one minute.
TEST_F(EvalCoordinatorTest, TestWaitForEvalTimeOut) {
  // If GetResult returns this, WaitForEval will keep trying until time out.
  EXPECT_CALL(eval_, GetResult(_)).WillRepeatedly(Return(CORDBG_E_FUNC_EVAL_NOT_COMPLETE));
  auto start = high_resolution_clock::now();
  minutes one_minute = minutes(1);

  HRESULT hr = eval_coordinator.WaitForEval(&exception_thrown, &eval_, &eval_result_);
  auto end = high_resolution_clock::now();
  // Checks that the WaitForEval times out after 1 minute.
  EXPECT_TRUE(end - start > one_minute);
  
  EXPECT_EQ(hr, CORDBG_E_FUNC_EVAL_NOT_COMPLETE);
}

// Tests that WaitForEval will time out after one minute.
TEST_F(EvalCoordinatorTest, TestPrintBreakpoint) {
  EXPECT_CALL(debug_stack_walk_, GetFrame(_))
    .WillRepeatedly(Return(S_FALSE));
  EXPECT_CALL(breakpoint_collection_, WriteBreakpoint(_))
    .Times(1)
    .WillRepeatedly(Return(S_OK));
  HRESULT hr = eval_coordinator.PrintBreakpoint(&debug_stack_walk_, &debug_thread_,
    &breakpoint_collection_, &breakpoint_, pdb_files_);

  std::this_thread::sleep_for(minutes(1));
}

}  // namespace google_cloud_debugger_test
