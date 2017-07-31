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

#include "evalcoordinator.h"

#include <iostream>
#include <memory>
#include <thread>

#include "dbgbreakpoint.h"
#include "variablemanager.h"

namespace google_cloud_debugger {
using std::cerr;
using std::lock_guard;
using std::mutex;
using std::unique_lock;
using std::unique_ptr;

HRESULT EvalCoordinator::CreateEval(ICorDebugEval **eval) {
  lock_guard<mutex> lk(mutex_);

  if (active_debug_thread_ == nullptr) {
    return E_FAIL;
  }
  return active_debug_thread_->CreateEval(eval);
}

HRESULT EvalCoordinator::WaitForEval(BOOL *exception_thrown,
                                     ICorDebugEval *eval,
                                     ICorDebugValue **eval_result) {
  // Let the debugger continue so we can get back the eval result.
  unique_lock<mutex> lk(mutex_);

  debuggercallback_can_continue_ = TRUE;
  eval_exception_occurred_ = FALSE;
  HRESULT hr = CORDBG_E_FUNC_EVAL_NOT_COMPLETE;

  // Wait until evaluation is done.
  // TODO(quoct): Add a timeout.
  while (hr == CORDBG_E_FUNC_EVAL_NOT_COMPLETE ||
         hr == CORDBG_E_PROCESS_NOT_SYNCHRONIZED) {
    hr = eval->GetResult(eval_result);

    if (hr == CORDBG_E_FUNC_EVAL_NOT_COMPLETE ||
        hr == CORDBG_E_PROCESS_NOT_SYNCHRONIZED) {
      // Wake up the debugger thread to do the evaluation.
      debuggercallback_can_continue_ = TRUE;
      debugger_callback_cv_.notify_one();
      variable_threads_cv_.wait(lk);
    } else {
      break;
    }
  }

  // We got our lock back!
  // Tells the debugger to chill out until our next eval call or we reach the
  // end.
  debuggercallback_can_continue_ = FALSE;

  *exception_thrown = eval_exception_occurred_;
  return hr;
}

void EvalCoordinator::SignalFinishedEval(ICorDebugThread *debug_thread) {
  unique_lock<mutex> lk(mutex_);

  debuggercallback_can_continue_ = FALSE;
  active_debug_thread_ = debug_thread;
  // Wake up all variable threads and so one of them can
  // use the evaluation result.
  variable_threads_cv_.notify_all();

  // finished_printing_variables_ is set to true if the VariableManager
  // decides to call SignalFinishedPrintingVariable to signal that it has
  // finished printing the variables.
  // debuggercallback_can_continue_ is set to true if the VariableManager
  // makes another evaluation by calling WaitForEval.
  debugger_callback_cv_.wait(lk, [&] {
    return debuggercallback_can_continue_;
  });
}

HRESULT EvalCoordinator::PrintLocalVariables(ICorDebugValueEnum *local_enum,
                                             ICorDebugThread *debug_thread,
                                             DbgBreakpoint *breakpoint) {
  if (!breakpoint) {
    cerr << "Breakpoint is null.";
    return E_INVALIDARG;
  }

  if (!local_enum) {
    cerr << "Local variable enum is null.";
    return E_INVALIDARG;
  }

  unique_ptr<VariableManager> manager(new (std::nothrow) VariableManager);
  if (!manager) {
    cerr << "Failed to create VariableManager.";
    return E_FAIL;
  }

  HRESULT hr = manager->PopulateLocalVariable(local_enum, breakpoint);
  if (FAILED(hr)) {
    return hr;
  }

  unique_lock<mutex> lk(mutex_);

  std::thread local_thread = std::thread(
      [](unique_ptr<VariableManager> variable_manager,
         EvalCoordinator *eval_coordinator) {
        // TODO(quoct): Add logic to let the main thread know about this hr.
        HRESULT hr = variable_manager->PrintVariables(eval_coordinator);
        if (FAILED(hr)) {
          cerr << "Failed to print out variables: " << hr;
        }
      },
      std::move(manager), this);
  variable_threads_.push_back(std::move(local_thread));

  ready_to_print_variables_ = TRUE;
  debuggercallback_can_continue_ = FALSE;
  active_debug_thread_ = debug_thread;

  // Notify the VariableManager threads we are ready.
  variable_threads_cv_.notify_all();

  // The VariableManager in active_debug_thread_ will have to set
  // debuggerCallBackCanContinue to TRUE by either calling WaitForEval
  // or SignalFinishPrintingVariable.
  debugger_callback_cv_.wait(lk, [&] {
    return debuggercallback_can_continue_;
  });

  return S_OK;
}

void EvalCoordinator::HandleException() {
  lock_guard<mutex> lk(mutex_);
  eval_exception_occurred_ = TRUE;
}

void EvalCoordinator::WaitForReadySignal() {
  {
    unique_lock<mutex> lk(mutex_);

    // Wait for ready signal from debugger calback.
    variable_threads_cv_.wait(lk, [&] { return ready_to_print_variables_; });
  }
}

void EvalCoordinator::SignalFinishedPrintingVariable() {
  {
    lock_guard<mutex> lk(mutex_);

    debuggercallback_can_continue_ = TRUE;
  }
  debugger_callback_cv_.notify_one();
}

HRESULT EvalCoordinator::GetActiveDebugThread(ICorDebugThread **debug_thread) {
  if (!debug_thread) {
    return E_INVALIDARG;
  }

  if (active_debug_thread_) {
    (*debug_thread) = active_debug_thread_;
    active_debug_thread_->AddRef();
    return S_OK;
  }

  return E_FAIL;
}

}  //  namespace google_cloud_debugger
