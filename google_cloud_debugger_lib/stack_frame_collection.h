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

#include "dbg_stack_frame.h"
#include "i_stack_frame_collection.h"

namespace google_cloud_debugger {

class StackFrameCollection : public IStackFrameCollection {
 public:
  // Using vector of PDB files, we walk through the stack debug_stack_walk at
  // breakpoint breakpoint and try to populate the stack frames vector.
  HRESULT Initialize(
      ICorDebugStackWalk *debug_stack_walk,
      const std::vector<
          std::unique_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
          &pdb_files);

  // Populates the stack frames of a breakpoint using stack_frames.
  // eval_coordinator will be used to perform eval coordination during function
  // evaluation if needed.
  HRESULT PopulateStackFrames(
      google::cloud::diagnostics::debug::Breakpoint *breakpoint,
      IEvalCoordinator *eval_coordinator);

 private:
  // Given a PDB file, this function tries to find the metadata of the function
  // with token target_function_token in the PDB file. If found, this function
  // will populate dbg_stack_frame using the metadata found and the
  // ICorDebugILFrame il_frame object.
  HRESULT PopulateLocalVarsAndMethodArgs(
      mdMethodDef target_function_token, DbgStackFrame *dbg_stack_frame,
      ICorDebugILFrame *il_frame, IMetaDataImport *metadata_import,
      const google_cloud_debugger_portable_pdb::IPortablePdbFile &pdb_files);

  // Populates the module, class and function name of a stack frame
  // using function_token (represents function the frame is in)
  // and IMetaDataImport (from the module the frame is in).
  HRESULT PopulateModuleClassAndFunctionName(DbgStackFrame *dbg_stack_frame,
                                             mdMethodDef function_token,
                                             IMetaDataImport *metadata_import);

  // Vectors of stack frames that this collection owns.
  std::vector<DbgStackFrame> stack_frames_;

  // Number of processed IL frames in stack_frames_.
  int number_of_processed_il_frames_ = 0;

  // Maximum number of stack frames to be parsed.
  static const std::uint32_t kMaximumStackFrames = 20;

  // Maximum number of stack frames with populated variables to be parsed.
  static const std::uint32_t kMaximumStackFramesWithVariables = 4;

  // PopulateStackFrames should not fill up breakpoint proto in
  // PopulateStackFrames with more bytes of information than this number.
  // (65536 bytes = 64kb).
  static const std::uint32_t kMaximumBreakpointSize = 65536;
};

}  //  namespace google_cloud_debugger

#endif  // STACK_FRAME_COLLECTION_H_
