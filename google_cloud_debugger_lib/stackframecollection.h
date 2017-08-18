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

#ifndef STACK_FRAME_COLLECTION_H_
#define STACK_FRAME_COLLECTION_H_

#include <vector>

#include "portablepdbfile.h"
#include "breakpointcollection.h"
#include "ccomptr.h"
#include "cor.h"
#include "cordebug.h"
#include "dbgstackframe.h"

namespace google_cloud_debugger {

class StackFrameCollection {
public:
  // Using vector of PDB files, we walk through the stack debug_stack_walk at
  // breakpoint breakpoint and try to populate the stack frames vector.
  HRESULT Initialize(
      DbgBreakpoint *breakpoint,
      ICorDebugStackWalk *debug_stack_walk,
      const std::vector<google_cloud_debugger_portable_pdb::PortablePdbFile> &pdb_files);

  // Prints out the stack frame to the breakpoint pipeline, using
  // eval_coordinator to perform eval coordination if needed.
  HRESULT PrintStackFrames(EvalCoordinator *eval_coordinator);

private:
  // Given a PDB file, this function tries to find the metadata of the function with
  // token target_function_token in the PDB file. If found, this function will
  // populate dbg_stack_frame using the metadata found and the ICorDebugILFrame
  // il_frame object.
  HRESULT PopulateLocalVarsAndMethodArgs(
    mdMethodDef target_function_token,
    DbgStackFrame *dbg_stack_frame,
    ICorDebugILFrame *il_frame,
    const google_cloud_debugger_portable_pdb::PortablePdbFile &pdb_files);

  // Vectors of stack frames that this collection owns.
  std::vector<DbgStackFrame> stack_frames_;

  // Name of the file the variables are in.
  std::string file_name_;

  // The line number where the variables are in.
  std::uint32_t line_number_;

  // The ID of the breakpoint for this StackFrame.
  std::string breakpoint_id_;
};

}  //  namespace google_cloud_debugger

#endif  // DBG_STACK_H_
