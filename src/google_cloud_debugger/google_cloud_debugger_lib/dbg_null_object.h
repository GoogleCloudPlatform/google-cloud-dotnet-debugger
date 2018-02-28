// Copyright 2018 Google Inc. All Rights Reserved.
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

#ifndef DBG_NULL_OBJECT_H_
#define DBG_NULL_OBJECT_H_

#include "dbg_object.h"
#include "class_names.h"

namespace google_cloud_debugger {
class EvalCoordinator;

// This class represents a null .NET object.
// We have to create a separate class for this because
// there is no ICorDebugValue for a generic null object (need to know
// the type to create it).
class DbgNullObject : public DbgObject {
 public:
  DbgNullObject(ICorDebugType *pType) : DbgObject(pType, 0) {}

  // Inherited via DbgObject.
  virtual void Initialize(ICorDebugValue * debug_value, BOOL is_null) {}

  // This should not be called because we cannot create a
  // ICorDebugValue that represents a null object.
  HRESULT GetICorDebugValue(ICorDebugValue **debug_value,
    ICorDebugEval *debug_eval) override {
    return E_NOTIMPL;
  }

  // Returns "System.Object".
  HRESULT GetTypeString(std::string *type_string) override {
    *type_string = kObjectClassName;
    return S_OK;
  }
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_NULL_OBJECT_H_
