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

#include "dbg_array.h"

#include <iostream>

#include "i_dbg_object_factory.h"
#include "i_cor_debug_helper.h"
#include "variable_wrapper.h"

using google::cloud::diagnostics::debug::Variable;
using std::string;
using std::unique_ptr;
using std::vector;

namespace google_cloud_debugger {

void DbgArray::Initialize(ICorDebugValue *debug_value, BOOL is_null) {
  SetIsNull(is_null);
  CComPtr<ICorDebugType> debug_type;
  ULONG32 array_rank;
  debug_type = GetDebugType();

  if (!debug_type) {
    WriteError("ICorDebugType for array is null.");
    initialize_hr_ = E_INVALIDARG;
    return;
  }

  initialize_hr_ = debug_type->GetFirstTypeParameter(&array_type_);
  if (FAILED(initialize_hr_)) {
    WriteError("Failed to get the array type.");
    return;
  }

  // Create an empty object for the type.
  initialize_hr_ =
      object_factory_->CreateDbgObject(array_type_, &empty_object_,
                                       GetErrorStream());
  if (FAILED(initialize_hr_)) {
    WriteError("Failed to create an empty object for the array type.");
    if (empty_object_) {
      WriteError(empty_object_->GetErrorString());
    }
    return;
  }

  initialize_hr_ = debug_type->GetRank(&array_rank);
  if (FAILED(initialize_hr_)) {
    WriteError("Failed to get the array rank.");
    return;
  }

  dimensions_.resize(array_rank);

  if (is_null) {
    return;
  }

  if (!debug_value) {
    WriteError("ICorDebugValue is null for non-null array.");
    initialize_hr_ = E_INVALIDARG;
    return;
  }

  initialize_hr_ = debug_helper_->CreateStrongHandle(
      debug_value, &object_handle_, GetErrorStream());
  if (FAILED(initialize_hr_)) {
    WriteError("Failed to create a handle for the array.");
    return;
  }

  CComPtr<ICorDebugArrayValue> debug_array_value;

  initialize_hr_ = debug_value->QueryInterface(
      __uuidof(ICorDebugArrayValue),
      reinterpret_cast<void **>(&debug_array_value));

  // This means the array is not initialized.
  if (initialize_hr_ == E_NOINTERFACE) {
    SetIsNull(TRUE);
    initialize_hr_ = S_OK;
  } else if (FAILED(initialize_hr_)) {
    WriteError("Failed to get ICorDebugArrayValue.");
    return;
  }

  if (debug_array_value) {
    initialize_hr_ =
        debug_array_value->GetDimensions(array_rank, dimensions_.data());
    if (FAILED(initialize_hr_)) {
      WriteError("Failed to get dimensions of the array.");
      return;
    }
  }
}

// We have to keep dereferencing the array item while traversing the array
// instead of just dereferencing it once at the start because we can't store the
// the dereferenced value directly as it may be lost when pAppDomain->Continue
// is called.
HRESULT DbgArray::GetArrayItem(int position, ICorDebugValue **array_item) {
  HRESULT hr = E_FAIL;
  CComPtr<ICorDebugArrayValue> array_value;
  CComPtr<ICorDebugValue> dereferenced_value;

  if (!object_handle_) {
    WriteError("Cannot retrieve the array.");
    return E_FAIL;
  }

  hr = object_handle_->Dereference(&dereferenced_value);
  if (FAILED(hr)) {
    WriteError("Failed to dereference array handle.");
    return hr;
  }

  hr = dereferenced_value->QueryInterface(
      __uuidof(ICorDebugArrayValue), reinterpret_cast<void **>(&array_value));
  if (FAILED(hr)) {
    WriteError("Failed to get ICorDebugArrayValue.");
    return hr;
  }

  return array_value->GetElementAtPosition(position, array_item);
}

HRESULT DbgArray::PopulateMembers(
    google::cloud::diagnostics::debug::Variable *variable_proto,
    std::vector<VariableWrapper> *members,
    IEvalCoordinator *eval_coordinator) {
  if (FAILED(initialize_hr_)) {
    return initialize_hr_;
  }

  if (!members || !variable_proto || !eval_coordinator) {
    return E_INVALIDARG;
  }

  if (GetIsNull()) {
    return S_OK;
  }

  int total_items = 1;
  for (int i = 0; i < dimensions_.size(); ++i) {
    total_items *= dimensions_[i];
  }

  // We use this dimensions_tracker to help us track which combination
  // of the array dimensions we are currently at (see comments just before the
  // while loop).
  vector<ULONG32> dimensions_tracker(dimensions_.size(), 0);

  int current_index = 0;
  // In this while loop, we visit all possible combinations of the dimensions_
  // array to print out all the items. For example, let's assume that the array
  // has dimensions 2x3x4, then the while loop will go in this direction:
  // 0 0 0 -> 0 0 1 -> 0 0 2 -> 0 0 3 ->
  // 0 1 0 -> 0 1 1 -> 0 1 2 -> 0 1 3 ->
  // 0 2 0 -> 0 2 1 -> 0 2 2 -> 0 2 3 ->
  // 1 0 0 -> 1 0 1 -> 1 0 2 -> 1 0 3 ->
  // 1 1 0 -> 1 1 1 -> 1 1 2 -> 1 1 3 ->
  // 1 2 0 -> 1 2 1 -> 1 2 2 -> 1 2 3 ->
  // 2 0 0 -> 2 0 1 -> 2 0 2 -> 2 0 3 ->
  // 2 1 0 -> 2 1 1 -> 2 1 2 -> 2 1 3 ->
  // 2 2 0 -> 2 2 1 -> 2 2 2 -> 2 2 3
  while (current_index < total_items &&
         current_index < max_items_to_retrieved_) {
    // Uses the current combination as the name.
    string name = "[";
    for (int index = 0; index < dimensions_tracker.size(); ++index) {
      name += std::to_string(dimensions_tracker[index]);
      if (index != dimensions_tracker.size() - 1) {
        name += ", ";
      }
    }
    name += "]";

    ++current_index;
    // Increase the combination by 1. For example: 0 0 0 becomes 0 0 1,
    // 0 1 0 becomes 0 1 1, 0 1 1 becomes 1 0 0.
    // First, we will find an index that we can increase.
    if (current_index < total_items) {
      int current_dimension_index = dimensions_.size() - 1;
      while (current_dimension_index >= 0) {
        // Spill over the addition until we can't.
        ++dimensions_tracker[current_dimension_index];
        if (dimensions_tracker[current_dimension_index] ==
            dimensions_[current_dimension_index]) {
          dimensions_tracker[current_dimension_index] = 0;
          current_dimension_index -= 1;
        } else {
          break;
        }
      }
    }

    // Adds a member at this index.
    Variable *member = variable_proto->add_members();
    member->set_name(name);

    CComPtr<ICorDebugValue> array_item;
    // Minus one here since we increase it above.
    HRESULT hr = GetArrayItem(current_index - 1, &array_item);

    if (FAILED(hr)) {
      // Output the error on why we failed to print out.
      SetErrorStatusMessage(member, this);
      continue;
    }

    unique_ptr<DbgObject> result_object;
    hr = object_factory_->CreateDbgObject(
        array_item, GetCreationDepth() - 1,
        &result_object, GetErrorStream());
    if (FAILED(hr)) {
      if (result_object) {
        WriteError(result_object->GetErrorString());
      }
      // Output the error on why we failed to print out.
      SetErrorStatusMessage(member, this);
      continue;
    }

    members->push_back(VariableWrapper(member, std::move(result_object)));
  }

  return S_OK;
}

HRESULT DbgArray::GetTypeString(std::string *type_string) {
  if (FAILED(initialize_hr_)) {
    return initialize_hr_;
  }

  if (!type_string) {
    return E_INVALIDARG;
  }

  if (!empty_object_) {
    WriteError("Cannot determine base type of the array.");
    return E_FAIL;
  }

  // Gets the base type first.
  HRESULT hr = empty_object_->GetTypeString(type_string);
  if (FAILED(hr)) {
    return hr;
  }

  for (int i = 0; i < dimensions_.size(); ++i) {
    *type_string += "[]";
  }

  return S_OK;
}

}  // namespace google_cloud_debugger
