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

#include "dbgclassproperty.h"

#include <condition_variable>
#include <iostream>
#include <mutex>

#include "dbgobject.h"
#include "i_evalcoordinator.h"

using google::cloud::diagnostics::debug::Variable;
using std::vector;

namespace google_cloud_debugger {

void DbgClassProperty::Initialize(mdProperty property_def,
                                  IMetaDataImport *metadata_import) {
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
      &property_attributes_, &signature_metadata_, &sig_metadata_length_,
      &default_value_type_flags, &default_value_, &default_value_len_,
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
      wchar_property_name.size(), &property_name_length, &property_attributes_,
      &signature_metadata_, &sig_metadata_length_, &default_value_type_flags,
      &default_value_, &default_value_len_, &property_setter_function,
      &property_getter_function, other_methods_.data(), other_methods_.size(),
      &other_methods_length);

  if (FAILED(initialized_hr_)) {
    WriteError("Failed to get property metadata.");
  }

  property_name_ = ConvertWCharPtrToString(wchar_property_name);
}

HRESULT DbgClassProperty::PopulateVariableValue(
    Variable *variable, ICorDebugReferenceValue *reference_value,
    IEvalCoordinator *eval_coordinator,
    vector<CComPtr<ICorDebugType>> *generic_types, int depth) {
  if (!generic_types) {
    WriteError("Generic types array cannot be null.");
    return E_INVALIDARG;
  }

  if (!eval_coordinator) {
    WriteError("Eval Coordinator cannot be null.");
    return E_INVALIDARG;
  }

  if (!reference_value) {
    WriteError("Reference value cannot be null.");
    return E_INVALIDARG;
  }

  if (!variable) {
    WriteError("Variable proto cannot be null.");
    return E_INVALIDARG;
  }

  if (FAILED(initialized_hr_)) {
    return initialized_hr_;
  }

  CComPtr<ICorDebugValue> debug_value;
  CComPtr<ICorDebugObjectValue> debug_obj_value;
  CComPtr<ICorDebugClass> debug_class;
  CComPtr<ICorDebugModule> debug_module;
  CComPtr<ICorDebugFunction> debug_function;
  CComPtr<ICorDebugEval> debug_eval;
  CComPtr<ICorDebugEval2> debug_eval_2;
  HRESULT hr;

  hr = reference_value->Dereference(&debug_value);
  if (FAILED(hr)) {
    WriteError("Failed to dereference value to evaluate property.");
    return hr;
  }

  hr = debug_value->QueryInterface(__uuidof(ICorDebugObjectValue),
                                   reinterpret_cast<void **>(&debug_obj_value));
  if (FAILED(hr)) {
    WriteError("Failed to dereference value to evaluate property.");
    return hr;
  }

  hr = debug_obj_value->GetClass(&debug_class);
  if (FAILED(hr)) {
    WriteError("Failed to get class from ICorDebugObjectValue.");
    return hr;
  }

  hr = debug_class->GetModule(&debug_module);
  if (FAILED(hr)) {
    WriteError("Failed to get module from ICorDebugClass.");
    return hr;
  }

  hr = debug_module->GetFunctionFromToken(property_getter_function,
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

  hr = debug_eval->QueryInterface(__uuidof(ICorDebugEval2),
                                  reinterpret_cast<void **>(&debug_eval_2));

  if (FAILED(hr)) {
    WriteError("Failed to cast ICorDebugEval to ICorDebugEval2.");
    return hr;
  }

  vector<ICorDebugValue *> arg_values;
  // Add "this" if method is not static.
  if ((*signature_metadata_ & 0x20) != 0) {
    arg_values.push_back(reference_value);
  }

  vector<ICorDebugType *> local_generic_types;
  local_generic_types.reserve(generic_types->size());

  for (const auto &item : *generic_types) {
    local_generic_types.push_back(item);
  }

  hr = debug_eval_2->CallParameterizedFunction(
      debug_function, local_generic_types.size(), local_generic_types.data(),
      arg_values.size(), arg_values.data());

  if (FAILED(hr)) {
    WriteError("Failed to call parameterized function.");
    return hr;
  }

  CComPtr<ICorDebugValue> eval_result;

  hr = eval_coordinator->WaitForEval(&exception_occurred_, debug_eval,
                                     &eval_result);

  if (FAILED(hr)) {
    WriteError("Failed to evaluate the property.");
    return hr;
  }

  hr = DbgObject::CreateDbgObject(eval_result, depth - 1, &property_value_,
                                  GetErrorStream());
  if (FAILED(hr)) {
    if (property_value_) {
      WriteError(property_value_->GetErrorString());
    }
    WriteError("Failed to create class property object.");
    return hr;
  }

  return PopulateVariableValueHelper(variable, eval_coordinator);
}

HRESULT DbgClassProperty::PopulateVariableValueHelper(
    Variable *variable, IEvalCoordinator *eval_coordinator) {
  if (!variable) {
    return E_INVALIDARG;
  }

  if (!property_value_) {
    WriteError("Property value not initialized.");
    return E_FAIL;
  }

  if (exception_occurred_) {
    // If there is an exception, the property_value_ will be the exception.
    std::ostringstream stream_type;
    property_value_->PopulateType(variable);
    WriteError("throws exception " + variable->type());
    return E_FAIL;
  }

  HRESULT hr =
      property_value_->PopulateVariableValue(variable, eval_coordinator);

  if (FAILED(hr)) {
    WriteError(property_value_->GetErrorString());
    return hr;
  }

  return hr;
}

}  // namespace google_cloud_debugger
