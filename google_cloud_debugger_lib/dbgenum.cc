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

#include "dbgenum.h"

#include "class_names.h"
#include "evalcoordinator.h"
#include "icordebughelper.h"

using google::cloud::diagnostics::debug::Variable;
using std::array;
using std::char_traits;
using std::min;
using std::string;
using std::vector;

namespace google_cloud_debugger {

const string DbgEnum::kEnumValue = "value__";

HRESULT DbgEnum::PopulateValue(Variable *variable) {
  if (!variable) {
    return E_INVALIDARG;
  }

  if (FAILED(initialize_hr_)) {
    return initialize_hr_;
  }

  if (!HasValue()) {
    return E_FAIL;
  }

  // Checks whether we already cache it in enum_string_.
  if (!enum_string_.empty()) {
    variable->set_value(enum_string_);
    return S_OK;
  }

  CorElementType enum_type;
  bool enum_type_found = false;
  // We have enum type.
  // First, gets the underlying type. This is from the non-static field __value.
  for (auto it = begin(class_fields_); it != end(class_fields_); ++it) {
    if (*it) {
      if ((*it)->IsStatic()) {
        continue;
      }

      if (kEnumValue.compare((*it)->GetFieldName()) != 0) {
        continue;
      }

      PCCOR_SIGNATURE field_signature = (*it)->GetSignature();
      enum_type = CorSigUncompressElementType(field_signature);
      enum_type_found = true;
      break;
    }
  }

  if (!enum_type_found) {
    WriteError("Cannot find the type of the enum.");
    return E_FAIL;
  }

  ULONG64 enum_value = ExtractEnumValue(enum_type, enum_value_array_.data());
  for (auto it = begin(class_fields_); it != end(class_fields_); ++it) {
    if (*it) {
      if ((*it)->IsStatic() || kEnumValue.compare((*it)->GetFieldName()) == 0) {
        continue;
      }

      UVCP_CONSTANT raw_default_value = (*it)->GetDefaultValue();
      ULONG64 const_value =
          ExtractEnumValue(enum_type, (void *)raw_default_value);

      // If enum_value is different from const_value, but const_value
      // corresponds to bits in enum_value, then this const_value string is part
      // of the enum_value string representation.
      if (enum_value != const_value &&
          (const_value == 0 || (const_value & enum_value) != const_value)) {
        continue;
      }

      // Zero out the bits of const_value in enum_value;
      enum_value = enum_value & (~const_value);

      if (enum_string_.empty()) {
        enum_string_.append((*it)->GetFieldName());
      } else {
        enum_string_.append(" | " + (*it)->GetFieldName());
      }
    }
  }

  variable->set_value(enum_string_);
  return S_OK;
}

HRESULT DbgEnum::ProcessClassType(ICorDebugValue *debug_value,
                                  ICorDebugClass *debug_class,
                                  IMetaDataImport *metadata_import) {
  class_type_ = ClassType::ENUM;
  CComPtr<ICorDebugGenericValue> generic_value;
  HRESULT hr =
      debug_value->QueryInterface(__uuidof(ICorDebugGenericValue),
                                  reinterpret_cast<void **>(&generic_value));
  if (FAILED(hr)) {
    WriteError("Failed to extract ICorDebugGenericValue from value type.");
    return hr;
  }

  ULONG32 value_size;
  hr = generic_value->GetSize(&value_size);
  if (FAILED(hr)) {
    WriteError("Failed to get size of ICorDebugGenericValue.");
    return hr;
  }

  enum_value_array_.resize(value_size);
  hr = generic_value->GetValue(
      reinterpret_cast<void *>(enum_value_array_.data()));
  if (FAILED(hr)) {
    WriteError("Failed to extract value out from ICorDebugGenericValue.");
    return hr;
  }

  // To get the different enum values, we have to process the field
  // of this class.
  CComPtr<ICorDebugObjectValue> obj_value;
  hr = debug_value->QueryInterface(__uuidof(ICorDebugObjectValue),
                                   reinterpret_cast<void **>(&obj_value));
  if (FAILED(hr)) {
    WriteError(
        "Failed to get ICorDebugObjectValue from ICorDebugValue while "
        "evaluating Enum.");
    return hr;
  }

  hr = ProcessFields(metadata_import, obj_value, debug_class);
  if (FAILED(hr)) {
    WriteError("Failed to populate class fields while evaluating Enum.");
    return hr;
  }

  return S_OK;
}

ULONG64 DbgEnum::ExtractEnumValue(CorElementType enum_type, void *enum_value) {
  switch (enum_type) {
    case ELEMENT_TYPE_I:
      return (ULONG64)(*((intptr_t *)enum_value));
    case ELEMENT_TYPE_U:
      return (ULONG64)(*((uintptr_t *)enum_value));
    case ELEMENT_TYPE_CHAR:
    case ELEMENT_TYPE_I1:
      return (ULONG64)(*((int8_t *)enum_value));
    case ELEMENT_TYPE_U1:
      return (ULONG64)(*((uint8_t *)enum_value));
    case ELEMENT_TYPE_I2:
      return (ULONG64)(*((int16_t *)enum_value));
    case ELEMENT_TYPE_U2:
      return (ULONG64)(*((uint16_t *)enum_value));
    case ELEMENT_TYPE_I4:
      return (ULONG64)(*((int32_t *)enum_value));
    case ELEMENT_TYPE_U4:
      return (ULONG64)(*((uint32_t *)enum_value));
    case ELEMENT_TYPE_I8:
      return (ULONG64)(*((int64_t *)enum_value));
    case ELEMENT_TYPE_U8:
      return (ULONG64)(*((uint64_t *)enum_value));
    default:
      return 0;
  }
}

}  // namespace google_cloud_debugger
