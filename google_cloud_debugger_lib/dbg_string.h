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

#ifndef DBG_STRING_H_
#define DBG_STRING_H_

#include "dbg_object.h"

namespace google_cloud_debugger {
class EvalCoordinator;

// This class represents .NET System.String.
// A strong handle to the underlying string object is stored
// so we won't lose reference to it.
class DbgString : public DbgObject {
 public:
  DbgString(ICorDebugType *pType) : DbgObject(pType, 0) {}

  // Creates a strong handle to the object and stores it in string_handle_.
  void Initialize(ICorDebugValue *debug_value, BOOL is_null) override;

  // Dereferences string_handle_ to get the underlying object
  // and sets the value of variable to that object.
  HRESULT PopulateValue(
      google::cloud::diagnostics::debug::Variable *variable) override;

  // Sets type of variable to System.String.
  HRESULT PopulateType(
      google::cloud::diagnostics::debug::Variable *variable) override;

  // Extracts string from DbgObject.
  // Fails if DbgObject is not a DbgString.
  static HRESULT GetString(DbgObject *object, std::string *returned_string);

 private:
  // Dereferences the string handle and extracts out the string
  // into string_obj_. Will not do anything if string_obj_set_ is true.
  HRESULT ExtractStringFromReference();
   
  // Handle to the underlying string object.
  CComPtr<ICorDebugHandleValue> string_handle_;

  // The underlying string object.
  std::string string_obj_;

  // True if string_obj_ is set.
  bool string_obj_set_;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_STRING_H_
