// Copyright 2015-2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#include "dbg_object.h"

#include <assert.h>
#include <cstdint>
#include <iostream>

#include "dbg_array.h"
#include "dbg_class.h"
#include "dbg_primitive.h"
#include "dbg_string.h"
#include "i_cor_debug_helper.h"
#include "i_eval_coordinator.h"
#include "type_signature.h"

using google::cloud::diagnostics::debug::Variable;
using std::ostream;
using std::string;
using std::unique_ptr;
using std::vector;

namespace google_cloud_debugger {

const std::int32_t DbgObject::collection_size_ = 10;

DbgObject::DbgObject(ICorDebugType *debug_type, int depth) {
  debug_type_ = debug_type;
  depth_ = depth;
}

HRESULT DbgObject::PopulateType(Variable *variable) {
  std::string type_string;
  HRESULT hr = GetTypeString(&type_string);
  if (FAILED(hr)) {
    return hr;
  }

  variable->set_type(type_string);
  return S_OK;
}

HRESULT DbgObject::GetTypeSignature(TypeSignature *type_signature) {
  std::string type_string;
  HRESULT hr = GetTypeString(&type_string);
  if (FAILED(hr)) {
    return hr;
  }

  *type_signature = {
    GetCorElementType(),
    type_string
  };
  return S_OK;
}

HRESULT DbgObject::GetNonStaticField(const std::string &field_name,
                                     std::shared_ptr<DbgObject> *field_value) {
  if (!object_handle_) {
    return E_INVALIDARG;
  }

  HRESULT hr;

  // Dereferences the object to get ICorDebugObjectValue.
  BOOL is_null = FALSE;
  CComPtr<ICorDebugValue> debug_value;
  HRESULT hr = Dereference(object_handle_, &debug_value, 
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

  if (!is_static) {
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

HRESULT DbgObject::CreateDbgObjectHelper(
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
      temp_object =
          unique_ptr<DbgObject>(new (std::nothrow) DbgString(debug_type));
      break;
    case CorElementType::ELEMENT_TYPE_SZARRAY:
    case CorElementType::ELEMENT_TYPE_ARRAY:
      temp_object =
          unique_ptr<DbgObject>(new (std::nothrow) DbgArray(debug_type, depth));
      break;
    case CorElementType::ELEMENT_TYPE_CLASS:
    case CorElementType::ELEMENT_TYPE_VALUETYPE:
    case CorElementType::ELEMENT_TYPE_OBJECT:
      hr = DbgClass::CreateDbgClassObject(debug_type, depth, debug_value,
                                          is_null, &temp_object, err_stream);
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

  temp_object->cor_element_type_ = cor_element_type;
  if (debug_value != nullptr) {
    hr = debug_value->GetAddress(&temp_object->address_);
    if (FAILED(hr)) {
      *err_stream << "Failed to get address of the object.";
      return hr;
    }
  }

  (*result_object) = std::move(temp_object);
  return S_OK;
}

HRESULT DbgObject::CreateDbgObject(ICorDebugType *debug_type,
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

  return DbgObject::CreateDbgObjectHelper(nullptr, debug_type, cor_element_type,
                                          TRUE, 0, result_object, err_stream);
}

HRESULT DbgObject::CreateDbgObject(ICorDebugValue *debug_value, int depth,
                                   unique_ptr<DbgObject> *result_object,
                                   ostream *err_stream) {
  assert(err_stream != nullptr);

  HRESULT hr;
  BOOL is_null = FALSE;
  CComPtr<ICorDebugValue> dereferenced_and_unboxed_value;
  CComPtr<ICorDebugType> debug_type;
  CorElementType cor_element_type;

  hr = DereferenceAndUnbox(debug_value, &dereferenced_and_unboxed_value,
                           &is_null, err_stream);
  if (FAILED(hr)) {
    *err_stream << "Failed to dereference and unbox.";
    return hr;
  }

  hr = GetICorDebugType(dereferenced_and_unboxed_value,
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

}  //  namespace google_cloud_debugger
