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

#include "dbg_string.h"

#include <iostream>

#include "class_names.h"
#include "i_cor_debug_helper.h"
#include "i_eval_coordinator.h"

using google::cloud::diagnostics::debug::Variable;
using std::string;

namespace google_cloud_debugger {

void DbgString::Initialize(ICorDebugValue *debug_value, BOOL is_null) {
  SetIsNull(is_null);

  if (is_null) {
    return;
  }

  initialize_hr_ = debug_helper_->CreateStrongHandle(
      debug_value, &object_handle_, GetErrorStream());
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

  HRESULT hr = ExtractStringFromReference();
  if (FAILED(hr)) {
    return hr;
  }

  variable->set_value(string_obj_);
  return S_OK;
}

HRESULT DbgString::GetTypeString(std::string *type_string) {
  if (!type_string) {
    return E_INVALIDARG;
  }

  *type_string = kStringClassName;
  return S_OK;
}

HRESULT DbgString::GetString(DbgObject *object, string *returned_string) {
  if (object == nullptr || returned_string == nullptr) {
    return E_INVALIDARG;
  }

  DbgString *dbg_string = dynamic_cast<DbgString *>(object);
  if (object == nullptr) {
    return E_INVALIDARG;
  }

  HRESULT hr = dbg_string->ExtractStringFromReference();
  if (FAILED(hr)) {
    return hr;
  }

  *returned_string = dbg_string->string_obj_;
  return S_OK;
}

HRESULT DbgString::ExtractStringFromReference() {
  if (string_obj_set_) {
    return S_OK;
  }

  if (!object_handle_) {
    return E_INVALIDARG;
  }

  HRESULT hr;
  CComPtr<ICorDebugValue> debug_value;
  CComPtr<ICorDebugStringValue> debug_string;

  hr = object_handle_->Dereference(&debug_value);

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

  hr = debug_helper_->ExtractStringFromICorDebugStringValue(
      debug_string, &string_obj_, GetErrorStream());
  if (FAILED(hr)) {
    return hr;
  }

  string_obj_set_ = true;
  return S_OK;
}

}  // namespace google_cloud_debugger
