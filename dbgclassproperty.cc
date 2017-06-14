// Copyright 2015-2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#include "dbgclassproperty.h"

#include <condition_variable>
#include <iostream>
#include <mutex>

#include "dbgobject.h"
#include "evalcoordinator.h"

namespace google_cloud_debugger {
using std::cout;
using std::cerr;
using std::vector;

HRESULT DbgClassProperty::Initialize(mdProperty property_def,
                                     IMetaDataImport *metadata_import) {
  ULONG property_name_length;
  ULONG other_methods_length;

  property_def_ = property_def;
  // First call to get length of array and length of other methods.
  HRESULT hr = metadata_import->GetPropertyProps(
      property_def_, &parent_token_, nullptr, 0, &property_name_length,
      &property_attributes_, &signature_metadata_, &sig_metadata_length_,
      &default_value_type_flags, &default_value_, &default_value_len_,
      &property_setter_function, &property_getter_function, nullptr, 0,
      &other_methods_length);

  if (FAILED(hr)) {
    cerr << "Failed to get property metadata.";
    return hr;
  }

  property_name_.resize(property_name_length);
  other_methods_.resize(other_methods_length);

  hr = metadata_import->GetPropertyProps(
      property_def_, &parent_token_, property_name_.data(),
      property_name_.size(), &property_name_length, &property_attributes_,
      &signature_metadata_, &sig_metadata_length_, &default_value_type_flags,
      &default_value_, &default_value_len_, &property_setter_function,
      &property_getter_function, other_methods_.data(), other_methods_.size(),
      &other_methods_length);

  if (FAILED(hr)) {
    cerr << "Failed to get property metadata.";
  }

  return hr;
}

HRESULT DbgClassProperty::Print(ICorDebugReferenceValue *reference_value,
                                EvalCoordinator *eval_coordinator,
                                vector<CComPtr<ICorDebugType>> *generic_types,
                                int depth) {
  if (!generic_types) {
    cerr << "Generic types array cannot be null.";
    return E_INVALIDARG;
  }

  if (!eval_coordinator) {
    cerr << "Eval Coordinator cannot be null.";
    return E_INVALIDARG;
  }

  if (!reference_value) {
    cerr << "Reference value cannot be null.";
    return E_INVALIDARG;
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
    cerr << "Failed to dereference value to evaluate property.";
    return hr;
  }

  hr = debug_value->QueryInterface(__uuidof(ICorDebugObjectValue),
                                   reinterpret_cast<void **>(&debug_obj_value));
  if (FAILED(hr)) {
    cerr << "Failed to dereference value to evaluate property.";
    return hr;
  }

  hr = debug_obj_value->GetClass(&debug_class);
  if (FAILED(hr)) {
    cerr << "Failed to get class from ICorDebugObjectValue.";
    return hr;
  }

  hr = debug_class->GetModule(&debug_module);
  if (FAILED(hr)) {
    cerr << "Failed to get module from ICorDebugClass.";
    return hr;
  }

  hr = debug_module->GetFunctionFromToken(property_getter_function,
                                          &debug_function);
  if (FAILED(hr)) {
    cerr << "Failed to get ICorDebugFunction from function token.";
    return hr;
  }

  hr = eval_coordinator->CreateEval(&debug_eval);
  if (FAILED(hr)) {
    cerr << "Failed to create ICorDebugEval.";
    return hr;
  }

  hr = debug_eval->QueryInterface(__uuidof(ICorDebugEval2),
                                  reinterpret_cast<void **>(&debug_eval_2));

  if (FAILED(hr)) {
    cerr << "Failed to cast ICorDebugEval to ICorDebugEval2.";
    return hr;
  }

  vector<ICorDebugValue *> arg_values{reference_value};
  vector<ICorDebugType *> local_generic_types;
  local_generic_types.reserve(generic_types->size());

  for (const auto &item : *generic_types) {
    local_generic_types.push_back(item);
  }

  hr = debug_eval_2->CallParameterizedFunction(
      debug_function, local_generic_types.size(), local_generic_types.data(),
      arg_values.size(), arg_values.data());

  CComPtr<ICorDebugValue> eval_result;

  hr = eval_coordinator->WaitForEval(&exception_occurred_, debug_eval,
                                     &eval_result);

  if (FAILED(hr)) {
    std::cout << "Failed to evaluate the property.";
    return E_FAIL;
  }

  hr = DbgObject::CreateDbgObject(eval_result, depth - 1, &property_value_);
  if (FAILED(hr)) {
    std::cerr << "Failed to create class property object.";
    return hr;
  }

  return PrintHelper(eval_coordinator);
}

HRESULT DbgClassProperty::PrintHelper(EvalCoordinator *eval_coordinator) {
  if (!property_name_.empty()) {
    PrintWcharString(property_name_);
    cout << "  ";
  }

  if (exception_occurred_) {
    // If there is an exception, the property_value_ will be the exception.
    cout << "throws exception ";
  }

  if (!property_value_) {
    cerr << "Property value not initialized.";
    return E_FAIL;
  }

  HRESULT hr;
  hr = property_value_->PrintType();
  if (FAILED(hr)) {
    cerr << "Failed to print type of property.";
    return hr;
  }

  cout << "  ";

  hr = property_value_->PrintValue(eval_coordinator);
  if (FAILED(hr)) {
    cerr << "Failed to print value of property.";
  }

  return hr;
}

}  // namespace google_cloud_debugger
