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

#include "dbgstring.h"

#include <iostream>

#include "evalcoordinator.h"

using google::cloud::diagnostics::debug::Variable;
using std::string;

namespace google_cloud_debugger {

void DbgString::Initialize(ICorDebugValue *debug_value, BOOL is_null) {
  SetIsNull(is_null);

  if (is_null) {
    return;
  }

  // Create a handle so we won't lose the object.
  CComPtr<ICorDebugHeapValue2> heap_value;

  initialize_hr_ =
      CreateStrongHandle(debug_value, &string_handle_, GetErrorStream());
  if (FAILED(initialize_hr_)) {
    WriteError("Failed to create a handle for the string.");
  }
}

HRESULT DbgString::PopulateValue(Variable *variable) {
  if (!variable) {
    return E_INVALIDARG;
  }

  if (FAILED(initialize_hr_)) {
    return initialize_hr_;
  }

  if (GetIsNull()) {
    variable->clear_value();
    return S_OK;
  }

  HRESULT hr;
  CComPtr<ICorDebugValue> debug_value;
  CComPtr<ICorDebugStringValue> debug_string;

  hr = string_handle_->Dereference(&debug_value);

  if (FAILED(hr)) {
    WriteError("Failed to dereference string reference.");
    return hr;
  }

  hr = debug_value->QueryInterface(__uuidof(ICorDebugStringValue),
                                   reinterpret_cast<void **>(&debug_string));

  if (FAILED(hr)) {
    WriteError("Failed to convert to ICorDebugStringValue.");
    return hr;
  }

  std::string value;
  hr = GetString(debug_string, &value);
  if (FAILED(hr)) {
    return hr;
  }

  variable->set_value(value);
  return S_OK;
}

HRESULT DbgString::PopulateType(Variable *variable) {
  if (!variable) {
    return E_INVALIDARG;
  }

  variable->set_type("System.String");
  return S_OK;
}

HRESULT DbgString::GetString(ICorDebugStringValue *debug_string,
                             std::string *returned_string) {
  if (!returned_string || !debug_string) {
    return E_INVALIDARG;
  }

  HRESULT hr;
  std::unique_ptr<WCHAR[]> string_value;
  ULONG32 str_len;
  ULONG32 str_returned_len;

  hr = debug_string->GetLength(&str_len);
  if (FAILED(hr)) {
    WriteError("Failed to get length of string.");
    return hr;
  }

  // Plus 1 for the NULL at the end of the string.
  str_len += 1;
  string_value =
      std::unique_ptr<WCHAR[]>(new (std::nothrow) WCHAR[str_len]);
  if (!string_value) {
    return E_OUTOFMEMORY;
  }

  hr = debug_string->GetString(str_len, &str_returned_len,
                               string_value.get());
  if (FAILED(hr)) {
    WriteError("Failed to extract the string.");
    return hr;
  }

  *returned_string = ConvertWCharPtrToString(string_value.get());
  return S_OK;
}

}  // namespace google_cloud_debugger
