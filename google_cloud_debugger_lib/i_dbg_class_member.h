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

#include "string_stream_wrapper.h"

namespace google_cloud_debugger {

class IEvalCoordinator;
class DbgObject;

// This class represents a member (property or field) in a .NET class.
class IDbgClassMember : public StringStreamWrapper {
 public:
  virtual ~IDbgClassMember() = default;

  // Initialize the member names, metadata signature, flags and values.
  // HRESULT will be stored and returned through GetInitializedHr.
  virtual void Initialize(mdToken fieldDef,
                          IMetaDataImport *metadata_import) = 0;

  // Evaluates the member and populates proto variable's fields.
  // reference_value is a reference to the class object that this member
  // belongs to.
  // eval_coordinator is needed to perform the function evaluation if needed.
  // generic_types is an array of the generic types that the class has.
  // An example is if the class is Dictionary<string, int> then the generic
  // type array is (string, int).
  // Depth represents the level of inspection that we should perform on the
  // member's object.
  virtual HRESULT PopulateVariableValue(
      google::cloud::diagnostics::debug::Variable *variable,
      ICorDebugReferenceValue *reference_value,
      IEvalCoordinator *eval_coordinator,
      std::vector<CComPtr<ICorDebugType>> *generic_types, int depth) = 0;

  virtual HRESULT GetInitializeHr() const = 0;

  // Gets the member name.
  virtual const std::string &GetMemberName() const = 0;

  // Gets the underlying value of the member.
  virtual DbgObject *GetMemberValue() = 0;

  // Returns true if the member is static.
  virtual bool IsStatic() const = 0;

  // Returns the signature of the member.
  virtual PCCOR_SIGNATURE GetSignature() const = 0;

  // Returns the default value of the member.
  virtual UVCP_CONSTANT GetDefaultValue() const = 0;
};

}  //  namespace google_cloud_debugger

#endif  //  I_DBG_CLASS_MEMBER_H_
