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
#include <vector>

#include "ccomptr.h"
#include "class_names.h"
#include "compiler_helpers.h"
#include "constants.h"
#include "dbg_class_property.h"
#include "dbg_stack_frame.h"
#include "error_messages.h"
#include "string_stream_wrapper.h"

using std::cerr;
using std::ostream;
using std::vector;

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

HRESULT GetMetadataImportFromICorDebugModule(ICorDebugModule *debug_module,
                                             IMetaDataImport **metadata_import,
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
                         ICorDebugType **debug_type, ostream *err_stream) {
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
                           ICorDebugHandleValue **handle, ostream *err_stream) {
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
    ICorDebugStringValue *debug_string, std::string *returned_string,
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

  hr = debug_string->GetString(str_len, &str_returned_len, string_value.data());
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
                                             ICorDebugModule **debug_module,
                                             std::ostream *err_stream) {
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

HRESULT ParseFieldSig(PCCOR_SIGNATURE signature, ULONG *sig_len,
                      IMetaDataImport *metadata_import,
                      std::string *field_type_name) {
  // Field signature has the form: FIELD CustomModification* Type
  // However, we don't support Custom Modification (CMOD_OPT and CMOD_REQ).
  // I think this is not used in C#?
  // Parses the first bit.
  HRESULT hr = ParseAndCheckFirstByte(
      signature, sig_len, CorCallingConvention::IMAGE_CEE_CS_CALLCONV_FIELD);
  if (FAILED(hr)) {
    return hr;
  }

  // We don't support custom modifiers.
  return ParseTypeFromSig(signature, sig_len, metadata_import, field_type_name);
}

HRESULT ParsePropertySig(PCCOR_SIGNATURE signature, ULONG *sig_len,
                         IMetaDataImport *metadata_import,
                         std::string *property_type_name) {
  // Field signature has the form:
  // PROPERTY [HasThis] NumeberOfParameters CustomMod* Type Param*
  // However, we don't support Custom Modification (CMOD_OPT and CMOD_REQ).
  // I think this is not used in C#?
  // Parses the first bit.
  HRESULT hr = ParseAndCheckFirstByte(
      signature, sig_len, CorCallingConvention::IMAGE_CEE_CS_CALLCONV_PROPERTY);
  if (FAILED(hr)) {
    return hr;
  }

  // Parses the number of parameters for the property.
  ULONG bytes_read;
  ULONG no_of_params;
  hr = CorSigUncompressData(signature, *sig_len, &no_of_params, &bytes_read);
  if (FAILED(hr)) {
    return hr;
  }
  *sig_len -= bytes_read;

  // Parses the type.
  return ParseTypeFromSig(signature, sig_len, metadata_import,
                          property_type_name);
}

HRESULT ParseTypeFromSig(PCCOR_SIGNATURE signature, ULONG *sig_len,
                         IMetaDataImport *metadata_import,
                         std::string *type_name) {
  HRESULT hr;
  CorElementType cor_type = CorSigUncompressElementType(signature);
  *sig_len -= 1;

  // This handles "simple" type like numeric, boolean or string.
  hr = TypeCompilerHelper::ConvertCorElementTypeToString(cor_type, type_name);
  if (SUCCEEDED(hr)) {
    return hr;
  }

  // TODO(quoct): We are not supporting custom mod for parameters
  // CMOD_OPT and CMOD_REQ at the moment. Some quick research suggests
  // that C# compiler does not generate modopt so maybe we don't need
  // this yet?
  switch (cor_type) {
    case CorElementType::ELEMENT_TYPE_SZARRAY: {
      // Extracts the array type.
      hr = ParseTypeFromSig(signature, sig_len, metadata_import, type_name);
      if (FAILED(hr)) {
        return hr;
      }
      *type_name += "[]";
      return hr;
    }
    case CorElementType::ELEMENT_TYPE_ARRAY: {
      // First we read the type.
      hr = ParseTypeFromSig(signature, sig_len, metadata_import, type_name);
      if (FAILED(hr)) {
        return hr;
      }

      // Now we read the array ranks.
      ULONG array_rank = 0;
      ULONG bytes_read = 0;
      hr = CorSigUncompressData(signature, *sig_len, &array_rank, &bytes_read);
      if (FAILED(hr)) {
        return hr;
      }
      *sig_len -= bytes_read;

      // All we actually need is the rank but we have to skip
      // the additional information about the array stored.

      // Next byte will be number of sizes of the array.
      hr = ParseAndSkipBasedOnFirstByteSignature(signature, sig_len);
      if (FAILED(hr)) {
        return hr;
      }

      // Next, we do the same thing but for lower bounds.
      hr = ParseAndSkipBasedOnFirstByteSignature(signature, sig_len);
      if (FAILED(hr)) {
        return hr;
      }

      *type_name += "[";
      for (int i = 0; i < array_rank - 1; ++i) {
        *type_name += ",";
      }
      *type_name = "]";
      return S_OK;
    }
    case CorElementType::ELEMENT_TYPE_GENERICINST: {
      // First we read the type without generics.
      hr = ParseTypeFromSig(signature, sig_len, metadata_import, type_name);
      if (FAILED(hr)) {
        return hr;
      }

      // Next byte would be the number of generic arguments.
      ULONG generic_param_count = 0;
      ULONG bytes_read = 0;
      hr = CorSigUncompressData(signature, *sig_len, &generic_param_count,
                                &bytes_read);
      if (FAILED(hr)) {
        return hr;
      }
      *sig_len -= bytes_read;

      // Now we get the generic arguments.
      *type_name += "<";
      for (size_t i = 0; i < generic_param_count; ++i) {
        std::string generic_type;
        hr = ParseTypeFromSig(signature, sig_len, metadata_import,
                              &generic_type);
        if (FAILED(hr)) {
          return hr;
        }
        *type_name += generic_type;
        if (i != generic_param_count - 1) {
          *type_name += ", ";
        }
      }
      *type_name += ">";
      return S_OK;
    }
    case CorElementType::ELEMENT_TYPE_CLASS:
    case CorElementType::ELEMENT_TYPE_VALUETYPE: {
      ULONG bytes_read = 0;
      mdToken extracted_token;
      hr = CorSigUncompressToken(signature, *sig_len, &extracted_token,
                                 &bytes_read);
      if (FAILED(hr)) {
        return hr;
      }
      *sig_len -= bytes_read;
      // Checks whether this token is mdTypeDef or mdTypeRef. Don't support
      // other types.
      CorTokenType token_type = (CorTokenType)(TypeFromToken(extracted_token));
      mdToken base_token;
      if (token_type == CorTokenType::mdtTypeDef) {
        return GetTypeNameFromMdTypeDef(extracted_token, metadata_import,
                                        type_name, &base_token, &std::cerr);
      } else if (token_type == CorTokenType::mdtTypeRef) {
        return GetTypeNameFromMdTypeRef(extracted_token, metadata_import,
                                        type_name, &std::cerr);
      } else {
        // Don't process this.
        return E_FAIL;
      }
    }
    default:
      return E_NOTIMPL;
  }
}

HRESULT ParseAndSkipBasedOnFirstByteSignature(PCCOR_SIGNATURE signature,
                                              ULONG *sig_len) {
  ULONG bytes_read = 0;
  ULONG number_of_compressed_bytes_to_skip = 0;
  HRESULT hr = CorSigUncompressData(signature, *sig_len,
                            &number_of_compressed_bytes_to_skip, &bytes_read);
  if (FAILED(hr)) {
    return hr;
  }
  *sig_len -= bytes_read;

  // Skip the next number_of_sizes byte since we don't need
  // to know the sizes.
  for (size_t i = 0; i < number_of_compressed_bytes_to_skip; ++i) {
    ULONG compressed_byte = 0;
    hr = CorSigUncompressData(signature, *sig_len, &compressed_byte,
                              &bytes_read);
    if (FAILED(hr)) {
      return hr;
    }
    *sig_len -= bytes_read;
  }

  return S_OK;
}

HRESULT ParseAndCheckFirstByte(PCCOR_SIGNATURE signature, ULONG *sig_len,
                               CorCallingConvention calling_convention) {
  ULONG bytes_read = 0;
  ULONG field_bit = 0;
  HRESULT hr =
      CorSigUncompressData(signature, *sig_len, &field_bit, &bytes_read);
  if (FAILED(hr)) {
    return hr;
  }

  if (bytes_read != 1 || (field_bit & calling_convention) == 0) {
    return META_E_BAD_SIGNATURE;
  }
  *sig_len -= bytes_read;

  return S_OK;
}

HRESULT GetFieldInfo(IMetaDataImport *metadata_import, mdTypeDef class_token,
                     const std::string &field_name, mdFieldDef *field_def,
                     bool *is_static, PCCOR_SIGNATURE *field_sig,
                     ULONG *signature_len, std::ostream *err_stream) {
  std::vector<WCHAR> wchar_field_name = ConvertStringToWCharPtr(field_name);
  HRESULT hr = metadata_import->FindField(class_token, wchar_field_name.data(),
                                          nullptr, 0, field_def);
  if (FAILED(hr)) {
    return hr;
  }

  ULONG len_field;
  DWORD field_attributes;
  DWORD default_value_flags = 0;
  UVCP_CONSTANT default_value;
  ULONG default_value_len = 0;
  // Now we need to sets whether the field is static.
  hr = metadata_import->GetFieldProps(*field_def, &class_token, nullptr, 0,
                                      &len_field, &field_attributes, field_sig,
                                      signature_len, &default_value_flags,
                                      &default_value, &default_value_len);
  if (FAILED(hr)) {
    return hr;
  }

  *is_static = IsFdStatic(field_attributes);
  return S_OK;
}

HRESULT GetPropertyInfo(IMetaDataImport *metadata_import,
                        mdProperty class_token, const std::string &prop_name,
                        std::unique_ptr<DbgClassProperty> *result,
                        std::ostream *err_stream) {
  HRESULT hr;
  std::vector<mdProperty> property_defs(kDefaultVectorSize, 0);
  HCORENUM cor_enum = nullptr;
  ULONG property_defs_returned = 1;

  // Enumerates through the properties and extracts out
  // the one that matches prop_name.
  while (property_defs_returned != 0) {
    hr = metadata_import->EnumProperties(
        &cor_enum, class_token, property_defs.data(), property_defs.size(),
        &property_defs_returned);
    if (FAILED(hr)) {
      *err_stream << "Failed to enumerate class properties.";
      metadata_import->CloseEnum(cor_enum);
      return hr;
    }

    for (int i = 0; i < property_defs_returned; ++i) {
      // We creates DbgClassProperty object and calls the Initialize
      // function to populate the name of the property.
      std::unique_ptr<DbgClassProperty> class_property(new (std::nothrow)
                                                           DbgClassProperty());
      if (!class_property) {
        *err_stream
            << "Ran out of memory while trying to initialize class property ";
        return E_OUTOFMEMORY;
      }

      class_property->Initialize(property_defs[i], metadata_import,
                                 // The depth does not matter for now because we
                                 // are not evaluating any object.
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
  }

  if (cor_enum != nullptr) {
    metadata_import->CloseEnum(cor_enum);
  }

  return S_FALSE;
}

HRESULT GetTypeNameFromMdTypeDef(mdTypeDef type_token,
                                 IMetaDataImport *metadata_import,
                                 std::string *type_name, mdToken *base_token,
                                 std::ostream *err_stream) {
  if (metadata_import == nullptr || type_name == nullptr) {
    return E_INVALIDARG;
  }

  ULONG type_name_len = 0;
  DWORD type_flags = 0;
  mdToken extend_tokens;
  HRESULT hr = metadata_import->GetTypeDefProps(
      type_token, nullptr, 0, &type_name_len, &type_flags, &extend_tokens);
  if (FAILED(hr)) {
    *err_stream << "Failed to get type name's length.";
    return hr;
  }

  vector<WCHAR> wchar_type_name(type_name_len, 0);
  hr = metadata_import->GetTypeDefProps(type_token, wchar_type_name.data(),
                                        wchar_type_name.size(), &type_name_len,
                                        &type_flags, &extend_tokens);
  if (FAILED(hr)) {
    *err_stream << "Failed to get type name.";
    return hr;
  }

  *type_name = ConvertWCharPtrToString(wchar_type_name);
  return hr;
}

HRESULT GetTypeNameFromMdTypeRef(mdTypeRef type_token,
                                 IMetaDataImport *metadata_import,
                                 std::string *type_name,
                                 std::ostream *err_stream) {
  if (metadata_import == nullptr || type_name == nullptr) {
    return E_INVALIDARG;
  }

  ULONG type_name_len = 0;
  HRESULT hr = metadata_import->GetTypeRefProps(type_token, nullptr, nullptr, 0,
                                                &type_name_len);
  if (FAILED(hr)) {
    *err_stream << "Failed to get type name's length.";
    return hr;
  }

  vector<WCHAR> wchar_type_name(type_name_len, 0);
  hr = metadata_import->GetTypeRefProps(type_token, nullptr,
                                        wchar_type_name.data(),
                                        wchar_type_name.size(), &type_name_len);
  if (FAILED(hr)) {
    *err_stream << "Failed to get type name.";
    return hr;
  }

  *type_name = ConvertWCharPtrToString(wchar_type_name);
  return hr;
}

HRESULT GetMdTypeDefAndMetaDataFromTypeRef(
    mdTypeRef type_ref_token, IMetaDataImport *type_ref_token_metadata,
    mdTypeDef *result_type_def, IMetaDataImport **result_type_def_metadata) {
  CComPtr<IUnknown> i_unknown;
  HRESULT hr = type_ref_token_metadata->ResolveTypeRef(
      type_ref_token, IID_IMetaDataImport, &i_unknown, result_type_def);
  if (FAILED(hr)) {
    return hr;
  }

  return i_unknown->QueryInterface(
      IID_IMetaDataImport, reinterpret_cast<void **>(result_type_def_metadata));
}

HRESULT GetAppDomainFromICorDebugFrame(ICorDebugFrame *debug_frame,
                                       ICorDebugAppDomain **app_domain,
                                       std::ostream *err_stream) {
  CComPtr<ICorDebugFunction> debug_function;
  HRESULT hr = debug_frame->GetFunction(&debug_function);
  if (FAILED(hr)) {
    *err_stream << "Failed to get ICorDebugFunction from ICorDebugFrame.";
    return hr;
  }

  CComPtr<ICorDebugModule> debug_module;
  hr = debug_function->GetModule(&debug_module);
  if (FAILED(hr)) {
    *err_stream << "Failed to get ICorDebugModule from ICorDebugFunction.";
    return hr;
  }

  CComPtr<ICorDebugAssembly> debug_assembly;
  hr = debug_module->GetAssembly(&debug_assembly);
  if (FAILED(hr)) {
    *err_stream << "Failed to get ICorDebugAssembly from ICorDebugModule.";
    return hr;
  }

  hr = debug_assembly->GetAppDomain(app_domain);
  if (FAILED(hr)) {
    *err_stream << "Failed to get ICorDebugAppDomain from ICorDebugAssembly.";
  }

  return hr;
}

HRESULT CountGenericParams(IMetaDataImport *metadata_import,
                           const mdToken &token, uint32_t *result) {
  HRESULT hr;
  CComPtr<IMetaDataImport2> metadata_import_2;

  hr = metadata_import->QueryInterface(
      IID_IMetaDataImport2, reinterpret_cast<void **>(&metadata_import_2));
  if (FAILED(hr)) {
    return hr;
  }

  HCORENUM cor_enum = nullptr;
  *result = 0;
  vector<mdGenericParam> generic_params(100, 0);
  while (true) {
    ULONG generic_params_returned = 0;
    hr = metadata_import_2->EnumGenericParams(
        &cor_enum, token, generic_params.data(), generic_params.size(),
        &generic_params_returned);
    if (FAILED(hr)) {
      metadata_import_2->CloseEnum(cor_enum);
      return hr;
    }

    if (generic_params_returned == 0) {
      break;
    }
    *result += generic_params_returned;
  }

  metadata_import_2->CloseEnum(cor_enum);
  return S_OK;
}

}  // namespace google_cloud_debugger
