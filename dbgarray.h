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

#ifndef DBG_ARRAY_H_
#define DBG_ARRAY_H_

#include <memory>
#include <vector>

#include "dbgobject.h"

namespace google_cloud_debugger {

class EvalCoordinator;

// This class represents a .NET array object.
// This includes multi-dimensional as well as jagged arrays.
// TODO(quoct): Currently, we have a depth that we use to determine
// how deep we will inspect a jagged array.
// However, I'm not sure how we can use this depth in the case
// of a multi-dimensional array?
class DbgArray : public DbgObject {
 public:
  DbgArray(ICorDebugType *debug_type, int depth)
      : DbgObject(debug_type, depth) {}

  // Retrieves information about array rank, array dimensions, array type and
  // creates a strong handle to the array.
  void Initialize(ICorDebugValue *debug_value, BOOL is_null) override;

  // Gets the object at a given position in the array.
  // Note that we treat multi-dimensional array as zero-based,
  // single-dimensional array. The layout of this array follows C++ style of
  // array layout.
  // In the case of an array of array, this simply gets the
  // inner array at position i.
  //
  // For example, if the array is an array of array like
  // double[][] jagged = new double[10][10],
  // then GetArrayItem(1, array_item) will return the inner item
  // jagged[1].
  // If the array is a jagged array like
  // double[,] multi = new double[10, 10],
  // then GetArrayItem(1, array_item) will return multi[0, 1]
  // and GetArrayItem(10, array_item) will return multi[1, 0].
  HRESULT GetArrayItem(int position, ICorDebugValue **array_item);

  // Sets the members of variable to the items in the array.
  HRESULT PopulateMembers(google::cloud::diagnostics::debug::Variable *variable,
                          EvalCoordinator *eval_coordinator) override;

  // Sets the type of variable to this array type.
  HRESULT PopulateType(
      google::cloud::diagnostics::debug::Variable *variable) override;

  BOOL HasMembers() override { return true; }

  BOOL HasValue() override { return false; }

  // TODO(quoct): Add an argument on the debugger to change this value.
  static const int kMaxArrayItemsToRetrieve = 1000;

 private:
  // A strong handle to the underlying array object.
  CComPtr<ICorDebugHandleValue> array_handle_;

  // The type of the array.
  CComPtr<ICorDebugType> array_type_;

  // This empty_object_ is an object of the type array_type_.
  // We use this object to help us determine the array type.
  std::unique_ptr<DbgObject> empty_object_;

  // An array that stores the dimensions of the array.
  // Each value in this array specifies the number of elements in
  // a dimension in this array.
  std::vector<ULONG32> dimensions_;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_ARRAY_H_
