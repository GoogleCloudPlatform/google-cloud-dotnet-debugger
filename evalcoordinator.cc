// Copyright 2015-2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#include "evalcoordinator.h"

#include <iostream>
#include <memory>
#include <thread>

#include "variablemanager.h"

namespace google_cloud_debugger {
using std::unique_ptr;
using std::cerr;
using std::mutex;
using std::lock_guard;
using std::unique_lock;

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
      condition_variable_.notify_one();
      condition_variable_.wait(lk);
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
  condition_variable_.notify_one();

  // finished_printing_variables_ is set to true if the VariableManager
  // decides to call SignalFinishedPrintingVariable to signal that it has
  // finished printing the variables.
  // debuggercallback_can_continue_ is set to true if the VariableManager
  // makes another evaluation by calling WaitForEval.
  condition_variable_.wait(lk, [&] {
    return debuggercallback_can_continue_ || finished_printing_variables_;
  });

  if (finished_printing_variables_) {
    if (variable_thread_.joinable()) {
      variable_thread_.join();
    }
  }
}

HRESULT EvalCoordinator::PrintLocalVariables(ICorDebugValueEnum *local_enum,
                                             ICorDebugThread *debug_thread) {
  unique_ptr<VariableManager> manager(new (std::nothrow) VariableManager);
  if (!manager) {
    cerr << "Failed to create VariableManager.";
    return E_FAIL;
  }

  HRESULT hr = manager->PopulateLocalVariable(local_enum);
  if (FAILED(hr)) {
    return hr;
  }

  ready_to_print_variables_ = FALSE;
  debuggercallback_can_continue_ = FALSE;
  finished_printing_variables_ = FALSE;

  variable_thread_ = std::thread(
      [](unique_ptr<VariableManager> variable_manager,
         EvalCoordinator *eval_coordinator) {
        // TODO(quoct): Add logic to let the main thread know about this hr.
        HRESULT hr = variable_manager->PrintVariables(eval_coordinator);
        if (FAILED(hr)) {
          cerr << "Failed to print out variables: " << hr;
        }
      },
      std::move(manager), this);

  unique_lock<mutex> lk(mutex_);

  ready_to_print_variables_ = TRUE;
  debuggercallback_can_continue_ = FALSE;
  active_debug_thread_ = debug_thread;

  // Notify the VariableManager we are ready.
  condition_variable_.notify_one();

  // The VariableManager in active_debug_thread_ will have to set
  // debuggerCallBackCanContinue to TRUE by either calling WaitForEval
  // or SignalFinishPrintingVariable.
  condition_variable_.wait(lk, [&] {
    return debuggercallback_can_continue_ || finished_printing_variables_;
  });

  // Got our lock back!
  if (finished_printing_variables_) {
    // The thread can either be joined here (if no evaluation occurs)
    // or in SignalFinishedEval if an evaluation occurs.
    if (variable_thread_.joinable()) {
      variable_thread_.join();
    }
  }

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
    condition_variable_.wait(lk, [&] { return ready_to_print_variables_; });
  }
}

void EvalCoordinator::SignalFinishedPrintingVariable() {
  {
    lock_guard<mutex> lk(mutex_);

    debuggercallback_can_continue_ = TRUE;
    finished_printing_variables_ = TRUE;
  }
  condition_variable_.notify_one();
}

}  //  namespace google_cloud_debugger
