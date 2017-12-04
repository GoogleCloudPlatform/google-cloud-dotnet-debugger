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

#include "dbg_class.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>

#include "dbg_array.h"
#include "dbg_builtin_collection.h"
#include "dbg_enum.h"
#include "dbg_stack_frame.h"
#include "i_cor_debug_helper.h"
#include "i_eval_coordinator.h"

using google::cloud::diagnostics::debug::Variable;
using std::array;
using std::char_traits;
using std::min;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;

namespace google_cloud_debugger {

std::map<std::string, std::map<std::string, std::shared_ptr<IDbgClassMember>>>
    DbgClass::static_class_members_;

HRESULT DbgClass::CreateDbgClassObject(ICorDebugType *debug_type, int depth,
                                       ICorDebugValue *debug_value,
                                       BOOL is_null,
                                       unique_ptr<DbgObject> *result_object,
                                       std::ostringstream *err_stream) {
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
  hr = GetMetadataImportFromICorDebugModule(debug_module, &metadata_import);
  if (FAILED(hr)) {
    *err_stream << "Failed to get metadata";
    return hr;
  }

  string class_name;
  hr = ProcessClassName(class_token, metadata_import, &class_name, err_stream);
  if (FAILED(hr)) {
    *err_stream << "Failed to get class name.";
    return hr;
  }

  std::vector<WCHAR> wchar_module_name;
  hr = GetModuleNameFromICorDebugModule(debug_module, &wchar_module_name);
  if (FAILED(hr)) {
    *err_stream << "Failed to get module name.";
    return hr;
  }

  string module_name = ConvertWCharPtrToString(wchar_module_name);

  if (is_null) {
    unique_ptr<DbgClass> null_obj(new (std::nothrow)
                                      DbgClass(debug_type, depth));
    if (!null_obj) {
      *err_stream << "Ran out of memory to create null class object.";
      return E_OUTOFMEMORY;
    }
    null_obj->module_name_ = module_name;
    null_obj->class_name_ = class_name;
    null_obj->class_token_ = class_token;
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
      class_obj =
          unique_ptr<DbgEnum>(new (std::nothrow) DbgEnum(debug_type, depth));
    } else if (kListClassName.compare(class_name) == 0 ||
               kHashSetClassName.compare(class_name) == 0 ||
               kDictionaryClassName.compare(class_name) == 0) {
      class_obj = unique_ptr<DbgBuiltinCollection>(
          new (std::nothrow) DbgBuiltinCollection(debug_type, depth));
    } else {
      class_obj =
          unique_ptr<DbgClass>(new (std::nothrow) DbgClass(debug_type, depth));
    }

    if (!class_obj) {
      *err_stream << "Ran out of memory to create class object.";
      return E_OUTOFMEMORY;
    }

    class_obj->module_name_ = module_name;
    class_obj->class_name_ = class_name;
    class_obj->class_token_ = class_token;

    hr = class_obj->ProcessClassType(debug_value, debug_class, metadata_import);
    if (FAILED(hr)) {
      *err_stream << "Failed to process class based on their types.";
      return hr;
    }

    *result_object = std::move(class_obj);
    return hr;
  } else {
    *err_stream << "Failed to process value type.";
    return hr;
  }
}

HRESULT DbgClass::ProcessClassName(mdTypeDef class_token,
                                   IMetaDataImport *metadata_import,
                                   std::string *class_name,
                                   std::ostringstream *err_stream) {
  if (!class_name || !metadata_import || !err_stream) {
    return E_INVALIDARG;
  }

  HRESULT hr;
  ULONG len_class_name;
  DWORD class_flags = 0;
  mdToken parent_class = 0;

  // We have to call this function twice, once to get the length of the class
  // name and the second time to get the actual class name.
  // len_class_name includes the \0 at the end.
  hr = metadata_import->GetTypeDefProps(
      class_token, nullptr, 0, &len_class_name, &class_flags, &parent_class);
  if (FAILED(hr)) {
    *err_stream << "Failed to get class name's length.";
    return hr;
  }

  vector<WCHAR> wchar_class_name(len_class_name, 0);

  hr = metadata_import->GetTypeDefProps(class_token, wchar_class_name.data(),
                                        len_class_name, &len_class_name,
                                        &class_flags, &parent_class);
  if (FAILED(hr)) {
    *err_stream << "Failed to get class name.";
    return hr;
  }

  *class_name = ConvertWCharPtrToString(wchar_class_name);

  return S_OK;
}

HRESULT DbgClass::ProcessBaseClassName(ICorDebugType *debug_type,
                                       std::string *base_class_name,
                                       std::ostringstream *err_stream) {
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
  hr =
      GetMetadataImportFromICorDebugModule(base_debug_module, &metadata_import);
  if (FAILED(hr)) {
    *err_stream << "Failed to get metadata for base class.";
    return hr;
  }

  return ProcessClassName(base_class_token, metadata_import, base_class_name,
                          err_stream);
}

HRESULT DbgClass::ExtractField(const std::string &field_name,
                               std::shared_ptr<DbgObject> *field_value) {
  // Try to find the field field_name of this object.
  const auto &find_field = std::find_if(
      class_fields_.begin(), class_fields_.end(),
      [&](std::shared_ptr<IDbgClassMember> &class_field) {
        return class_field->GetMemberName().compare(field_name) == 0;
      });
  if (find_field == class_fields_.end()) {
    WriteError("Class does not have field " + field_name);
    return E_FAIL;
  }

  // Gets the underlying DbgObject that represents the field field_name
  // of this object.
  *field_value = (*find_field)->GetMemberValue();
  if (!field_value) {
    WriteError("Failed to evaluate value for field " + field_name);
    return E_FAIL;
  }

  return S_OK;
}

HRESULT DbgClass::ProcessParameterizedType() {
  HRESULT hr;
  CComPtr<ICorDebugTypeEnum> type_enum;
  CComPtr<ICorDebugType> debug_type;
  debug_type = GetDebugType();
  if (!debug_type) {
    return E_INVALIDARG;
  }

  hr = debug_type->EnumerateTypeParameters(&type_enum);
  if (FAILED(hr)) {
    WriteError("Failed to populate parameterized type for class.");
    return hr;
  }

  ULONG num_types = 0;
  ULONG num_types_fetched = 0;

  hr = type_enum->GetCount(&num_types);
  if (FAILED(hr)) {
    WriteError("Failed to get the number of class parameter types.");
    return hr;
  }

  if (num_types == 0) {
    return S_OK;
  }

  std::vector<ICorDebugType *> temp_types;
  temp_types.resize(num_types);
  generic_types_.resize(num_types);
  hr = type_enum->Next(num_types, temp_types.data(), &num_types_fetched);

  if (FAILED(hr)) {
    WriteError("Failed to fetch parameter types.");
    return hr;
  }

  for (int i = 0; i < num_types_fetched; ++i) {
    generic_types_[i] = temp_types[i];
    temp_types[i]->Release();
  }

  if (num_types_fetched == num_types && num_types_fetched != 0) {
    empty_generic_objects_.resize(num_types);
    for (int i = 0; i < num_types_fetched; ++i) {
      unique_ptr<DbgObject> empty_object;
      hr = DbgObject::CreateDbgObject(generic_types_[i], &empty_object,
                                      GetErrorStream());
      if (SUCCEEDED(hr)) {
        empty_generic_objects_[i] = std::move(empty_object);
      } else {
        WriteError("Failed to create a generic type object.");
        if (empty_object) {
          WriteError(empty_object->GetErrorString());
        }
        return hr;
      }
    }
  }

  return S_OK;
}

HRESULT DbgClass::ProcessFields(IMetaDataImport *metadata_import,
                                ICorDebugObjectValue *debug_obj_value,
                                ICorDebugClass *debug_class) {
  CComPtr<ICorDebugType> debug_type;
  HCORENUM cor_enum = nullptr;

  debug_type = GetDebugType();
  while (true) {
    HRESULT hr;
    array<mdFieldDef, 100> field_defs;
    ULONG field_defs_returned = 0;

    hr = metadata_import->EnumFields(&cor_enum, class_token_, field_defs.data(),
                                     field_defs.size(), &field_defs_returned);
    if (FAILED(hr)) {
      WriteError("Failed to enumerate class fields.");
      return hr;
    }

    if (field_defs_returned != 0) {
      class_fields_.reserve(class_fields_.size() + field_defs_returned);

      for (int i = 0; i < field_defs_returned; ++i) {
        unique_ptr<DbgClassField> class_field(new (std::nothrow)
                                                  DbgClassField());
        if (!class_field) {
          WriteError("Run out of memory when trying to create field ");
          WriteError(std::to_string(field_defs[i]));
          return E_OUTOFMEMORY;
        }

        class_field->Initialize(field_defs[i], metadata_import, debug_obj_value,
                                debug_class, debug_type, GetEvaluationDepth() - 1);
        if (class_field->IsBackingField()) {
          // Insert class names into set so we can use it to check later
          // for backing fields.
          class_backing_fields_names_.insert(class_field->GetMemberName());
        }

        AddStaticClassMemberToVector(std::move(class_field), &class_fields_);
      }
    } else {
      break;
    }
  }

  if (cor_enum) {
    metadata_import->CloseEnum(cor_enum);
  }

  return S_OK;
}

HRESULT DbgClass::ProcessProperties(IMetaDataImport *metadata_import) {
  HRESULT hr;
  array<mdProperty, 100> property_defs;
  HCORENUM cor_enum = nullptr;
  ULONG property_defs_returned = 0;

  while (true) {
    hr = metadata_import->EnumProperties(
        &cor_enum, class_token_, property_defs.data(), property_defs.size(),
        &property_defs_returned);
    if (FAILED(hr)) {
      WriteError("Failed to enumerate class properties.");
      return hr;
    }

    if (property_defs_returned != 0) {
      class_properties_.reserve(class_properties_.size() + property_defs_returned);

      for (int i = 0; i < property_defs_returned; ++i) {
        unique_ptr<DbgClassProperty> class_property(new (std::nothrow)
                                                        DbgClassProperty());
        if (!class_property) {
          WriteError(
              "Ran out of memory while trying to initialize class property ");
          WriteError(std::to_string(property_defs[i]));
          return E_OUTOFMEMORY;
        }

        class_property->Initialize(property_defs[i], metadata_import,
            GetEvaluationDepth() - 1);
        // If property name is MyProperty, checks whether there is a backing
        // field with the name <MyProperty>k__BackingField. Note that we have
        // logic to process backing fields' names to strip out the "<" and
        // ">k__BackingField" of the field name and places them in the set
        // class_backing_fields_names. Hence, we only need to check whether
        // MyProperty is in this set or not. If it is, then it is backed
        // by a field already, so don't add it to class_properties_.
        if (class_backing_fields_names_.find(class_property->GetMemberName()) !=
            class_backing_fields_names_.end()) {
          continue;
        }

        if (class_property->IsStatic()) {
          // Checks whether we already have a shared pointer of this property
          // in the cache. If not, moves the unique_ptr there.
          shared_ptr<IDbgClassMember> static_property_value =
              GetStaticClassMember(module_name_, class_name_,
                                   class_property->GetMemberName());
          if (!static_property_value) {
            std::string property_name = class_property->GetMemberName();
            static_property_value =
                shared_ptr<IDbgClassMember>(class_property.release());
            StoreStaticClassMember(module_name_, class_name_, property_name,
                                   static_property_value);
          }
          class_properties_.emplace_back(static_property_value);
        } else {
          class_properties_.push_back(std::move(class_property));
        }
      }
    } else {
      break;
    }
  }

  if (cor_enum != nullptr) {
    metadata_import->CloseEnum(cor_enum);
  }

  return S_OK;
}

HRESULT DbgClass::ProcessClassType(ICorDebugValue *debug_value,
                                   ICorDebugClass *debug_class,
                                   IMetaDataImport *metadata_import) {
  class_type_ = ClassType::DEFAULT;
  HRESULT hr;

  CComPtr<ICorDebugObjectValue> debug_obj_value;
  hr = debug_value->QueryInterface(__uuidof(ICorDebugObjectValue),
                                   reinterpret_cast<void **>(&debug_obj_value));
  if (FAILED(hr)) {
    WriteError("Failed to cast ICorDebugValue to ICorDebugObjValue.");
    return hr;
  }

  if (GetEvaluationDepth() == 0) {
    return S_OK;
  }

  // Populates the fields first before the properties in case
  // we have backing fields for properties.
  hr = ProcessFields(metadata_import, debug_obj_value, debug_class);
  if (FAILED(hr)) {
    WriteError("Failed to populate class fields.");
    return hr;
  }

  hr = ProcessProperties(metadata_import);
  if (FAILED(hr)) {
    WriteError("Failed to populate class properties.");
    return hr;
  }

  return hr;
}

HRESULT DbgClass::ProcessPrimitiveType(ICorDebugValue *debug_value,
                                       const std::string &class_name,
                                       unique_ptr<DbgObject> *result_class_obj,
                                       std::ostringstream *err_stream) {
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

HRESULT DbgClass::ExtractField(ICorDebugObjectValue *debug_obj_value,
                               ICorDebugClass *debug_class,
                               IMetaDataImport *metadata_import,
                               const std::string &field_name,
                               std::unique_ptr<DbgObject> *field_value) {
  HRESULT hr;
  mdFieldDef field_def;

  std::vector<WCHAR> wchar_field_name = ConvertStringToWCharPtr(field_name);
  hr = metadata_import->FindField(class_token_, wchar_field_name.data(),
                                  nullptr, 0, &field_def);
  if (FAILED(hr)) {
    WriteError("Failed to find field that represents items of the list.");
    return hr;
  }

  CComPtr<ICorDebugValue> debug_field_value;
  hr = debug_obj_value->GetFieldValue(debug_class, field_def,
                                      &debug_field_value);
  if (FAILED(hr)) {
    WriteError("Failed to get field that represents list's items.");
    return hr;
  }

  hr = CreateDbgObject(debug_field_value, GetEvaluationDepth() - 1, field_value,
                       GetErrorStream());
  if (FAILED(hr)) {
    WriteError("Failed to evaluate the items of the list.");
  }

  return hr;
}

void DbgClass::Initialize(ICorDebugValue *debug_value, BOOL is_null) {
  SetIsNull(is_null);
  CComPtr<ICorDebugType> debug_type;

  debug_type = GetDebugType();

  if (debug_type) {
    initialize_hr_ = ProcessParameterizedType();
    if (FAILED(initialize_hr_)) {
      WriteError("Fail to populate parameterized type.");
      return;
    }

    initialize_hr_ = debug_type->GetType(&cor_type_);
    if (FAILED(initialize_hr_)) {
      WriteError("Failed to get CorElementType from ICorDebugType.");
      return;
    }

    // Create a handle if it is a class so we won't lose the object.
    if (cor_type_ != CorElementType::ELEMENT_TYPE_VALUETYPE && !is_null) {
      initialize_hr_ =
          CreateStrongHandle(debug_value, &class_handle_, GetErrorStream());
      // E_NOINTERFACE is returned if object is a value type. In that
      // case, we don't need to create a handle.
      if (FAILED(initialize_hr_)) {
        WriteError("Failed to create strong handle for the class object.");
        return;
      }
    }
  }
}

ULONG64 DbgClass::ExtractEnumValue(CorElementType enum_type, void *enum_value) {
  switch (enum_type) {
    case ELEMENT_TYPE_I:
      return (ULONG64)(*((intptr_t *)enum_value));
    case ELEMENT_TYPE_U:
      return (ULONG64)(*((uintptr_t *)enum_value));
    case ELEMENT_TYPE_CHAR:
    case ELEMENT_TYPE_I1:
      return (ULONG64)(*((int8_t *)enum_value));
    case ELEMENT_TYPE_U1:
      return (ULONG64)(*((uint8_t *)enum_value));
    case ELEMENT_TYPE_I2:
      return (ULONG64)(*((int16_t *)enum_value));
    case ELEMENT_TYPE_U2:
      return (ULONG64)(*((uint16_t *)enum_value));
    case ELEMENT_TYPE_I4:
      return (ULONG64)(*((int32_t *)enum_value));
    case ELEMENT_TYPE_U4:
      return (ULONG64)(*((uint32_t *)enum_value));
    case ELEMENT_TYPE_I8:
      return (ULONG64)(*((int64_t *)enum_value));
    case ELEMENT_TYPE_U8:
      return (ULONG64)(*((uint64_t *)enum_value));
    default:
      return 0;
  }
}

void DbgClass::StoreStaticClassMember(const string &module_name,
                                      const string &class_name,
                                      const string &member_name,
                                      shared_ptr<IDbgClassMember> object) {
  string key = GetStaticCacheKey(module_name, class_name);
  if (static_class_members_.find(key) == static_class_members_.end()) {
    static_class_members_[key] =
        std::map<string, shared_ptr<IDbgClassMember>>();
  }

  static_class_members_[key][member_name] = object;
}

void DbgClass::AddStaticClassMemberToVector(
  unique_ptr<IDbgClassMember> class_member,
  vector<shared_ptr<IDbgClassMember>>* member_vector) {
  // If this is a static member, we will just use the member from the cache.
  if (class_member->IsStatic()) {
    // Checks whether we already have a shared pointer of this field
    // in the cache. If not, moves the unique_ptr there.
    shared_ptr<IDbgClassMember> static_member_value = GetStaticClassMember(
        module_name_, class_name_, class_member->GetMemberName());
    if (!static_member_value) {
      std::string field_name = class_member->GetMemberName();
      static_member_value =
          shared_ptr<IDbgClassMember>(class_member.release());
      StoreStaticClassMember(module_name_, class_name_, field_name,
                             static_member_value);
    }
    class_fields_.emplace_back(static_member_value);
  } else {
    class_fields_.push_back(std::move(class_member));
  }
}

void DbgClass::PopulateClassMembers(
  Variable *variable_proto,
  std::vector<VariableWrapper> *members,
  IEvalCoordinator *eval_coordinator,
  vector<shared_ptr<IDbgClassMember>> *class_members) {
  for (auto it = class_members->begin(); it != class_members->end(); ++it) {
    if (*it) {
      Variable *class_member_var = variable_proto->add_members();
      class_member_var->set_name((*it)->GetMemberName());

      HRESULT hr = (*it)->PopulateVariableValue(class_handle_,
        eval_coordinator, &generic_types_, GetEvaluationDepth() - 1);
      if (FAILED(hr)) {
        SetErrorStatusMessage(class_member_var, (*it).get());
        continue;
      }

      members->push_back(VariableWrapper(class_member_var, (*it)->GetMemberValue()));
    }
  }
}

shared_ptr<IDbgClassMember> DbgClass::GetStaticClassMember(
    const string &module_name, const string &class_name,
    const string &member_name) {
  string key = GetStaticCacheKey(module_name, class_name);
  if (static_class_members_.find(key) == static_class_members_.end() ||
      static_class_members_[key].find(member_name) ==
          static_class_members_[key].end()) {
    return shared_ptr<IDbgClassMember>();
  }

  return static_class_members_[key][member_name];
}

HRESULT DbgClass::PopulateMembers(
  Variable *variable_proto,
  std::vector<VariableWrapper> *members,
  IEvalCoordinator *eval_coordinator) {
  if (!members) {
    return E_INVALIDARG;
  }

  // No members to get.
  if (class_type_ == ClassType::PRIMITIVETYPE ||
    class_type_ == ClassType::ENUM) {
    return S_FALSE;
  }

  if (!eval_coordinator) {
    WriteError("No Eval Coordinator, cannot do evaluation of properties.");
    return E_INVALIDARG;
  }

  if (FAILED(initialize_hr_)) {
    return initialize_hr_;
  }

  if (GetIsNull()) {
    return S_OK;
  }

  if (GetEvaluationDepth() <= 0) {
    WriteError("... Object Inspection Depth Limit reached.");
    return E_FAIL;
  }

  PopulateClassMembers(variable_proto, members, eval_coordinator, &class_fields_);

  // Don't evaluate class properties if we don't need to.
  if (!eval_coordinator->PropertyEvaluation()) {
    return S_OK;
  }

  PopulateClassMembers(variable_proto, members, eval_coordinator, &class_properties_);

  return S_OK;
}

HRESULT DbgClass::PopulateType(Variable *variable) {
  if (!variable) {
    return E_INVALIDARG;
  }

  if (FAILED(initialize_hr_)) {
    return initialize_hr_;
  }

  if (class_name_.empty()) {
    WriteError("Cannot get class name.");
    return E_FAIL;
  }

  if (empty_generic_objects_.size() == 0) {
    variable->set_type(class_name_);
    return S_OK;
  }

  string class_name_with_generic = class_name_ + "<";

  unique_ptr<Variable> place_holder(new (std::nothrow) Variable());

  for (int i = 0; i < empty_generic_objects_.size(); ++i) {
    if (empty_generic_objects_[i]) {
      HRESULT hr = empty_generic_objects_[i]->PopulateType(place_holder.get());
      if (FAILED(hr)) {
        WriteError("Failed to print generic type in class");
        return hr;
      }
      class_name_with_generic += place_holder->type();

      if (empty_generic_objects_.size() > 1 &&
          i != empty_generic_objects_.size() - 1) {
        class_name_with_generic += ", ";
      }
    }
  }

  class_name_with_generic += ">";
  variable->set_type(class_name_with_generic);

  return S_OK;
}

}  // namespace google_cloud_debugger
