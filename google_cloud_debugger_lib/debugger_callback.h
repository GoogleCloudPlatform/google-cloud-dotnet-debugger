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

#ifndef DEBUGGERCALLBACK_H_
#define DEBUGGERCALLBACK_H_

#include <atomic>
#include <iostream>
#include <memory>

#include "i_breakpoint_collection.h"
#include "cor.h"
#include "cordebug.h"
#include "corsym.h"
#include "i_eval_coordinator.h"

namespace google_cloud_debugger {

class BreakpointClient;

// A DebuggerCallback object is used to set the managed handler of an ICorDebug
// interface. Whenever an interesting event happens, the ICorDebug object
// will fire the corresponding callback method. For example, if a breakpoint
// is reached, the Breakpoint callback method will be called. All the callback
// methods are defined in ICorDebugManagedCallback interface.
class DebuggerCallback final : public ICorDebugManagedCallback,
                               ICorDebugManagedCallback2,
                               ICorDebugManagedCallback3 {
 public:
  DebuggerCallback(std::string pipe_name) : pipe_name_(pipe_name) {} 
  HRESULT Initialize();

  // IUnknown interface.
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **object) override;
  ULONG STDMETHODCALLTYPE AddRef(void) override;
  ULONG STDMETHODCALLTYPE Release(void) override;

  // ICorDebugManagedCallback interface.
  HRESULT STDMETHODCALLTYPE
  Breakpoint(ICorDebugAppDomain *appdomain, ICorDebugThread *debug_thread,
             ICorDebugBreakpoint *breakpoint) override;
  // This method is called when an exception is thrown by the debuggee.
  HRESULT STDMETHODCALLTYPE Exception(ICorDebugAppDomain *appdomain,
                                      ICorDebugThread *debug_thread,
                                      BOOL unhandled) override;
  // This method is called when a function evaluation is completed.
  HRESULT STDMETHODCALLTYPE EvalComplete(ICorDebugAppDomain *appdomain,
                                         ICorDebugThread *debug_thread,
                                         ICorDebugEval *eval) override;
  // This method is called when a function evaluation thows an exception.
  HRESULT STDMETHODCALLTYPE EvalException(ICorDebugAppDomain *appdomain,
                                          ICorDebugThread *debug_thread,
                                          ICorDebugEval *eval) override;

  // This method is called when a module is loaded.
  HRESULT STDMETHODCALLTYPE LoadModule(ICorDebugAppDomain *appdomain,
                                       ICorDebugModule *debug_module) override;

// This macro creates a callback function stub to override methods in
// ICorDebugManagedCallback and ICorDebugManagedCallback2 interfaces
// that we are not interested in processing.
//
// The function created will have the name methodname.
// It will have first argument of type debugcontrollertypename.
// These types are subclasses of ICorDebugController, which has
// a Continue method that we call in the function stub. This is
// necessary to allow the debuggee to continue execution.
#define DEBUGGERCALLBACK_STUB(methodname, debugcontrollertypename, ...)     \
  HRESULT STDMETHODCALLTYPE methodname(debugcontrollertypename *controller, \
                                       ##__VA_ARGS__) override {            \
    controller->Continue(FALSE);                                            \
    return E_NOINTERFACE;                                                   \
  }

  // Callback stubs for ICorDebugManagedCallback.
  DEBUGGERCALLBACK_STUB(Break, ICorDebugAppDomain,
                        ICorDebugThread *debug_thread);
  DEBUGGERCALLBACK_STUB(StepComplete, ICorDebugAppDomain,
                        ICorDebugThread *debug_thread,
                        ICorDebugStepper *stepper, CorDebugStepReason reason);
  DEBUGGERCALLBACK_STUB(ExitProcess, ICorDebugProcess);
  DEBUGGERCALLBACK_STUB(CreateThread, ICorDebugAppDomain,
                        ICorDebugThread *debug_thread);
  DEBUGGERCALLBACK_STUB(ExitThread, ICorDebugAppDomain,
                        ICorDebugThread *debug_thread);
  DEBUGGERCALLBACK_STUB(UnloadModule, ICorDebugAppDomain,
                        ICorDebugModule *debug_module);
  DEBUGGERCALLBACK_STUB(LoadClass, ICorDebugAppDomain,
                        ICorDebugClass *debug_class);
  DEBUGGERCALLBACK_STUB(UnloadClass, ICorDebugAppDomain,
                        ICorDebugClass *debug_class);
  DEBUGGERCALLBACK_STUB(DebuggerError, ICorDebugProcess, HRESULT hr,
                        DWORD error_code);
  DEBUGGERCALLBACK_STUB(LogMessage, ICorDebugAppDomain,
                        ICorDebugThread *debug_thread, LONG level,
                        WCHAR *log_switch_name, WCHAR *message);
  DEBUGGERCALLBACK_STUB(LogSwitch, ICorDebugAppDomain,
                        ICorDebugThread *debug_thread, LONG level, ULONG reason,
                        WCHAR *log_switch_name, WCHAR *parent_name);
  DEBUGGERCALLBACK_STUB(CreateAppDomain, ICorDebugProcess,
                        ICorDebugAppDomain *appdomain);
  DEBUGGERCALLBACK_STUB(ExitAppDomain, ICorDebugProcess,
                        ICorDebugAppDomain *appdomain);
  DEBUGGERCALLBACK_STUB(LoadAssembly, ICorDebugAppDomain,
                        ICorDebugAssembly *assembly);
  DEBUGGERCALLBACK_STUB(UnloadAssembly, ICorDebugAppDomain,
                        ICorDebugAssembly *assembly);
  DEBUGGERCALLBACK_STUB(ControlCTrap, ICorDebugProcess);
  DEBUGGERCALLBACK_STUB(NameChange, ICorDebugAppDomain,
                        ICorDebugThread *debug_thread);
  DEBUGGERCALLBACK_STUB(UpdateModuleSymbols, ICorDebugAppDomain,
                        ICorDebugModule *debug_module, IStream *symbol_stream);
  DEBUGGERCALLBACK_STUB(EditAndContinueRemap, ICorDebugAppDomain,
                        ICorDebugThread *debug_thread,
                        ICorDebugFunction *debug_function, BOOL accurate);
  DEBUGGERCALLBACK_STUB(BreakpointSetError, ICorDebugAppDomain,
                        ICorDebugThread *debug_thread,
                        ICorDebugBreakpoint *debug_breakpoint, DWORD error);
  DEBUGGERCALLBACK_STUB(CreateProcess, ICorDebugProcess);

  // ICorDebugManagedCallback2 interface.
  // Callback stub for ICorDebugManagedCallback2.
  DEBUGGERCALLBACK_STUB(FunctionRemapOpportunity, ICorDebugAppDomain,
                        ICorDebugThread *debug_thread,
                        ICorDebugFunction *old_function,
                        ICorDebugFunction *new_function, ULONG32 old_il_offset);
  DEBUGGERCALLBACK_STUB(CreateConnection, ICorDebugProcess,
                        CONNID connection_id, WCHAR *connection_name);
  DEBUGGERCALLBACK_STUB(ChangeConnection, ICorDebugProcess,
                        CONNID connection_id);
  DEBUGGERCALLBACK_STUB(DestroyConnection, ICorDebugProcess,
                        CONNID connection_id);
  DEBUGGERCALLBACK_STUB(Exception, ICorDebugAppDomain,
                        ICorDebugThread *debug_thread, ICorDebugFrame *frame,
                        ULONG32 offset,
                        CorDebugExceptionCallbackType event_type, DWORD flags);
  DEBUGGERCALLBACK_STUB(ExceptionUnwind, ICorDebugAppDomain,
                        ICorDebugThread *debug_thread,
                        CorDebugExceptionUnwindCallbackType event_type,
                        DWORD flags);
  DEBUGGERCALLBACK_STUB(FunctionRemapComplete, ICorDebugAppDomain,
                        ICorDebugThread *debug_thread,
                        ICorDebugFunction *debug_function);
  DEBUGGERCALLBACK_STUB(MDANotification, ICorDebugController,
                        ICorDebugThread *debug_thread, ICorDebugMDA *mda);

#undef DEBUGGERCALLBACK_STUB

  // ICorDebugManagedCallback3 interface.
  HRESULT STDMETHODCALLTYPE CustomNotification(
      ICorDebugThread *debug_thread, ICorDebugAppDomain *appdomain) override;

  // Sets the process being debugged by this callback.
  void SetDebugProcess(ICorDebugProcess *debug_process) {
    debug_process_ = debug_process;
  };

  // Returns all the PDB files that are parsed.
  const std::vector<
      std::unique_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
      &GetPdbFiles() const {
    return portable_pdbs_;
  }

  // Reads, parses and activates/deactivates incoming breakpoints.
  HRESULT SyncBreakpoints() {
    return breakpoint_collection_->SyncBreakpoints();
  }

  // Cancels syncing breakpoints.
  HRESULT CancelSyncBreakpoints() {
    return breakpoint_collection_->CancelSyncBreakpoints();
  }

  // Sets whether property evaluation should be performed.
  void SetPropertyEvaluation(BOOL eval) {
    eval_coordinator_->SetPropertyEvaluation(eval);
  }

  // Template function to enumerate different ICorDebug enumerations.
  // All the enumerated items will be stored in vector result.
  // Even if HRESULT returned is not SUCCEED, the result array may
  // be filled too.
  template <typename ICorDebugSpecifiedTypeEnum,
            typename ICorDebugSpecifiedType>
  static HRESULT EnumerateICorDebugSpecifiedType(
      ICorDebugSpecifiedTypeEnum *debug_enum,
      std::vector<CComPtr<ICorDebugSpecifiedType>> *result) {
    if (!result) {
      return E_INVALIDARG;
    }

    size_t result_index = 0;
    result->clear();
    HRESULT hr = E_FAIL;
    while (true) {
      ULONG value_to_retrieve = 20;
      ULONG value_retrieved = 0;

      std::vector<ICorDebugSpecifiedType *> temp_values(value_to_retrieve,
                                                        nullptr);

      hr = debug_enum->Next(value_to_retrieve, temp_values.data(),
                            &value_retrieved);
      if (value_retrieved == 0) {
        break;
      }

      result->resize(result->size() + value_retrieved);
      for (size_t k = 0; k < value_retrieved; ++k) {
        (*result)[result_index] = temp_values[k];
        temp_values[k]->Release();
        ++result_index;
      }

      if (FAILED(hr)) {
        std::cerr << "Failed to enumerate ICorDebug " << std::hex << hr;
        return hr;
      }
    }

    return S_OK;

  }

  // Gets the name of the pipe the debugger will use to communicate with
  // the agent.
  std::string GetPipeName() { return pipe_name_; }
  
 private:
  // Given an ICorDebugBreakpoint, gets the function token, IL offset
  // and metadata of the function that the breakpoint is in.
  HRESULT GetFunctionTokenAndILOffset(ICorDebugBreakpoint *debug_breakpoint,
                                      mdMethodDef *function_token,
                                      ULONG32 *il_offset,
                                      IMetaDataImport **metadata_import);

  // An EvalCoordinator is used to coordinate between DebuggerCallback object
  // and a StackFrame object when an evaluation is needed. See the
  // EvalCoordinator class for comments on how to use it.
  std::unique_ptr<IEvalCoordinator> eval_coordinator_;

  // This field is used for reference counting (AddRef and Release).
  std::atomic<ULONG> ref_count_;

  // Vector containing all the portable PDB files that are parsed.
  std::vector<
      std::unique_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
      portable_pdbs_;

  // The ICorDebugProcess of the debugged process.
  CComPtr<ICorDebugProcess> debug_process_;

  // The collection of breakpoint used by the callback to store and
  // manage breakpoints.
  std::unique_ptr<IBreakpointCollection> breakpoint_collection_;

  bool initialized_success_ = false;

  // The name of the pipe the debugger will use to communicate with the agent.
  std::string pipe_name_;
};

}  //  namespace google_cloud_debugger

#endif  // DEBUGGERCALLBACK_H_
