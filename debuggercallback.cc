// Copyright 2015-2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#include "debuggercallback.h"

#include <stdio.h>

#include <atomic>
#include <iostream>
#include <string>

#include "ccomptr.h"
#include "evalcoordinator.h"
#include "variablemanager.h"

namespace google_cloud_debugger {

using std::cout;
using std::cerr;

HRESULT DebuggerCallback::Initialize() {
  // TODO(quoct): We are compiling with C++11 on Linux so we don't have
  // make_unique. We should look into upgrading to C++14.
  eval_coordinator_ =
      std::unique_ptr<EvalCoordinator>(new (std::nothrow) EvalCoordinator);
  if (!eval_coordinator_) {
    cerr << "Failed to create EvalCoordinator.";
    return E_OUTOFMEMORY;
  }
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

HRESULT STDMETHODCALLTYPE DebuggerCallback::Break(
    ICorDebugAppDomain *appdomain, ICorDebugThread *debug_thread) {
  // We will get the IL frame to enumerate and print out all local variables.
  HRESULT hr;
  CComPtr<ICorDebugFrame> frame;
  CComPtr<ICorDebugILFrame> il_frame;
  CComPtr<ICorDebugFunction> debug_function;
  CComPtr<ICorDebugValueEnum> value_enum;
  CComPtr<ICorDebugProcess> debug_process;

  hr = debug_thread->GetActiveFrame(&frame);
  if (FAILED(hr)) {
    cerr << "Failed to get active frame.";
    appdomain->Continue(FALSE);
    return hr;
  }

  hr = frame->QueryInterface(__uuidof(ICorDebugILFrame),
                             reinterpret_cast<void **>(&il_frame));
  if (FAILED(hr)) {
    cerr << "Failed to get ILFrame";
    appdomain->Continue(FALSE);
    return hr;
  }

  hr = il_frame->EnumerateLocalVariables(&value_enum);
  if (FAILED(hr)) {
    cerr << "Failed to get local variable.";
    appdomain->Continue(FALSE);
    return hr;
  }

  hr = eval_coordinator_->PrintLocalVariables(value_enum, debug_thread);
  if (FAILED(hr)) {
    cerr << "Failed to print out local variable.";
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

HRESULT STDMETHODCALLTYPE DebuggerCallback::CustomNotification(
    ICorDebugThread *debug_thread, ICorDebugAppDomain *appdomain) {
  return appdomain->Continue(FALSE);
}

}  //  namespace google_cloud_debugger
