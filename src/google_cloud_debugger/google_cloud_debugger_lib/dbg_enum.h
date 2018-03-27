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

#include <memory>
#include <vector>

#include "dbg_class.h"

namespace google_cloud_debugger {

// Class that represents a .NET class as well as .NET value type
// (CorElementType as ELEMENT_TYPE_CLASS and ELEMENT_TYPE_VALUETYPE).
class DbgEnum : public DbgClass {
 public:
  DbgEnum(ICorDebugType *debug_type, int depth,
          std::shared_ptr<ICorDebugHelper> debug_helper,
          std::shared_ptr<IDbgObjectFactory> obj_factory)
      : DbgClass(debug_type, depth, debug_helper, obj_factory) {}

  // Populate variable with the value of the enum.
  HRESULT PopulateValue(
      google::cloud::diagnostics::debug::Variable *variable) override;

  // Extracts out the enum value of this Enum class and processes
  // the fields of this enum.
  HRESULT ProcessEnum(ICorDebugValue *debug_value, ICorDebugClass *debug_class,
                      IMetaDataImport *metadata_import);

 private:
  // Given a void pointer and type of the enum, extract out the enum
  // value.
  ULONG64 ExtractEnumValue(CorElementType enum_type, void *enum_value);

  // Array of bytes to contain enum value if this class is an enum.
  std::vector<std::uint8_t> enum_value_array_;

  // String that represents "__value" which is the field that determines
  // the value of an enum.
  static const std::string kEnumValue;

  // Stores the enum value as a string.
  std::string enum_string_;
};

}  //  namespace google_cloud_debugger

#endif  //  DBG_ENUM_H_
