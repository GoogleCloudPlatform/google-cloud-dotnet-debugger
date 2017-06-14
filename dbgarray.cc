// Copyright 2015-2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#include "dbgarray.h"

#include <iostream>

namespace google_cloud_debugger {
using std::cout;
using std::cerr;

HRESULT DbgArray::Initialize(ICorDebugValue *debug_value, BOOL is_null) {
  HRESULT hr;

  SetIsNull(is_null);
  CComPtr<ICorDebugType> debug_type;
  ULONG32 array_rank;
  debug_type = GetDebugType();

  if (!is_null) {
    hr = CreateStrongHandle(debug_value, &array_handle_);
    if (FAILED(hr)) {
      cerr << "Failed to create a handle for the array.";
      return hr;
    }
  }

  hr = debug_type->GetRank(&array_rank);
  if (FAILED(hr)) {
    cerr << "Failed to get the array rank.";
    return hr;
  }

  dimensions_.resize(array_rank);

  CComPtr<ICorDebugArrayValue> debug_array_value;

  hr = debug_value->QueryInterface(
      __uuidof(ICorDebugArrayValue),
      reinterpret_cast<void **>(&debug_array_value));
  if (FAILED(hr)) {
    cerr << "Failed to get ICorDebugArrayValue.";
    return hr;
  }

  hr = debug_array_value->GetDimensions(array_rank, dimensions_.data());
  if (FAILED(hr)) {
    cerr << "Failed to get dimensions of the array.";
    return hr;
  }

  hr = debug_type->GetFirstTypeParameter(&array_type_);
  if (FAILED(hr)) {
    cerr << "Failed to get the array type.";
    return hr;
  }

  // Create an empty object for the type.
  hr = DbgObject::CreateDbgObject(array_type_, &empty_object_);
  if (FAILED(hr)) {
    cerr << "Failed to create an empty object for the array type.";
    return hr;
  }

  return S_OK;
}

// We have to keep dereferencing the array item while traversing the array
// instead of just dereferencing it once at the start because we can't store the
// the dereferenced value directly as it may be lost when pAppDomain->Continue
// is called.
HRESULT DbgArray::GetArrayItem(int position, ICorDebugValue **array_item) {
  HRESULT hr = E_FAIL;
  CComPtr<ICorDebugArrayValue> array_value;
  CComPtr<ICorDebugValue> dereferenced_value;

  if (!array_handle_) {
    cerr << "Cannot retrieve the array.";
    return E_FAIL;
  }

  hr = array_handle_->Dereference(&dereferenced_value);
  if (FAILED(hr)) {
    cerr << "Failed to dereference array handle.";
    return hr;
  }

  hr = dereferenced_value->QueryInterface(
      __uuidof(ICorDebugArrayValue), reinterpret_cast<void **>(&array_value));
  if (FAILED(hr)) {
    cerr << "Failed to get ICorDebugArrayValue.";
    return hr;
  }

  return array_value->GetElementAtPosition(position, array_item);
}

HRESULT DbgArray::PrintValue(EvalCoordinator *eval_coordinator) {
  HRESULT hr;

  if (GetIsNull()) {
    cout << "NULL";
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
         current_index < kMaxArrayItemsToRetrieve) {
    // Prints out the current combination.
    cout << "[";
    for (int index = 0; index < dimensions_tracker.size(); ++index) {
      cout << dimensions_tracker[index];
      if (index != dimensions_tracker.size() - 1) {
        cout << ", ";
      }
    }
    cout << "] ";

    // Prints out the value at this index.
    CComPtr<ICorDebugValue> array_item;
    hr = GetArrayItem(current_index, &array_item);
    if (SUCCEEDED(hr)) {
      unique_ptr<DbgObject> result_object;
      hr = DbgObject::CreateDbgObject(array_item, GetEvaluationDepth() - 1,
                                      &result_object);
      if (SUCCEEDED(hr)) {
        result_object->PrintType();
        cout << "  ";
        result_object->PrintValue(eval_coordinator);
      } else {
        cerr << "Could not create DbgObject from item at position "
             << current_index << " because of " << std::hex << hr;
        return hr;
      }
    } else {
      cerr << "Could not get item at position " << current_index
           << " because of " << std::hex << hr;
      return hr;
    }

    cout << std::endl;

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
  }

  return S_OK;
}

HRESULT DbgArray::PrintType() {
  if (!empty_object_) {
    cout << "Cannot determine base type of the array.";
    return E_FAIL;
  }

  empty_object_->PrintType();
  for (int i = 0; i < dimensions_.size(); ++i) {
    cout << "[]";
  }

  return S_OK;
}
}  // namespace google_cloud_debugger
