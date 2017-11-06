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

#include "eval_coordinator.h"

#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <thread>

#include "breakpoint.pb.h"
#include "breakpoint_collection.h"
#include "stack_frame_collection.h"

using google::cloud::diagnostics::debug::Breakpoint;
using std::cerr;
using std::chrono::high_resolution_clock;
using std::chrono::minutes;
using std::lock_guard;
using std::mutex;
using std::unique_lock;
using std::unique_ptr;

namespace google_cloud_debugger {

minutes EvalCoordinator::one_minute = minutes(1);

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

  waiting_for_eval_ = TRUE;
  debuggercallback_can_continue_ = TRUE;
  eval_exception_occurred_ = FALSE;
  HRESULT hr = CORDBG_E_FUNC_EVAL_NOT_COMPLETE;
  auto start = high_resolution_clock::now();

  // Wait until evaluation is done.
  while (hr == CORDBG_E_FUNC_EVAL_NOT_COMPLETE ||
         hr == CORDBG_E_PROCESS_NOT_SYNCHRONIZED) {
    auto current = high_resolution_clock::now();
    if (current - start > one_minute) {
      hr = CORDBG_E_FUNC_EVAL_NOT_COMPLETE;
      cerr << "Timed out while trying to evaluate function.";
      break;
    }

    hr = eval->GetResult(eval_result);

    if (hr == CORDBG_E_FUNC_EVAL_NOT_COMPLETE ||
        hr == CORDBG_E_PROCESS_NOT_SYNCHRONIZED) {
      // Wake up the debugger thread to do the evaluation.
      debugger_callback_cv_.notify_one();
      variable_threads_cv_.wait_for(lk, one_minute);
    } else {
      break;
    }
  }

  // We got our lock back!
  // Tells the debugger to chill out until our next eval call or we reach the
  // end.
  debuggercallback_can_continue_ = FALSE;
  waiting_for_eval_ = FALSE;

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

  // finished_printing_variables_ is set to true if the StackFrame
  // decides to call SignalFinishedPrintingVariable to signal that it has
  // finished printing the variables.
  // debuggercallback_can_continue_ is set to true if the StackFrame
  // makes another evaluation by calling WaitForEval.
  debugger_callback_cv_.wait(lk,
                             [&] { return debuggercallback_can_continue_; });
}

HRESULT EvalCoordinator::PrintBreakpoint(
    ICorDebugStackWalk *debug_stack_walk, ICorDebugThread *debug_thread,
    IBreakpointCollection *breakpoint_collection, DbgBreakpoint *breakpoint,
    const std::vector<
        std::unique_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
        &pdb_files) {
  if (!breakpoint) {
    cerr << "Breakpoint is null.";
    return E_INVALIDARG;
  }

  if (!debug_stack_walk) {
    cerr << "Debug stack walk is null.";
    return E_INVALIDARG;
  }

  // Creates and initializes stack frame collection based on the
  // ICorDebugStackWalk object.
  unique_ptr<IStackFrameCollection> stack_frames(new (std::nothrow)
                                                     StackFrameCollection);
  if (!stack_frames) {
    cerr << "Failed to create DbgStack.";
    return E_FAIL;
  }

  HRESULT hr = stack_frames->Initialize(debug_stack_walk, pdb_files);
  if (FAILED(hr)) {
    return hr;
  }

  unique_lock<mutex> lk(mutex_);

  std::future<void> print_breakpoint_task = std::async(
      [](unique_ptr<IStackFrameCollection> stack_frames,
         EvalCoordinator *eval_coordinator,
         IBreakpointCollection *breakpoint_collection,
         DbgBreakpoint *breakpoint) {
        Breakpoint proto_breakpoint;
        HRESULT hr = breakpoint->PopulateBreakpoint(
            &proto_breakpoint, stack_frames.get(), eval_coordinator);
        if (FAILED(hr)) {
          cerr << "Failed to print out variables: " << std::hex << hr;
        }

        hr = breakpoint_collection->WriteBreakpoint(proto_breakpoint);
        if (FAILED(hr)) {
          cerr << "Failed to write breakpoint: " << std::hex << hr;
        }
      },
      std::move(stack_frames), this, breakpoint_collection, breakpoint);
  print_breakpoint_tasks_.push_back(std::move(print_breakpoint_task));

  ready_to_print_variables_ = TRUE;
  debuggercallback_can_continue_ = FALSE;
  active_debug_thread_ = debug_thread;

  // Notify the StackFrame threads we are ready.
  variable_threads_cv_.notify_all();

  // The StackFrame in active_debug_thread_ will have to set
  // debuggerCallBackCanContinue to TRUE by either calling WaitForEval
  // or SignalFinishPrintingVariable.
  debugger_callback_cv_.wait(lk,
                             [&] { return debuggercallback_can_continue_; });

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

BOOL EvalCoordinator::WaitingForEval() {
  lock_guard<mutex> lk(mutex_);
  return waiting_for_eval_;
}

}  //  namespace google_cloud_debugger
