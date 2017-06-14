// Copyright 2015-2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#ifndef DEBUGGERCALLBACK_H_
#define DEBUGGERCALLBACK_H_

#include <atomic>
#include <memory>

#include "cor.h"
#include "cordebug.h"
#include "corsym.h"
#include "evalcoordinator.h"

namespace google_cloud_debugger {

// A DebuggerCallback object is used to set the managed handler of an ICorDebug
// interface. Whenever an interesting event happens, the ICorDebug object
// will fire the corresponding callback method. For example, if a breakpoint
// is reached, the Breakpoint callback method will be called. All the callback
// methods are defined in ICorDebugManagedCallback interface.
class DebuggerCallback final : public ICorDebugManagedCallback,
                               ICorDebugManagedCallback2,
                               ICorDebugManagedCallback3 {
 public:
  HRESULT Initialize();

  // IUnknown interface.
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **object) override;
  ULONG STDMETHODCALLTYPE AddRef(void) override;
  ULONG STDMETHODCALLTYPE Release(void) override;

  // ICorDebugManagedCallback interface.
  // This method is called when System.Debugger.Break() line is reached.
  HRESULT STDMETHODCALLTYPE Break(ICorDebugAppDomain *appdomain,
                                  ICorDebugThread *thread) override;
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
  // TODO(quoct): Implement Breakpoint callback.
  DEBUGGERCALLBACK_STUB(Breakpoint, ICorDebugAppDomain,
                        ICorDebugThread *debug_thread,
                        ICorDebugBreakpoint *breakpoint);
  DEBUGGERCALLBACK_STUB(StepComplete, ICorDebugAppDomain,
                        ICorDebugThread *debug_thread,
                        ICorDebugStepper *stepper, CorDebugStepReason reason);
  DEBUGGERCALLBACK_STUB(ExitProcess, ICorDebugProcess);
  DEBUGGERCALLBACK_STUB(CreateThread, ICorDebugAppDomain,
                        ICorDebugThread *debug_thread);
  DEBUGGERCALLBACK_STUB(ExitThread, ICorDebugAppDomain,
                        ICorDebugThread *debug_thread);
  DEBUGGERCALLBACK_STUB(LoadModule, ICorDebugAppDomain,
                        ICorDebugModule *debug_module);
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

 private:
  // An EvalCoordinator is used to coordinate between DebuggerCallback object
  // and a VariableManager object when an evaluation is needed. See the
  // EvalCoordinator class for comments on how to use it.
  // TODO(quoct): Handle multi thread case with eval coordinator?
  // Also, this is just a temporary design and may be changed.
  std::unique_ptr<EvalCoordinator> eval_coordinator_;

  // This field is used for reference counting (AddRef and Release).
  std::atomic<ULONG> ref_count_;
};

}  //  namespace google_cloud_debugger

#endif  // DEBUGGERCALLBACK_H_
