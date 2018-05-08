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

#ifndef I_DBG_CLASS_MEMBER_H_
#define I_DBG_CLASS_MEMBER_H_

#include <memory>
#include <sstream>
#include <vector>

#include "constants.h"
#include "string_stream_wrapper.h"
#include "i_cor_debug_helper.h"

namespace google_cloud_debugger {

class IEvalCoordinator;
class DbgObject;
class IDbgObjectFactory;

// This class represents a member (property or field) in a .NET class.
class IDbgClassMember : public StringStreamWrapper {
 public:
  IDbgClassMember(std::shared_ptr<ICorDebugHelper> debug_helper,
                  std::shared_ptr<IDbgObjectFactory> obj_factory) {
    debug_helper_ = debug_helper;
    obj_factory_ = obj_factory;
  }

  virtual ~IDbgClassMember() = default;

  // Evaluates the member and stores the value in member_value_.
  // reference_value is a reference to the class object that this property
  // belongs to. eval_coordinator is needed to perform the function
  // evaluation (the getter property function).
  // generic_types is an array of the generic types that the class has.
  // An example is if the class is Dictionary<string, int> then the generic
  // type array is (string, int).
  virtual HRESULT Evaluate(
      ICorDebugValue *reference_value,
      IEvalCoordinator *eval_coordinator,
      std::vector<CComPtr<ICorDebugType>> *generic_types) = 0;

  // Returns true if the member is static.
  virtual bool IsStatic() const = 0;

  // Gets the member name.
  const std::string &GetMemberName() const { return member_name_; }

  // Returns the signature of the member.
  PCCOR_SIGNATURE GetSignature() const { return signature_metadata_; }

  // Returns the default value of the member.
  UVCP_CONSTANT GetDefaultValue() const { return default_value_; }

  // Returns the HRESULT when Initialize function is called.
  HRESULT GetInitializeHr() const { return initialized_hr_; }

  // Gets the underlying DbgObject of this field's value.
  std::shared_ptr<DbgObject> GetMemberValue() { return member_value_; }

 protected:
  // Factory to create DbgObject.
  std::shared_ptr<IDbgObjectFactory> obj_factory_;

  // Helper methods for ICorDebug objects.
  std::shared_ptr<ICorDebugHelper> debug_helper_;

  // Token to the type that implements the member.
  mdTypeDef parent_token_ = 0;
  
  // Attribute flags applied to the member.
  DWORD member_attributes_ = 0;

  // Pointer to metadata signature of the member.
  PCCOR_SIGNATURE signature_metadata_ = 0;

  // The number of bytes returned in signature_metadata_.
  ULONG sig_metadata_length_ = 0;

  // A flag specifying the type of the constant that is the default value of the
  // member. This value is from the CorElementType enumeration.
  DWORD default_value_type_flags_ = 0;

  // A pointer to the bytes that store the default value of the member.
  UVCP_CONSTANT default_value_ = 0;

  // The size in wide characters of default_value_ if default_value_type_flags
  // is ELEMENT_TYPE_STRING. Otherwise, it is not relevant.
  ULONG default_value_len_ = 0;

  // Name of the member.
  std::string member_name_;

  // Value of the member.
  std::shared_ptr<DbgObject> member_value_;

  // HRESULT when the Initialize function is called.
  HRESULT initialized_hr_ = S_OK;

  // Depth used when creating a DbgObject representing this member.
  int creation_depth_ = kDefaultObjectEvalDepth;
};

}  //  namespace google_cloud_debugger

#endif  //  I_DBG_CLASS_MEMBER_H_
