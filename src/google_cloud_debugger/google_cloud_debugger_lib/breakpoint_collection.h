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

#ifndef BREAKPOINT_COLLECTION_H_
#define BREAKPOINT_COLLECTION_H_

#include <memory>
#include <mutex>
#include <vector>

#include "breakpoint_client.h"
#include "ccomptr.h"
#include "dbg_breakpoint.h"
#include "i_breakpoint_collection.h"

namespace google_cloud_debugger {

class DebuggerCallback;
class IEvalCoordinator;

// Class for managing a collection of breakpoints.
class BreakpointCollection : public IBreakpointCollection {
 public:
  // The character that is used to split up different parts
  // of a breakpoint string. For example, a breakpoint string is
  // Program.cs:35:breakpoint-id.
  static const std::string kSplit;

  // Delimiter for separating different breakpoint strings.
  static const std::string kDelimiter;

  // Sets the Debugger Callback field, which is used to get a list of
  // Portable PDB files applicable to this collection.
  HRESULT SetDebuggerCallback(DebuggerCallback *debugger_callback) override;

  // Given a breakpoint, try to activate it or deactivate it (based on
  // the Activated() method of the breakpoint). We first do this by
  // looking through the existing breakpoints and see whether we can find
  // this breakpoint in there. If so, we activate (or deactivate) it. If it is
  // not and we need to activate it, we add this to the breakpoints collection
  // and call the private ActivateBreakpointHelper function to activate it.
  // If it is not and we do not need to activate it, simply don't do anything.
  // This means duplicate breakpoints will be silently rejected.
  HRESULT UpdateBreakpoint(const DbgBreakpoint &breakpoint) override;

  // Using the breakpoint_client_read_ name pipe, try to read and parse
  // any incoming breakpoints that are written to the named pipe.
  // This method will then try to activate or deactivate these breakpoints.
  // This method will block and wait until a breakpoint arrives.
  // It will only terminate if the connection to the named pipe server
  // is cut off.
  HRESULT SyncBreakpoints() override;

  // Cancel SyncBreakpoints operation (should be called from another thread).
  HRESULT CancelSyncBreakpoints() override;

  // Writes a breakpoint to the named pipe server.
  HRESULT WriteBreakpoint(
      const google::cloud::diagnostics::debug::Breakpoint &breakpoint) override;

  // Reads a breakpoint from the named pipe server.
  HRESULT ReadBreakpoint(
      google::cloud::diagnostics::debug::Breakpoint *breakpoint) override;

  // Evaluates and prints out the breakpoint that corresponds to
  // the IL offset il_offset inside the function with token
  // function_token.
  HRESULT EvaluateAndPrintBreakpoint(
      mdMethodDef function_token, ULONG32 il_offset,
      IEvalCoordinator *eval_coordinator, ICorDebugThread *debug_thread,
      const std::vector<
          std::shared_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
          &pdb_files) override;

 private:
  // Reads an incoming breakpoint from the named pipe and populates
  // The DbgBreakpoint object based on that.
  HRESULT ReadAndParseBreakpoint(DbgBreakpoint *breakpoint);

  // The underlying list of breakpoints that this collection manages.
  std::vector<std::shared_ptr<DbgBreakpoint>> breakpoints_;

  // Activate a breakpoint in a portable pdb file.
  // This function should only be used if breakpoint is already set, i.e.
  // the TryGetBreakpoint method is called on the breakpoint.
  HRESULT ActivateBreakpointHelper(
      DbgBreakpoint *breakpoint,
      google_cloud_debugger_portable_pdb::IPortablePdbFile *portable_pdb);

  // Helper function to updates an existing breakpoint that
  // has the same ID as breakpoint.
  HRESULT UpdateBpUsingExistingBpSameId(const DbgBreakpoint &breakpoint,
                                        BOOL activate);

  // This function searches for a breakpoint in breakpoints_ with the same
  // location (file name and line number as breakpoint). If found, it will
  // use some existing information in that breakpoint to activate breakpoint.
  HRESULT ActivateBpUsingExistingBpWithSameLocation(DbgBreakpoint *breakpoint);

  // Helper function to get type definition token, signature, virtual address
  // and name of a method (identified using method_def).
  HRESULT GetMethodData(IMetaDataImport *metadata_import, uint32_t method_def,
                        mdTypeDef *type_def, PCCOR_SIGNATURE *signature,
                        ULONG *virtual_address,
                        std::vector<WCHAR> *method_name);

  // Helper function to create and initialize a breakpoint client.
  static HRESULT CreateAndInitializeBreakpointClient(
      std::unique_ptr<BreakpointClient> *client, std::string pipe_name);

  // COM Pointer to the DebuggerCallback that this breakpoint collection
  // is associated with. This is used to get the list of Portable PDB Files
  // that the DebuggerCallback object has.
  CComPtr<DebuggerCallback> debugger_callback_;

  // Named pipe server for reading breakpoints.
  std::unique_ptr<BreakpointClient> breakpoint_client_read_;

  // Named pipe server for writing breakpoints.
  std::unique_ptr<BreakpointClient> breakpoint_client_write_;

  std::mutex mutex_;
};

// Returns true if the first string and the second string are equal
// ignoring case.
bool EqualsIgnoreCase(const std::string &first_string,
                      const std::string &second_string);

}  // namespace google_cloud_debugger

#endif
