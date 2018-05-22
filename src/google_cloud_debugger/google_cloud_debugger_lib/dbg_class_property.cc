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

#include "dbg_class_property.h"

#include <condition_variable>
#include <iostream>
#include <mutex>

#include "breakpoint.pb.h"
#include "compiler_helpers.h"
#include "constants.h"
#include "i_cor_debug_helper.h"
#include "i_dbg_object_factory.h"
#include "i_eval_coordinator.h"

using google::cloud::diagnostics::debug::Variable;
using std::vector;

namespace google_cloud_debugger {

void DbgClassProperty::Initialize(mdProperty property_def,
                                  IMetaDataImport *metadata_import,
                                  ICorDebugModule *debug_module,
                                  int creation_depth) {
  if (metadata_import == nullptr) {
    WriteError("MetaDataImport is null.");
    initialized_hr_ = E_INVALIDARG;
    return;
  }

  ULONG property_name_length;
  ULONG other_methods_length;

  property_def_ = property_def;
  // First call to get length of array and length of other methods.
  initialized_hr_ = metadata_import->GetPropertyProps(
      property_def_, &parent_token_, nullptr, 0, &property_name_length,
      &member_attributes_, &signature_metadata_, &sig_metadata_length_,
      &default_value_type_flags_, &default_value_, &default_value_len_,
      &property_setter_function, &property_getter_function, nullptr, 0,
      &other_methods_length);

  if (FAILED(initialized_hr_)) {
    WriteError("Failed to get property metadata.");
    return;
  }

  std::vector<WCHAR> wchar_property_name(property_name_length, 0);
  other_methods_.resize(other_methods_length);

  initialized_hr_ = metadata_import->GetPropertyProps(
      property_def_, &parent_token_, wchar_property_name.data(),
      wchar_property_name.size(), &property_name_length, &member_attributes_,
      &signature_metadata_, &sig_metadata_length_, &default_value_type_flags_,
      &default_value_, &default_value_len_, &property_setter_function,
      &property_getter_function, other_methods_.data(), other_methods_.size(),
      &other_methods_length);

  if (FAILED(initialized_hr_)) {
    WriteError("Failed to get property metadata.");
  }

  member_name_ = ConvertWCharPtrToString(wchar_property_name);
  creation_depth_ = creation_depth;
  debug_module_ = debug_module;
}

HRESULT DbgClassProperty::Evaluate(
    ICorDebugValue *debug_value, IEvalCoordinator *eval_coordinator,
    vector<CComPtr<ICorDebugType>> *generic_types) {
  // If this property is already evaluated, don't do it again.
  if (member_value_) {
    return S_OK;
  }

  if (!generic_types) {
    WriteError("Generic types array cannot be null.");
    return E_INVALIDARG;
  }

  if (!eval_coordinator) {
    WriteError("Eval Coordinator cannot be null.");
    return E_INVALIDARG;
  }

  if (FAILED(initialized_hr_)) {
    return initialized_hr_;
  }

  CComPtr<ICorDebugFunction> debug_function;
  CComPtr<ICorDebugEval> debug_eval;
  CComPtr<ICorDebugEval2> debug_eval_2;
  HRESULT hr;

  if (!debug_module_) {
    WriteError("ICorDebugModule is needed to get the property getter.");
    return E_FAIL;
  }

  hr = debug_module_->GetFunctionFromToken(property_getter_function,
                                           &debug_function);
  if (FAILED(hr)) {
    WriteError("Failed to get ICorDebugFunction from function token.");
    return hr;
  }

  hr = eval_coordinator->CreateEval(&debug_eval);
  if (FAILED(hr)) {
    WriteError("Failed to create ICorDebugEval.");
    return hr;
  }

  vector<ICorDebugValue *> arg_values;
  // If the property is non-static, then when we call getter
  // method, we have to supply "this" (reference to the current object)
  // as a parameter.
  if (!IsStatic()) {
    if (!debug_value) {
      WriteError("Reference value cannot be null.");
      return E_INVALIDARG;
    }

    arg_values.push_back(debug_value);
  }

  vector<ICorDebugType *> local_generic_types;
  local_generic_types.assign(generic_types->begin(), generic_types->end());

  std::unique_ptr<DbgObject> member_value;
  hr = obj_factory_->EvaluateAndCreateDbgObject(
    std::move(local_generic_types), std::move(arg_values),
    debug_function, debug_eval, eval_coordinator,
    &member_value, GetErrorStream());

  if (FAILED(hr)) {
    WriteError("Failed to evaluate the property.");
    return hr;
  }

  member_value_ = std::move(member_value);
  return S_OK;
}

HRESULT DbgClassProperty::SetTypeSignature(
    IMetaDataImport *metadata_import,
    const std::vector<TypeSignature> &generic_class_types) {
  std::string type_name;
  // Use a copy of the pointer to the signature because the function
  // ParseTypeFromSig will modify it.
  PCCOR_SIGNATURE signature_pointer_copy = signature_metadata_;
  ULONG signature_length_copy = sig_metadata_length_;
  HRESULT hr = debug_helper_->ParsePropertySig(
      &signature_pointer_copy, &signature_length_copy, metadata_import,
      generic_class_types, &type_signature_);
  if (FAILED(hr)) {
    return hr;
  }

  type_signature_set_ = true;
  return S_OK;
}

HRESULT DbgClassProperty::GetTypeSignature(TypeSignature *type_signature) {
  if (!type_signature_set_) {
    return E_FAIL;
  }

  if (!type_signature) {
    return E_INVALIDARG;
  }

  *type_signature = type_signature_;
  return S_OK;
}

}  // namespace google_cloud_debugger
