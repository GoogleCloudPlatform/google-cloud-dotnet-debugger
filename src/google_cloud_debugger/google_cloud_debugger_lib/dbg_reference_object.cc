// Copyright 2018 Google Inc. All Rights Reserved.
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

#include "dbg_reference_object.h"

#include "i_cor_debug_helper.h"

namespace google_cloud_debugger {
HRESULT DbgReferenceObject::GetNonStaticField(
    const std::string &field_name,
    std::shared_ptr<DbgObject> *field_value) {
  if (!object_handle_) {
    return E_INVALIDARG;
  }

  HRESULT hr;

  // Dereferences the object to get ICorDebugObjectValue.
  BOOL is_null = FALSE;
  CComPtr<ICorDebugValue> debug_value;
  hr = Dereference(object_handle_, &debug_value,
                   &is_null, GetErrorStream());
  if (FAILED(hr)) {
    return hr;
  }

  if (is_null) {
    return E_FAIL;
  }

  CComPtr<ICorDebugObjectValue> object_value;
  hr = debug_value->QueryInterface(__uuidof(ICorDebugObjectValue),
      reinterpret_cast<void **>(&object_value));
  if (FAILED(hr)) {
    WriteError("Failed to cast to ICorDebugObjectValue.");
    return hr;
  }

  // Next, we gets the ICorDebugClass so we can get the
  // class token and the IMetaDataImport of the module the class is in.
  CComPtr<ICorDebugClass> debug_class;
  hr = object_value->GetClass(&debug_class);
  if (FAILED(hr)) {
    return hr;
  }

  mdTypeDef class_token;
  hr = debug_class->GetToken(&class_token);
  if (FAILED(hr)) {
    WriteError("Failed to get class token.");
    return hr;
  }

  CComPtr<IMetaDataImport> metadata_import;
  hr = GetMetadataImportFromICorDebugClass(debug_class,
      &metadata_import, GetErrorStream());
  if (FAILED(hr)) {
    return hr;
  }

  mdFieldDef field_def;
  bool is_static;
  PCCOR_SIGNATURE field_sig;
  ULONG field_sig_len = 0;
  hr = GetFieldInfo(metadata_import, class_token,
      field_name, &field_def, &is_static, &field_sig,
      &field_sig_len, GetErrorStream());
  if (FAILED(hr)) {
    return hr;
  }

  if (is_static) {
    WriteError("GetNonStaticField cannot get static field.");
    return E_FAIL;
  }

  CComPtr<ICorDebugValue> field_debug_value;
  hr = object_value->GetFieldValue(debug_class, field_def, &field_debug_value);
  if (FAILED(hr)) {
    return hr;
  }

  std::unique_ptr<DbgObject> dbg_object;
  hr = DbgObject::CreateDbgObject(field_debug_value, GetCreationDepth() - 1,
      &dbg_object, GetErrorStream());
  if (FAILED(hr)) {
    return hr;
  }

  *field_value = std::move(dbg_object);
  return hr;
}

HRESULT DbgReferenceObject::GetICorDebugValue(
    ICorDebugValue **debug_value,
    ICorDebugEval *debug_eval) {
  if (object_handle_) {
    *debug_value = object_handle_;
    object_handle_->AddRef();
    return S_OK;
  }
  return E_FAIL;
}

HRESULT DbgReferenceObject::GetDebugHandle(ICorDebugHandleValue **result) {
  return GetICorDebugValue(reinterpret_cast<ICorDebugValue **>(result), nullptr);
}

}
