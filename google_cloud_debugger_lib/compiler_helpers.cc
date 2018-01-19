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

bool NumericCompilerHelper::BinaryNumericalPromotion(
  const CorElementType &arg1,
  const CorElementType &arg2,
  CorElementType *result) {
  if (!IsNumericalType(arg1) && !IsNumericalType(arg2)) {
    return false;
  }

  // TODO(quoct): Add support for Decimal.
  if (arg1 == CorElementType::ELEMENT_TYPE_R8
    || arg2 == CorElementType::ELEMENT_TYPE_R8) {
    *result = CorElementType::ELEMENT_TYPE_R8;
    return true;
  }
  else if (arg1 == CorElementType::ELEMENT_TYPE_R4
    || arg2 == CorElementType::ELEMENT_TYPE_R4) {
    *result = CorElementType::ELEMENT_TYPE_R4;
    return true;
  }
  else if (arg1 == CorElementType::ELEMENT_TYPE_U8
    || arg2 == CorElementType::ELEMENT_TYPE_U8) {
    // If the other operand is of type sbyte, short, int or long,
    // an error will occur. This is because no integral type exists that can
    // represent the full range of ulong as well as the signed integral types.
    if (arg1 == CorElementType::ELEMENT_TYPE_I1
      || arg1 == CorElementType::ELEMENT_TYPE_I2
      || arg1 == CorElementType::ELEMENT_TYPE_I4
      || arg1 == CorElementType::ELEMENT_TYPE_I8
      || arg2 == CorElementType::ELEMENT_TYPE_I1
      || arg2 == CorElementType::ELEMENT_TYPE_I2
      || arg2 == CorElementType::ELEMENT_TYPE_I4
      || arg2 == CorElementType::ELEMENT_TYPE_I8) {
      return false;
    }
    *result = CorElementType::ELEMENT_TYPE_U8;
    return true;
  }
  else if (arg1 == CorElementType::ELEMENT_TYPE_I8
    || arg2 == CorElementType::ELEMENT_TYPE_I8) {
    *result = CorElementType::ELEMENT_TYPE_I8;
    return true;
  }
  else if (arg1 == CorElementType::ELEMENT_TYPE_U4
    || arg2 == CorElementType::ELEMENT_TYPE_U4) {
    // If 1 of the operand is sbyte, short or int,
    // the result will be long.
    if (arg1 == CorElementType::ELEMENT_TYPE_I1
      || arg1 == CorElementType::ELEMENT_TYPE_I2
      || arg1 == CorElementType::ELEMENT_TYPE_I4) {
      *result = CorElementType::ELEMENT_TYPE_I8;
      return true;
    }

    if (arg2 == CorElementType::ELEMENT_TYPE_I1
      || arg2 == CorElementType::ELEMENT_TYPE_I2
      || arg2 == CorElementType::ELEMENT_TYPE_I4) {
      *result = CorElementType::ELEMENT_TYPE_I8;
      return true;
    }

    *result = CorElementType::ELEMENT_TYPE_U4;
    return true;
  }
  else {
    *result = CorElementType::ELEMENT_TYPE_I4;
    return true;
  }
}

bool NumericCompilerHelper::IsNumericalType(const CorElementType &cor_type) {
  return IsIntegralType(cor_type)
    || cor_type == CorElementType::ELEMENT_TYPE_R4
    || cor_type == CorElementType::ELEMENT_TYPE_R8;
}

bool NumericCompilerHelper::IsIntegralType(const CorElementType &cor_type) {
  return cor_type == CorElementType::ELEMENT_TYPE_I1
    || cor_type == CorElementType::ELEMENT_TYPE_U1
    || cor_type == CorElementType::ELEMENT_TYPE_I2
    || cor_type == CorElementType::ELEMENT_TYPE_U2
    || cor_type == CorElementType::ELEMENT_TYPE_I4
    || cor_type == CorElementType::ELEMENT_TYPE_U4
    || cor_type == CorElementType::ELEMENT_TYPE_I8
    || cor_type == CorElementType::ELEMENT_TYPE_U8
    || cor_type == CorElementType::ELEMENT_TYPE_CHAR;
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
