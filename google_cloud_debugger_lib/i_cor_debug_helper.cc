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

#include "i_cor_debug_helper.h"

#include <assert.h>
#include <iostream>

#include "ccomptr.h"

using std::cerr;
using std::ostream;

namespace google_cloud_debugger {

HRESULT GetMetadataImportFromICorDebugClass(ICorDebugClass *debug_class,
    IMetaDataImport **metadata_import,
    ostream *err_stream) {
  if (!debug_class) {
    *err_stream << "ICorDebugClass cannot be null.";
    return E_INVALIDARG;
  }

  CComPtr<ICorDebugModule> debug_module;
  HRESULT hr = debug_class->GetModule(&debug_module);
  if (FAILED(hr)) {
    *err_stream << "Failed to extract ICorDebugModule from ICorDebugClass.";
    return hr;
  }

  return GetMetadataImportFromICorDebugModule(debug_module, metadata_import,
                                              err_stream);
}

HRESULT GetMetadataImportFromICorDebugModule(
    ICorDebugModule *debug_module, IMetaDataImport **metadata_import,
    ostream *err_stream) {
  if (!debug_module) {
    *err_stream << "ICorDebugModule cannot be null.";
    return E_INVALIDARG;
  }

  CComPtr<IUnknown> temp_import;
  HRESULT hr;

  hr = debug_module->GetMetaDataInterface(IID_IMetaDataImport, &temp_import);

  if (FAILED(hr)) {
    *err_stream << "Failed to get metadata import.";
    return hr;
  }

  hr = temp_import->QueryInterface(IID_IMetaDataImport,
                                   reinterpret_cast<void **>(metadata_import));
  if (FAILED(hr)) {
    *err_stream << "Failed to import metadata from module";
    return hr;
  }

  return S_OK;
}

HRESULT GetModuleNameFromICorDebugModule(ICorDebugModule *debug_module,
                                         std::vector<WCHAR> *module_name,
                                         ostream *err_stream) {
  if (!module_name || !debug_module) {
    return E_INVALIDARG;
  }

  ULONG32 module_name_len = 0;
  HRESULT hr = debug_module->GetName(0, &module_name_len, nullptr);
  if (FAILED(hr)) {
    *err_stream << "Failed to get length of the module name.";
    return hr;
  }

  module_name->resize(module_name_len);
  hr = debug_module->GetName(module_name->size(), &module_name_len,
                             module_name->data());
  if (FAILED(hr)) {
    *err_stream << "Failed to get module name.";
  }

  return hr;
}

HRESULT GetICorDebugType(ICorDebugValue *debug_value,
                         ICorDebugType **debug_type,
                         ostream *err_stream) {
  if (!debug_value || !debug_type) {
    return E_INVALIDARG;
  }

  CComPtr<ICorDebugValue2> debug_value_2;
  HRESULT hr = debug_value->QueryInterface(
      __uuidof(ICorDebugValue2), reinterpret_cast<void **>(&debug_value_2));

  if (FAILED(hr)) {
    *err_stream << "Failed to query ICorDebugValue2 from ICorDebugValue.";
    return hr;
  }

  hr = debug_value_2->GetExactType(debug_type);
  if (FAILED(hr)) {
    *err_stream << "Failed to get exact type from ICorDebugValue2.";
  }

  return hr;
}

HRESULT Dereference(ICorDebugValue *debug_value,
                    ICorDebugValue **dereferenced_value, BOOL *is_null,
                    ostream *err_stream) {
  assert(err_stream != nullptr);

  if (!debug_value || !dereferenced_value) {
    *err_stream
        << "debug_value and dereferenced_value cannot be a null pointer.";
    return E_INVALIDARG;
  }

  BOOL local_is_null = FALSE;
  HRESULT hr;
  int reference_depth = 0;
  CComPtr<ICorDebugValue> temp_value;
  temp_value = debug_value;

  while (reference_depth < kReferenceDepth) {
    CComPtr<ICorDebugReferenceValue> debug_reference;

    hr =
        temp_value->QueryInterface(__uuidof(ICorDebugReferenceValue),
                                   reinterpret_cast<void **>(&debug_reference));

    // If not a reference value, don't do anything.
    if (hr == E_NOINTERFACE) {
      *is_null = FALSE;
      break;
    } else if (FAILED(hr)) {
      *err_stream
          << "Failed to convert ICorDebugValue to ICorDebugReferenceValue";
      return hr;
    }

    hr = debug_reference->IsNull(&local_is_null);

    if (FAILED(hr)) {
      *err_stream << "Failed to check whether reference is null or not.";
      return hr;
    }

    // Null reference;
    if (local_is_null) {
      break;
    }

    hr = debug_reference->Dereference(&temp_value);
    if (FAILED(hr)) {
      return hr;
    }

    reference_depth++;
  }

  if (reference_depth == kReferenceDepth) {
    *err_stream << "Cannot dereference more than " << kReferenceDepth
                << " times!";
    return E_FAIL;
  }

  *is_null = local_is_null;
  (*dereferenced_value) = temp_value;
  temp_value->AddRef();
  return S_OK;
}

HRESULT Unbox(ICorDebugValue *debug_value, ICorDebugValue **unboxed_value,
              ostream *err_stream) {
  if (!unboxed_value) {
    *err_stream << "dereferenced_value cannot be a null pointer.";
    return E_INVALIDARG;
  }

  HRESULT hr;
  CComPtr<ICorDebugBoxValue> boxed_value;

  // If it's not a boxed value, don't do anything.
  hr = debug_value->QueryInterface(__uuidof(ICorDebugBoxValue),
                                   reinterpret_cast<void **>(&boxed_value));
  if (hr == E_NOINTERFACE) {
    (*unboxed_value) = debug_value;
    debug_value->AddRef();
    return S_OK;
  } else if (FAILED(hr)) {
    *err_stream << "Failed to query ICorDebugBoxValue.";
    return hr;
  }

  // Unboxing!
  CComPtr<ICorDebugObjectValue> debug_object_value;
  hr = boxed_value->GetObject(&debug_object_value);
  if (FAILED(hr)) {
    *err_stream << "Failed get underlying object from boxed object.";
    return hr;
  }

  (*unboxed_value) = debug_object_value;
  (*unboxed_value)->AddRef();

  return S_OK;
}

HRESULT DereferenceAndUnbox(ICorDebugValue *debug_value,
                            ICorDebugValue **dereferenced_and_unboxed_value,
                            BOOL *isNull, ostream *err_stream) {
  assert(err_stream != nullptr);

  HRESULT hr;
  CComPtr<ICorDebugValue> dereferenced_value;
  CComPtr<ICorDebugValue> unboxed_value;

  hr = Dereference(debug_value, &dereferenced_value, isNull, err_stream);
  if (FAILED(hr)) {
    *err_stream << "Failed to dereference value.";
    return hr;
  }

  hr = Unbox(dereferenced_value, &unboxed_value, err_stream);
  if (FAILED(hr)) {
    *err_stream << "Failed to unbox value.";
    return hr;
  }

  (*dereferenced_and_unboxed_value) = unboxed_value;
  unboxed_value->AddRef();
  return S_OK;
}

HRESULT CreateStrongHandle(ICorDebugValue *debug_value,
                           ICorDebugHandleValue **handle,
                           ostream *err_stream) {
  assert(err_stream != nullptr);

  if (!debug_value) {
    *err_stream << "debug_value should not be null.";
    return E_INVALIDARG;
  }

  HRESULT hr;
  CComPtr<ICorDebugHeapValue2> heap_value;

  hr = debug_value->QueryInterface(__uuidof(ICorDebugHeapValue2),
                                   reinterpret_cast<void **>(&heap_value));
  if (FAILED(hr)) {
    *err_stream << "Failed to get heap value from ICorDebugValue.";
    return hr;
  }

  return heap_value->CreateHandle(CorDebugHandleType::HANDLE_STRONG, handle);
}

}  // namespace google_cloud_debugger
