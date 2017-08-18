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

#include "debuggercallback.h"

#include <stdio.h>

#include <atomic>
#include <iostream>
#include <string>

#include "ccomptr.h"
#include "dbgstackframe.h"

namespace google_cloud_debugger {

using google_cloud_debugger_portable_pdb::PortablePdbFile;
using std::cerr;
using std::cout;
using std::string;
using std::vector;

const string DebuggerCallback::kDllExtension = ".dll";
const string DebuggerCallback::kPdbExtension = ".pdb";

HRESULT DebuggerCallback::Initialize() {
  if (initialized_success) {
    return S_OK;
  }

  // TODO(quoct): We are compiling with C++11 on Linux so we don't have
  // make_unique. We should look into upgrading to C++14.
  eval_coordinator_ =
      std::unique_ptr<EvalCoordinator>(new (std::nothrow) EvalCoordinator);
  breakpoint_collection_ = std::unique_ptr<BreakpointCollection>(
      new (std::nothrow) BreakpointCollection);
  if (!eval_coordinator_) {
    cerr << "Failed to create EvalCoordinator.";
    return E_OUTOFMEMORY;
  }

  HRESULT hr = breakpoint_collection_->Initialize(this);
  if (FAILED(hr)) {
    cerr << "Breakpoint collection failed to initialize.";
    return hr;
  }

  initialized_success = true;
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
  // Otherwise, this can lead to infinite loop situation.
  if (eval_coordinator_->WaitingForEval()) {
    return appdomain->Continue(FALSE);
  }

  // We will get the IL frame to enumerate and print out all local variables.
  HRESULT hr;
  CComPtr<ICorDebugFrame> frame;
  CComPtr<ICorDebugILFrame> il_frame;
  CComPtr<ICorDebugValueEnum> variable_value_enum;
  CComPtr<ICorDebugValueEnum> arg_value_enum;
  CComPtr<IMetaDataImport> metadata_import;
  CComPtr<ICorDebugThread3> debug_thread3;
  CComPtr<ICorDebugStackWalk> debug_stackwalk;

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

  for (auto &&breakpoint : breakpoint_collection_->GetBreakpoints()) {
    if (breakpoint.GetMethodToken() == function_token &&
        il_offset == breakpoint.GetILOffset()) {
      hr = debug_thread3->CreateStackWalk(&debug_stackwalk);
      if (FAILED(hr)) {
        cerr << "Failed to create stack walk.";
        appdomain->Continue(FALSE);
        return hr;
      }

      hr = eval_coordinator_->PrintBreakpointStacks(
          debug_stackwalk, debug_thread, &breakpoint, portable_pdbs_);
      if (FAILED(hr)) {
        cerr << "Failed to get stack frame's information.";
        appdomain->Continue(FALSE);
        return hr;
      }
      else {
        break;
      }
    }
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
  ULONG32 name_len = 0;
  HRESULT hr;
  hr = debug_module->GetName(0, &name_len, nullptr);
  if (FAILED(hr)) {
    cerr << "Failed to get module name with HRESULT" << std::hex << hr;
    appdomain->Continue(FALSE);
    return hr;
  }

  vector<WCHAR> my_name(name_len, 0);
  hr = debug_module->GetName(name_len, &name_len, my_name.data());
  if (FAILED(hr)) {
    cerr << "Failed to get module name with HRESULT" << std::hex << hr;
    appdomain->Continue(FALSE);
    return hr;
  }

  // Removes extra null terminator. We do this so rfind does not include the
  // null terminator.
  if (my_name.back() == 0) {
    my_name.pop_back();
  }

  std::string module_name(my_name.begin(), my_name.end());
  size_t last_dll_extension_pos = module_name.rfind(kDllExtension);
  // We are only interested in dll.
  // Is there other possible extensions?
  if (last_dll_extension_pos != module_name.size() - kDllExtension.size()) {
    cerr << "Only Dlls are supported.";
    appdomain->Continue(FALSE);
    return hr;
  }

  module_name.replace(last_dll_extension_pos, kDllExtension.size(),
                      kPdbExtension);

  std::ifstream file(module_name);
  // No PDB file for this module.
  if (!file) {
    cerr << "Cannot find PDB file for module " << module_name;
    appdomain->Continue(FALSE);
    return hr;
  }

  PortablePdbFile portable_pdb;
  if (!portable_pdb.InitializeFromFile(module_name)) {
    cerr << "Failed to parse pdb file for module " << module_name;
    appdomain->Continue(FALSE);
    return hr;
  }

  hr = portable_pdb.SetDebugModule(debug_module);
  if (FAILED(hr)) {
    cerr << "Failed set debug module for PortablePdbFile.";
    return appdomain->Continue(FALSE);
  }

  // Initialize possible breakpoints with this pdb.
  hr = breakpoint_collection_->InitializeBreakpoints(portable_pdb);
  if (FAILED(hr)) {
    cerr << "Failed to update breakpoint.";
    return appdomain->Continue(FALSE);
  }

  portable_pdbs_.push_back(std::move(portable_pdb));

  return appdomain->Continue(FALSE);
}

HRESULT STDMETHODCALLTYPE DebuggerCallback::CustomNotification(
    ICorDebugThread *debug_thread, ICorDebugAppDomain *appdomain) {
  return appdomain->Continue(FALSE);
}

HRESULT DebuggerCallback::EnumerateAppDomains(
    std::vector<CComPtr<ICorDebugAppDomain>> *result) {
  HRESULT hr;
  CComPtr<ICorDebugAppDomainEnum> appdomain_enum;
  hr = debug_process_->EnumerateAppDomains(&appdomain_enum);
  if (FAILED(hr)) {
    cerr << "Failed to get ICorDebugAppDomainEnum.";
    return hr;
  }

  return DebuggerCallback::EnumerateICorDebugSpecifiedType<
      ICorDebugAppDomainEnum, ICorDebugAppDomain>(appdomain_enum, result);
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
  CComPtr<IUnknown> temp_import;

  hr = debug_function->GetModule(&debug_module);
  if (FAILED(hr)) {
    cerr << "Failed to get debug module from ICorDebugFunction.";
    return hr;
  }

  hr = debug_module->GetMetaDataInterface(IID_IMetaDataImport, &temp_import);
  if (FAILED(hr)) {
    cerr << "Failed to get metadata import from ICorDebugModule.";
    return hr;
  }

  hr = temp_import->QueryInterface(IID_IMetaDataImport,
                                   reinterpret_cast<void **>(metadata_import));
  if (FAILED(hr)) {
    cerr << "Failed to get IMetaDataImport.";
    return hr;
  }

  return S_OK;
}
}  //  namespace google_cloud_debugger
