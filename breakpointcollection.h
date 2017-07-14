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

#include <vector>

#include "dbgbreakpoint.h"

namespace google_cloud_debugger {
class DebuggerCallback;

// Class for managing a collection of breakpoints.
class BreakpointCollection {
 public:
  // The character that is used to split up different parts
  // of a breakpoint string. For example, a breakpoint string is
  // Program.cs:35:breakpoint-id.
  static const std::string kSplit;

  // Delimiter for separating different breakpoint strings.
  static const std::string kDelimiter;

  // Given a breakpoint string, try to parse it and populate
  // a list of breakpoint for this collection.
  // Given a Portable PDB file, try to activate all existing breakpoints.
  // Also set the Debugger Callback field, which is used to get a list of
  // Portable PDB files applicable to this collection.
  bool Initialize(std::string breakpoint_string,
                  DebuggerCallback *debugger_callback);

  // Given a Portable PDB file, try to activate as many breakpoints
  // as possible in the collection.
  // When a breakpoint is activated, the Breakpoint function in
  // DebuggerCallback will be invoked whenever the breakpoint is hit.
  HRESULT ActivateBreakpoints(
      const google_cloud_debugger_portable_pdb::PortablePdbFile &portable_pdb);

  // Given a breakpoint, try to activate it. We first do this by
  // looking through the existing breakpoints and see whether we can find
  // this breakpoint in there. If so, we activate it. If not,
  // we add this to the breakpoints collection and call the private
  // ActivateBreakpointHelper function to activate it.
  HRESULT ActivateBreakpoint(const DbgBreakpoint &breakpoint);

  // Given a breakpoint string, try to parse it and sync the current
  // breakpoint collection. Any breakpoint not in the breakpoint string
  // will be removed from the collection and any breakpoint not in the
  // collection will be added. So the breakpoint string from this function
  // will become the source of truth.
  HRESULT SyncBreakpoints(const std::string &breakpoint_string);

  // Returns all the breakpoints in the collection.
  std::vector<DbgBreakpoint> &GetBreakpoints() { return breakpoints_; }

 private:
  // Parse a breakpoint string and populate breakpoints vector.
  bool ParseBreakpoints(std::string input_string,
                        std::vector<DbgBreakpoint> *breakpoints);

  // Parse input string and stores the result in breakpoint.
  bool ParseBreakpoint(const std::string &input, DbgBreakpoint *breakpoint);

  // The underlying list of breakpoints that this collection manages.
  std::vector<DbgBreakpoint> breakpoints_;

  // Activate a breakpoint in a portable pdb file.
  // This function should only be used if breakpoint is already set, i.e.
  // the TryGetBreakpoint method is called on the breakpoint.
  HRESULT ActivateBreakpointHelper(
      DbgBreakpoint *breakpoint, ICorDebugModule *debug_module,
      IMetaDataImport *metadata_import,
      const google_cloud_debugger_portable_pdb::PortablePdbFile &portable_pdb);

  // Helper function to activate or deactivate an existing breakpoint.
  HRESULT ActivateOrDeactivateExistingBreakpoint(
      const DbgBreakpoint &breakpoint, BOOL activate);

  // Helper function to get type definition token, signature and name
  // of a method (identified using method_def).
  HRESULT GetMethodData(IMetaDataImport *metadata_import, uint32_t method_def,
                        mdTypeDef *type_def, PCCOR_SIGNATURE *signature,
                        std::vector<WCHAR> *method_name);

  // COM Pointer to the DebuggerCallback that this breakpoint collection
  // is associated with. This is used to get the list of Portable PDB Files
  // that the DebuggerCallback object has.
  CComPtr<DebuggerCallback> debugger_callback_;
};

// Returns true if the first string and the second string are equal
// ignoring case.
bool EqualsIgnoreCase(const std::string &first_string,
                      const std::string &second_string);

}  // namespace google_cloud_debugger

#endif
