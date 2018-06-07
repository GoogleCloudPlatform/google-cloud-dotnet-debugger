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

#include "cor_debug_helper.h"

#include <assert.h>
#include <iostream>
#include <vector>

#include "ccomptr.h"
#include "class_names.h"
#include "compiler_helpers.h"
#include "constants.h"
#include "dbg_array.h"
#include "dbg_class.h"
#include "dbg_class_property.h"
#include "dbg_object_factory.h"
#include "dbg_primitive.h"
#include "dbg_stack_frame.h"
#include "dbg_string.h"
#include "error_messages.h"
#include "string_stream_wrapper.h"

using std::cerr;
using std::ostream;
using std::unique_ptr;
using std::vector;

namespace google_cloud_debugger {

HRESULT CorDebugHelper::GetMetadataImportFromICorDebugClass(
    ICorDebugClass *debug_class, IMetaDataImport **metadata_import,
    ostream *err_stream) {
  if (!debug_class) {
    *err_stream << "ICorDebugClass cannot be null.";
    return E_INVALIDARG;
  }

  if (!metadata_import) {
    *err_stream << "IMetaDataImport cannot be null.";
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

HRESULT CorDebugHelper::GetMetadataImportFromICorDebugModule(
    ICorDebugModule *debug_module, IMetaDataImport **metadata_import,
    ostream *err_stream) {
  if (!debug_module) {
    *err_stream << "ICorDebugModule cannot be null.";
    return E_INVALIDARG;
  }

  if (!metadata_import) {
    *err_stream << "IMetaDataImport cannot be null.";
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

HRESULT CorDebugHelper::GetModuleNameFromICorDebugModule(
    ICorDebugModule *debug_module, std::vector<WCHAR> *module_name,
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

HRESULT CorDebugHelper::GetAppDomainFromICorDebugModule(
    ICorDebugModule *debug_module, ICorDebugAppDomain **app_domain,
    std::ostream *err_stream) {
  CComPtr<ICorDebugAssembly> debug_assembly;
  HRESULT hr = debug_module->GetAssembly(&debug_assembly);
  if (FAILED(hr)) {
    *err_stream << "Failed to get ICorDebugAssembly from ICorDebugModule.";
    return hr;
  }

  return debug_assembly->GetAppDomain(app_domain);
}

HRESULT CorDebugHelper::GetICorDebugType(ICorDebugValue *debug_value,
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

HRESULT CorDebugHelper::Dereference(ICorDebugValue *debug_value,
                                    ICorDebugValue **dereferenced_value,
                                    BOOL *is_null, ostream *err_stream) {
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

  while (reference_depth < ICorDebugHelper::kReferenceDepth) {
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

  if (reference_depth == ICorDebugHelper::kReferenceDepth) {
    *err_stream << "Cannot dereference more than "
                << ICorDebugHelper::kReferenceDepth << " times!";
    return E_FAIL;
  }

  *is_null = local_is_null;
  (*dereferenced_value) = temp_value;
  temp_value->AddRef();
  return S_OK;
}

HRESULT CorDebugHelper::Unbox(ICorDebugValue *debug_value,
                              ICorDebugValue **unboxed_value,
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

HRESULT CorDebugHelper::DereferenceAndUnbox(
    ICorDebugValue *debug_value,
    ICorDebugValue **dereferenced_and_unboxed_value, BOOL *isNull,
    ostream *err_stream) {
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

HRESULT CorDebugHelper::CreateStrongHandle(ICorDebugValue *debug_value,
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

HRESULT CorDebugHelper::ExtractStringFromICorDebugStringValue(
    ICorDebugStringValue *debug_string, std::string *returned_string,
    std::ostream *err_stream) {
  if (!returned_string || !debug_string || !err_stream) {
    return E_INVALIDARG;
  }

  HRESULT hr;
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

HRESULT CorDebugHelper::ExtractParamName(IMetaDataImport *metadata_import,
                                         mdParamDef param_token,
                                         std::string *param_name,
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

HRESULT CorDebugHelper::GetICorDebugModuleFromICorDebugFrame(
    ICorDebugFrame *debug_frame, ICorDebugModule **debug_module,
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

HRESULT CorDebugHelper::ParseCompressedBytes(PCCOR_SIGNATURE *signature,
                                             ULONG *sig_len, ULONG *result) {
  ULONG bytes_read;
  HRESULT hr = CorSigUncompressData(*signature, *sig_len, result, &bytes_read);
  if (FAILED(hr)) {
    return hr;
  }

  *sig_len -= bytes_read;
  *signature += bytes_read;
  return hr;
}

HRESULT CorDebugHelper::ParseFieldSig(
    PCCOR_SIGNATURE *signature, ULONG *sig_len,
    IMetaDataImport *metadata_import,
    const std::vector<TypeSignature> &generic_class_types,
    TypeSignature *type_signature) {
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
  return ParseTypeFromSig(signature, sig_len, metadata_import,
                          generic_class_types, type_signature);
}

HRESULT CorDebugHelper::ParsePropertySig(
    PCCOR_SIGNATURE *signature, ULONG *sig_len,
    IMetaDataImport *metadata_import,
    const std::vector<TypeSignature> &generic_class_types,
    TypeSignature *type_signature) {
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
  ULONG no_of_params;
  hr = ParseCompressedBytes(signature, sig_len, &no_of_params);
  if (FAILED(hr)) {
    return hr;
  }

  // Parses the type.
  return ParseTypeFromSig(signature, sig_len, metadata_import,
                          generic_class_types, type_signature);
}

HRESULT CorDebugHelper::ParseTypeFromSig(
    PCCOR_SIGNATURE *signature, ULONG *sig_len,
    IMetaDataImport *metadata_import,
    const std::vector<TypeSignature> &generic_class_types,
    TypeSignature *type_signature) {
  HRESULT hr;
  type_signature->cor_type = CorSigUncompressElementType(*signature);
  *sig_len -= 1;

  // This handles "simple" type like numeric, boolean or string.
  std::string type_name;
  hr = TypeCompilerHelper::ConvertCorElementTypeToString(
      type_signature->cor_type, &type_name);
  if (SUCCEEDED(hr)) {
    type_signature->type_name = type_name;
    return hr;
  }

  // TODO(quoct): We are not supporting custom mod for parameters
  // CMOD_OPT and CMOD_REQ at the moment. Some quick research suggests
  // that C# compiler does not generate modopt so maybe we don't need
  // this yet?
  switch (type_signature->cor_type) {
    case CorElementType::ELEMENT_TYPE_SZARRAY: {
      // Extracts the array type.
      TypeSignature array_type;
      hr = ParseTypeFromSig(signature, sig_len, metadata_import,
                            generic_class_types, &array_type);
      if (FAILED(hr)) {
        return hr;
      }
      type_signature->type_name = kArrayClassName;
      type_signature->is_array = true;
      type_signature->generic_types.push_back(std::move(array_type));
      return hr;
    }
    case CorElementType::ELEMENT_TYPE_ARRAY: {
      // First we read the type.
      TypeSignature array_type;
      hr = ParseTypeFromSig(signature, sig_len, metadata_import,
                            generic_class_types, &array_type);
      if (FAILED(hr)) {
        return hr;
      }

      // Now we read the array ranks.
      ULONG array_rank = 0;
      hr = ParseCompressedBytes(signature, sig_len, &array_rank);
      if (FAILED(hr)) {
        return hr;
      }

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

      type_signature->type_name = kArrayClassName;
      type_signature->is_array = true;
      type_signature->array_rank = array_rank;
      type_signature->generic_types.push_back(std::move(array_type));
      return S_OK;
    }
    case CorElementType::ELEMENT_TYPE_GENERICINST: {
      // First we read the type without generics.
      TypeSignature main_type;
      hr = ParseTypeFromSig(signature, sig_len, metadata_import,
                            generic_class_types, &main_type);
      if (FAILED(hr)) {
        return hr;
      }

      type_signature->type_name = main_type.type_name;

      // Next byte would be the number of generic arguments.
      ULONG generic_param_count = 0;
      hr = ParseCompressedBytes(signature, sig_len, &generic_param_count);
      if (FAILED(hr)) {
        return hr;
      }

      // Now we get the generic arguments.
      for (size_t i = 0; i < generic_param_count; ++i) {
        TypeSignature generic_type;
        hr = ParseTypeFromSig(signature, sig_len, metadata_import,
                              generic_class_types, &generic_type);
        if (FAILED(hr)) {
          return hr;
        }
        type_signature->generic_types.push_back(std::move(generic_type));
      }
      return S_OK;
    }
    case CorElementType::ELEMENT_TYPE_CLASS:
    case CorElementType::ELEMENT_TYPE_VALUETYPE: {
      ULONG encoded_token = 0;
      hr = ParseCompressedBytes(signature, sig_len, &encoded_token);
      if (FAILED(hr)) {
        return hr;
      }

      // The first 2 least significant bits of the token tells us whether
      // the token is a type def or type ref.
      mdToken token_type = g_tkCorEncodeToken[encoded_token & 0x3];
      // Remove the last 2 bits of the token and use the token type to
      // decode it to either a mdTypeDef or mdTypeRef token.
      mdToken decoded_token = TokenFromRid(encoded_token >> 2, token_type);

      std::string type_name;
      hr = GetTypeNameFromMdToken(decoded_token, metadata_import,
                                  &type_name, &std::cerr);
      if (FAILED(hr)) {
        return hr;
      }

      type_signature->cor_type =
          TypeCompilerHelper::ConvertStringToCorElementType(type_name),
      type_signature->type_name = type_name;
      return S_OK;
    }
    case CorElementType::ELEMENT_TYPE_VAR: {
      // Var_number here is used to get the type from a generic class.
      // For example, if the generic class is Class<Int, String>
      // and var_number is 1, then the type referred to here is String.
      // If var_number is 0, then the type referred here is Int.
      ULONG var_number = 0;
      hr = ParseCompressedBytes(signature, sig_len, &var_number);
      if (FAILED(hr)) {
        return hr;
      }

      if (var_number >= generic_class_types.size()) {
        std::cerr << "Variable number in ELEMENT_TYPE_VAR cannot be found.";
        return E_FAIL;
      }

      *type_signature = generic_class_types[var_number];
      return S_OK;
    }
    default:
      return E_NOTIMPL;
  }
}

HRESULT CorDebugHelper::ParseAndSkipBasedOnFirstByteSignature(
    PCCOR_SIGNATURE *signature, ULONG *sig_len) {
  ULONG number_of_compressed_bytes_to_skip = 0;
  HRESULT hr = ParseCompressedBytes(signature, sig_len,
                                    &number_of_compressed_bytes_to_skip);
  if (FAILED(hr)) {
    return hr;
  }

  // Skip the next number_of_sizes byte since we don't need
  // to know the sizes.
  for (size_t i = 0; i < number_of_compressed_bytes_to_skip; ++i) {
    ULONG compressed_byte = 0;
    hr = ParseCompressedBytes(signature, sig_len, &compressed_byte);
    if (FAILED(hr)) {
      return hr;
    }
  }

  return S_OK;
}

HRESULT CorDebugHelper::ParseAndCheckFirstByte(
    PCCOR_SIGNATURE *signature, ULONG *sig_len,
    CorCallingConvention calling_convention) {
  ULONG field_bit = 0;
  HRESULT hr = ParseCompressedBytes(signature, sig_len, &field_bit);
  if (FAILED(hr)) {
    return hr;
  }

  if ((field_bit & calling_convention) == 0) {
    return META_E_BAD_SIGNATURE;
  }

  return S_OK;
}

HRESULT CorDebugHelper::GetFieldInfo(IMetaDataImport *metadata_import,
                                     mdTypeDef class_token,
                                     const std::string &field_name,
                                     mdFieldDef *field_def, bool *is_static,
                                     PCCOR_SIGNATURE *field_sig,
                                     ULONG *signature_len,
                                     std::ostream *err_stream) {
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

HRESULT CorDebugHelper::GetPropertyInfo(
    IMetaDataImport *metadata_import, mdProperty class_token,
    const std::string &prop_name, std::unique_ptr<DbgClassProperty> *result,
    ICorDebugModule *debug_module, std::ostream *err_stream) {
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

    std::shared_ptr<ICorDebugHelper> debug_helper(new CorDebugHelper());
    std::shared_ptr<IDbgObjectFactory> obj_factory(new DbgObjectFactory());
    for (int i = 0; i < property_defs_returned; ++i) {
      // We creates DbgClassProperty object and calls the Initialize
      // function to populate the name of the property.
      std::unique_ptr<DbgClassProperty> class_property(
          new (std::nothrow) DbgClassProperty(debug_helper, obj_factory));
      if (!class_property) {
        *err_stream
            << "Ran out of memory while trying to initialize class property ";
        return E_OUTOFMEMORY;
      }

      class_property->Initialize(property_defs[i], metadata_import,
                                 // The depth does not matter for now because we
                                 // are not evaluating any object.
                                 debug_module, kDefaultObjectEvalDepth);
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

HRESULT CorDebugHelper::GetTypeNameFromMdTypeDef(
    mdTypeDef type_token, IMetaDataImport *metadata_import,
    std::string *type_name, mdToken *base_token, std::ostream *err_stream) {
  if (metadata_import == nullptr || type_name == nullptr) {
    return E_INVALIDARG;
  }

  ULONG type_name_len = 0;
  DWORD type_flags = 0;
  HRESULT hr = metadata_import->GetTypeDefProps(
      type_token, nullptr, 0, &type_name_len, &type_flags, base_token);
  if (hr == S_FALSE) {
    hr = E_FAIL;
  }

  if (FAILED(hr)) {
    *err_stream << "Failed to get type name's length.";
    return hr;
  }

  vector<WCHAR> wchar_type_name(type_name_len, 0);
  hr = metadata_import->GetTypeDefProps(type_token, wchar_type_name.data(),
                                        wchar_type_name.size(), &type_name_len,
                                        &type_flags, base_token);
  if (FAILED(hr)) {
    *err_stream << "Failed to get type name.";
    return hr;
  }

  *type_name = ConvertWCharPtrToString(wchar_type_name);
  return hr;
}

HRESULT CorDebugHelper::GetTypeNameFromMdTypeRef(
    mdTypeRef type_token, IMetaDataImport *metadata_import,
    std::string *type_name, std::ostream *err_stream) {
  if (metadata_import == nullptr || type_name == nullptr) {
    return E_INVALIDARG;
  }

  ULONG type_name_len = 0;
  HRESULT hr = metadata_import->GetTypeRefProps(type_token, nullptr, nullptr, 0,
                                                &type_name_len);
  if (hr == S_FALSE) {
    hr = E_FAIL;
  }

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

HRESULT CorDebugHelper::GetTypeNameFromMdToken(
    mdToken type_token, IMetaDataImport *metadata_import,
    std::string *type_name, std::ostream *err_stream) {
  // Retrieves the parent class of the class that implements this field.
  CorTokenType token_type = (CorTokenType)(TypeFromToken(type_token));
  mdToken base_token;
  if (token_type == CorTokenType::mdtTypeDef) {
    return GetTypeNameFromMdTypeDef(
        type_token, metadata_import, type_name,
        &base_token, err_stream);
  } else if (token_type == CorTokenType::mdtTypeRef) {
    return GetTypeNameFromMdTypeRef(
        type_token, metadata_import, type_name, err_stream);
  }

  *err_stream << "Getting type name from other type of mdToken "
              << "is not supported.";
  return E_NOTIMPL;
}

HRESULT CorDebugHelper::GetMdTypeDefAndMetaDataFromTypeRef(
    mdTypeRef type_ref_token,
    const std::vector<CComPtr<ICorDebugAssembly>> &loaded_assemblies,
    IMetaDataImport *type_ref_token_metadata, mdTypeDef *result_type_def,
    IMetaDataImport **result_type_def_metadata, std::ostream *err_stream) {
  // We cannot use ResolveTypeRef here. It will just return a E_NOTIMPL
  // See
  // https://blogs.msdn.microsoft.com/davbr/2011/10/17/metadata-tokens-run-time-ids-and-type-loading/
  mdToken assembly_ref;
  ULONG type_ref_name_len = 0;
  HRESULT hr = type_ref_token_metadata->GetTypeRefProps(
      type_ref_token, &assembly_ref, nullptr, 0, &type_ref_name_len);
  if (FAILED(hr)) {
    *err_stream << "Failed to get type name's length.";
    return hr;
  }

  std::vector<WCHAR> type_ref_name_wchar(type_ref_name_len, 0);
  hr = type_ref_token_metadata->GetTypeRefProps(
      type_ref_token, &assembly_ref, type_ref_name_wchar.data(),
      type_ref_name_wchar.size(), &type_ref_name_len);
  if (FAILED(hr)) {
    *err_stream << "Failed to get type name.";
    return hr;
  }

  // Look through all available modules in all loaded assemblies and check
  // for a type name that matches type_ref_name_wchar.
  for (auto &enumerated_assembly : loaded_assemblies) {
    CComPtr<ICorDebugModuleEnum> module_enum;
    hr = enumerated_assembly->EnumerateModules(&module_enum);
    if (FAILED(hr)) {
      *err_stream << "Failed to get ICorDebugModuleEnum.";
      return hr;
    }

    std::vector<CComPtr<ICorDebugModule>> enumerated_modules;
    hr = EnumerateICorDebugSpecifiedType<ICorDebugModuleEnum, ICorDebugModule>(
        module_enum, &enumerated_modules);
    if (FAILED(hr)) {
      *err_stream << "Failed to enumerate modules.";
      return hr;
    }

    for (auto &enumerated_module : enumerated_modules) {
      CComPtr<IMetaDataImport> metadata_import;
      hr = GetMetadataImportFromICorDebugModule(enumerated_module,
                                                &metadata_import, err_stream);
      if (FAILED(hr)) {
        *err_stream << "Failed to get IMetaDataImport from ICorDebugModule.";
        return hr;
      }

      hr = metadata_import->FindTypeDefByName(type_ref_name_wchar.data(), NULL,
                                              result_type_def);
      if (hr != S_OK) {
        continue;
      }

      *result_type_def_metadata = metadata_import;
      metadata_import->AddRef();
      return S_OK;
    }
  }

  return E_FAIL;
}

HRESULT CorDebugHelper::GetAppDomainFromICorDebugFrame(
    ICorDebugFrame *debug_frame, ICorDebugAppDomain **app_domain,
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

HRESULT CorDebugHelper::CountGenericParams(IMetaDataImport *metadata_import,
                                           const mdToken &token,
                                           uint32_t *result) {
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
  ULONG generic_params_returned = -1;
  while (generic_params_returned != 0) {
    hr = metadata_import_2->EnumGenericParams(
        &cor_enum, token, generic_params.data(), generic_params.size(),
        &generic_params_returned);
    if (FAILED(hr)) {
      metadata_import_2->CloseEnum(cor_enum);
      return hr;
    }

    if (generic_params_returned == 0 || generic_params_returned == -1) {
      break;
    }
    *result += generic_params_returned;
  }

  metadata_import_2->CloseEnum(cor_enum);
  return S_OK;
}

HRESULT CorDebugHelper::GetInstantiatedClassType(
    ICorDebugClass *debug_class,
    std::vector<CComPtr<ICorDebugType>> *parameter_types,
    ICorDebugType **result_type, std::ostream *err_stream) {
  if (!debug_class || !parameter_types) {
    return E_INVALIDARG;
  }

  CComPtr<ICorDebugClass2> debug_class_2;
  HRESULT hr = debug_class->QueryInterface(
      __uuidof(ICorDebugClass2), reinterpret_cast<void **>(&debug_class_2));
  if (FAILED(hr)) {
    *err_stream << "Cannot convert ICorDebugClass to ICorDebugClass2.";
    return hr;
  }

  std::vector<ICorDebugType *> class_generic_type_pointers;
  class_generic_type_pointers.assign(parameter_types->begin(),
                                     parameter_types->end());

  return debug_class_2->GetParameterizedType(
      CorElementType::ELEMENT_TYPE_CLASS, class_generic_type_pointers.size(),
      class_generic_type_pointers.data(), result_type);
}

HRESULT CorDebugHelper::GetEnumInfoFromFieldMetaDataSignature(
    PCCOR_SIGNATURE metadata_signature,
    ULONG metadata_signature_len,
    ICorDebugModule *debug_module,
    IMetaDataImport *metadata_import,
    std::string *enum_name,
    mdTypeDef *enum_token,
    IMetaDataImport **resolved_metadata_import) {
  // We need to get the enum name. The name of the enum can be retrieved
  // from the third byte of the metadata signature.
  if (metadata_signature_len < 3) {
    return E_FAIL;
  }
  ULONG encoded_token = *(metadata_signature + 2);

  // Now we retrieve the name, metadata token and metadata import
  // for the enum.
  return GetTypeInfoFromEncodedToken(
      encoded_token, debug_module, metadata_import,
      enum_name, enum_token, resolved_metadata_import);
}

HRESULT CorDebugHelper::GetTypeInfoFromEncodedToken(
    ULONG encoded_token, ICorDebugModule *debug_module,
    IMetaDataImport *metadata_import, std::string *type_name,
    mdTypeDef *type_def, IMetaDataImport **resolved_metadata_import) {
  mdToken token_type = g_tkCorEncodeToken[encoded_token & 0x3];
  mdToken decoded_token = TokenFromRid(encoded_token >> 2, token_type);
  mdToken base_token;

  if (token_type != CorTokenType::mdtTypeDef &&
      token_type != CorTokenType::mdtTypeRef) {
    return E_FAIL;
  }

  if (token_type == CorTokenType::mdtTypeDef) {
    *resolved_metadata_import = metadata_import;
    metadata_import->AddRef();
    *type_def = decoded_token;
    return GetTypeNameFromMdTypeDef(decoded_token, metadata_import, type_name,
                                    &base_token, &std::cerr);
  }

  if (!debug_module) {
    return E_INVALIDARG;
  }

  // We have to get all the assemblies in order to get
  // the IMetaDataImport and mdTypeDef that corresponds with the
  // mdTypeRef.
  CComPtr<ICorDebugAppDomain> app_domain;
  HRESULT hr =
      GetAppDomainFromICorDebugModule(debug_module, &app_domain, &std::cerr);
  if (FAILED(hr)) {
    std::cerr << "Failed to get ICorDebugAppDomain from ICorDebugAssembly.";
    return hr;
  }

  CComPtr<ICorDebugAssemblyEnum> assembly_enum;
  hr = app_domain->EnumerateAssemblies(&assembly_enum);
  if (FAILED(hr)) {
    std::cerr << "Cannot get ICorDebugAssemblyEnum.";
    return hr;
  }

  std::vector<CComPtr<ICorDebugAssembly>> debug_assemblies;
  hr =
      EnumerateICorDebugSpecifiedType<ICorDebugAssemblyEnum, ICorDebugAssembly>(
          assembly_enum, &debug_assemblies);
  if (FAILED(hr)) {
    std::cerr << "Failed to enumerate assemblies.";
    return hr;
  }

  hr = GetMdTypeDefAndMetaDataFromTypeRef(decoded_token, debug_assemblies,
                                          metadata_import, type_def,
                                          resolved_metadata_import, &std::cerr);
  if (FAILED(hr)) {
    std::cerr << "Failed to get mdTypeDef and MetaDataImport";
    return hr;
  }

  return GetTypeNameFromMdTypeDef(*type_def, *resolved_metadata_import,
                                  type_name, &base_token, &std::cerr);
}

HRESULT CorDebugHelper::PopulateGenericClassTypesFromClassObject(
    ICorDebugValue *class_object,
    std::vector<CComPtr<ICorDebugType>> *generic_types,
    std::ostream *err_stream) {
  CComPtr<ICorDebugValue2> debug_value_2;
  HRESULT hr = class_object->QueryInterface(
      __uuidof(ICorDebugValue2), reinterpret_cast<void **>(&debug_value_2));

  if (FAILED(hr)) {
    *err_stream << "Failed to query ICorDebugValue2 from ICorDebugValue.";
    return hr;
  }

  CComPtr<ICorDebugType> debug_type;
  hr = debug_value_2->GetExactType(&debug_type);
  if (FAILED(hr)) {
    *err_stream << "Failed to get exact type from ICorDebugValue2.";
  }

  CComPtr<ICorDebugTypeEnum> type_enum;
  hr = debug_type->EnumerateTypeParameters(&type_enum);
  if (FAILED(hr)) {
    return hr;
  }

  return EnumerateICorDebugSpecifiedType<ICorDebugTypeEnum, ICorDebugType>(
      type_enum, generic_types);
}

HRESULT CorDebugHelper::ProcessConstantSigBlob(
    const std::vector<uint8_t> &signature_blob,
    CorElementType *cor_type,
    UVCP_CONSTANT *constant_value,
    ULONG *value_len,
    std::vector<uint8_t> *remaining_bytes) {
  static std::map<CorElementType, uint32_t> cor_type_to_bytes_size{
      {CorElementType::ELEMENT_TYPE_BOOLEAN, 1},
      {CorElementType::ELEMENT_TYPE_I1, 1},
      {CorElementType::ELEMENT_TYPE_CHAR, 1},
      {CorElementType::ELEMENT_TYPE_U1, 1},
      {CorElementType::ELEMENT_TYPE_I2, 2},
      {CorElementType::ELEMENT_TYPE_U2, 2},
      {CorElementType::ELEMENT_TYPE_I4, 4},
      {CorElementType::ELEMENT_TYPE_U4, 4},
      {CorElementType::ELEMENT_TYPE_I8, 8},
      {CorElementType::ELEMENT_TYPE_U8, 8},
      {CorElementType::ELEMENT_TYPE_R4, 4},
      {CorElementType::ELEMENT_TYPE_R8, 8},
      {CorElementType::ELEMENT_TYPE_I, 1},
      {CorElementType::ELEMENT_TYPE_U, 1},
      {CorElementType::ELEMENT_TYPE_STRING, 0}};

  if (signature_blob.empty()) {
    cerr << "Signature blob of constant is empty.";
    return E_INVALIDARG;
  }

  // First byte is the type of the signature.
  *cor_type = (CorElementType)signature_blob[0];

  // Next few bytes are the constant value. We have to check that there are
  // enough bytes.
  if (cor_type_to_bytes_size.find(*cor_type) ==
      cor_type_to_bytes_size.end()) {
    cerr << "Cannot process type of constant.";
    return E_FAIL;
  }

  // Check that there are enough bytes in the signature to read
  // the constant value.
  uint32_t const_size = cor_type_to_bytes_size[*cor_type];
  if (signature_blob.size() <= const_size) {
    cerr << "Not enough bytes to retrieve constant value.";
    return E_FAIL;
  }

  // If this is a string, we need the data in the signature to be
  // divisible by sizeof(WCHAR) and get the length of the string.
  if (*cor_type == CorElementType::ELEMENT_TYPE_STRING) {
    if ((signature_blob.size() - 1) % sizeof(WCHAR) != 0) {
      cerr << "Not enough bytes to read string value for constant.";
      return E_FAIL;
    }
    *value_len = (signature_blob.size() - 1) / sizeof(WCHAR);
  }

  // The first byte of the blob is the CorElementType and the next
  // few bytes will be the constant_value.
  *constant_value = (UVCP_CONSTANT)(signature_blob.data() + 1);

  // If there are bytes left, and the constant is not a string,
  // then this constant is an Enum and the remaining bytes are the metadata
  // token for the Enum class.
  uint32_t bytes_left = signature_blob.size() - 1 - const_size;
  if (bytes_left == 0 ||
      *cor_type == CorElementType::ELEMENT_TYPE_STRING) {
    return S_OK;
  }

  remaining_bytes->assign(signature_blob.end() - bytes_left,
                          signature_blob.end());
  return S_OK;
}

}  // namespace google_cloud_debugger
