// Copyright 2018 Google Inc. All Rights Reserved.
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

#ifndef COMPILER_HELPERS_H_
#define COMPILER_HELPERS_H_

#include "cor.h"
#include <iostream>

// Various helper functions for compiling such as numeric conversions,
// numeric promotions, etc.
namespace google_cloud_debugger {

struct TypeSignature;

class NumericCompilerHelper {
 public:
  // Returns true if source can be implicitly converted to target.
  // If source and target are not numeric type, the function
  // return false.
  // If source and target are the same numeric type, the function
  // will also return true.
  // TODO(quoct): Add support for decimal type.
  static bool IsImplicitNumericConversionable(const TypeSignature &source,
                                              const TypeSignature &target);

  // Returns true if source can be numerically promoted to int.
  static bool IsNumericallyPromotedToInt(const CorElementType &source);

  // Apply numeric promotions for +,-,*,/,%, &, |, ^, ==, !=, >, <, >= and <=
  // based on:
  // https://docs.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/expressions#binary-numeric-promotions
  // Returns false if an error occurs.
  static bool BinaryNumericalPromotion(const CorElementType &arg1,
    const CorElementType &arg2, CorElementType *result, std::ostream *err_stream);

  // Given a DbgObject, try to convert it into a DbgPrimitive and extract
  // the underlying value of the DbgPrimitive object and cast it to
  // type T.
  template <typename T>
  static HRESULT ExtractPrimitiveValue(DbgObject *dbg_object, T *cast_result);
};

// Contains functions to determine the type of a CorElementType.
class TypeCompilerHelper {
 public:
  // Returns true if CorElementType is a numerical type.
  // TODO(quoct): This does not handle Decimal.
  static bool IsNumericalType(const CorElementType &cor_type);

  // Returns true if CorElementType is an integral type.
  static bool IsIntegralType(const CorElementType &cor_type);

  // Returns true if CorElementType is an array type.
  static bool IsArrayType(const CorElementType &array_type);

  // Returns true if CorElementType is either an object,
  // valuetype or class.
  static bool IsObjectType(const CorElementType &cor_type);

  // Converts string type_string to CorElementType.
  // Returns Object by default.
  static CorElementType ConvertStringToCorElementType(const std::string &type_string);

  // Converts CorElementType cor_type to a string.
  // This function only works for numerical type, boolean and string.
  // Returns an empty string otherwise.
  static std::string ConvertCorElementTypeToString(const CorElementType &cor_type);
};

}  //  namespace google_cloud_debugger

#endif  //  COMPILER_HELPERS_H_
