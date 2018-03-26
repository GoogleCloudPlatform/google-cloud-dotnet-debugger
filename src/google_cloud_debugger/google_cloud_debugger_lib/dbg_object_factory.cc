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

#include "dbg_object_factory.h"

#include <assert.h>
#include <cstdint>
#include <iostream>

#include "cor_debug_helper.h"
#include "dbg_array.h"
#include "dbg_builtin_collection.h"
#include "dbg_class.h"
#include "dbg_enum.h"
#include "dbg_primitive.h"
#include "dbg_string.h"
#include "i_eval_coordinator.h"
#include "type_signature.h"

using google::cloud::diagnostics::debug::Variable;
using std::ostream;
using std::string;
using std::unique_ptr;
using std::vector;

namespace google_cloud_debugger {

HRESULT DbgObjectFactory::CreateDbgObjectHelper(
    ICorDebugValue *debug_value, ICorDebugType *debug_type,
    CorElementType cor_element_type, BOOL is_null, int depth,
    std::unique_ptr<DbgObject> *result_object, ostream *err_stream) {
  assert(err_stream != nullptr);

  if (!result_object) {
    *err_stream << "result_object cannot be a null pointer";
    return E_INVALIDARG;
  }

  HRESULT hr;
  unique_ptr<DbgObject> temp_object;
  switch (cor_element_type) {
    case CorElementType::ELEMENT_TYPE_BOOLEAN:
      temp_object = unique_ptr<DbgObject>(new (std::nothrow)
                                              DbgPrimitive<bool>(debug_type));
      break;
    case CorElementType::ELEMENT_TYPE_CHAR:
      temp_object = unique_ptr<DbgObject>(new (std::nothrow)
                                              DbgPrimitive<char>(debug_type));
      break;
    case CorElementType::ELEMENT_TYPE_I:
      temp_object = unique_ptr<DbgObject>(
          new (std::nothrow) DbgPrimitive<intptr_t>(debug_type));
      break;
    case CorElementType::ELEMENT_TYPE_U:
      temp_object = unique_ptr<DbgObject>(
          new (std::nothrow) DbgPrimitive<uintptr_t>(debug_type));
      break;
    case CorElementType::ELEMENT_TYPE_I1:
      temp_object = unique_ptr<DbgObject>(new (std::nothrow)
                                              DbgPrimitive<int8_t>(debug_type));
      break;
    case CorElementType::ELEMENT_TYPE_U1:
      temp_object = unique_ptr<DbgObject>(
          new (std::nothrow) DbgPrimitive<uint8_t>(debug_type));
      break;
    case CorElementType::ELEMENT_TYPE_I2:
      temp_object = unique_ptr<DbgObject>(
          new (std::nothrow) DbgPrimitive<int16_t>(debug_type));
      break;
    case CorElementType::ELEMENT_TYPE_U2:
      temp_object = unique_ptr<DbgObject>(
          new (std::nothrow) DbgPrimitive<uint16_t>(debug_type));
      break;
    case CorElementType::ELEMENT_TYPE_I4:
      temp_object = unique_ptr<DbgObject>(
          new (std::nothrow) DbgPrimitive<int32_t>(debug_type));
      break;
    case CorElementType::ELEMENT_TYPE_U4:
      temp_object = unique_ptr<DbgObject>(
          new (std::nothrow) DbgPrimitive<uint32_t>(debug_type));
      break;
    case CorElementType::ELEMENT_TYPE_I8:
      temp_object = unique_ptr<DbgObject>(
          new (std::nothrow) DbgPrimitive<int64_t>(debug_type));
      break;
    case CorElementType::ELEMENT_TYPE_U8:
      temp_object = unique_ptr<DbgObject>(
          new (std::nothrow) DbgPrimitive<uint64_t>(debug_type));
      break;
    case CorElementType::ELEMENT_TYPE_R4:
      temp_object = unique_ptr<DbgObject>(new (std::nothrow)
                                              DbgPrimitive<float>(debug_type));
      break;
    case CorElementType::ELEMENT_TYPE_R8:
      temp_object = unique_ptr<DbgObject>(new (std::nothrow)
                                              DbgPrimitive<double>(debug_type));
      break;
    case CorElementType::ELEMENT_TYPE_STRING:
      temp_object = unique_ptr<DbgObject>(
          new (std::nothrow) DbgString(debug_type, debug_helper_));
      break;
    case CorElementType::ELEMENT_TYPE_SZARRAY:
    case CorElementType::ELEMENT_TYPE_ARRAY:
      temp_object = unique_ptr<DbgObject>(new (std::nothrow) DbgArray(
          debug_type, depth, debug_helper_,
          std::shared_ptr<DbgObjectFactory>(new DbgObjectFactory)));
      break;
    case CorElementType::ELEMENT_TYPE_CLASS:
    case CorElementType::ELEMENT_TYPE_VALUETYPE:
    case CorElementType::ELEMENT_TYPE_OBJECT:
      hr = CreateDbgClassObject(debug_type, depth, debug_value, is_null,
                                &temp_object, err_stream);
      if (FAILED(hr)) {
        return hr;
      }
      break;
    default:
      return E_NOTIMPL;
  }

  if (temp_object) {
    temp_object->Initialize(debug_value, is_null);
  } else {
    *err_stream << "Failed to create DbgObject.";
    return E_OUTOFMEMORY;
  }

  temp_object->SetCorElementType(cor_element_type);
  if (debug_value != nullptr) {
    CORDB_ADDRESS address = 0;
    hr = debug_value->GetAddress(&address);
    if (FAILED(hr)) {
      *err_stream << "Failed to get address of the object.";
      return hr;
    }
    temp_object->SetAddress(address);
  }

  (*result_object) = std::move(temp_object);
  return S_OK;
}

HRESULT DbgObjectFactory::CreateDbgObject(ICorDebugType *debug_type,
                                          unique_ptr<DbgObject> *result_object,
                                          ostream *err_stream) {
  assert(err_stream != nullptr);

  CorElementType cor_element_type;
  HRESULT hr;

  hr = debug_type->GetType(&cor_element_type);
  if (FAILED(hr)) {
    *err_stream << "Failed to get type: " << std::hex << hr;
    return hr;
  }

  return CreateDbgObjectHelper(nullptr, debug_type, cor_element_type, TRUE, 0,
                               result_object, err_stream);
}

DbgObjectFactory::DbgObjectFactory() : debug_helper_(new CorDebugHelper()) {}

HRESULT DbgObjectFactory::CreateDbgObject(ICorDebugValue *debug_value,
                                          int depth,
                                          unique_ptr<DbgObject> *result_object,
                                          ostream *err_stream) {
  assert(err_stream != nullptr);

  HRESULT hr;
  BOOL is_null = FALSE;
  CComPtr<ICorDebugValue> dereferenced_and_unboxed_value;
  CComPtr<ICorDebugType> debug_type;
  CorElementType cor_element_type;

  hr = debug_helper_->DereferenceAndUnbox(
      debug_value, &dereferenced_and_unboxed_value, &is_null, err_stream);
  if (FAILED(hr)) {
    *err_stream << "Failed to dereference and unbox.";
    return hr;
  }

  hr = debug_helper_->GetICorDebugType(dereferenced_and_unboxed_value,
                                       &debug_type, err_stream);

  if (SUCCEEDED(hr)) {
    hr = debug_type->GetType(&cor_element_type);
  } else if (hr == E_NOINTERFACE) {
    hr = debug_value->GetType(&cor_element_type);
  } else {
    // Nothing we can do here.
    return hr;
  }

  return CreateDbgObjectHelper(dereferenced_and_unboxed_value, debug_type,
                               cor_element_type, is_null, depth, result_object,
                               err_stream);
}

HRESULT DbgObjectFactory::CreateDbgClassObject(
    ICorDebugType *debug_type, int depth, ICorDebugValue *debug_value,
    BOOL is_null, unique_ptr<DbgObject> *result_object,
    std::ostream *err_stream) {
  HRESULT hr;
  CComPtr<ICorDebugClass> debug_class;
  if (!debug_type) {
    CComPtr<ICorDebugObjectValue> debug_obj_value;

    hr = debug_value->QueryInterface(
        __uuidof(ICorDebugObjectValue),
        reinterpret_cast<void **>(&debug_obj_value));

    if (FAILED(hr)) {
      *err_stream << "Cannot get class information.";
      return hr;
    }
    hr = debug_obj_value->GetClass(&debug_class);
  } else {
    hr = debug_type->GetClass(&debug_class);
  }

  if (FAILED(hr)) {
    *err_stream << "Failed to get class from object value.";
    return hr;
  }

  mdTypeDef class_token;
  hr = debug_class->GetToken(&class_token);
  if (FAILED(hr)) {
    *err_stream << "Failed to get class token.";
    return hr;
  }

  CComPtr<ICorDebugModule> debug_module;
  hr = debug_class->GetModule(&debug_module);
  if (FAILED(hr)) {
    *err_stream << "Failed to get module";
    return hr;
  }

  CComPtr<IMetaDataImport> metadata_import;
  hr = debug_helper_->GetMetadataImportFromICorDebugModule(
      debug_module, &metadata_import, err_stream);
  if (FAILED(hr)) {
    *err_stream << "Failed to get metadata";
    return hr;
  }

  string class_name;
  hr = ProcessClassName(class_token, metadata_import, &class_name, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  std::vector<WCHAR> wchar_module_name;
  hr = debug_helper_->GetModuleNameFromICorDebugModule(
      debug_module, &wchar_module_name, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  string module_name = ConvertWCharPtrToString(wchar_module_name);

  if (is_null) {
    unique_ptr<DbgClass> null_obj(new (std::nothrow) DbgClass(
        debug_type, depth, debug_helper_,
        std::shared_ptr<DbgObjectFactory>(new DbgObjectFactory())));
    if (!null_obj) {
      *err_stream << "Ran out of memory to create null class object.";
      return E_OUTOFMEMORY;
    }
    null_obj->SetModuleName(module_name);
    null_obj->SetClassName(class_name);
    null_obj->SetClassToken(class_token);
    *result_object = std::move(null_obj);
    return S_OK;
  }

  hr = ProcessPrimitiveType(debug_value, class_name, result_object, err_stream);
  if (SUCCEEDED(hr)) {
    // Nothing to do here as ProcessPrimitiveType already creates
    // a primitive object.
    return hr;
  } else if (hr == E_NOTIMPL) {
    string base_class_name;
    hr = ProcessBaseClassName(debug_type, &base_class_name, err_stream);
    if (FAILED(hr)) {
      *err_stream << "Failed to get the base class.";
      return hr;
    }

    unique_ptr<DbgClass> class_obj;

    if (kEnumClassName.compare(base_class_name) == 0) {
      unique_ptr<DbgEnum> enum_obj =
          unique_ptr<DbgEnum>(new (std::nothrow) DbgEnum(debug_type, depth));
      // We need the class token to process the enum.
      enum_obj->SetClassToken(class_token);
      // Only process class type for enum (since it is ValueType and we don't
      // store reference to the class object). Delay processing fields and
      // properties of non-ValueType class until we need them.
      hr = enum_obj->ProcessEnum(debug_value, debug_class, metadata_import);
      if (FAILED(hr)) {
        *err_stream << "Failed to process class based on their types.";
        return hr;
      }
      class_obj = std::move(enum_obj);
    } else if (kListClassName.compare(class_name) == 0 ||
               kHashSetClassName.compare(class_name) == 0 ||
               kDictionaryClassName.compare(class_name) == 0) {
      class_obj = unique_ptr<DbgBuiltinCollection>(
          new (std::nothrow) DbgBuiltinCollection(
              debug_type, depth, debug_helper_,
              std::shared_ptr<DbgObjectFactory>(new DbgObjectFactory())));
    } else {
      class_obj = unique_ptr<DbgClass>(new (std::nothrow) DbgClass(
          debug_type, depth, debug_helper_,
          std::shared_ptr<DbgObjectFactory>(new DbgObjectFactory())));
    }

    if (!class_obj) {
      *err_stream << "Ran out of memory to create class object.";
      return E_OUTOFMEMORY;
    }

    class_obj->SetModuleName(module_name);
    class_obj->SetClassName(class_name);
    class_obj->SetClassToken(class_token);

    // If this is a ValueType class, we have to process its members
    // because we can't store the reference to this class.
    CorElementType element_type;
    hr = debug_value->GetType(&element_type);
    if (FAILED(hr)) {
      *err_stream << "Failed to extract CorElementType.";
      return hr;
    }

    if (element_type == CorElementType::ELEMENT_TYPE_VALUETYPE &&
        kEnumClassName.compare(base_class_name) != 0) {
      hr = class_obj->ProcessClassMembersHelper(debug_value, debug_class,
                                                metadata_import);
      if (FAILED(hr)) {
        *err_stream << "Failed to process class members for ValueType.";
        return hr;
      }
      class_obj->SetProcessedClassMembers(true);
    }

    *result_object = std::move(class_obj);
    return hr;
  } else {
    *err_stream << "Failed to process value type.";
    return hr;
  }
}

HRESULT DbgObjectFactory::ProcessClassName(mdTypeDef class_token,
                                           IMetaDataImport *metadata_import,
                                           std::string *class_name,
                                           std::ostream *err_stream) {
  if (!class_name || !metadata_import || !err_stream) {
    return E_INVALIDARG;
  }

  mdToken parent_class = 0;
  return debug_helper_->GetTypeNameFromMdTypeDef(
      class_token, metadata_import, class_name, &parent_class, err_stream);
}

HRESULT DbgObjectFactory::ProcessBaseClassName(ICorDebugType *debug_type,
                                               std::string *base_class_name,
                                               std::ostream *err_stream) {
  if (!debug_type || !base_class_name || !err_stream) {
    return E_INVALIDARG;
  }

  HRESULT hr;
  CComPtr<ICorDebugType> base_type;

  hr = debug_type->GetBase(&base_type);
  if (FAILED(hr)) {
    *err_stream << "Failed to get base type.";
    return hr;
  }

  // This can happen if the current type is already
  // System.Object.
  if (!base_type) {
    *base_class_name = "";
    return S_OK;
  }

  CComPtr<ICorDebugClass> base_class;
  hr = base_type->GetClass(&base_class);
  if (FAILED(hr)) {
    *err_stream << "Failed to get base class.";
    return hr;
  }

  mdToken base_class_token = 0;
  hr = base_class->GetToken(&base_class_token);
  if (FAILED(hr)) {
    *err_stream << "Failed to get base class token.";
    return hr;
  }

  CComPtr<ICorDebugModule> base_debug_module;
  hr = base_class->GetModule(&base_debug_module);
  if (FAILED(hr)) {
    *err_stream << "Failed to get module for base class.";
    return hr;
  }

  CComPtr<IMetaDataImport> metadata_import;
  hr = debug_helper_->GetMetadataImportFromICorDebugModule(
      base_debug_module, &metadata_import, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  return ProcessClassName(base_class_token, metadata_import, base_class_name,
                          err_stream);
}

HRESULT DbgObjectFactory::ProcessPrimitiveType(
    ICorDebugValue *debug_value, const std::string &class_name,
    unique_ptr<DbgObject> *result_class_obj, std::ostream *err_stream) {
  if (kCharClassName.compare(class_name) == 0) {
    return ProcessValueTypeHelper<char>(debug_value, result_class_obj,
                                        err_stream);
  } else if (kBooleanClassName.compare(class_name) == 0) {
    return ProcessValueTypeHelper<bool>(debug_value, result_class_obj,
                                        err_stream);
  } else if (kSByteClassName.compare(class_name) == 0) {
    return ProcessValueTypeHelper<int8_t>(debug_value, result_class_obj,
                                          err_stream);
  } else if (kByteClassName.compare(class_name) == 0) {
    return ProcessValueTypeHelper<uint8_t>(debug_value, result_class_obj,
                                           err_stream);
  } else if (kInt16ClassName.compare(class_name) == 0) {
    return ProcessValueTypeHelper<int16_t>(debug_value, result_class_obj,
                                           err_stream);
  } else if (kUInt16ClassName.compare(class_name) == 0) {
    return ProcessValueTypeHelper<uint16_t>(debug_value, result_class_obj,
                                            err_stream);
  } else if (kInt32ClassName.compare(class_name) == 0) {
    return ProcessValueTypeHelper<int32_t>(debug_value, result_class_obj,
                                           err_stream);
  } else if (kUInt32ClassName.compare(class_name) == 0) {
    return ProcessValueTypeHelper<uint32_t>(debug_value, result_class_obj,
                                            err_stream);
  } else if (kInt64ClassName.compare(class_name) == 0) {
    return ProcessValueTypeHelper<int64_t>(debug_value, result_class_obj,
                                           err_stream);
  } else if (kUInt64ClassName.compare(class_name) == 0) {
    return ProcessValueTypeHelper<uint64_t>(debug_value, result_class_obj,
                                            err_stream);
  } else if (kSingleClassName.compare(class_name) == 0) {
    return ProcessValueTypeHelper<float>(debug_value, result_class_obj,
                                         err_stream);
  } else if (kDoubleClassName.compare(class_name) == 0) {
    return ProcessValueTypeHelper<double>(debug_value, result_class_obj,
                                          err_stream);
  } else if (kIntPtrClassName.compare(class_name) == 0) {
    return ProcessValueTypeHelper<intptr_t>(debug_value, result_class_obj,
                                            err_stream);
  } else if (kUIntPtrClassName.compare(class_name) == 0) {
    return ProcessValueTypeHelper<uintptr_t>(debug_value, result_class_obj,
                                             err_stream);
  }

  return E_NOTIMPL;
}

}  //  namespace google_cloud_debugger
