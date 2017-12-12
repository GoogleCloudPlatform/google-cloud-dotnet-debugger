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

#include "debugger_callback.h"

#include <stdio.h>

#include <atomic>
#include <iostream>
#include <string>

#include "breakpoint_collection.h"
#include "ccomptr.h"
#include "constants.h"
#include "dbg_stack_frame.h"
#include "i_cor_debug_helper.h"
#include "portable_pdb_file.h"
#include "eval_coordinator.h"

using google_cloud_debugger_portable_pdb::IPortablePdbFile;
using google_cloud_debugger_portable_pdb::PortablePdbFile;
using std::cerr;
using std::cout;
using std::string;
using std::vector;

namespace google_cloud_debugger {

HRESULT DebuggerCallback::Initialize() {
  if (initialized_success_) {
    return S_OK;
  }

  // TODO(quoct): We are compiling with C++11 on Linux so we don't have
  // make_unique. We should look into upgrading to C++14.
  eval_coordinator_ =
      std::unique_ptr<EvalCoordinator>(new (std::nothrow) EvalCoordinator);
  breakpoint_collection_ = std::unique_ptr<IBreakpointCollection>(
      new (std::nothrow) BreakpointCollection);
  if (!eval_coordinator_) {
    cerr << "Failed to create EvalCoordinator.";
    return E_OUTOFMEMORY;
  }

  HRESULT hr = breakpoint_collection_->SetDebuggerCallback(this);
  if (FAILED(hr)) {
    cerr << "Breakpoint collection failed to initialize.";
    return hr;
  }

  initialized_success_ = true;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE DebuggerCallback::QueryInterface(REFIID riid,
                                                           void **object) {
  if (riid == __uuidof(ICorDebugManagedCallback)) {
    *object = static_cast<ICorDebugManagedCallback *>(this);
    AddRef();
    return S_OK;
  }

  if (riid == __uuidof(ICorDebugManagedCallback2)) {
    *object = static_cast<ICorDebugManagedCallback2 *>(this);
    AddRef();
    return S_OK;
  }

  if (riid == __uuidof(ICorDebugManagedCallback3)) {
    *object = static_cast<ICorDebugManagedCallback3 *>(this);
    AddRef();
    return S_OK;
  }

  if (riid == __uuidof(IUnknown)) {
    *object = static_cast<ICorDebugManagedCallback *>(this);
    AddRef();
    return S_OK;
  }

  *object = nullptr;
  return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE DebuggerCallback::AddRef(void) {
  // atomic_fetch_add will return the value that ref_count_ held previously.
  ULONG previous_count = std::atomic_fetch_add(&ref_count_, 1u);
  return previous_count + 1;
}

ULONG STDMETHODCALLTYPE DebuggerCallback::Release(void) {
  // atomic_fetch_sub will return the value that ref_count_ held previously.
  ULONG count = std::atomic_fetch_sub(&ref_count_, 1u) - 1;
  if (count <= 0) {
    delete this;
  }
  return count;
}

HRESULT STDMETHODCALLTYPE DebuggerCallback::Breakpoint(
    ICorDebugAppDomain *appdomain, ICorDebugThread *debug_thread,
    ICorDebugBreakpoint *debug_breakpoint) {
  // If a function evaluation is going on, we don't hit breakpoint.
  // Otherwise, this can lead to infinite loop situation. For example,
  // if a user sets a breakpoint in a getter method of property X and we
  // performs function evaluation to get property X, this breakpoint will
  // be hit. However, since we are filling up stack frames at a breakpoint,
  // this means that the frame of the caller of the breakpoint will be hit. When
  // we evaluate the caller frame of the breakpoint, we will then have to
  // evaluate property X again, leading to a loop.
  //
  // Visual Studio also seems to skip a breakpoint if it is hit during function
  // evaluation.
  if (eval_coordinator_->WaitingForEval()) {
    return appdomain->Continue(FALSE);
  }

  // We will get the IL frame to enumerate and print out all local variables.
  HRESULT hr;
  CComPtr<IMetaDataImport> metadata_import;
  CComPtr<ICorDebugThread3> debug_thread3;
  CComPtr<ICorDebugStackWalk> debug_stack_walk;

  hr = debug_thread->QueryInterface(__uuidof(ICorDebugThread3),
                                    reinterpret_cast<void **>(&debug_thread3));
  if (FAILED(hr)) {
    cerr << "Failed to cast ICorDebugThread to ICorDebugThread3.";
    appdomain->Continue(FALSE);
    return hr;
  }

  mdMethodDef function_token;
  ULONG32 il_offset = 0;
  hr = GetFunctionTokenAndILOffset(debug_breakpoint, &function_token,
                                   &il_offset, &metadata_import);
  if (FAILED(hr)) {
    cerr << "Failed to get function token and IL Offset from breakpoint.";
    appdomain->Continue(FALSE);
    return hr;
  }

  hr = debug_thread3->CreateStackWalk(&debug_stack_walk);
  if (FAILED(hr)) {
    cerr << "Failed to create stack walk.";
    appdomain->Continue(FALSE);
    return hr;
  }

  hr = breakpoint_collection_->EvaluateAndPrintBreakpoint(
      function_token, il_offset, eval_coordinator_.get(), debug_thread,
      debug_stack_walk, portable_pdbs_);
  if (FAILED(hr)) {
    cerr << "Failed to get stack frame's information.";
    appdomain->Continue(FALSE);
    return hr;
  }

  return appdomain->Continue(FALSE);
}

HRESULT STDMETHODCALLTYPE
DebuggerCallback::Exception(ICorDebugAppDomain *appdomain,
                            ICorDebugThread *debug_thread, BOOL unhandled) {
  eval_coordinator_->HandleException();
  return appdomain->Continue(FALSE);
}

HRESULT STDMETHODCALLTYPE DebuggerCallback::EvalComplete(
    ICorDebugAppDomain *appdomain, ICorDebugThread *debug_thread,
    ICorDebugEval *eval) {
  // FinishEval method will signal to the waiting thread that we completed
  // the function evaluation.
  eval_coordinator_->SignalFinishedEval(debug_thread);
  return appdomain->Continue(FALSE);
}

HRESULT STDMETHODCALLTYPE DebuggerCallback::EvalException(
    ICorDebugAppDomain *appdomain, ICorDebugThread *debug_thread,
    ICorDebugEval *eval) {
  eval_coordinator_->HandleException();
  eval_coordinator_->SignalFinishedEval(debug_thread);
  return appdomain->Continue(FALSE);
}

HRESULT DebuggerCallback::LoadModule(ICorDebugAppDomain *appdomain,
                                     ICorDebugModule *debug_module) {
  std::unique_ptr<IPortablePdbFile> portable_pdb(new (std::nothrow)
                                                     PortablePdbFile());
  if (!portable_pdb) {
    cerr << "Cannot create PortablePdbFile object.";
    appdomain->Continue(FALSE);
    return E_OUTOFMEMORY;
  }

  HRESULT hr = portable_pdb->Initialize(debug_module);
  if (FAILED(hr)) {
    cerr << "Failed set debug module for PortablePdbFile.";
    return appdomain->Continue(FALSE);
  }

  portable_pdbs_.push_back(std::move(portable_pdb));

  return appdomain->Continue(FALSE);
}

HRESULT STDMETHODCALLTYPE DebuggerCallback::CustomNotification(
    ICorDebugThread *debug_thread, ICorDebugAppDomain *appdomain) {
  return appdomain->Continue(FALSE);
}

HRESULT DebuggerCallback::GetFunctionTokenAndILOffset(
    ICorDebugBreakpoint *debug_breakpoint, mdMethodDef *function_token,
    ULONG32 *il_offset, IMetaDataImport **metadata_import) {
  CComPtr<ICorDebugFunctionBreakpoint> function_breakpoint;
  CComPtr<ICorDebugFunction> debug_function;

  HRESULT hr = debug_breakpoint->QueryInterface(
      __uuidof(ICorDebugFunctionBreakpoint),
      reinterpret_cast<void **>(&function_breakpoint));
  if (FAILED(hr)) {
    cerr << "Failed to get ICorDebugFunctionBreakpoint.";
    return hr;
  }

  hr = function_breakpoint->GetFunction(&debug_function);
  if (FAILED(hr)) {
    cerr << "Failed to get ICorDebugFunction.";
    return hr;
  }

  hr = debug_function->GetToken(function_token);
  if (FAILED(hr)) {
    cerr << "Failed to get function token.";
    return hr;
  }

  hr = function_breakpoint->GetOffset(il_offset);
  if (FAILED(hr)) {
    cerr << "Failed to get function offset.";
    return hr;
  }

  CComPtr<ICorDebugModule> debug_module;

  hr = debug_function->GetModule(&debug_module);
  if (FAILED(hr)) {
    cerr << "Failed to get debug module from ICorDebugFunction.";
    return hr;
  }

  hr = GetMetadataImportFromICorDebugModule(debug_module, metadata_import,
                                            &cerr);
  if (FAILED(hr)) {
    return hr;
  }

  return S_OK;
}
}  //  namespace google_cloud_debugger
