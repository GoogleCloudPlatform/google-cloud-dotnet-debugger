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

#ifndef DEBUGGER_H_
#define DEBUGGER_H_

#include <string>

#include "ccomptr.h"
#include "debuggercallback.h"

namespace google_cloud_debugger {

// Debugger class to start debugging a process.
// The debugger will attach to a given process by its PID.
// It will do this by calling the method RegisterForRuntimeStartup
// of the dbgshim library. This method takes a callback function
// which will be invoked (by the library and not the user)
// when the registration is done.
class Debugger final {
 public:
  ~Debugger();

  // Given a process ID, run RegisterForRuntimeStartup
  // (a function from dbgshim) and pass in CallbackFunction as callback
  // function and this debugger as a parameter.
  // If registration is successful, dbgshim will invoke CallbackFunction,
  // which will set the cordebug_, cordebug_process_ and debugger_callback_
  // fields of this debugger.
  HRESULT StartDebugging(DWORD process_id);

  // Reads, parses and activates/deactivates incoming breakpoints from
  // C# debuglet.
  HRESULT SyncBreakpoints();

 private:
  // The unregister token that is used in the callback function to
  // unregister for runtime startup.
  void *unregister_token_;

  // The process that we are debugging.
  DWORD proc_id_;

  // The ICorDebug object set by CallbackFunction.
  CComPtr<ICorDebug> cordebug_;

  // ICorDebugProcess set by CallbackFunction.
  CComPtr<ICorDebugProcess> cordebug_process_;

  // Managed callback that is used by the debugger.
  CComPtr<DebuggerCallback> debugger_callback_;

  // Callback function that is returned when the process is ready.
  // In StartDebugging, we will call RegisterForRuntimeStartup and pass
  // in this CallbackFunction as a callback function. When the process is
  // ready to be debugged, it will call the callback function that we passed
  // in (i.e. this function).
  //
  // The pUnk argument is an ICorDebug object that we can use for debugging.
  // The parameter argument is the parameter that we passed in to
  // RegisterForRuntimeStartup initially.
  // The hr argument will return whether the process can be debugged or not.
  static void CallbackFunction(IUnknown *unknown, void *parameter, HRESULT hr);

  // Helper function to deactivate all breakpoints.
  void DeactivateBreakpoints();
};

}  // namespace google_cloud_debugger

#endif  // DEBUGGER_H_
