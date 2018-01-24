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
#include "string_stream_wrapper.h"
#include "error_messages.h"
#include "dbg_class_property.h"
#include "constants.h"

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

HRESULT ExtractStringFromICorDebugStringValue(
    ICorDebugStringValue *debug_string,
    std::string *returned_string,
    std::ostream *err_stream) {
  if (!returned_string || !debug_string || !err_stream) {
    return E_INVALIDARG;
  }

  HRESULT hr;
  std::vector<WCHAR> string_value;
  ULONG32 str_len;
  ULONG32 str_returned_len;

  hr = debug_string->GetLength(&str_len);
  if (FAILED(hr)) {
    *err_stream << "Failed to extract the string.";
    return hr;
  }

  if (str_len == 0) {
    *returned_string = "";
    return S_OK;
  }

  // Plus 1 for the NULL at the end of the string.
  str_len += 1;
  std::vector<WCHAR> string_value(str_len, 0);

  hr = debug_string->GetString(str_len, &str_returned_len,
                               string_value.data());
  if (FAILED(hr)) {
    *err_stream << "Failed to extract the string.";
    return hr;
  }

  *returned_string = ConvertWCharPtrToString(string_value);
  return S_OK;
}

HRESULT ExtractParamName(IMetaDataImport *metadata_import,
    mdParamDef param_token, std::string *param_name,
    std::ostream *err_stream) {
  mdMethodDef method_token;
  ULONG ordinal_position;
  ULONG param_name_size;
  DWORD param_attributes;
  DWORD value_type_flag;
  UVCP_CONSTANT const_string_value;
  ULONG const_string_value_size;
  std::vector<WCHAR> wchar_param_name;

  HRESULT hr = metadata_import->GetParamProps(
      param_token, &method_token, &ordinal_position, nullptr, 0,
      &param_name_size, &param_attributes, &value_type_flag,
      &const_string_value, &const_string_value_size);
  if (FAILED(hr) || param_name_size == 0) {
    *err_stream << "Failed to get length of name of method argument: "
          << method_token << " with hr: " << std::hex << hr;
    return hr;
  }

  wchar_param_name.resize(param_name_size);
  hr = metadata_import->GetParamProps(
      param_token, &method_token, &ordinal_position, wchar_param_name.data(),
      wchar_param_name.size(), &param_name_size, &param_attributes,
      &value_type_flag, &const_string_value, &const_string_value_size);
  if (FAILED(hr)) {
    cerr << "Failed to get name of method argument: " << param_token
          << " with hr: " << std::hex << hr;
    return hr;
  }

  *param_name = ConvertWCharPtrToString(wchar_param_name);
  return hr;
}

HRESULT GetICorDebugModuleFromICorDebugFrame(ICorDebugFrame *debug_frame,
    ICorDebugModule **debug_module, std::ostream *err_stream) {
  CComPtr<ICorDebugFunction> debug_function;
  HRESULT hr = debug_frame->GetFunction(&debug_function);
  if (FAILED(hr)) {
    *err_stream << kFailedDebugFuncFromFrame;
    return hr;
  }

  hr = debug_function->GetModule(debug_module);
  if (FAILED(hr)) {
    *err_stream << kFailedDebugModuleFromFunc;
  }

  return hr;
}

HRESULT ExtractFieldInfo(IMetaDataImport *metadata_import,
    mdTypeDef class_token, const std::string &field_name,
    mdFieldDef *field_def, bool *is_static, std::ostream *err_stream) {
  std::vector<WCHAR> wchar_field_name = ConvertStringToWCharPtr(field_name);
  HRESULT hr = metadata_import->FindField(class_token, wchar_field_name.data(), nullptr, 0, field_def);
  if (FAILED(hr)) {
    return hr;
  }

  ULONG len_field;
  DWORD field_attributes;
  PCCOR_SIGNATURE field_sig;
  ULONG sig_len = 0;
  DWORD default_value_flags = 0;
  UVCP_CONSTANT default_value;
  ULONG default_value_len = 0;
  // Now we need to sets whether the field is static.
  hr = metadata_import->GetFieldProps(*field_def, &class_token, nullptr, 0,
      &len_field, &field_attributes, &field_sig, &sig_len,
      &default_value_flags, &default_value, &default_value_len);
  if (FAILED(hr)) {
    return hr;
  }

  *is_static = IsFdStatic(field_attributes);
>>>>>>> dd8b132... Implement identifier evaluator:google_cloud_debugger_lib/i_cor_debug_helper.cc
  return S_OK;
}

HRESULT ExtractPropertyInfo(IMetaDataImport *metadata_import,
    mdProperty class_token, const std::string &prop_name,
    std::unique_ptr<DbgClassProperty> *result, std::ostream *err_stream) {
  HRESULT hr;
  std::vector<mdProperty> property_defs(100, 0);
  HCORENUM cor_enum = nullptr;
  ULONG property_defs_returned = 0;

  // Enumerates through the properties and extracts out
  // the one that matches prop_name.
  while (true) {
    hr = metadata_import->EnumProperties(
        &cor_enum, class_token, property_defs.data(), property_defs.size(),
        &property_defs_returned);
    if (FAILED(hr)) {
      *err_stream << "Failed to enumerate class properties.";
      metadata_import->CloseEnum(cor_enum);
      return hr;
    }

    if (property_defs_returned != 0) {
      for (int i = 0; i < property_defs_returned; ++i) {
        // We creates DbgClassProperty object and calls the Initialize
        // function to populate the name of the property.
        std::unique_ptr<DbgClassProperty> class_property(new (std::nothrow)
                                                        DbgClassProperty());
        if (!class_property) {
          *err_stream <<
              "Ran out of memory while trying to initialize class property ";
          return E_OUTOFMEMORY;
        }

        class_property->Initialize(property_defs[i], metadata_import,
            // The depth does not matter for now because we are
            // not evaluating any object.
            kDefaultObjectEvalDepth);
        if (FAILED(class_property->GetInitializeHr())) {
          *err_stream << "Failed to get property information.";
          metadata_import->CloseEnum(cor_enum);
          return class_property->GetInitializeHr();
        }

        if (prop_name.compare(class_property->GetMemberName()) == 0) {
          *result = std::move(class_property);
          metadata_import->CloseEnum(cor_enum);
          return S_OK;
        }
      }
    } else {
      break;
    }
  }

  if (cor_enum != nullptr) {
    metadata_import->CloseEnum(cor_enum);
  }

  return S_FALSE;
}

}  // namespace google_cloud_debugger
