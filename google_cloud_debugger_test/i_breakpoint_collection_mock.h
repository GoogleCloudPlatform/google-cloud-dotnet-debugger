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

#ifndef I_BREAKPOINT_COLLECTION_MOCK_H_
#define I_BREAKPOINT_COLLECTION_MOCK_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "breakpoint.pb.h"
#include "i_breakpoint_collection.h"

namespace google_cloud_debugger {
class IPortablePdbFile;
class IEvalCoordinator;
class DbgBreakpoint;
class DebuggerCallback;
};  // namespace google_cloud_debugger

namespace google_cloud_debugger_test {

// Mock class for named pipe.
class IBreakpointCollectionMock
    : public google_cloud_debugger::IBreakpointCollection {
 public:
  MOCK_METHOD1(
      Initialize,
      HRESULT(google_cloud_debugger::DebuggerCallback *debugger_callback));
  MOCK_METHOD1(
      InitializeBreakpoints,
      HRESULT(const google_cloud_debugger_portable_pdb::IPortablePdbFile
                  &portable_pdb));
  MOCK_METHOD1(ActivateOrDeactivate,
               HRESULT(const google_cloud_debugger::DbgBreakpoint &breakpoint));
  MOCK_METHOD0(SyncBreakpoints, HRESULT());
  MOCK_METHOD0(CancelSyncBreakpoints, HRESULT());
  MOCK_METHOD1(
      WriteBreakpoint,
      HRESULT(const google::cloud::diagnostics::debug::Breakpoint &breakpoint));
  MOCK_METHOD1(
      ReadBreakpoint,
      HRESULT(google::cloud::diagnostics::debug::Breakpoint *breakpoint));
  MOCK_METHOD6(
      EvaluateAndPrintBreakpoint,
      HRESULT(mdMethodDef function_token, ULONG32 il_offset,
              google_cloud_debugger::IEvalCoordinator *eval_coordinator,
              ICorDebugThread *debug_thread,
              ICorDebugStackWalk *debug_stack_walk,
              const std::vector<std::unique_ptr<
                  google_cloud_debugger_portable_pdb::IPortablePdbFile>>
                  &pdb_files));
};

}  // namespace google_cloud_debugger_test

#endif  //  I_BREAKPOINT_COLLECTION_MOCK_H_
