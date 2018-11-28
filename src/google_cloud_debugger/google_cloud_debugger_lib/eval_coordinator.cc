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
#include "cor_debug_helper.h"
#include "dbg_breakpoint.h"
#include "dbg_class.h"
#include "dbg_object_factory.h"
#include "stack_frame_collection.h"

using google::cloud::diagnostics::debug::Breakpoint;
using std::cerr;
using std::lock_guard;
using std::mutex;
using std::unique_lock;
using std::unique_ptr;
using std::chrono::high_resolution_clock;
using std::chrono::minutes;

namespace google_cloud_debugger {

minutes EvalCoordinator::one_minute = minutes(1);

HRESULT EvalCoordinator::CreateEval(ICorDebugEval **eval) {
  lock_guard<mutex> lk(mutex_);

  if (active_debug_thread_ == nullptr) {
    std::cerr << "Active debug thread is missing";
    return E_FAIL;
  }
  return active_debug_thread_->CreateEval(eval);
}

HRESULT EvalCoordinator::CreateStackWalk(
    ICorDebugStackWalk **debug_stack_walk) {
  if (!active_debug_thread_) {
    cerr << "Active debug thread is missing";
    return E_FAIL;
  }

  HRESULT hr;
  CComPtr<ICorDebugThread3> debug_thread3;

  hr = active_debug_thread_->QueryInterface(
      __uuidof(ICorDebugThread3), reinterpret_cast<void **>(&debug_thread3));
  if (FAILED(hr)) {
    cerr << "Failed to cast ICorDebugThread to ICorDebugThread3.";
    return hr;
  }

  return debug_thread3->CreateStackWalk(debug_stack_walk);
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

HRESULT EvalCoordinator::ProcessBreakpoints(
    ICorDebugThread *debug_thread, IBreakpointCollection *breakpoint_collection,
    std::vector<std::shared_ptr<DbgBreakpoint>> breakpoints,
    const std::vector<
        std::shared_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
        &pdb_files) {
  if (!debug_thread) {
    cerr << "Debug stack walk is null.";
    return E_INVALIDARG;
  }

  active_debug_thread_ = debug_thread;

  unique_lock<mutex> lk(mutex_);

  std::future<HRESULT> print_breakpoint_task = std::async(
      std::launch::async, &EvalCoordinator::ProcessBreakpointsTask, this,
      breakpoint_collection, std::move(breakpoints), pdb_files);
  print_breakpoint_tasks_.push_back(std::move(print_breakpoint_task));

  ready_to_print_variables_ = TRUE;
  debuggercallback_can_continue_ = FALSE;

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
    DbgClass::ClearStaticCache();
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

HRESULT EvalCoordinator::GetActiveDebugFrame(
    ICorDebugILFrame **debug_il_frame) {
  if (!debug_il_frame) {
    return E_INVALIDARG;
  }

  if (active_debug_thread_) {
    CComPtr<ICorDebugFrame> debug_frame;
    HRESULT hr = active_debug_thread_->GetActiveFrame(&debug_frame);
    if (FAILED(hr)) {
      cerr << "Failed to get active frame.";
      return hr;
    }

    return debug_frame->QueryInterface(
        __uuidof(ICorDebugILFrame), reinterpret_cast<void **>(debug_il_frame));
  }

  return E_FAIL;
}

BOOL EvalCoordinator::WaitingForEval() {
  lock_guard<mutex> lk(mutex_);
  return waiting_for_eval_;
}

HRESULT EvalCoordinator::ProcessBreakpointsTask(
    IBreakpointCollection *breakpoint_collection,
    std::vector<std::shared_ptr<DbgBreakpoint>> breakpoints,
    const std::vector<
        std::shared_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
        &pdb_files) {
  // Vector of PDB files that are parsed successfully.
  std::vector<
      std::shared_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
      parsed_pdb_files;
  for (auto &&pdb_file : pdb_files) {
    if (!pdb_file) {
      continue;
    }

    if (pdb_file->ParsePdbFile()) {
      parsed_pdb_files.push_back(pdb_file);
    }
  }

  // Creates and initializes stack frame collection based on the
  // ICorDebugStackWalk object.
  unique_ptr<IStackFrameCollection> stack_frames(
      new (std::nothrow) StackFrameCollection(
          std::shared_ptr<ICorDebugHelper>(new CorDebugHelper()),
          std::shared_ptr<IDbgObjectFactory>(new DbgObjectFactory())));
  if (!stack_frames) {
    cerr << "Failed to create DbgStack.";
    return E_OUTOFMEMORY;
  }

  HRESULT hr = S_OK;
  for (auto &&breakpoint : breakpoints) {
    hr = stack_frames->ProcessBreakpoint(parsed_pdb_files, breakpoint.get(),
                                         this);
    if (FAILED(hr)) {
      std::cerr << "Failed to process breakpoint \"" << breakpoint->GetId()
                << "\" with HRESULT: " << std::hex << hr;
      Breakpoint error_breakpoint;
      error_breakpoint.set_id(breakpoint->GetId());
      error_breakpoint.set_log_point(breakpoint->IsLogPoint());

      SetErrorStatusMessage(&error_breakpoint, breakpoint->GetErrorString());

      hr = breakpoint_collection->WriteBreakpoint(error_breakpoint);
      if (FAILED(hr)) {
        cerr << "Failed to write error breakpoint: " << std::hex << hr;
        break;
      }
      continue;
    }

    if (!breakpoint->GetEvaluatedCondition()) {
      std::cerr << "Breakpoint condition \"" << breakpoint->GetCondition()
                << "\" for breakpoint \"" << breakpoint->GetId()
                << "\" is not met.";
      continue;
    }

    Breakpoint proto_breakpoint;
    hr = breakpoint->PopulateBreakpoint(&proto_breakpoint, stack_frames.get(),
                                        this);
    if (FAILED(hr)) {
      // We should still write the breakpoint to report the error to the user.
      cerr << "Failed to print out variables: " << std::hex << hr;
    }

    hr = breakpoint_collection->WriteBreakpoint(proto_breakpoint);
    if (FAILED(hr)) {
      cerr << "Failed to write breakpoint: " << std::hex << hr;
      break;
    }
  }

  stack_frames.reset();
  SignalFinishedPrintingVariable();
  return hr;
}

}  //  namespace google_cloud_debugger
