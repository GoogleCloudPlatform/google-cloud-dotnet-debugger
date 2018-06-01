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

#include "dbg_enum.h"

#include "class_names.h"
#include "i_eval_coordinator.h"

using google::cloud::diagnostics::debug::Variable;
using std::array;
using std::char_traits;
using std::min;
using std::string;
using std::vector;

namespace google_cloud_debugger {

const string DbgEnum::kEnumValue = "value__";
std::map<std::string, std::vector<std::tuple<UVCP_CONSTANT, std::string>>>
    DbgEnum::enum_values_dict;

HRESULT DbgEnum::PopulateValue(Variable *variable) {
  if (!variable) {
    return E_INVALIDARG;
  }

  if (FAILED(initialize_hr_)) {
    return initialize_hr_;
  }

  // Checks whether we already cache it in enum_string_.
  if (!enum_string_.empty()) {
    variable->set_value(enum_string_);
    return S_OK;
  }

  if (enum_type_ == CorElementType::ELEMENT_TYPE_END) {
    WriteError("Cannot find the type of the enum.");
    return E_FAIL;
  }

  if (enum_values_dict.find(class_name_) == enum_values_dict.end()) {
    WriteError("Enum values are not populated.");
    return E_FAIL;
  }

  // This mutable enum value may be zeroed out during the loop.
  ULONG64 mutable_enum_value = enum_value_;
  for (auto &&enum_value_tuple : enum_values_dict[class_name_]) {
    UVCP_CONSTANT raw_default_value = std::get<0>(enum_value_tuple);
    ULONG64 const_value =
        ExtractEnumValue(enum_type_, (void *)raw_default_value);

    // If the original_enum_value matches the const_value exactly,
    // uses that instead of the "|" string.
    if (enum_value_ == const_value) {
      enum_string_ = std::get<1>(enum_value_tuple);
      break;
    }

    // If mutable_enum_value is different from const_value, but const_value
    // corresponds to bits in mutable_enum_value, then this const_value string
    // is part of the mutable_enum_value string representation.
    if (mutable_enum_value != const_value &&
        (const_value == 0 ||
         (const_value & mutable_enum_value) != const_value)) {
      continue;
    }

    // Zero out the bits of const_value in enum_value;
    mutable_enum_value = mutable_enum_value & (~const_value);

    if (enum_string_.empty()) {
      enum_string_.append(std::get<1>(enum_value_tuple));
    } else {
      enum_string_.append(" | " + std::get<1>(enum_value_tuple));
    }
  }

  variable->set_value(enum_string_);
  return S_OK;
}

HRESULT DbgEnum::ProcessEnum(ICorDebugValue *debug_value,
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

  hr = ProcessEnumFields(metadata_import);
  if (FAILED(hr)) {
    WriteError("Failed to populate class fields while evaluating Enum.");
    return hr;
  }

  // Sets the underlying enum type.
  // This is from the non-static field __value.
  for (auto &class_field : class_fields_) {
    if (!class_field || class_field->IsStatic()) {
      continue;
    }

    if (kEnumValue.compare(class_field->GetMemberName()) != 0) {
      continue;
    }

    PCCOR_SIGNATURE field_signature = class_field->GetSignature();
    enum_type_ = CorSigUncompressElementType(field_signature);
    break;
  }

  if (enum_type_ == CorElementType::ELEMENT_TYPE_END) {
    initialize_hr_ = E_FAIL;
  }

  enum_value_ = ExtractEnumValue(enum_type_, enum_value_array_.data());

  return S_OK;
}

HRESULT DbgEnum::ProcessEnumFields(IMetaDataImport *metadata_import) {
  HRESULT hr = ProcessFields(metadata_import, nullptr, nullptr);
  if (FAILED(hr)) {
    WriteError("Failed to process enum fields.");
    return hr;
  }

  AddEnumValuesToDict();
  return S_OK;
}

void DbgEnum::AddEnumValuesToDict() {
  if (enum_values_dict.find(class_name_) == enum_values_dict.end()) {
    std::vector<std::tuple<UVCP_CONSTANT, std::string>> enum_values;
    for (auto &class_field : class_fields_) {
      if (!class_field || !class_field->IsStatic()) {
        continue;
      }

      UVCP_CONSTANT raw_default_value = class_field->GetDefaultValue();
      std::string value_name = class_field->GetMemberName();
      enum_values.push_back(std::make_tuple<UVCP_CONSTANT, std::string>(
          std::move(raw_default_value), std::move(value_name)));
    }
    enum_values_dict[class_name_] = std::move(enum_values);
  }
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
