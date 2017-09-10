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

#include "breakpoint.pb.h"
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

// Helper function to convert a string to null-terminated WCHAR vector.
// If target_string is empty or if there are failures,
// this function returns an empty vector.
std::vector<WCHAR> ConvertStringToWCharPtr(const std::string &target_string);

// Helper function to convert a null-terminated WCHAR array to string.
// If wchar_string is null or if there are failures,
// this function returns an empty string.
std::string ConvertWCharPtrToString(const WCHAR *wchar_string);

// ConvertWCharPtrToString functions that takes in a vector
// instead of WCHAR array.
std::string ConvertWCharPtrToString(const std::vector<WCHAR> &wchar_vector);

// Sets the Status field of variable using error string err_string.
void SetErrorStatusMessage(google::cloud::diagnostics::debug::Variable *var,
                           const std::string &err_string);

class IEvalCoordinator;

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

  // Sets the type of proto variable to the type of this object.
  virtual HRESULT PopulateType(
      google::cloud::diagnostics::debug::Variable *variable) = 0;

  // Sets the value of proto variable to the value of this object.
  virtual HRESULT PopulateValue(
      google::cloud::diagnostics::debug::Variable *variable) {
    return S_OK;
  }

  // Sets the members of proto variable to the members of this object,
  // using eval_coordinator to perform any sort of evaluation if needed.
  virtual HRESULT PopulateMembers(
      google::cloud::diagnostics::debug::Variable *variable,
      IEvalCoordinator *eval_coordinator) {
    return S_OK;
  }

  // Returns true if this object has members.
  virtual BOOL HasMembers() { return false; }

  // Returns true if this object has value.
  virtual BOOL HasValue() { return true; }

  // Populate variable proto with type, member and value from this object.
  virtual HRESULT PopulateVariableValue(
      google::cloud::diagnostics::debug::Variable *variable,
      IEvalCoordinator *eval_coordinator);

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

  // Returns the HRESULT when Initialize function is called.
  HRESULT GetInitializeHr() const { return initialize_hr_; }

 private:
  // Helper function to create a DbgObject.
  static HRESULT CreateDbgObjectHelper(
      ICorDebugValue *debug_value, ICorDebugType *debug_type,
      CorElementType cor_element_type, BOOL is_null, int depth,
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
  // The HRESULT when Initialize is called.
  HRESULT initialize_hr_ = S_OK;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_OBJECT_H_
