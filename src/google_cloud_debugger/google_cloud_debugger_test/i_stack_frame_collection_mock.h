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

#ifndef I_STACK_FRAME_COLLECTION_MOCK_H_
#define I_STACK_FRAME_COLLECTION_MOCK_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstdint>

#include "i_stack_frame_collection.h"

namespace google_cloud_debugger_test {

// Mock for IPortablePdbFile
class IStackFrameCollectionMock
    : public google_cloud_debugger::IStackFrameCollection {
 public:
  MOCK_METHOD3(
      ProcessBreakpoint,
      HRESULT(
          const std::vector<std::shared_ptr<
              google_cloud_debugger_portable_pdb::IPortablePdbFile>> &pdb_files,
          google_cloud_debugger::DbgBreakpoint *breakpoint,
          google_cloud_debugger::IEvalCoordinator *eval_coordinator));
  MOCK_METHOD2(
      PopulateStackFrames,
      HRESULT(
          google::cloud::diagnostics::debug::Breakpoint *breakpoint,
          google_cloud_debugger::IEvalCoordinator *eval_coordinator));
};

}  // namespace google_cloud_debugger_test

#endif  //  I_STACK_FRAME_COLLECTION_H_
