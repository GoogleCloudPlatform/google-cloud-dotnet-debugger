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

#ifndef DBG_CLASS_FIELD_H_
#define DBG_CLASS_FIELD_H_

#include <memory>
#include <vector>

#include "dbg_object.h"
#include "i_dbg_class_member.h"
#include "string_stream_wrapper.h"

namespace google_cloud_debugger {

// Class that represents a field in a .NET class.
class DbgClassField : public IDbgClassMember {
 public:
  // Initialize the field names, metadata signature, flags and values.
  // HRESULT will be stored in initialized_hr_.
  void Initialize(mdToken fieldDef, IMetaDataImport *metadata_import) override;

  // Sets the value of variable to the value of this field to an
  // evaluation depth of depth. reference_value is a reference to
  // the class that contains this field.
  HRESULT PopulateVariableValue(
      google::cloud::diagnostics::debug::Variable *variable,
      ICorDebugReferenceValue *reference_value,
      IEvalCoordinator *eval_coordinator,
      std::vector<CComPtr<ICorDebugType>> *generic_types, int depth) override;

  // Returns the field name.
  const std::string &GetMemberName() const override { return field_name_; }

  // Returns true if this is a backing field for a property.
  const bool IsBackingField() const { return is_backing_field_; }

  // Returns the HRESULT when Initialize function is called.
  HRESULT GetInitializeHr() const override { return initialized_hr_; }

  // Returns true if this is a static field.
  bool IsStatic() const override { return IsFdStatic(field_attributes_); }

  // Returns the signature of the field.
  PCCOR_SIGNATURE GetSignature() const override { return signature_metadata_; }

  // Returns the default value of the field (useful if the field is an enum).
  UVCP_CONSTANT GetDefaultValue() const override { return default_value_; }

  // Gets the underlying DbgObject of this field's value.
  DbgObject *GetMemberValue() override { return field_value_.get(); }

 private:
  // Extracts out static field value (with name field_name_) using the
  // ICorDebugValue class_value that represents the class object (may be null
  // since this is a static field). Depth of the static field object will
  // be kDefaultObjectEvalDepth.
  HRESULT ExtractStaticFieldValue(ICorDebugValue *class_value,
                                  IEvalCoordinator *eval_coordinator);

  // Extracts out non-static field value (with name field_name_) using
  // ICorDebugValue class_value that represents the class object (must not be
  // null since this is a non-static field). Depth of the non-static field
  // object will be depth.
  HRESULT ExtractNonStaticFieldValue(ICorDebugValue *class_value, int depth);

  // Token for the class that the field belongs to.
  mdTypeDef class_token_ = 0;

  // Token that represents this field.
  mdFieldDef field_def_ = 0;

  // Flags associated with the field's metadata.
  DWORD field_attributes_ = 0;

  // Pointer to signature meatdata value of the field.
  PCCOR_SIGNATURE signature_metadata_ = 0;

  // Size of signature_metadata_
  ULONG signature_metadata_len_ = 0;

  // A flag specifying the type of the constant that is the default value of the
  // property. This value is from the CorElementType enumeration.
  DWORD default_value_type_flags_ = 0;

  // The size in wide characters of default_value_ if default_value_type_flags
  // is ELEMENT_TYPE_STRING. Otherwise, it is not relevant.
  UVCP_CONSTANT default_value_ = 0;

  // The size in wide characters of default_value_ if default_value_type_flags
  // is ELEMENT_TYPE_STRING. Otherwise, it is not relevant.
  ULONG default_value_len_ = 0;

  // Name of the field.
  std::string field_name_;

  // True if this is a backing field for a property.
  bool is_backing_field_ = false;

  // Value of the field.
  std::unique_ptr<DbgObject> field_value_;

  // The HRESULT of initialization.
  HRESULT initialized_hr_ = S_OK;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_CLASS_FIELD_H_
