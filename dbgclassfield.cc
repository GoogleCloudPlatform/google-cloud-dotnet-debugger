// Copyright 2015-2016 Google Inc. All Rights Reserved.
// Licensed under the Apache License Version 2.0.

#include "dbgclassfield.h"

#include <iostream>

#include "ccomptr.h"
#include "dbgobject.h"
#include "evalcoordinator.h"

namespace google_cloud_debugger {
using std::cerr;
using std::cout;

HRESULT DbgClassField::Initialize(mdFieldDef field_def,
                                  IMetaDataImport *metadata_import,
                                  ICorDebugObjectValue *debug_obj_value,
                                  ICorDebugClass *debug_class, int depth) {
  HRESULT hr;
  CComPtr<ICorDebugValue> field_value;
  ULONG len_field_name;

  field_def_ = field_def;

  // First call to get length of array.
  hr = metadata_import->GetFieldProps(
      field_def_, &class_token_, nullptr, 0, &len_field_name,
      &field_attributes_, &signature_metadata_, &signature_metadata_len_,
      &default_value_type_flags_, &default_value_, &default_value_len_);
  if (FAILED(hr)) {
    cerr << "Failed to populate field metadata.";
    return hr;
  }

  field_name_.resize(len_field_name);

  // Second call to get the actual name.
  hr = metadata_import->GetFieldProps(
      field_def_, &class_token_, field_name_.data(), len_field_name,
      &len_field_name, &field_attributes_, &signature_metadata_,
      &signature_metadata_len_, &default_value_type_flags_, &default_value_,
      &default_value_len_);
  if (FAILED(hr)) {
    cerr << "Failed to populate field metadata.";
    return hr;
  }

  hr = debug_obj_value->GetFieldValue(debug_class, field_def_, &field_value);
  // This means the field is optimized away and not available.
  if (hr == CORDBG_E_FIELD_NOT_AVAILABLE) {
    return S_OK;
  }

  // TODO(quoct): Add logic for static field.
  if (hr == CORDBG_E_FIELD_NOT_INSTANCE) {
    return S_OK;
  }

  if (FAILED(hr)) {
    cerr << "Failed to get field value.";
    return hr;
  }

  return DbgObject::CreateDbgObject(field_value, depth, &field_value_);
}

HRESULT DbgClassField::Print(EvalCoordinator *eval_coordinator) {
  if (!field_name_.empty()) {
    PrintWcharString(field_name_);
    cout << "  ";
  }

  if (!field_value_) {
    cerr << "Cannot get field value.";
    return E_FAIL;
  }

  HRESULT hr = field_value_->PrintType();
  if (FAILED(hr)) {
    cerr << "Failed to print field type.";
    return hr;
  }

  cout << "  ";
  hr = field_value_->PrintValue(eval_coordinator);
  if (FAILED(hr)) {
    cerr << "Failed to print field value.";
  }

  return hr;
}

}  // namespace google_cloud_debugger
