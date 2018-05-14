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
#include "i_cor_debug_helper.h"
#include "i_dbg_object_factory.h"
#include "i_eval_coordinator.h"
#include "variable_wrapper.h"

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

HRESULT DbgClass::GetNonStaticField(const std::string &field_name,
                                    std::shared_ptr<DbgObject> *field_value) {
  if (!class_fields_.empty()) {
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

  return DbgReferenceObject::GetNonStaticField(field_name, field_value);
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
      hr = object_factory_->CreateDbgObject(
        generic_types_[i], &empty_object, GetErrorStream());
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
                                                  DbgClassField(debug_helper_,
                                                    object_factory_));
        if (!class_field) {
          WriteError("Run out of memory when trying to create field ");
          WriteError(std::to_string(field_defs[i]));
          return E_OUTOFMEMORY;
        }

        class_field->Initialize(field_defs[i], metadata_import, debug_obj_value,
                                debug_class, debug_type, GetCreationDepth() - 1);
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
      class_properties_.reserve(class_properties_.size() +
                                property_defs_returned);

      for (int i = 0; i < property_defs_returned; ++i) {
        unique_ptr<DbgClassProperty> class_property(
            new (std::nothrow) DbgClassProperty(debug_helper_, object_factory_));
        if (!class_property) {
          WriteError(
              "Ran out of memory while trying to initialize class property ");
          WriteError(std::to_string(property_defs[i]));
          return E_OUTOFMEMORY;
        }

        class_property->Initialize(property_defs[i], metadata_import,
                                   debug_module_, GetCreationDepth() - 1);
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

HRESULT DbgClass::ProcessClassMembers() {
  if (processed_) {
    return S_OK;
  }

  BOOL is_null = FALSE;
  CComPtr<ICorDebugValue> debug_value;
  HRESULT hr = debug_helper_->Dereference(object_handle_, &debug_value,
                                          &is_null, GetErrorStream());
  // Error already written into the error stream.
  if (FAILED(hr)) {
    return hr;
  }

  if (is_null) {
    return S_FALSE;
  }

  CComPtr<ICorDebugObjectValue> object_value;
  hr = debug_value->QueryInterface(__uuidof(ICorDebugObjectValue),
                                   reinterpret_cast<void **>(&object_value));
  if (FAILED(hr)) {
    WriteError("Failed to cast to ICorDebugObjectValue.");
    return hr;
  }

  CComPtr<ICorDebugClass> debug_class;
  hr = object_value->GetClass(&debug_class);
  if (FAILED(hr)) {
    WriteError("Failed to get ICorDebugClass.");
    return hr;
  }

  CComPtr<IMetaDataImport> metadata_import;
  hr = debug_helper_->GetMetadataImportFromICorDebugClass(
      debug_class, &metadata_import, GetErrorStream());
  if (FAILED(hr)) {
    return hr;
  }

  hr = ProcessClassMembersHelper(debug_value, debug_class,
                                 metadata_import);
  if (FAILED(hr)) {
    WriteError("Failed to process class members.");
    return hr;
  }

  processed_ = true;
  return S_OK;
}

HRESULT DbgClass::ProcessClassMembersHelper(ICorDebugValue *debug_value,
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

  if (GetCreationDepth() <= 0) {
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

HRESULT DbgClass::ExtractField(ICorDebugObjectValue *debug_obj_value,
                               ICorDebugClass *debug_class,
                               IMetaDataImport *metadata_import,
                               const std::string &field_name,
                               std::unique_ptr<DbgObject> *field_value) {
  if (!debug_class || !debug_obj_value || !metadata_import || !field_value) {
    return E_INVALIDARG;
  }

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

  hr = object_factory_->CreateDbgObject(debug_field_value,
      GetCreationDepth() - 1, field_value, GetErrorStream());
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
      initialize_hr_ = debug_helper_->CreateStrongHandle(
          debug_value, &object_handle_, GetErrorStream());
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
    vector<shared_ptr<IDbgClassMember>> *member_vector) {
  // If this is a static member, we will just use the member from the cache.
  if (class_member->IsStatic()) {
    // Checks whether we already have a shared pointer of this field
    // in the cache. If not, moves the unique_ptr there.
    shared_ptr<IDbgClassMember> static_member_value = GetStaticClassMember(
        module_name_, class_name_, class_member->GetMemberName());
    if (!static_member_value) {
      std::string field_name = class_member->GetMemberName();
      static_member_value = shared_ptr<IDbgClassMember>(class_member.release());
      StoreStaticClassMember(module_name_, class_name_, field_name,
                             static_member_value);
    }
    class_fields_.emplace_back(static_member_value);
  } else {
    class_fields_.push_back(std::move(class_member));
  }
}

void DbgClass::PopulateClassMembers(
    Variable *variable_proto, std::vector<VariableWrapper> *members,
    IEvalCoordinator *eval_coordinator,
    vector<shared_ptr<IDbgClassMember>> *class_members) {
  for (auto it = class_members->begin(); it != class_members->end(); ++it) {
    if (*it) {
      Variable *class_member_var = variable_proto->add_members();
      class_member_var->set_name((*it)->GetMemberName());

      HRESULT hr =
          (*it)->Evaluate(object_handle_, eval_coordinator,
                          &generic_types_);
      if (FAILED(hr)) {
        SetErrorStatusMessage(class_member_var, (*it).get());
        continue;
      }

      members->push_back(
          VariableWrapper(class_member_var, (*it)->GetMemberValue()));
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

HRESULT DbgClass::PopulateMembers(Variable *variable_proto,
                                  std::vector<VariableWrapper> *members,
                                  IEvalCoordinator *eval_coordinator) {
  if (!members || !variable_proto) {
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

  if (GetCreationDepth() <= 0) {
    WriteError("... Object Inspection Depth Limit reached.");
    return E_FAIL;
  }

  HRESULT hr = ProcessClassMembers();
  if (FAILED(hr)) {
    WriteError("Failed to process class members.");
    return hr;
  }

  PopulateClassMembers(variable_proto, members, eval_coordinator,
                       &class_fields_);

  // Don't evaluate class properties if we don't need to.
  if (!eval_coordinator->PropertyEvaluation()) {
    return S_OK;
  }

  PopulateClassMembers(variable_proto, members, eval_coordinator,
                       &class_properties_);

  return S_OK;
}

HRESULT DbgClass::GetTypeSignature(TypeSignature *type_signature) {
  type_signature->cor_type = cor_element_type_;
  type_signature->type_name = class_name_;

  for (int i = 0; i < empty_generic_objects_.size(); ++i) {
    if (empty_generic_objects_[i]) {
      TypeSignature generic_sig;
      HRESULT hr = empty_generic_objects_[i]->GetTypeSignature(&generic_sig);
      if (FAILED(hr)) {
        return hr;
      }
      type_signature->generic_types.push_back(std::move(generic_sig));
    }
  }

  return S_OK;
}

HRESULT DbgClass::GetTypeString(std::string *type_string) {
  if (!type_string) {
    return E_INVALIDARG;
  }

  if (FAILED(initialize_hr_)) {
    return initialize_hr_;
  }

  if (class_name_.empty()) {
    WriteError("Cannot get class name.");
    return E_FAIL;
  }

  *type_string = class_name_;

  if (empty_generic_objects_.size() == 0) {
    return S_OK;
  }

  *type_string += "<";

  unique_ptr<Variable> place_holder(new (std::nothrow) Variable());

  for (int i = 0; i < empty_generic_objects_.size(); ++i) {
    if (empty_generic_objects_[i]) {
      std::string generic_type_name;
      HRESULT hr = empty_generic_objects_[i]->GetTypeString(&generic_type_name);
      if (FAILED(hr)) {
        WriteError("Failed to print generic type in class");
        return hr;
      }
      *type_string += generic_type_name;

      if (empty_generic_objects_.size() > 1 &&
          i != empty_generic_objects_.size() - 1) {
        *type_string += ", ";
      }
    }
  }

  *type_string += ">";
  return S_OK;
}

}  // namespace google_cloud_debugger
