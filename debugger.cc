// Copyright 2015-2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#include "debugger.h"
#include <stdio.h>
#include <iostream>
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

Debugger::~Debugger() {
  // Stop and detach the debugger.
  if (!cordebug_process_) {
    // TODO(quoct): We may have to drain callback event queue,
    // remove function evaluations, breakpoints and steppers.
    cordebug_process_->Stop(-1);
    cordebug_process_->Detach();
  }

  if (!cordebug_) {
    cordebug_->Terminate();
  }
}

HRESULT Debugger::StartDebugging(DWORD processId) {
  HRESULT hr;

  debugger_callback_ = new (std::nothrow) DebuggerCallback();
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
}

}  // namespace google_cloud_debugger
