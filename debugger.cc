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

#include "debugger.h"

#include <stdio.h>
#include <iostream>
#include <vector>

#include "cor.h"
#include "cordebug.h"
#include "dbgshim.h"
#include "debuggercallback.h"

#ifdef PLATFORM_UNIX
// PAL is Platform Adaptation Layer which provides an abstraction
// layer between the runtime and the operating system.
// .NET Core uses this header when compiling on Linux.
#include "pal.h"
#endif

#pragma comment(lib, "dbgshim")

namespace google_cloud_debugger {

using std::cerr;
using std::cout;
using std::string;
using std::vector;

Debugger::~Debugger() {
  HRESULT hr;
  // Stop and detach the debugger.
  if (cordebug_process_) {
    hr = cordebug_process_->Stop(-1);
    DeactivateBreakpoints();

    // TODO(quoct): We may have to drain callback event queue,
    // remove function evaluations.
    hr = cordebug_process_->Detach();
  }

  if (cordebug_) {
    hr = cordebug_->Terminate();
  }
}

HRESULT Debugger::SyncBreakpoints() {
  if (!debugger_callback_) {
    cerr << "Debugger callback does not exist.";
    return E_FAIL;
  }

  return debugger_callback_->SyncBreakpoints();
}

HRESULT Debugger::StartDebugging(DWORD processId) {
  HRESULT hr;

  if (!debugger_callback_) {
    debugger_callback_ = new (std::nothrow) DebuggerCallback();
  }

  if (!debugger_callback_) {
    cerr << "Failed to create debugger_callback_";
    return E_FAIL;
  }

  hr = debugger_callback_->Initialize();
  if (FAILED(hr)) {
    cerr << "Failed to initialize debugger_callback_.";
    return hr;
  }

  // Using the processId, we register for debugging. If the process is ready,
  // it will call the CallbackFunction that we passed to
  // RegisterForRuntimeStartup.
  proc_id_ = processId;
  return RegisterForRuntimeStartup(proc_id_, CallbackFunction, this,
                                   &unregister_token_);
}

void Debugger::CallbackFunction(IUnknown *unknown, void *parameter,
                                HRESULT hr) {
  // CallbackFunction is invoked from dbgshim, which if failed
  // will pass in a failure HRESULT which we acknowledge.
  if (FAILED(hr)) {
    // TODO(quoct): Look into using google3's logging library.
    cout << "Failed to register for runtime startup.";
    return;
  }

  Debugger *debugger = reinterpret_cast<Debugger *>(parameter);
  DWORD process_id = debugger->proc_id_;
  void *unregisterToken = debugger->unregister_token_;

  UnregisterForRuntimeStartup(unregisterToken);

  hr = unknown->QueryInterface(
      __uuidof(ICorDebug), reinterpret_cast<void **>(&(debugger->cordebug_)));
  if (FAILED(hr)) {
    cerr << "Failed to query ICorDebug interface.";
    return;
  }

  ICorDebug *pCorDebug = debugger->cordebug_;
  // Initialize has to be called before we do anything.
  hr = pCorDebug->Initialize();
  if (FAILED(hr)) {
    cerr << "Failed to initialize the ICorDebug object.";
    return;
  }

  // Whenever an interesting event happens (breakpoint, function
  // enter, exit, etc.), the debugger will call the corresponding
  // callback function in DebuggerCallback.
  hr = pCorDebug->SetManagedHandler(debugger->debugger_callback_);
  if (FAILED(hr)) {
    cerr << "Failed to set managed callback for debugger.";
    return;
  }

  hr = pCorDebug->DebugActiveProcess(process_id, false,
                                     &(debugger->cordebug_process_));
  if (FAILED(hr)) {
    cerr << "Failed to debug process " << process_id;
    return;
  }

  debugger->debugger_callback_->SetDebugProcess(debugger->cordebug_process_);
}

void Debugger::DeactivateBreakpoints() {
  // Here, we enumerate all the domains, retrieve the breakpoints in all
  // the domains and deactivate the breakpoints.
  CComPtr<ICorDebugAppDomainEnum> appdomain_enum;
  HRESULT hr = cordebug_process_->EnumerateAppDomains(&appdomain_enum);
  if (FAILED(hr)) {
    cerr << "Failed to enumerate app domains: " << std::hex << hr;
    return;
  }

  vector<CComPtr<ICorDebugAppDomain>> appdomains;
  hr = DebuggerCallback::EnumerateICorDebugSpecifiedType<
      ICorDebugAppDomainEnum, ICorDebugAppDomain>(appdomain_enum,
                                                  &appdomains);

  if (FAILED(hr)) {
    cerr << "Failed to enumerate app domains: " << std::hex << hr;
    return;
  }

  for (auto &&appdomain : appdomains) {
    CComPtr<ICorDebugBreakpointEnum> breakpoint_enum;
    hr = appdomain->EnumerateBreakpoints(&breakpoint_enum);
    if (FAILED(hr)) {
      cerr << "Failed to enumerate breakpoints: " << std::hex << hr;
      return;
    }

    vector<CComPtr<ICorDebugBreakpoint>> breakpoints;
    hr = DebuggerCallback::EnumerateICorDebugSpecifiedType<
        ICorDebugBreakpointEnum, ICorDebugBreakpoint>(breakpoint_enum,
                                                      &breakpoints);

    if (FAILED(hr)) {
      cerr << "Failed to enumerate breakpoints: " << std::hex << hr;
      return;
    }

    for (auto &&breakpoint : breakpoints) {
      hr = breakpoint->Activate(FALSE);
      if (FAILED(hr)) {
        cerr << "Failed to deactivate breakpoint: " << std::hex << hr;
      }
    }
  }
}

}  // namespace google_cloud_debugger
