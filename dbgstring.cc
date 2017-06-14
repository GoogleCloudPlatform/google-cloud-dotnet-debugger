// Copyright 2015-2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#include "dbgstring.h"

#include <iostream>

#include "evalcoordinator.h"

namespace google_cloud_debugger {
using std::cout;
using std::cerr;

HRESULT DbgString::Initialize(ICorDebugValue *debug_value, BOOL is_null) {
  SetIsNull(is_null);

  if (is_null) {
    return S_OK;
  }

  // Create a handle so we won't lose the object.
  HRESULT hr;
  CComPtr<ICorDebugHeapValue2> heap_value;

  hr = CreateStrongHandle(debug_value, &string_handle_);
  if (FAILED(hr)) {
    cerr << "Failed to create a handle for the string.";
    return hr;
  }

  return S_OK;
}

HRESULT DbgString::PrintValue(EvalCoordinator *eval_coordinator) {
  if (GetIsNull()) {
    cout << "NULL";
    return S_OK;
  }

  HRESULT hr;
  std::unique_ptr<WCHAR[]> string_value;
  ULONG32 str_len;
  ULONG32 str_returned_len;
  CComPtr<ICorDebugValue> debug_value;
  CComPtr<ICorDebugStringValue> debug_string;

  hr = string_handle_->Dereference(&debug_value);

  if (FAILED(hr)) {
    cerr << "Failed to dereference string reference.";
    return hr;
  }

  hr = debug_value->QueryInterface(__uuidof(ICorDebugStringValue),
                                   reinterpret_cast<void **>(&debug_string));

  if (FAILED(hr)) {
    cerr << "Failed to convert to ICorDebugStringValue.";
    return hr;
  }

  hr = debug_string->GetLength(&str_len);
  if (FAILED(hr)) {
    cerr << "Failed to get length of string.";
    return hr;
  }

  // Plus 1 for the NULL at the end of the string.
  str_len += 1;
  string_value =
      std::unique_ptr<WCHAR[]>(new (std::nothrow) WCHAR[str_len + 1]);
  if (!string_value) {
    return E_OUTOFMEMORY;
  }

  hr = debug_string->GetString(str_len + 1, &str_returned_len,
                               string_value.get());
  if (FAILED(hr)) {
    cerr << "Failed to extract the string.";
    return hr;
  }

  PrintWcharString(string_value.get());
  return S_OK;
}

HRESULT DbgString::PrintType() {
  cout << "System.String";
  return S_OK;
}

}  // namespace google_cloud_debugger
