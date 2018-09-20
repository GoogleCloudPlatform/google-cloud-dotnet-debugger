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
#include "string_stream_wrapper.h"

namespace google_cloud_debugger {

class IEvalCoordinator;
class VariableWrapper;
class ICorDebugHelper;
struct TypeSignature;

// This class represents a .NET object.
// We try to store either a copy of the object itself (with value type)
// or a copy of the reference (with reference type). This is because
// if we issue a call to ICorDebugController->Continue, then the
// ICorDebugValue and many other ICorDebug* interfaces will be lost.
class DbgObject : public StringStreamWrapper {
 public:
  // Create a DbgObject with ICorDebugType debug_type.
  // The object will only be created to a depth of depth.
  DbgObject(ICorDebugType *debug_type, int depth,
            std::shared_ptr<ICorDebugHelper> debug_helper);

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
  HRESULT PopulateType(google::cloud::diagnostics::debug::Variable *variable);

  // Gets a string of object type.
  virtual HRESULT GetTypeString(std::string *type_string) = 0;

  // Sets the value of proto variable to the value of this object.
  // PopulateValue will return S_FALSE if this object has members.
  virtual HRESULT PopulateValue(
      google::cloud::diagnostics::debug::Variable *variable) {
    return S_OK;
  }

  // Extracts the type signature of this object.
  virtual HRESULT GetTypeSignature(TypeSignature *type_signature);

  // Populates the members vector using this object's members.
  // Returns S_FALSE by default (no members).
  // Variable_proto is used to create children variable protos.
  // These protos, combined with this object's members' values
  // will be used to populate members vector.
  // object_factory is needed to create new DbgObjects for members.
  virtual HRESULT PopulateMembers(
      google::cloud::diagnostics::debug::Variable *variable_proto,
      std::vector<VariableWrapper> *members,
      IEvalCoordinator *eval_coordinator) {
    return S_FALSE;
  }

  // Returns an ICorDebugValue representing the object.
  virtual HRESULT GetICorDebugValue(ICorDebugValue **debug_value,
                                    ICorDebugEval *debug_eval) = 0;

  // Returns the ICorDebugType of the object.
  ICorDebugType *GetDebugType() const { return debug_type_; }

  // Set this object to null.
  void SetIsNull(BOOL value) { is_null_ = value; }

  // Returns true if this object is null.
  BOOL GetIsNull() const { return is_null_; }

  // Returns the current creation depth of the object.
  // See comments on depth_ field for more information.
  int GetCreationDepth() const { return depth_; }

  // Returns the HRESULT when Initialize function is called.
  HRESULT GetInitializeHr() const { return initialize_hr_; }

  // Returns the CorElementType of this object.
  CorElementType GetCorElementType() const { return cor_element_type_; }

  // Sets the CorElementType of the object.
  void SetCorElementType(const CorElementType &element_type) {
    cor_element_type_ = element_type;
  }

  // Returns the address of the object.
  CORDB_ADDRESS GetAddress() const { return address_; }

  // Sets the address of the object.
  void SetAddress(const CORDB_ADDRESS &address) { address_ = address; }

 private:
  // The underlying type of the object.
  CComPtr<ICorDebugType> debug_type_;

  // True if the object is null.
  BOOL is_null_ = FALSE;

  // The address of the object.
  CORDB_ADDRESS address_ = 0;

  // The depth of creation for this object.
  // Once this is 0, we don't create the fields and properties of the object.
  // Note that even though we use BFS in dbg_stack_frame to control how deep
  // down we evaluate an object, we will still need this for object creation
  // for ValueType object and field. For example, if an object is a normal
  // class, object creation is stopped once we create a reference to the class.
  // But if the object is a ValueType, we will have to store the fields
  // and properties of the object (since we can't create a reference to
  // a ValueType). If the fields and properties are in turn ValueType,
  // we will have to keep creating them. So we need this depth_ to
  // prevent infinite recursion.
  int depth_;

 protected:
  // The CorElementType of the underlying .NET object.
  CorElementType cor_element_type_;

  // Contains helper methods used for ICorDebug objects.
  std::shared_ptr<ICorDebugHelper> debug_helper_;

  // The HRESULT when Initialize is called.
  HRESULT initialize_hr_ = S_OK;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_OBJECT_H_
