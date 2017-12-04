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

#ifndef DBG_CLASS_PROPERTY_H_
#define DBG_CLASS_PROPERTY_H_

#include <memory>
#include <vector>

#include "dbg_object.h"
#include "i_dbg_class_member.h"

namespace google_cloud_debugger {

// This class represents a property in a .NET class.
// The property is not evaluated by default unless EvaluateProperty
// function is called.
class DbgClassProperty : public IDbgClassMember {
 public:
  // Initialize the property name, metadata signature, attributes
  // as well the tokens for the getter and setter function of this property.
  // property_def is the metadata token for the property.
  // metadata_import is used to extract metadata from the property.
  // Creation_depth sets the depth used when creating a DbgObject
  // representing this property.
  void Initialize(mdProperty property_def, IMetaDataImport *metadata_import,
      int creation_depth);

  // Evaluate the property and stores the value in member_value_.
  // reference_value is a reference to the class object that this property
  // belongs to. eval_coordinator is needed to perform the function
  // evaluation (the getter property function).
  // generic_types is an array of the generic types that the class has.
  // An example is if the class is Dictionary<string, int> then the generic
  // type array is (string, int).
  // Depth represents the level of inspection that we should perform on the
  // object we get back from the getter function.
  HRESULT PopulateVariableValue(
      ICorDebugReferenceValue *reference_value,
      IEvalCoordinator *eval_coordinator,
      std::vector<CComPtr<ICorDebugType>> *generic_types, int evaluation_depth);

  // Returns true if the property is static.
  // If the property is static, the metadata won't have a bit mask
  // at 0x20.
  bool IsStatic() const override {
    return (*signature_metadata_ & kNonStaticPropertyMask) == 0;
  }

  static const int8_t kNonStaticPropertyMask = 0x20;

 private:
  // Helper function to set the value of variable to this property's value.
  // This function assumes that member_value_ is not null.
  HRESULT PopulateVariableValueHelper(
      IEvalCoordinator *eval_coordinator);

  // The token that represents the property getter.
  mdMethodDef property_getter_function = 0;

  // The token that represents the property setter.
  mdMethodDef property_setter_function = 0;

  // Token that represents the property.
  mdProperty property_def_ = 0;

  // True if an exception is thrown when trying to evaluate the property.
  BOOL exception_occurred_ = FALSE;

  // Vector of tokens that represent other methods associated with the property.
  std::vector<mdMethodDef> other_methods_;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_CLASS_PROPERTY_H_
