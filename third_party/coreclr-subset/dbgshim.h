#ifndef DBG_SHIM_H_
#define DBG_SHIM_H_

// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
//*****************************************************************************
// DbgShim.h
//
//*****************************************************************************

#include <windows.h>

typedef VOID (*PSTARTUP_CALLBACK)(IUnknown* pCordb, PVOID parameter,
                                  HRESULT hr);

EXTERN_C HRESULT CreateProcessForLaunch(
    LPWSTR lpCommandLine, BOOL bSuspendProcess, LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory, PDWORD pProcessId, HANDLE* pResumeHandle);

EXTERN_C HRESULT ResumeProcess(HANDLE hResumeHandle);

EXTERN_C HRESULT CloseResumeHandle(HANDLE hResumeHandle);

EXTERN_C HRESULT RegisterForRuntimeStartup(DWORD dwProcessId,
                                           PSTARTUP_CALLBACK pfnCallback,
                                           PVOID parameter,
                                           PVOID* ppUnregisterToken);

EXTERN_C HRESULT UnregisterForRuntimeStartup(PVOID pUnregisterToken);

EXTERN_C HRESULT GetStartupNotificationEvent(DWORD debuggeePID,
                                             HANDLE* phStartupEvent);

EXTERN_C HRESULT EnumerateCLRs(DWORD debuggeePID, HANDLE** ppHandleArrayOut,
                               LPWSTR** ppStringArrayOut,
                               DWORD* pdwArrayLengthOut);

EXTERN_C HRESULT CloseCLREnumeration(HANDLE* pHandleArray, LPWSTR* pStringArray,
                                     DWORD dwArrayLength);

EXTERN_C HRESULT CreateVersionStringFromModule(DWORD pidDebuggee,
                                               LPCWSTR szModuleName,
                                               LPWSTR pBuffer, DWORD cchBuffer,
                                               DWORD* pdwLength);

EXTERN_C HRESULT CreateDebuggingInterfaceFromVersionEx(
    int iDebuggerVersion, LPCWSTR szDebuggeeVersion, IUnknown** ppCordb);

EXTERN_C HRESULT CreateDebuggingInterfaceFromVersion(LPCWSTR szDebuggeeVersion,
                                                     IUnknown** ppCordb);

#endif  // DBG_SHIM_H_
