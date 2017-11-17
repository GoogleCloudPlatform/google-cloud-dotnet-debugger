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
#include "i_cor_debug_helper.h"
#include "i_eval_coordinator.h"

using google::cloud::diagnostics::debug::Variable;
using std::string;

namespace google_cloud_debugger {
void DbgClassField::Initialize(mdFieldDef field_def,
                               IMetaDataImport *metadata_import,
                               ICorDebugObjectValue *debug_obj_value,
                               ICorDebugClass *debug_class,
                               ICorDebugType *class_type, int creation_depth) {
  // If a field is a backing field of a property, its name will
  // end with this.
  static const string kBackingField = ">k__BackingField";

  if (metadata_import == nullptr) {
    WriteError("MetaDataImport is null.");
    initialized_hr_ = E_INVALIDARG;
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

  CComPtr<ICorDebugValue> field_value;
  ULONG len_field_name;

  field_def_ = field_def;
  creation_depth_ = creation_depth;
  class_type_ = class_type;

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

  if (IsStatic()) {
    initialized_hr_ = S_OK;
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

  initialized_hr_ = DbgObject::CreateDbgObject(field_value, creation_depth_,
                                               &member_value_, GetErrorStream());

  if (FAILED(initialized_hr_)) {
    WriteError("Failed to create DbgObject for field.");
    if (member_value_) {
      WriteError(member_value_->GetErrorString());
    }
  }
}

HRESULT DbgClassField::PopulateVariableValue(
    Variable *variable,
    ICorDebugReferenceValue *reference_value,
    IEvalCoordinator *eval_coordinator,
    std::vector<CComPtr<ICorDebugType>> *generic_types,
    int evaluation_depth) {
  if (FAILED(initialized_hr_)) {
    return initialized_hr_;
  }

  if (!variable || !eval_coordinator) {
    return E_INVALIDARG;
  }

  if (evaluation_depth == 0) {
    WriteError("Object inspection limit reached.");
    return E_FAIL;
  }

  HRESULT hr;

  // In case member_value_ is cached, sets the evaluation depth again.
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

  // In case member_value_ is cached, sets the evaluation depth again.
  int previous_eval_depth = member_value_->GetEvaluationDepth();
  member_value_->SetEvaluationDepth(evaluation_depth);
  hr = member_value_->PopulateVariableValue(variable, eval_coordinator);
  member_value_->SetEvaluationDepth(previous_eval_depth);
  if (FAILED(hr)) {
    WriteError(member_value_->GetErrorString());
  }

  return hr;
}

HRESULT DbgClassField::ExtractStaticFieldValue(IEvalCoordinator *eval_coordinator) {
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

  // TODO(quoct): String that starts with @ cannot be retrieved.
  // For static field, use default evaluation depth.
  hr = DbgObject::CreateDbgObject(debug_value, creation_depth_,
                                  &member_value_, GetErrorStream());
  if (FAILED(hr)) {
    if (member_value_) {
      WriteError(member_value_->GetErrorString());
    }
    WriteError("Failed to create DbgObject for static field value.");
    member_value_.release();
    return hr;
  }

  return hr;
}

}  // namespace google_cloud_debugger
