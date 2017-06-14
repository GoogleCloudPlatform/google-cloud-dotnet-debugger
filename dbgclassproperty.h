// Copyright 2015-2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#ifndef DBG_CLASS_PROPERTY_H_
#define DBG_CLASS_PROPERTY_H_

#include <memory>
#include <vector>

#include "ccomptr.h"
#include "dbgobject.h"

namespace google_cloud_debugger {
class EvalCoordinator;

// This class represents a property in a .NET class.
// The property is not evaluated by default unless EvaluateProperty
// function is called.
// TODO(quoct): Investigate static property.
class DbgClassProperty {
 public:
  // Initialize the property name, metadata signature, attributes
  // as well the tokens for the getter and setter function of this property.
  HRESULT Initialize(mdProperty property_def, IMetaDataImport *metadata_import);

  // Evaluate the property and stores the value in property_value_.
  // reference_value is a reference to the class object that this property
  // belongs to. eval_coordinator is needed to perform the function
  // evaluation (the getter property function).
  // generic_types is an array of the generic types that the class has.
  // An example is if the class is Dictionary<string, int> then the generic
  // type array is (string, int).
  // Depth represents the level of inspection that we should perform on the
  // object we get back from the getter function.
  HRESULT Print(ICorDebugReferenceValue *reference_value,
                EvalCoordinator *eval_coordinator,
                std::vector<CComPtr<ICorDebugType>> *generic_types, int depth);

 private:
  // Helper function to print out the property value.
  HRESULT PrintHelper(EvalCoordinator *eval_coordinator);

  // Attribute flags applied to the property.
  DWORD property_attributes_ = 0;

  // Pointer to metadata signature of the property.
  PCCOR_SIGNATURE signature_metadata_ = 0;

  // The number of bytes returned in signature_metadata_
  ULONG sig_metadata_length_ = 0;

  // A flag specifying the type of the constant that is the default value of the
  // property. This value is from the CorElementType enumeration.
  DWORD default_value_type_flags = 0;

  // A pointer to the bytes that store the default value of the property.
  UVCP_CONSTANT default_value_ = 0;

  // The size in wide characters of default_value_ if default_value_type_flags
  // is ELEMENT_TYPE_STRING. Otherwise, it is not relevant.
  ULONG default_value_len_ = 0;

  // The token that represents the property getter.
  mdMethodDef property_getter_function = 0;

  // The token that represents the property setter.
  mdMethodDef property_setter_function = 0;

  // Token to the type that implements the property.
  mdTypeDef parent_token_ = 0;

  // Token that represents the property.
  mdProperty property_def_ = 0;

  // True if an exception is thrown when trying to evaluate the property.
  BOOL exception_occurred_ = FALSE;

  // Name of the property.
  std::vector<WCHAR> property_name_;

  // Vector of tokens that represent other methods associated with the property.
  std::vector<mdMethodDef> other_methods_;

  // Value of the property.
  std::unique_ptr<DbgObject> property_value_;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_CLASS_PROPERTY_H_
