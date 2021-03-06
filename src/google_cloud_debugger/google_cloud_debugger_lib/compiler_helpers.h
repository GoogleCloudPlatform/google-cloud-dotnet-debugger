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

#include <iostream>
#include "common_headers.h"
#include "dbg_primitive.h"

// Various helper functions for compiling such as numeric conversions,
// numeric promotions, etc.
namespace google_cloud_debugger {

class DbgObject;
class ICorDebugHelper;

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
                                       const CorElementType &arg2,
                                       CorElementType *result,
                                       std::ostream *err_stream);

  // Given a DbgObject, try to convert it into a DbgPrimitive and extract
  // the underlying value of the DbgPrimitive object and cast it to
  // type T.
  template <typename T>
  static HRESULT ExtractPrimitiveValue(DbgObject *dbg_object, T *cast_result) {
    if (!dbg_object) {
      return E_INVALIDARG;
    }

    HRESULT hr;
    CorElementType cor_type = dbg_object->GetCorElementType();
    switch (cor_type) {
      case CorElementType::ELEMENT_TYPE_BOOLEAN: {
        bool value;
        hr = DbgPrimitive<bool>::GetValue(dbg_object, &value);
        if (SUCCEEDED(hr)) {
          *cast_result = static_cast<T>(value);
        }
        return hr;
      }
      case CorElementType::ELEMENT_TYPE_CHAR: {
        char value;
        hr = DbgPrimitive<char>::GetValue(dbg_object, &value);
        if (SUCCEEDED(hr)) {
          *cast_result = static_cast<T>(value);
        }
        return hr;
      }
      case CorElementType::ELEMENT_TYPE_I: {
        intptr_t value;
        hr = DbgPrimitive<intptr_t>::GetValue(dbg_object, &value);
        if (SUCCEEDED(hr)) {
          *cast_result = static_cast<T>(value);
        }
        return hr;
      }
      case CorElementType::ELEMENT_TYPE_U: {
        uintptr_t value;
        hr = DbgPrimitive<uintptr_t>::GetValue(dbg_object, &value);
        if (SUCCEEDED(hr)) {
          *cast_result = static_cast<T>(value);
        }
        return hr;
      }
      case CorElementType::ELEMENT_TYPE_I1: {
        int8_t value;
        hr = DbgPrimitive<int8_t>::GetValue(dbg_object, &value);
        if (SUCCEEDED(hr)) {
          *cast_result = static_cast<T>(value);
        }
        return hr;
      }
      case CorElementType::ELEMENT_TYPE_U1: {
        uint8_t value;
        hr = DbgPrimitive<uint8_t>::GetValue(dbg_object, &value);
        if (SUCCEEDED(hr)) {
          *cast_result = static_cast<T>(value);
        }
        return hr;
      }
      case CorElementType::ELEMENT_TYPE_I2: {
        int16_t value;
        hr = DbgPrimitive<int16_t>::GetValue(dbg_object, &value);
        if (SUCCEEDED(hr)) {
          *cast_result = static_cast<T>(value);
        }
        return hr;
      }
      case CorElementType::ELEMENT_TYPE_U2: {
        uint16_t value;
        hr = DbgPrimitive<uint16_t>::GetValue(dbg_object, &value);
        if (SUCCEEDED(hr)) {
          *cast_result = static_cast<T>(value);
        }
        return hr;
      }
      case CorElementType::ELEMENT_TYPE_I4: {
        int32_t value;
        hr = DbgPrimitive<int32_t>::GetValue(dbg_object, &value);
        if (SUCCEEDED(hr)) {
          *cast_result = static_cast<T>(value);
        }
        return hr;
      }
      case CorElementType::ELEMENT_TYPE_U4: {
        uint32_t value;
        hr = DbgPrimitive<uint32_t>::GetValue(dbg_object, &value);
        if (SUCCEEDED(hr)) {
          *cast_result = static_cast<T>(value);
        }
        return hr;
      }
      case CorElementType::ELEMENT_TYPE_I8: {
        int64_t value;
        hr = DbgPrimitive<int64_t>::GetValue(dbg_object, &value);
        if (SUCCEEDED(hr)) {
          *cast_result = static_cast<T>(value);
        }
        return hr;
      }
      case CorElementType::ELEMENT_TYPE_U8: {
        uint64_t value;
        hr = DbgPrimitive<uint64_t>::GetValue(dbg_object, &value);
        if (SUCCEEDED(hr)) {
          *cast_result = static_cast<T>(value);
        }
        return hr;
      }
      case CorElementType::ELEMENT_TYPE_R4: {
        float_t value;
        hr = DbgPrimitive<float_t>::GetValue(dbg_object, &value);
        if (SUCCEEDED(hr)) {
          *cast_result = static_cast<T>(value);
        }
        return hr;
      }
      case CorElementType::ELEMENT_TYPE_R8: {
        double_t value;
        hr = DbgPrimitive<double_t>::GetValue(dbg_object, &value);
        if (SUCCEEDED(hr)) {
          *cast_result = static_cast<T>(value);
        }
        return hr;
      }
      default: { return E_FAIL; }
    }
  }
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
  static CorElementType ConvertStringToCorElementType(
      const std::string &type_string);

  // Converts CorElementType cor_type to a string.
  // This function only works for numerical type, boolean and string.
  // Returns E_FAIL if conversion cannot be done.
  static HRESULT ConvertCorElementTypeToString(const CorElementType &cor_type,
                                               std::string *result);

  // Given a source class token and its metadata import,
  // this function will traverse its base class all the way
  // to System.Object to check whether any of them matches
  // target_class.
  // Returns S_OK if there is a match and E_FAIL otherwise.
  // TODO(quoct): Verify that whether this works on multiple
  // interfaces.
  static HRESULT IsBaseClass(
      mdTypeDef source_class, IMetaDataImport *source_class_metadata,
      const std::vector<CComPtr<ICorDebugAssembly>> &loaded_assemblies,
      const std::string &target_class, ICorDebugHelper *debug_helper,
      std::ostream *err_stream);
};

}  //  namespace google_cloud_debugger

#endif  //  COMPILER_HELPERS_H_
