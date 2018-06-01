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

#ifndef DBG_ENUM_H_
#define DBG_ENUM_H_

#include <map>
#include <memory>
#include <tuple>
#include <vector>

#include "dbg_class.h"

namespace google_cloud_debugger {

// Class that represents a .NET class as well as .NET value type
// (CorElementType as ELEMENT_TYPE_CLASS and ELEMENT_TYPE_VALUETYPE).
class DbgEnum : public DbgClass {
 public:
  // This constructor should be used we have an ICorDebugType
  // that represents the type of the enum.
  DbgEnum(ICorDebugType *debug_type, int depth, const std::string &enum_name,
          mdTypeDef enum_token, std::shared_ptr<ICorDebugHelper> debug_helper,
          std::shared_ptr<IDbgObjectFactory> obj_factory)
      : DbgClass(debug_type, depth, debug_helper, obj_factory) {
    class_name_ = enum_name;
    class_type_ = ClassType::ENUM;
    cor_element_type_ = CorElementType::ELEMENT_TYPE_VALUETYPE;
    class_token_ = enum_token;
  }

  // This constructor should be used when we do not have an ICorDebugType
  // that represents the type of the enum.
  // This will sets the underlying numerical value of the enum directly.
  // This constructor is used for creating enum from constant literal
  // as we don't have an ICorDebugType generated for those.
  DbgEnum(int depth, const std::string &enum_name, mdTypeDef enum_token,
          ULONG64 enum_value, const CorElementType &enum_type,
          std::shared_ptr<ICorDebugHelper> debug_helper,
          std::shared_ptr<IDbgObjectFactory> obj_factory)
      : DbgClass(nullptr, depth, debug_helper, obj_factory) {
    class_name_ = enum_name;
    class_type_ = ClassType::ENUM;
    cor_element_type_ = CorElementType::ELEMENT_TYPE_VALUETYPE;
    class_token_ = enum_token;
    enum_value_ = enum_value;
    enum_type_ = enum_type;
  }

  // Populate variable with the value of the enum.
  HRESULT PopulateValue(
      google::cloud::diagnostics::debug::Variable *variable) override;

  // Extracts out the enum value of this Enum class and processes
  // the fields of this enum.
  HRESULT ProcessEnum(ICorDebugValue *debug_value,
                      IMetaDataImport *metadata_import);

  // Processes the fields of this enum.
  HRESULT ProcessEnumFields(IMetaDataImport *metadata_import);

  // Sets the underlying integral value of the enum.
  void SetEnumValue(ULONG64 enum_value) { enum_value_ = enum_value; }

  // Sets the underlying enum type.
  void SetEnumType(const CorElementType &enum_type) { enum_type_ = enum_type; }

 private:
  // Assuming the fields of this enum has been processed,
  // add their values to the enum_values_dict.
  void AddEnumValuesToDict();

  // Key in the dictionary is the name of the enum.
  // Values are the list of possible values of that enum.
  // For example, enum Week { Monday, Tuesday } will have key
  // as Week and values as (0, "Monday") and (1, "Tuesday").
  static std::map<std::string,
                  std::vector<std::tuple<UVCP_CONSTANT, std::string>>>
      enum_values_dict;

  // Given a void pointer and type of the enum, extract out the enum
  // value.
  ULONG64 ExtractEnumValue(CorElementType enum_type, void *enum_value);

  // Array of bytes to contain enum value if this class is an enum.
  std::vector<std::uint8_t> enum_value_array_;

  // String that represents "__value" which is the field that determines
  // the value of an enum.
  static const std::string kEnumValue;

  // The integral type of the enum.
  CorElementType enum_type_ = CorElementType::ELEMENT_TYPE_END;

  // Stores the enum value as a string.
  std::string enum_string_;

  // The underlying integral value of the enum.
  ULONG64 enum_value_;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_ENUM_H_
