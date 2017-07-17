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

#ifndef DBG_OBJECT_H_
#define DBG_OBJECT_H_

#include <string>
#include <vector>

#include "ccomptr.h"
#include "cor.h"
#include "cordebug.h"
#include "stringstreamwrapper.h"

namespace google_cloud_debugger {

// Given an ICorDebugValue, keep trying to dereference it until we cannot
// anymore. This function will set is_null to true if this is a null
// reference. If debug_value is not a reference, this function will simply set
// dereferenced_value to debug_value. dereferenced_value cannot be null.
HRESULT Dereference(ICorDebugValue *debug_value,
                    ICorDebugValue **dereferenced_value, BOOL *is_null,
                    std::ostringstream *err_stream);

// The depth at which we will stop dereferencing.
const int kReferenceDepth = 10;

// Given an ICorDebugValue, try to unbox it if possible.
// If debug_value is not a boxed value, this function will simply set
// unboxed_value to debug_value.
HRESULT Unbox(ICorDebugValue *debug_value, ICorDebugValue **unboxed_value,
              std::ostringstream *err_stream);

// Given an ICorDebugValue, dereference and unbox it to get the underlying
// object. Set is_null to true if the object is null.
HRESULT DereferenceAndUnbox(ICorDebugValue *debug_value,
                            ICorDebugValue **dereferenced_and_unboxed_value,
                            BOOL *is_null, std::ostringstream *err_stream);

// Given an ICorDebugValue, creates a strong handle to the underlying
// object. ICorDebugValue must represents an object type that can
// be stored on the heap.
HRESULT CreateStrongHandle(ICorDebugValue *debug_value,
                           ICorDebugHandleValue **handle,
                           std::ostringstream *err_stream);

// Helper function to print out WCHAR string.
// If wchar_string is null, this function won't print out anything.
// TODO(quoct): On Linux, WCHAR is defined as char16_t.
// Is there a better library that can handle printing out char16_t string?
// Tried using wcout but that does not work on char16_t. We can
// add a routine that converts from WCHAR to wchar_t.
std::string ConvertWCharPtrToString(const WCHAR *wchar_string);

// PrintWcharString functions that takes in a vector instead of WCHAR array.
std::string ConvertWCharPtrToString(const std::vector<WCHAR> &wchar_vector);

class EvalCoordinator;

// This class represents a .NET object.
// We try to store either a copy of the object itself (with value type)
// or a copy of the reference (with reference type). This is because
// if we issue a call to ICorDebugController->Continue, then the
// ICorDebugValue and many other ICorDebug* interfaces will be lost.
class DbgObject : public StringStreamWrapper {
 public:
  // Create a DbgObject with ICorDebugType debug_type.
  // The object will only be evaluated to a depth of depth.
  DbgObject(ICorDebugType *debug_type, int depth);
  virtual ~DbgObject() {}

  // Initialize the DbgObject based on an ICorDebugValue object
  // and a boolean that indicates whether the object is null or not.
  // This function will set initialize_hr_ if there is any error.
  // Any error during initialization should be written to the
  // error stream of this object using OutputError.
  // With this, the PrintJSON object can just check and print
  // out the object with the correct error status.
  virtual void Initialize(ICorDebugValue *debug_value, BOOL is_null) = 0;

  // Print out the type of the object to the member output_stream_.
  virtual HRESULT OutputType() = 0;

  // Same as OutputType but prints to a different output_stream.
  // The current output_stream will be preserved.
  HRESULT OutputType(std::ostringstream *output_stream);

  // Print out to the member output_stream_ the value of the object,
  virtual HRESULT OutputValue() { return S_OK; }

  // Same as OutputValue but prints to a different output_stream.
  // The current output_stream will be preserved.
  HRESULT OutputValue(std::ostringstream *output_stream);

  // Print out to the member output_stream_ the members of the object,
  // using eval_coordinator to perform any sort of evaluation if needed.
  virtual HRESULT OutputMembers(EvalCoordinator *eval_coordinator) {
    return S_OK;
  }

  // Same as OutputMembers but prints to a different output_stream.
  // The current output_stream will be preserved.
  HRESULT OutputMembers(std::ostringstream *output_stream,
                        EvalCoordinator *eval_coordinator);

  // Returns true if this object has members that can be printed out.
  virtual BOOL HasMembers() { return false; }

  // Returns true if this object has value that can be printed out.
  virtual BOOL HasValue() { return true; }

  // Prints out a JSON representation of the object to output_stream.
  // The JSON will have the keys name, type. It may have either value
  // or members keys depending on the object type. For example, an integer
  // has members and not value and its JSON will be:
  // { "name": "i", "type": "System.Int32", "value": "4" } where
  // i is the obj_name.
  // An array has members instead of value and its JSON will be:
  // {
  //  "name": "Test",
  //  "type": "[]",
  //  "members": [{
  //    "name": "[0]",
  //    "type": "System.Int32",
  //    "value": "3"
  //  }, {
  //    "name": "[1]",
  //    "type": "System.Int32",
  //    "value": "4"
  //  }]
  // }
  virtual HRESULT OutputJSON(const std::string &obj_name,
                            EvalCoordinator *eval_coordinator);

  // Create a DbgObject with an evaluation depth of depth.
  static HRESULT CreateDbgObject(ICorDebugValue *debug_value, int depth,
                                 std::unique_ptr<DbgObject> *result_object,
                                 std::ostringstream *err_stream);

  // Create an empty DbgObject. This object is mainly used
  // to store complex type and printing them out later.
  static HRESULT CreateDbgObject(ICorDebugType *debug_type,
                                 std::unique_ptr<DbgObject> *result_object,
                                 std::ostringstream *err_stream);

  // Returns the ICorDebugType of the object.
  ICorDebugType *GetDebugType() const { return debug_type_; }

  // Set this object to null.
  void SetIsNull(BOOL value) { is_null_ = value; }

  // Returns true if this object is null.
  BOOL GetIsNull() const { return is_null_; }

  // Returns the current evaluation depth of the object.
  int GetEvaluationDepth() const { return depth_; }

 private:
  // Helper function to create a DbgObject.
  static HRESULT CreateDbgObjectHelper(ICorDebugValue *debug_value,
                                       ICorDebugType *debug_type,
                                       CorElementType cor_element_type,
                                       BOOL is_null, int depth,
                                       std::unique_ptr<DbgObject> *result_object,
                                       std::ostringstream *err_stream);

  // The underlying type of the object.
  CComPtr<ICorDebugType> debug_type_;

  // True if the object is null.
  BOOL is_null_ = FALSE;

  // The depth of evaluation for this object.
  // Once this is 0, we don't evaluate the fields and properties of the object.
  int depth_;

 protected:
  HRESULT initialize_hr_ = S_OK;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_OBJECT_H_
