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

#ifndef I_BREAKPOINT_COLLECTION_H_
#define I_BREAKPOINT_COLLECTION_H_

#include <memory>
#include <mutex>
#include <vector>

#include "breakpoint.pb.h"
#include "breakpoint_client.h"
#include "dbg_breakpoint.h"

namespace google_cloud_debugger {

class DebuggerCallback;
class IEvalCoordinator;

// Class for managing a collection of breakpoints.
class IBreakpointCollection {
 public:
  // Destructor.
  virtual ~IBreakpointCollection() = default;

  // Given a breakpoint string, try to parse it and populate
  // a list of breakpoints for this collection.
  // Given a Portable PDB file, try to activate all existing breakpoints.
  // Also set the Debugger Callback field, which is used to get a list of
  // Portable PDB files applicable to this collection.
  virtual HRESULT Initialize(DebuggerCallback *debugger_callback) = 0;

  // Given a PortablePDBFile object, try to activate as many breakpoints
  // as possible in the collection.
  // When a breakpoint is activated, the Breakpoint function in
  // DebuggerCallback will be invoked whenever the breakpoint is hit.
  // This function should only be called once.
  // Subsequent update for breakpoints should be done with SyncBreakpoints
  // functions.
  virtual HRESULT InitializeBreakpoints(
      const google_cloud_debugger_portable_pdb::IPortablePdbFile &portable_pdb) = 0;

  // Given a breakpoint, try to activate it or deactivate it (based on
  // the Activated() method of the breakpoint). We first do this by
  // looking through the existing breakpoints and see whether we can find
  // this breakpoint in there. If so, we activate (or deactivate) it. If it is
  // not and we need to activate it, we add this to the breakpoints collection
  // and call the private ActivateBreakpointHelper function to activate it.
  // If it is not and we do not need to activate it, simply don't do anything.
  virtual HRESULT ActivateOrDeactivate(const DbgBreakpoint &breakpoint) = 0;

  // Using the breakpoint_client_read_ name pipe, try to read and parse
  // any incoming breakpoints that are written to the named pipe.
  // This method will then try to activate or deactivate these breakpoints.
  virtual HRESULT SyncBreakpoints() = 0;

  // Cancel SyncBreakpoints operation (should be called from another thread).
  virtual HRESULT CancelSyncBreakpoints() = 0;

  // Writes a breakpoint to the named pipe server.
  virtual HRESULT WriteBreakpoint(
      const google::cloud::diagnostics::debug::Breakpoint &breakpoint) = 0;

  // Reads a breakpoint from the named pipe server.
  virtual HRESULT ReadBreakpoint(
      google::cloud::diagnostics::debug::Breakpoint *breakpoint) = 0;

  // Evaluates and prints out the breakpoint in function with token
  // function_token at IL offset il_offset.
  virtual HRESULT EvaluateAndPrintBreakpoint(
      mdMethodDef function_token, ULONG32 il_offset,
      IEvalCoordinator *eval_coordinator, ICorDebugThread *debug_thread,
      ICorDebugStackWalk *debug_stack_walk,
      const std::vector<
          std::unique_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
          &pdb_files) = 0;
};

}  // namespace google_cloud_debugger

#endif
