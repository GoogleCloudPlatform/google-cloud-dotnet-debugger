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
  StackFrameCollection(std::shared_ptr<ICorDebugHelper> debug_helper,
                       std::shared_ptr<IDbgObjectFactory> obj_factory);

  // This function first checks whether breakpoint has a condition.
  // If the condition evaluated to false, do nothing.
  // If there is no condition or the condition evaluated to true,
  // any expressions in the breakpoint will be evaluated.
  // Afterwards, WalkStackAndProcessStackFrame will be called to
  // populate stack_frames_ vector.
  HRESULT ProcessBreakpoint(
      const std::vector<
          std::shared_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
          &pdb_files,
      DbgBreakpoint *breakpoint, IEvalCoordinator *eval_coordinator);

  // Populates the stack frames of a breakpoint using stack_frames.
  // eval_coordinator will be used to perform eval coordination during function
  // evaluation if needed.
  HRESULT PopulateStackFrames(
      google::cloud::diagnostics::debug::Breakpoint *breakpoint,
      IEvalCoordinator *eval_coordinator);

 private:
  // Class that contains helper method for ICorDebug objects.
  std::shared_ptr<ICorDebugHelper> debug_helper_;

  // Factory for creating DbgObject.
  std::shared_ptr<IDbgObjectFactory> obj_factory_;

  // Given a PDB file, this function tries to find the metadata of the function
  // with token target_function_token in the PDB file. If found, this function
  // will populate dbg_stack_frame using the metadata found and the
  // ICorDebugILFrame il_frame object.
  HRESULT PopulateLocalVarsAndMethodArgs(
      mdMethodDef target_function_token, DbgStackFrame *dbg_stack_frame,
      ICorDebugILFrame *il_frame, IMetaDataImport *metadata_import,
      google_cloud_debugger_portable_pdb::IPortablePdbFile *pdb_files);

  // Populates the module, class and function name of a stack frame
  // using function_token (represents function the frame is in)
  // and IMetaDataImport (from the module the frame is in).
  HRESULT PopulateModuleClassAndFunctionName(DbgStackFrame *dbg_stack_frame,
                                             mdMethodDef function_token,
                                             IMetaDataImport *metadata_import);

  // Helper function to walk the stack, process each frame and store them
  // into stack_frames_. If the stack is already walked, this function will
  // do nothing.
  // IEvalCoordinator eval_coordinator is used to create the stack walk.
  // Parsed_pdb_files vector is needed for mapping each stack frame to a file
  // location.
  HRESULT WalkStackAndProcessStackFrame(
      IEvalCoordinator *eval_coordinator,
      const std::vector<
          std::shared_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
          &parsed_pdb_files);

  // Helper function to evaluate the condition stored in DbgBreakpoint breakpoint.
  // IEvalCoordinator is needed to get the active debug thread and frame.
  // Parsed_pdb_files vector is needed to retrieve local variables names.
  HRESULT EvaluateBreakpointCondition(
      DbgBreakpoint *breakpoint, IEvalCoordinator *eval_coordinator,
      const std::vector<
          std::shared_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
          &parsed_pdb_files);

  // Helper function to process information in ICorDebugFrame debug_frame
  // and initialize DbgStackFrame stack_frame with that information.
  // If process_il_frame is set to true, this function will try to convert
  // debug_frame to an ICorDebugILFrame and retrieve local variables and
  // method arguments from the frame.
  HRESULT PopulateDbgStackFrameHelper(
      const std::vector<
          std::shared_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
          &parsed_pdb_files,
      ICorDebugFrame *debug_frame, DbgStackFrame *stack_frame,
      bool process_il_frame);

  // Vectors of stack frames that this collection owns.
  std::vector<std::shared_ptr<DbgStackFrame>> stack_frames_;

  // The very top stack frame of this collection.
  std::shared_ptr<DbgStackFrame> first_stack_;

  // Number of processed IL frames in stack_frames_.
  int number_of_processed_il_frames_ = 0;

  // True if the stack has been walked and processed.
  // This means stack_frames_ vector should have been populated.
  bool stack_walked = false;

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
