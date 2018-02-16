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

#ifndef DBG_REFERENCE_OBJECT_H_
#define DBG_REFERENCE_OBJECT_H_

#include "dbg_object.h"

namespace google_cloud_debugger {

// This class represents a .NET object of reference type.
class DbgReferenceObject : public DbgObject {
 public:
  DbgReferenceObject(ICorDebugType *debug_type, int depth)
      : DbgObject(debug_type, depth) {}
   
  // Searches the object for non-static field field_name and returns
  // the value in field_value.
  virtual HRESULT GetNonStaticField(const std::string &field_name,
                                    std::shared_ptr<DbgObject> *field_value);

 protected:
  // Handle for the object.
  // Only applicable for class, array and string.
  CComPtr<ICorDebugHandleValue> object_handle_;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_OBJECT_H_
