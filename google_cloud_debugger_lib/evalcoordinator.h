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

#ifndef EVAL_COORDINATOR_H_
#define EVAL_COORDINATOR_H_

#include "i_evalcoordinator.h"

namespace google_cloud_debugger {

// TODO(quoct): Add a switch to turn off function evaluation by default.
// Also, we have to investigate function evaluation for multi-threading case.
//
// An EvalCoordinator object is used by DebuggerCallback object to evaluate
// and print out variables. It does so by creating a StackFrame on a new
// thread and coordinates between the StackFrame and DebuggerCallback.
//
// We need an EvalCoordinator for coordination because if we want to print
// out properties and perform function evaluation, we would have to do it
// from a different thread. This is because for an evaluation to succeed,
// the DebuggerCallback object has to call ICorDebugController->Continue
// method and return control to the debuggee by returning from whatever
// callback it is in.
//
// For example, if the DebuggerCallback is in the Break
// callback method when it uses EvalCoordinator to print out variables,
// then it will have to call appdomain->Continue(FALSE) and exits the method.
// When the evaluation is finished, the EvalComplete or EvalException callback
// of DebuggerCallback class will be invoked and that is when we know that
// the evaluation has finished.
//
// For this reason, we have to do the variable enumeration and value
// inspection on a different thread than the thread that the DebuggerCallback
// is on. Otherwise, the DebuggerCallback thread will be blocked and
// cannot perform evaluation.
class EvalCoordinator : public IEvalCoordinator {
 public:
  // This method is used to create an ICorDebugEval object
  // from the active thread.
  HRESULT CreateEval(ICorDebugEval **eval);

  // StackFrame calls this to get evaluation result.
  // This method will block until an evaluation is complete.
  HRESULT WaitForEval(BOOL *exception_thrown, ICorDebugEval *eval,
                      ICorDebugValue **eval_result);

  // DebuggerCallback calls this function to signal that an evaluation is
  // finished.
  void SignalFinishedEval(ICorDebugThread *debug_thread);

  // DebuggerCallback calls this function to signal that an exception has
  // occurred.
  void HandleException();

  // Prints out the stack frames at DbgBreakpoint breakpoint based on
  // debug_stack_walk.
  HRESULT PrintBreakpoint(
      ICorDebugStackWalk *debug_stack_walk, ICorDebugThread *debug_thread,
      BreakpointCollection *breakpoint_collection, DbgBreakpoint *breakpoint,
      const std::vector<
          std::unique_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
          &pdb_files);

  // StackFrame calls this to signal that it already processed all the
  // variables and it is just waiting to perform evaluation (if necessary) and
  // print them out.
  void WaitForReadySignal();

  // StackFrame calls this to signal to the DebuggerCallback that it
  // finished all the evaluation.
  void SignalFinishedPrintingVariable();

  // Returns the active debug thread.
  HRESULT GetActiveDebugThread(ICorDebugThread **debug_thread);

  // Returns true if we are waiting for an evaluation result.
  BOOL WaitingForEval();

  // Sets this to stop property evaluation.
  void SetPropertyEvaluation(BOOL eval) { property_evaluation_ = eval; }

  // Returns whether property evaluation should be performed.
  BOOL PropertyEvaluation() { return property_evaluation_; }

 private:
  // If sets to true, object evaluation will not be performed.
  BOOL property_evaluation_ = FALSE;

  // The threads that we are enumerating and printing the stack frames from.
  std::vector<std::thread> stack_frames_threads_;

  // The ICorDebugThread that the active StackFrame is on.
  CComPtr<ICorDebugThread> active_debug_thread_;

  // variable_thread_ and the thread that DebuggerCallback object is on
  // will use this condition_variable_ and mutex_ to communicate.
  std::condition_variable variable_threads_cv_;

  std::condition_variable debugger_callback_cv_;

  std::mutex mutex_;

  BOOL ready_to_print_variables_ = FALSE;
  BOOL debuggercallback_can_continue_ = FALSE;
  BOOL eval_exception_occurred_ = FALSE;
  BOOL waiting_for_eval_ = FALSE;
};

}  //  namespace google_cloud_debugger

#endif  // EVAL_COORDINATOR_H_
