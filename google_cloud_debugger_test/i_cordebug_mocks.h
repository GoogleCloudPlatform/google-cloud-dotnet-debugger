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

#ifndef I_COR_DEBUG_MOCKS_H_
#define I_COR_DEBUG_MOCKS_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "cor.h"
#include "cordebug.h"
#include "dbgprimitive.h"
#include "dbgstring.h"

namespace google_cloud_debugger_test {

// This macro is for mocking IUnknown.
#define IUNKNOWN_MOCK                                                   \
  MOCK_METHOD2(QueryInterface, HRESULT(REFIID riid, void **ppvObject)); \
  MOCK_METHOD0(AddRef, ULONG());                                        \
  MOCK_METHOD0(Release, ULONG());

// This macro is for mocking ICorDebug.
#define ICORDEBUG_MOCK                                        \
  IUNKNOWN_MOCK                                               \
  MOCK_METHOD1(GetType, HRESULT(CorElementType *pType));      \
  MOCK_METHOD1(GetSize, HRESULT(ULONG32 *pSize));             \
  MOCK_METHOD1(GetAddress, HRESULT(CORDB_ADDRESS *pAddress)); \
  MOCK_METHOD1(CreateBreakpoint,                              \
               HRESULT(ICorDebugValueBreakpoint **ppBreakpoint));

// Mock class for ICorDebugStringValue.
class ICorDebugStringValueMock : public ICorDebugStringValue,
                                 public ICorDebugHeapValue2 {
 public:
  ICORDEBUG_MOCK

  MOCK_METHOD1(IsValid, HRESULT(BOOL *pbValid));

  MOCK_METHOD1(CreateRelocBreakpoint,
               HRESULT(ICorDebugValueBreakpoint **pBreakpoint));

  MOCK_METHOD1(GetLength, HRESULT(ULONG32 *pcchString));

  MOCK_METHOD3(GetString, HRESULT(ULONG32 cchString, ULONG32 *pcchString,
                                  WCHAR szString[]));

  MOCK_METHOD2(CreateHandle, HRESULT(CorDebugHandleType type,
                                     ICorDebugHandleValue **ppHandle));
};

// Mock class for ICorDebugStringValue.
class ICorDebugGenericValueMock : public ICorDebugGenericValue {
 public:
  ICORDEBUG_MOCK

  MOCK_METHOD1(GetValue, HRESULT(void *pTo));

  MOCK_METHOD1(SetValue, HRESULT(void *pFrom));
};

// Mock class for ICorDebugReferenceValue.
class ICorDebugReferenceValueMock : public ICorDebugReferenceValue {
 public:
  ICORDEBUG_MOCK

  MOCK_METHOD1(IsNull, HRESULT(BOOL *pbNull));

  MOCK_METHOD1(GetValue, HRESULT(CORDB_ADDRESS *pValue));

  MOCK_METHOD1(SetValue, HRESULT(CORDB_ADDRESS value));

  MOCK_METHOD1(Dereference, HRESULT(ICorDebugValue **ppValue));

  MOCK_METHOD1(DereferenceStrong, HRESULT(ICorDebugValue **ppValue));
};

// Mock class for ICorDebugReferenceValue.
class ICorDebugObjectValueMock : public ICorDebugObjectValue {
 public:
  ICORDEBUG_MOCK

  MOCK_METHOD1(GetClass, HRESULT(ICorDebugClass **ppClass));

  MOCK_METHOD3(GetFieldValue,
               HRESULT(ICorDebugClass *pClass, mdFieldDef fieldDef,
                       ICorDebugValue **ppValue));

  MOCK_METHOD2(GetVirtualMethod,
               HRESULT(mdMemberRef memberRef, ICorDebugFunction **ppFunction));

  MOCK_METHOD1(GetContext, HRESULT(ICorDebugContext **ppContext));

  MOCK_METHOD1(IsValueClass, HRESULT(BOOL *pbIsValueClass));

  MOCK_METHOD1(GetManagedCopy, HRESULT(IUnknown **ppObject));

  MOCK_METHOD1(SetFromManagedCopy, HRESULT(IUnknown *pObject));
};

// Mock class for ICorDebugClass.
class ICorDebugClassMock : public ICorDebugClass {
 public:
  IUNKNOWN_MOCK

  MOCK_METHOD1(GetModule, HRESULT(ICorDebugModule **pModule));

  MOCK_METHOD1(GetToken, HRESULT(mdTypeDef *pTypeDef));

  MOCK_METHOD3(GetStaticFieldValue,
               HRESULT(mdFieldDef fieldDef, ICorDebugFrame *pFrame,
                       ICorDebugValue **ppValue));
};

class ICorDebugHeapValue2Mock : public ICorDebugHeapValue2 {
 public:
  IUNKNOWN_MOCK

  MOCK_METHOD2(CreateHandle, HRESULT(CorDebugHandleType type,
                                     ICorDebugHandleValue **ppHandle));
};

// Mock class for ICorDebugClass.
class ICorDebugArrayValueMock : public ICorDebugArrayValue {
 public:
  ICORDEBUG_MOCK

  MOCK_METHOD1(IsValid, HRESULT(BOOL *pbValid));
  MOCK_METHOD1(CreateRelocBreakpoint,
               HRESULT(ICorDebugValueBreakpoint **ppBreakpoint));
  MOCK_METHOD1(GetElementType, HRESULT(CorElementType *pType));
  MOCK_METHOD1(GetRank, HRESULT(ULONG32 *pnRank));
  MOCK_METHOD1(GetCount, HRESULT(ULONG32 *pnCount));
  MOCK_METHOD2(GetDimensions, HRESULT(ULONG32 cdim, ULONG32 dims[]));
  MOCK_METHOD1(HasBaseIndicies, HRESULT(BOOL *pbHasBaseIndicies));
  MOCK_METHOD2(GetBaseIndicies, HRESULT(ULONG32 cdim, ULONG32 indicies[]));
  MOCK_METHOD3(GetElement, HRESULT(ULONG32 cdim, ULONG32 indices[],
                                   ICorDebugValue **ppValue));
  MOCK_METHOD2(GetElementAtPosition,
               HRESULT(ULONG32 nPosition, ICorDebugValue **ppValue));
};

class ICorDebugHandleValueMock : public ICorDebugHandleValue {
 public:
  ICORDEBUG_MOCK

  MOCK_METHOD1(IsNull, HRESULT(BOOL *pbNull));
  MOCK_METHOD1(GetValue, HRESULT(CORDB_ADDRESS *pValue));
  MOCK_METHOD1(SetValue, HRESULT(CORDB_ADDRESS value));
  MOCK_METHOD1(Dereference, HRESULT(ICorDebugValue **ppValue));
  MOCK_METHOD1(DereferenceStrong, HRESULT(ICorDebugValue **ppValue));
  MOCK_METHOD1(GetHandleType, HRESULT(CorDebugHandleType *pType));
  MOCK_METHOD0(Dispose, HRESULT(void));
};

class ICorDebugThreadMock : public ICorDebugThread {
 public:
  IUNKNOWN_MOCK

  MOCK_METHOD1(GetProcess, HRESULT(ICorDebugProcess **ppProcess));
  MOCK_METHOD1(GetID, HRESULT(DWORD *pdwThreadId));
  MOCK_METHOD1(GetHandle, HRESULT(HTHREAD *phThreadHandle));
  MOCK_METHOD1(GetAppDomain, HRESULT(ICorDebugAppDomain **ppAppDomain));
  MOCK_METHOD1(SetDebugState, HRESULT(CorDebugThreadState state));
  MOCK_METHOD1(GetDebugState, HRESULT(CorDebugThreadState *pState));
  MOCK_METHOD1(GetUserState, HRESULT(CorDebugUserState *pState));
  MOCK_METHOD1(GetCurrentException,
               HRESULT(ICorDebugValue **ppExceptionObject));
  MOCK_METHOD0(ClearCurrentException, HRESULT(void));
  MOCK_METHOD1(CreateStepper, HRESULT(ICorDebugStepper **ppStepper));
  MOCK_METHOD1(EnumerateChains, HRESULT(ICorDebugChainEnum **ppChains));
  MOCK_METHOD1(GetActiveChain, HRESULT(ICorDebugChain **ppChain));
  MOCK_METHOD1(GetActiveFrame, HRESULT(ICorDebugFrame **ppFrame));
  MOCK_METHOD1(GetRegisterSet, HRESULT(ICorDebugRegisterSet **ppRegisters));
  MOCK_METHOD1(CreateEval, HRESULT(ICorDebugEval **ppEval));
  MOCK_METHOD1(GetObject, HRESULT(ICorDebugValue **ppObject));
};

class ICorDebugModuleMock : public ICorDebugModule {
 public:
  IUNKNOWN_MOCK

  MOCK_METHOD1(GetProcess, HRESULT(ICorDebugProcess **ppProcess));
  MOCK_METHOD1(GetBaseAddress, HRESULT(CORDB_ADDRESS *pAddress));
  MOCK_METHOD1(GetAssembly, HRESULT(ICorDebugAssembly **ppAssembly));
  MOCK_METHOD3(GetName,
               HRESULT(ULONG32 cchName, ULONG32 *pcchName, WCHAR szName[]));
  MOCK_METHOD2(EnableJITDebugging,
               HRESULT(BOOL bTrackJITInfo, BOOL bAllowJitOpts));
  MOCK_METHOD1(EnableClassLoadCallbacks, HRESULT(BOOL bClassLoadCallbacks));
  MOCK_METHOD2(GetFunctionFromToken,
               HRESULT(mdMethodDef methodDef, ICorDebugFunction **ppFunction));
  MOCK_METHOD2(GetFunctionFromRVA,
               HRESULT(CORDB_ADDRESS rva, ICorDebugFunction **ppFunction));
  MOCK_METHOD2(GetClassFromToken,
               HRESULT(mdTypeDef typeDef, ICorDebugClass **ppClass));
  MOCK_METHOD1(CreateBreakpoint,
               HRESULT(ICorDebugModuleBreakpoint **ppBreakpoint));
  MOCK_METHOD1(
      GetEditAndContinueSnapshot,
      HRESULT(ICorDebugEditAndContinueSnapshot **ppEditAndContinueSnapshot));
  MOCK_METHOD2(GetMetaDataInterface, HRESULT(REFIID riid, IUnknown **ppObj));
  MOCK_METHOD1(GetToken, HRESULT(mdModule *pToken));
  MOCK_METHOD1(IsDynamic, HRESULT(BOOL *pDynamic));
  MOCK_METHOD2(GetGlobalVariableValue,
               HRESULT(mdFieldDef fieldDef, ICorDebugValue **ppValue));
  MOCK_METHOD1(GetSize, HRESULT(ULONG32 *pcBytes));
  MOCK_METHOD1(IsInMemory, HRESULT(BOOL *pInMemory));
};

class ICorDebugFrameMock : public ICorDebugFrame {
 public:
  IUNKNOWN_MOCK

  MOCK_METHOD1(GetChain, HRESULT(ICorDebugChain **ppChain));
  MOCK_METHOD1(GetCode, HRESULT(ICorDebugCode **ppCode));
  MOCK_METHOD1(GetFunction, HRESULT(ICorDebugFunction **ppFunction));
  MOCK_METHOD1(GetFunctionToken, HRESULT(mdMethodDef *pToken));
  MOCK_METHOD2(GetStackRange,
               HRESULT(CORDB_ADDRESS *pStart, CORDB_ADDRESS *pEnd));
  MOCK_METHOD1(GetCaller, HRESULT(ICorDebugFrame **ppFrame));
  MOCK_METHOD1(GetCallee, HRESULT(ICorDebugFrame **ppFrame));
  MOCK_METHOD1(CreateStepper, HRESULT(ICorDebugStepper **ppStepper));
};

class ICorDebugILFrameMock : public ICorDebugILFrame {
 public:
  IUNKNOWN_MOCK

  MOCK_METHOD1(GetChain, HRESULT(ICorDebugChain **ppChain));
  MOCK_METHOD1(GetCode, HRESULT(ICorDebugCode **ppCode));
  MOCK_METHOD1(GetFunction, HRESULT(ICorDebugFunction **ppFunction));
  MOCK_METHOD1(GetFunctionToken, HRESULT(mdMethodDef *pToken));
  MOCK_METHOD2(GetStackRange,
               HRESULT(CORDB_ADDRESS *pStart, CORDB_ADDRESS *pEnd));
  MOCK_METHOD1(GetCaller, HRESULT(ICorDebugFrame **ppFrame));
  MOCK_METHOD1(GetCallee, HRESULT(ICorDebugFrame **ppFrame));
  MOCK_METHOD1(CreateStepper, HRESULT(ICorDebugStepper **ppStepper));
  MOCK_METHOD2(GetIP, HRESULT(ULONG32 *pnOffset,
                              CorDebugMappingResult *pMappingResult));
  MOCK_METHOD1(SetIP, HRESULT(ULONG32 nOffset));
  MOCK_METHOD1(EnumerateLocalVariables,
               HRESULT(ICorDebugValueEnum **ppValueEnum));
  MOCK_METHOD2(GetLocalVariable,
               HRESULT(DWORD dwIndex, ICorDebugValue **ppValue));
  MOCK_METHOD1(EnumerateArguments, HRESULT(ICorDebugValueEnum **ppValueEnum));
  MOCK_METHOD2(GetArgument, HRESULT(DWORD dwIndex, ICorDebugValue **ppValue));
  MOCK_METHOD1(GetStackDepth, HRESULT(ULONG32 *pDepth));
  MOCK_METHOD2(GetStackValue, HRESULT(DWORD dwIndex, ICorDebugValue **ppValue));
  MOCK_METHOD1(CanSetIP, HRESULT(ULONG32 nOffset));
};

class ICorDebugFunctionMock : public ICorDebugFunction {
 public:
  IUNKNOWN_MOCK

  MOCK_METHOD1(GetModule, HRESULT(ICorDebugModule **ppModule));
  MOCK_METHOD1(GetClass, HRESULT(ICorDebugClass **ppClass));
  MOCK_METHOD1(GetToken, HRESULT(mdMethodDef *pMethodDef));
  MOCK_METHOD1(GetILCode, HRESULT(ICorDebugCode **ppCode));
  MOCK_METHOD1(GetNativeCode, HRESULT(ICorDebugCode **ppCode));
  MOCK_METHOD1(CreateBreakpoint,
               HRESULT(ICorDebugFunctionBreakpoint **ppBreakpoint));
  MOCK_METHOD1(GetLocalVarSigToken, HRESULT(mdSignature *pmdSig));
  MOCK_METHOD1(GetCurrentVersionNumber, HRESULT(ULONG32 *pnCurrentVersion));
};

class ICorDebugEvalMock : public ICorDebugEval {
 public:
  IUNKNOWN_MOCK

  MOCK_METHOD3(CallFunction, HRESULT(ICorDebugFunction *pFunction,
                                     ULONG32 nArgs, ICorDebugValue *ppArgs[]));
  MOCK_METHOD3(NewObject, HRESULT(ICorDebugFunction *pConstructor,
                                  ULONG32 nArgs, ICorDebugValue *ppArgs[]));
  MOCK_METHOD1(NewObjectNoConstructor, HRESULT(ICorDebugClass *pClass));
  MOCK_METHOD1(NewString, HRESULT(LPCWSTR string));
  MOCK_METHOD5(NewArray, HRESULT(CorElementType elementType,
                                 ICorDebugClass *pElementClass, ULONG32 rank,
                                 ULONG32 dims[], ULONG32 lowBounds[]));
  MOCK_METHOD1(IsActive, HRESULT(BOOL *pbActive));
  MOCK_METHOD0(Abort, HRESULT(void));
  MOCK_METHOD1(GetResult, HRESULT(ICorDebugValue **ppResult));
  MOCK_METHOD1(GetThread, HRESULT(ICorDebugThread **ppThread));
  MOCK_METHOD3(CreateValue, HRESULT(CorElementType elementType,
                                    ICorDebugClass *pElementClass,
                                    ICorDebugValue **ppValue));
};

class ICorDebugEval2Mock : public ICorDebugEval2 {
 public:
  IUNKNOWN_MOCK

  MOCK_METHOD5(CallParameterizedFunction,
               HRESULT(ICorDebugFunction *pFunction, ULONG32 nTypeArgs,
                       ICorDebugType *ppTypeArgs[], ULONG32 nArgs,
                       ICorDebugValue *ppArgs[]));
  MOCK_METHOD2(CreateValueForType,
               HRESULT(ICorDebugType *pType, ICorDebugValue **ppValue));
  MOCK_METHOD5(NewParameterizedObject,
               HRESULT(ICorDebugFunction *pConstructor, ULONG32 nTypeArgs,
                       ICorDebugType *ppTypeArgs[], ULONG32 nArgs,
                       ICorDebugValue *ppArgs[]));
  MOCK_METHOD3(NewParameterizedObjectNoConstructor,
               HRESULT(ICorDebugClass *pClass, ULONG32 nTypeArgs,
                       ICorDebugType *ppTypeArgs[]));
  MOCK_METHOD4(NewParameterizedArray,
               HRESULT(ICorDebugType *pElementType, ULONG32 rank,
                       ULONG32 dims[], ULONG32 lowBounds[]));
  MOCK_METHOD2(NewStringWithLength, HRESULT(LPCWSTR string, UINT uiLength));
  MOCK_METHOD0(RudeAbort, HRESULT(void));
};

class ICorDebugBreakpointMock : public ICorDebugBreakpoint {
 public:
  MOCK_METHOD2(QueryInterface, HRESULT(REFIID riid, void **ppvObject));
  MOCK_METHOD0(AddRef, ULONG(void));
  MOCK_METHOD0(Release, ULONG(void));
  MOCK_METHOD1(Activate, HRESULT(BOOL bActive));
  MOCK_METHOD1(IsActive, HRESULT(BOOL *pbActive));
};

class ICorDebugStackWalkMock : public ICorDebugStackWalk {
 public:
  IUNKNOWN_MOCK

  MOCK_METHOD4(GetContext, HRESULT(ULONG32 contextFlags, ULONG32 contextBufSize,
                                   ULONG32 *contextSize, BYTE contextBuf[]));
  MOCK_METHOD3(SetContext, HRESULT(CorDebugSetContextFlag flag,
                                   ULONG32 contextSize, BYTE context[]));
  MOCK_METHOD0(Next, HRESULT(void));
  MOCK_METHOD1(GetFrame, HRESULT(ICorDebugFrame **pFrame));
};

class ICorDebugAppDomainMock : public ICorDebugAppDomain {
 public:
  IUNKNOWN_MOCK

  MOCK_METHOD1(Stop, HRESULT(DWORD dwTimeoutIgnored));
  MOCK_METHOD1(Continue, HRESULT(BOOL fIsOutOfBand));
  MOCK_METHOD1(IsRunning, HRESULT(BOOL *pbRunning));
  MOCK_METHOD2(HasQueuedCallbacks,
               HRESULT(ICorDebugThread *pThread, BOOL *pbQueued));
  MOCK_METHOD1(EnumerateThreads, HRESULT(ICorDebugThreadEnum **ppThreads));
  MOCK_METHOD2(SetAllThreadsDebugState,
               HRESULT(CorDebugThreadState state,
                       ICorDebugThread *pExceptThisThread));
  MOCK_METHOD0(Detach, HRESULT(void));
  MOCK_METHOD1(Terminate, HRESULT(UINT exitCode));
  MOCK_METHOD3(CanCommitChanges,
               HRESULT(ULONG cSnapshots,
                       ICorDebugEditAndContinueSnapshot *pSnapshots[],
                       ICorDebugErrorInfoEnum **pError));
  MOCK_METHOD3(CommitChanges,
               HRESULT(ULONG cSnapshots,
                       ICorDebugEditAndContinueSnapshot *pSnapshots[],
                       ICorDebugErrorInfoEnum **pError));
  MOCK_METHOD1(GetProcess, HRESULT(ICorDebugProcess **ppProcess));
  MOCK_METHOD1(EnumerateAssemblies,
               HRESULT(ICorDebugAssemblyEnum **ppAssemblies));
  MOCK_METHOD2(GetModuleFromMetaDataInterface,
               HRESULT(IUnknown *pIMetaData, ICorDebugModule **ppModule));
  MOCK_METHOD1(EnumerateBreakpoints,
               HRESULT(ICorDebugBreakpointEnum **ppBreakpoints));
  MOCK_METHOD1(EnumerateSteppers, HRESULT(ICorDebugStepperEnum **ppSteppers));
  MOCK_METHOD1(IsAttached, HRESULT(BOOL *pbAttached));
  MOCK_METHOD3(GetName,
               HRESULT(ULONG32 cchName, ULONG32 *pcchName, WCHAR szName[]));
  MOCK_METHOD0(Attach, HRESULT(void));
  MOCK_METHOD1(GetID, HRESULT(ULONG32 *pId));
  MOCK_METHOD1(GetObject, HRESULT(ICorDebugValue **ppObject));
};

class ICorDebugThread3Mock : public ICorDebugThread3 {
 public:
  IUNKNOWN_MOCK

  MOCK_METHOD1(CreateStackWalk, HRESULT(ICorDebugStackWalk **ppStackWalk));
  MOCK_METHOD3(GetActiveInternalFrames, HRESULT(ULONG32 cInternalFrames, ULONG32 *pcInternalFrames, ICorDebugInternalFrame2 *ppInternalFrames[]));
};

class ICorDebugFunctionBreakpointMock : public ICorDebugFunctionBreakpoint {
 public:
  IUNKNOWN_MOCK

  MOCK_METHOD1(Activate, HRESULT(BOOL bActive));
  MOCK_METHOD1(IsActive, HRESULT(BOOL *pbActive));
  MOCK_METHOD1(GetFunction, HRESULT(ICorDebugFunction **ppFunction));
  MOCK_METHOD1(GetOffset, HRESULT(ULONG32 *pnOffset));
};

// Mock class for ICorDebugType.
class ICorDebugTypeMock : public ICorDebugType {
 public:
  IUNKNOWN_MOCK

  MOCK_METHOD1(GetType, HRESULT(CorElementType *pType));

  MOCK_METHOD1(GetClass, HRESULT(ICorDebugClass **ppClass));

  MOCK_METHOD1(EnumerateTypeParameters,
               HRESULT(ICorDebugTypeEnum **ppTyParEnum));

  MOCK_METHOD1(GetFirstTypeParameter, HRESULT(ICorDebugType **value));

  MOCK_METHOD1(GetBase, HRESULT(ICorDebugType **pBase));

  MOCK_METHOD3(GetStaticFieldValue,
               HRESULT(mdFieldDef fieldDef, ICorDebugFrame *pFrame,
                       ICorDebugValue **ppValue));

  MOCK_METHOD1(GetRank, HRESULT(ULONG32 *pnRank));
};

class ICorDebugTypeEnumMock : public ICorDebugTypeEnum {
 public:
  IUNKNOWN_MOCK

  MOCK_METHOD1(Skip, HRESULT(ULONG celt));
  MOCK_METHOD0(Reset, HRESULT(void));
  MOCK_METHOD1(Clone, HRESULT(ICorDebugEnum **ppEnum));
  MOCK_METHOD1(GetCount, HRESULT(ULONG *pcelt));
  MOCK_METHOD3(Next, HRESULT(ULONG celt, ICorDebugType *values[],
                             ULONG *pceltFetched));
};

class ICorDebugValueEnumMock : public ICorDebugValueEnum {
 public:
  IUNKNOWN_MOCK

  MOCK_METHOD1(Skip, HRESULT(ULONG celt));
  MOCK_METHOD0(Reset, HRESULT(void));
  MOCK_METHOD1(Clone, HRESULT(ICorDebugEnum **ppEnum));
  MOCK_METHOD1(GetCount, HRESULT(ULONG *pcelt));
  MOCK_METHOD3(Next, HRESULT(ULONG celt, ICorDebugValue *values[],
                             ULONG *pceltFetched));
};

}  // namespace google_cloud_debugger_test

#endif  //  I_COR_DEBUG_MOCKS_H_
