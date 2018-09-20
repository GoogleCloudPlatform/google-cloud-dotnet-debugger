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

#ifndef I_STACK_FRAME_COLLECTION_H_
#define I_STACK_FRAME_COLLECTION_H_

#include <unordered_map>
#include <vector>

#include "breakpoint.pb.h"
#include "cor.h"
#include "cordebug.h"
#include "i_portable_pdb_file.h"

namespace google_cloud_debugger {

class IEvalCoordinator;
class DbgBreakpoint;
class DbgObject;

class IStackFrameCollection {
 public:
  virtual ~IStackFrameCollection() = default;

  // This function first checks whether breakpoint has a condition.
  // If the condition evaluated to false, do nothing.
  // If there is no condition or the condition evaluated to true,
  // any expressions in the breakpoint will be evaluated.
  // Afterwards, stack information will be collected at the
  // breakpoint's location.
  virtual HRESULT ProcessBreakpoint(
      const std::vector<
          std::shared_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
          &pdb_files,
      DbgBreakpoint *breakpoint, IEvalCoordinator *eval_coordinator) = 0;

  // Populates the stack frames of a breakpoint using stack_frames.
  // eval_coordinator will be used to perform eval coordination during function
  // evaluation if needed.
  virtual HRESULT PopulateStackFrames(
      google::cloud::diagnostics::debug::Breakpoint *breakpoint,
      IEvalCoordinator *eval_coordinator) = 0;
};

}  //  namespace google_cloud_debugger

#endif  // I_STACK_FRAME_COLLECTION_H_
