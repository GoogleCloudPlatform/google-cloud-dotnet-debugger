// Copyright 2015-2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#ifndef DBG_CLASS_FIELD_H_
#define DBG_CLASS_FIELD_H_

#include <memory>
#include <vector>

#include "evalcoordinator.h"

namespace google_cloud_debugger {
class DbgObject;

// Class that represents a field in a .NET class.
class DbgClassField {
 public:
  // Initialize the field names, metadata signature, flags and values.
  HRESULT Initialize(mdFieldDef fieldDef, IMetaDataImport *metadata_import,
                     ICorDebugObjectValue *debug_obj_value,
                     ICorDebugClass *debug_class, int depth);

  // Prints the value of the field.
  HRESULT Print(EvalCoordinator *eval_coordinator);

 private:
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

  // Name of the field. We don't use wstring because GetFieldProps expect
  // a WCHAR array.
  std::vector<WCHAR> field_name_;

  // Value of the field.
  std::unique_ptr<DbgObject> field_value_;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_CLASS_FIELD_H_
