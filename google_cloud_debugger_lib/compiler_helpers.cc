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

#include "compiler_helpers.h"
#include "type_signature.h"
#include "dbg_primitive.h"

namespace google_cloud_debugger {

bool NumericCompilerHelper::IsImplicitNumericConversionable(
    const TypeSignature &source, const TypeSignature &target) {
  const CorElementType &source_type = source.cor_type;
  const CorElementType &target_type = target.cor_type;
  // Most of the logic here is from implicit numeric conversions:
  // https://docs.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/conversions#implicit-conversions
  switch (source_type) {
    case CorElementType::ELEMENT_TYPE_I1:
    {
      switch (target_type) {
      case CorElementType::ELEMENT_TYPE_I1:
      case CorElementType::ELEMENT_TYPE_I2:
      case CorElementType::ELEMENT_TYPE_I4:
      case CorElementType::ELEMENT_TYPE_I8:
      case CorElementType::ELEMENT_TYPE_R4:
      case CorElementType::ELEMENT_TYPE_R8:
        return true;
      default:
        return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_U1:
    {
      switch (target_type) {
      case CorElementType::ELEMENT_TYPE_U1:
      case CorElementType::ELEMENT_TYPE_I2:
      case CorElementType::ELEMENT_TYPE_U2:
      case CorElementType::ELEMENT_TYPE_I4:
      case CorElementType::ELEMENT_TYPE_U4:
      case CorElementType::ELEMENT_TYPE_I8:
      case CorElementType::ELEMENT_TYPE_U8:
      case CorElementType::ELEMENT_TYPE_R4:
      case CorElementType::ELEMENT_TYPE_R8:
        return true;
      default:
        return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_I2:
    {
      switch (target_type) {
      case CorElementType::ELEMENT_TYPE_I2:
      case CorElementType::ELEMENT_TYPE_I4:
      case CorElementType::ELEMENT_TYPE_I8:
      case CorElementType::ELEMENT_TYPE_R4:
      case CorElementType::ELEMENT_TYPE_R8:
        return true;
      default:
        return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_U2:
    {
      switch (target_type) {
      case CorElementType::ELEMENT_TYPE_U2:
      case CorElementType::ELEMENT_TYPE_I4:
      case CorElementType::ELEMENT_TYPE_U4:
      case CorElementType::ELEMENT_TYPE_I8:
      case CorElementType::ELEMENT_TYPE_U8:
      case CorElementType::ELEMENT_TYPE_R4:
      case CorElementType::ELEMENT_TYPE_R8:
        return true;
      default:
        return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_I4:
    {
      switch (target_type) {
      case CorElementType::ELEMENT_TYPE_I4:
      case CorElementType::ELEMENT_TYPE_I8:
      case CorElementType::ELEMENT_TYPE_R4:
      case CorElementType::ELEMENT_TYPE_R8:
        return true;
      default:
        return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_U4:
    {
      switch (target_type) {
      case CorElementType::ELEMENT_TYPE_U4:
      case CorElementType::ELEMENT_TYPE_I8:
      case CorElementType::ELEMENT_TYPE_U8:
      case CorElementType::ELEMENT_TYPE_R4:
      case CorElementType::ELEMENT_TYPE_R8:
        return true;
      default:
        return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_I8:
    {
      switch (target_type) {
      case CorElementType::ELEMENT_TYPE_I8:
      case CorElementType::ELEMENT_TYPE_R4:
      case CorElementType::ELEMENT_TYPE_R8:
        return true;
      default:
        return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_U8:
    {
      switch (target_type) {
      case CorElementType::ELEMENT_TYPE_U8:
      case CorElementType::ELEMENT_TYPE_R4:
      case CorElementType::ELEMENT_TYPE_R8:
        return true;
      default:
        return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_CHAR:
    {
      switch (target_type) {
      case CorElementType::ELEMENT_TYPE_CHAR:
      case CorElementType::ELEMENT_TYPE_U2:
      case CorElementType::ELEMENT_TYPE_I4:
      case CorElementType::ELEMENT_TYPE_U4:
      case CorElementType::ELEMENT_TYPE_I8:
      case CorElementType::ELEMENT_TYPE_U8:
      case CorElementType::ELEMENT_TYPE_R4:
      case CorElementType::ELEMENT_TYPE_R8:
        return true;
      default:
        return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_R4:
    {
      switch (target_type) {
      case CorElementType::ELEMENT_TYPE_R4:
      case CorElementType::ELEMENT_TYPE_R8:
        return true;
      default:
        return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_R8:
    {
      switch (target_type) {
      case CorElementType::ELEMENT_TYPE_R8:
        return true;
      default:
        return false;
      }
    }
    default:
      return false;
  }
}

bool NumericCompilerHelper::IsNumericallyPromotedToInt(const CorElementType &source) {
  return source == CorElementType::ELEMENT_TYPE_CHAR
      || source == CorElementType::ELEMENT_TYPE_I1
      || source == CorElementType::ELEMENT_TYPE_U1
      || source == CorElementType::ELEMENT_TYPE_I2;
}

template <typename T>
HRESULT NumericCompilerHelper::ExtractPrimitiveValue(DbgObject *dbg_object, T *cast_result) {
  if (!dbg_object) {
    return E_INVALIDARG;
  }

  HRESULT hr;
  CorElementType cor_type = dbg_object->GetCorElementType();
  switch (cor_type) {
    case CorElementType::ELEMENT_TYPE_BOOLEAN: {
      bool value;
      hr = DbgPrimitive<bool>::GetValue(dbg_object, &value);
      if (FAILED(hr)) {
        return hr;
      }
      *cast_result = static_cast<T>(value);
    }
    case CorElementType::ELEMENT_TYPE_CHAR: {
      char value;
      hr = DbgPrimitive<char>::GetValue(dbg_object, &value);
      if (FAILED(hr)) {
        return hr;
      }
      *cast_result = static_cast<T>(value);
    }
    case CorElementType::ELEMENT_TYPE_I: {
      intptr_t value;
      hr = DbgPrimitive<intptr_t>::GetValue(dbg_object, &value);
      if (FAILED(hr)) {
        return hr;
      }
      *cast_result = static_cast<T>(value);
    }
    case CorElementType::ELEMENT_TYPE_U: {
      uintptr_t value;
      hr = DbgPrimitive<uintptr_t>::GetValue(dbg_object, &value);
      if (FAILED(hr)) {
        return hr;
      }
      *cast_result = static_cast<T>(value);
    }
    case CorElementType::ELEMENT_TYPE_I1: {
      int8_t value;
      hr = DbgPrimitive<int8_t>::GetValue(dbg_object, &value);
      if (FAILED(hr)) {
        return hr;
      }
      *cast_result = static_cast<T>(value);
    }
    case CorElementType::ELEMENT_TYPE_U1: {
      uint8_t value;
      hr = DbgPrimitive<uint8_t>::GetValue(dbg_object, &value);
      if (FAILED(hr)) {
        return hr;
      }
      *cast_result = static_cast<T>(value);
    }
    case CorElementType::ELEMENT_TYPE_I2: {
      int16_t value;
      hr = DbgPrimitive<int16_t>::GetValue(dbg_object, &value);
      if (FAILED(hr)) {
        return hr;
      }
      *cast_result = static_cast<T>(value);
    }
    case CorElementType::ELEMENT_TYPE_U2: {
      uint16_t value;
      hr = DbgPrimitive<uint16_t>::GetValue(dbg_object, &value);
      if (FAILED(hr)) {
        return hr;
      }
      *cast_result = static_cast<T>(value);
    }
    case CorElementType::ELEMENT_TYPE_I4: {
      int32_t value;
      hr = DbgPrimitive<int32_t>::GetValue(dbg_object, &value);
      if (FAILED(hr)) {
        return hr;
      }
      *cast_result = static_cast<T>(value);
    }
    case CorElementType::ELEMENT_TYPE_U4: {
      uint32_t value;
      hr = DbgPrimitive<uint32_t>::GetValue(dbg_object, &value);
      if (FAILED(hr)) {
        return hr;
      }
      *cast_result = static_cast<T>(value);
    }
    case CorElementType::ELEMENT_TYPE_I8: {
      int64_t value;
      hr = DbgPrimitive<int64_t>::GetValue(dbg_object, &value);
      if (FAILED(hr)) {
        return hr;
      }
      *cast_result = static_cast<T>(value);
    }
    case CorElementType::ELEMENT_TYPE_U8: {
      uint64_t value;
      hr = DbgPrimitive<uint64_t>::GetValue(dbg_object, &value);
      if (FAILED(hr)) {
        return hr;
      }
      *cast_result = static_cast<T>(value);
    }
    case CorElementType::ELEMENT_TYPE_R4: {
      float_t value;
      hr = DbgPrimitive<float_t>::GetValue(dbg_object, &value);
      if (FAILED(hr)) {
        return hr;
      }
      *cast_result = static_cast<T>(value);
    }
    case CorElementType::ELEMENT_TYPE_R8: {
      double_t value;
      hr = DbgPrimitive<double_t>::GetValue(dbg_object, &value);
      if (FAILED(hr)) {
        return hr;
      }
      *cast_result = static_cast<T>(value);
    }
    default: { 
      return E_FAIL;
    }
  }
}

}  //  namespace google_cloud_debugger