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

#include "dbg_class_field.h"

#include <iostream>

#include "breakpoint.pb.h"
#include "constants.h"
#include "dbg_enum.h"
#include "dbg_primitive.h"
#include "dbg_string.h"
#include "i_cor_debug_helper.h"
#include "i_dbg_object_factory.h"
#include "i_eval_coordinator.h"
#include "type_signature.h"

using google::cloud::diagnostics::debug::Variable;
using std::string;
using std::unique_ptr;

namespace google_cloud_debugger {
DbgClassField::DbgClassField(mdFieldDef field_def, int creation_depth,
                             ICorDebugType *class_type,
                             std::shared_ptr<ICorDebugHelper> debug_helper,
                             std::shared_ptr<IDbgObjectFactory> obj_factory)
    : IDbgClassMember(debug_helper, obj_factory) {
  if (!obj_factory_) {
    std::cerr << "Object factory is null.";
    initialized_hr_ = E_INVALIDARG;
    return;
  }
  field_def_ = field_def;
  creation_depth_ = creation_depth;
  class_type_ = class_type;
}

void DbgClassField::Initialize(ICorDebugModule *debug_module,
                               IMetaDataImport *metadata_import,
                               ICorDebugObjectValue *debug_obj_value,
                               ICorDebugClass *debug_class) {
  if (metadata_import == nullptr) {
    WriteError("MetaDataImport is null.");
    initialized_hr_ = E_INVALIDARG;
    return;
  }

  CComPtr<ICorDebugValue> field_value;
  ULONG len_field_name;

  // First call to get length of array.
  initialized_hr_ = metadata_import->GetFieldProps(
      field_def_, &parent_token_, nullptr, 0, &len_field_name,
      &member_attributes_, &signature_metadata_, &sig_metadata_length_,
      &default_value_type_flags_, &default_value_, &default_value_len_);
  if (FAILED(initialized_hr_)) {
    WriteError("Failed to populate field metadata.");
    return;
  }

  std::vector<WCHAR> wchar_field_name(len_field_name, 0);

  // Second call to get the actual name.
  initialized_hr_ = metadata_import->GetFieldProps(
      field_def_, &parent_token_, wchar_field_name.data(), len_field_name,
      &len_field_name, &member_attributes_, &signature_metadata_,
      &sig_metadata_length_, &default_value_type_flags_, &default_value_,
      &default_value_len_);
  if (FAILED(initialized_hr_)) {
    WriteError("Failed to populate field metadata.");
    return;
  }

  member_name_ = ConvertWCharPtrToString(wchar_field_name);

  // If field name is <MyProperty>k__BackingField, change it to
  // MyProperty because it is the backing field of a property.
  if (member_name_.size() > kBackingField.size() + 1) {
    // Checks that field name is of the form <Property>k__BackingField.
    if (member_name_[0] == '<') {
      string::size_type position;
      // Checks that field_name_ ends with k_BackingField.
      position = member_name_.find(kBackingField,
                                   member_name_.size() - kBackingField.size());
      // Extracts out the field name.
      if (position != string::npos) {
        is_backing_field_ = true;
        member_name_ = member_name_.substr(1, position - 1);
      }
    }
  }

  // This will point to the value of the field if the field is const.
  if (default_value_ && IsFdLiteral(member_attributes_)) {
    initialized_hr_ = ProcessConstField(debug_module, metadata_import);
    return;
  }

  if (IsStatic()) {
    initialized_hr_ = S_OK;
    return;
  }

  if (debug_obj_value == nullptr) {
    WriteError("ICorDebugObjectValue is null.");
    initialized_hr_ = E_INVALIDARG;
    return;
  }

  if (debug_class == nullptr) {
    WriteError("ICorDebugClass is null.");
    initialized_hr_ = E_INVALIDARG;
    return;
  }

  initialized_hr_ =
      debug_obj_value->GetFieldValue(debug_class, field_def_, &field_value);
  if (initialized_hr_ == CORDBG_E_FIELD_NOT_AVAILABLE) {
    WriteError("Field is optimized away");
    return;
  }

  if (initialized_hr_ == CORDBG_E_CLASS_NOT_LOADED) {
    WriteError("Class of the field is not loaded.");
    return;
  }

  if (initialized_hr_ == CORDBG_E_VARIABLE_IS_ACTUALLY_LITERAL) {
    WriteError(
        "Field is a literal. It is optimized away and is not available.");
    return;
  }

  if (FAILED(initialized_hr_)) {
    WriteError("Failed to get field value.");
    return;
  }

  unique_ptr<DbgObject> member_value;
  initialized_hr_ = obj_factory_->CreateDbgObject(
      field_value, creation_depth_, &member_value, GetErrorStream());

  if (FAILED(initialized_hr_)) {
    WriteError("Failed to create DbgObject for field.");
    if (member_value) {
      WriteError(member_value->GetErrorString());
    }
  }

  member_value_ = std::move(member_value);
}

HRESULT DbgClassField::Evaluate(
    ICorDebugValue *debug_value, IEvalCoordinator *eval_coordinator,
    std::vector<CComPtr<ICorDebugType>> *generic_types) {
  if (FAILED(initialized_hr_)) {
    return initialized_hr_;
  }

  if (!eval_coordinator) {
    return E_INVALIDARG;
  }

  HRESULT hr;
  if (IsStatic() && !member_value_) {
    hr = ExtractStaticFieldValue(eval_coordinator);
    if (FAILED(hr)) {
      WriteError("Failed to extract static field.");
      return hr;
    }
  }

  if (!member_value_) {
    WriteError("Cannot get field value.");
    return E_FAIL;
  }

  return S_OK;
}

HRESULT DbgClassField::ProcessConstField(ICorDebugModule *debug_module,
                                         IMetaDataImport *metadata_import) {
  CorElementType field_type = (CorElementType)default_value_type_flags_;
  ULONG64 enum_numerical_value = 0;
  std::unique_ptr<DbgObject> dbg_object;
  HRESULT hr = obj_factory_->CreateDbgObjectFromLiteralConst(
      field_type, default_value_, default_value_len_, &enum_numerical_value,
      &dbg_object);

  if (FAILED(hr)) {
    return hr;
  }

  // If this is an enum, the signature length will be greater than 2
  // and the second byte will be CorElementType::ELEMENT_TYPE_VALUETYPE.
  CorElementType second_byte_type =
      (CorElementType) * (signature_metadata_ + 1);
  if (second_byte_type == CorElementType::ELEMENT_TYPE_VALUETYPE) {
    return ProcessConstEnumField(debug_module, metadata_import, field_type,
                                 enum_numerical_value);
  }

  member_value_ = std::move(dbg_object);
  return S_OK;
}

HRESULT DbgClassField::ProcessConstEnumField(ICorDebugModule *debug_module,
                                             IMetaDataImport *metadata_import,
                                             const CorElementType &enum_type,
                                             ULONG64 enum_numerical_value) {
  // Gets the class that implements this field.
  std::string class_name;
  mdToken base_token;
  HRESULT hr = debug_helper_->GetTypeNameFromMdTypeDef(
      parent_token_, metadata_import, &class_name, &base_token,
      GetErrorStream());

  std::string base_class_name;
  hr = debug_helper_->GetTypeNameFromMdToken(
      base_token, metadata_import, &base_class_name, GetErrorStream());
  if (FAILED(hr)) {
    return hr;
  }

  // Do not proceed if the class implementing this field is
  // an enum (which means the parent is System.Enum).
  // This can lead to an infinite loop.
  if (base_class_name.compare(kEnumClassName) == 0) {
    return S_OK;
  }

  // Now we retrieve the name, metadata token and metadata import
  // for the enum.
  std::string enum_name;
  CComPtr<IMetaDataImport> resolved_metadata_import;
  mdTypeDef enum_token;

  hr = debug_helper_->GetEnumInfoFromFieldMetaDataSignature(
      signature_metadata_, sig_metadata_length_, debug_module,
      metadata_import, &enum_name, &enum_token,
      &resolved_metadata_import);
  if (FAILED(hr)) {
    return hr;
  }

  std::unique_ptr<DbgObject> dbg_object;
  dbg_object = std::unique_ptr<DbgObject>(
      new DbgEnum(1, enum_name, enum_token, enum_numerical_value, enum_type,
                  debug_helper_, obj_factory_));
  DbgEnum *dbg_enum = dynamic_cast<DbgEnum *>(dbg_object.get());
  if (!dbg_enum) {
    return E_FAIL;
  }

  hr = dbg_enum->ProcessEnumFields(resolved_metadata_import);
  if (FAILED(hr)) {
    return hr;
  }

  member_value_ = std::move(dbg_object);
  return S_OK;
}

HRESULT DbgClassField::ExtractStaticFieldValue(
    IEvalCoordinator *eval_coordinator) {
  CComPtr<ICorDebugThread> active_thread;
  HRESULT hr = eval_coordinator->GetActiveDebugThread(&active_thread);
  if (FAILED(hr)) {
    WriteError("Failed to get active debug thread.");
    return hr;
  }

  CComPtr<ICorDebugFrame> debug_frame;
  hr = active_thread->GetActiveFrame(&debug_frame);
  if (FAILED(hr)) {
    WriteError("Failed to get the active frame.");
    return hr;
  }

  CComPtr<ICorDebugValue> debug_value;
  hr = class_type_->GetStaticFieldValue(field_def_, debug_frame, &debug_value);
  if (hr == CORDBG_E_STATIC_VAR_NOT_AVAILABLE) {
    WriteError("Static variable is not yet available.");
    return hr;
  }

  if (hr == CORDBG_E_VARIABLE_IS_ACTUALLY_LITERAL) {
    WriteError("Static variable is literal optimized away.");
    return hr;
  }

  if (FAILED(hr)) {
    WriteError("Failed to get static field value.");
    return hr;
  }

  unique_ptr<DbgObject> member_value;

  hr = obj_factory_->CreateDbgObject(debug_value, creation_depth_,
                                     &member_value, GetErrorStream());
  if (FAILED(hr)) {
    if (member_value) {
      WriteError(member_value->GetErrorString());
    }
    WriteError("Failed to create DbgObject for static field value.");
    return hr;
  }

  member_value_ = std::move(member_value);
  return hr;
}

}  // namespace google_cloud_debugger
