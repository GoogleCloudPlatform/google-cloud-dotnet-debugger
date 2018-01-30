/**
 * Copyright 2015 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "binary_expression_evaluator.h"

#include <cmath>
#include <limits>
#include "../../src/google_cloud_debugger/google_cloud_debugger_lib/compiler_helpers.h"
#include "../../src/google_cloud_debugger/google_cloud_debugger_lib/dbg_primitive.h"
#include "../../src/google_cloud_debugger/google_cloud_debugger_lib/dbg_string.h"
#include "../../src/google_cloud_debugger/google_cloud_debugger_lib/error_messages.h"
#include "../../src/google_cloud_debugger/google_cloud_debugger_lib/type_signature.h"

namespace google_cloud_debugger {

// Computes x % y. Handles floating point case too.
template<typename T> inline T ComputeModule(T x, T y) {
  if (std::is_floating_point<T>::value) {
    return std::fmod(x, y);
  }
  return x % y;
}

// Checks that the divisor will not trigger "division by zero" signal.
template<typename T> inline bool IsDivisionByZero(T divisor) {
  if (std::is_floating_point<T>::value) {
    return false; // Floating point division never triggers the signal
  }
  return divisor == 0;
}

// Detects edge case in integer division that causes SIGFPE signal.
static bool IsDivisionOverflow(int32_t value1, int32_t value2) {
  return (value1 == INT_MIN) &&
         (value2 == -1);
}


// Detects edge case in integer division that causes SIGFPE signal.
static bool IsDivisionOverflow(int64_t value1, int64_t value2) {
  return (value1 == LONG_MIN) &&
         (value2 == -1);
}


// Detects edge case in integer division that causes SIGFPE signal.
static bool IsDivisionOverflow(float_t value1, float_t value2) {
  return false;  // This condition does not apply to floating point.
}


// Detects edge case in integer division that causes SIGFPE signal.
static bool IsDivisionOverflow(double_t value1, double_t value2) {
  return false;  // This condition does not apply to floating point.
}

BinaryExpressionEvaluator::BinaryExpressionEvaluator(
    BinaryJavaExpression::Type type, std::unique_ptr<ExpressionEvaluator> arg1,
    std::unique_ptr<ExpressionEvaluator> arg2)
    : type_(type),
      arg1_(std::move(arg1)),
      arg2_(std::move(arg2)),
      computer_(nullptr) {
  result_type_ = TypeSignature::Object;
}

HRESULT BinaryExpressionEvaluator::Compile(DbgStackFrame *readers_factory,
                                           std::ostream *error_stream) {
  HRESULT hr;
  hr = arg1_->Compile(readers_factory, error_stream);
  if (FAILED(hr)) {
    return hr;
  }

  hr = arg2_->Compile(readers_factory, error_stream);
  if (FAILED(hr)) {
    return hr;
  }

  switch (type_) {
    case BinaryJavaExpression::Type::add:
    case BinaryJavaExpression::Type::sub:
    case BinaryJavaExpression::Type::mul:
    case BinaryJavaExpression::Type::div:
    case BinaryJavaExpression::Type::mod:
      return CompileArithmetical(error_stream);

    case BinaryJavaExpression::Type::eq:
    case BinaryJavaExpression::Type::ne:
    case BinaryJavaExpression::Type::le:
    case BinaryJavaExpression::Type::ge:
    case BinaryJavaExpression::Type::lt:
    case BinaryJavaExpression::Type::gt:
      return CompileRelational(error_stream);

    case BinaryJavaExpression::Type::conditional_and:
    case BinaryJavaExpression::Type::conditional_or:
      return CompileBooleanConditional(error_stream);

    case BinaryJavaExpression::Type::bitwise_and:
    case BinaryJavaExpression::Type::bitwise_or:
    case BinaryJavaExpression::Type::bitwise_xor:
      return CompileLogical(error_stream);

    case BinaryJavaExpression::Type::shl:
    case BinaryJavaExpression::Type::shr_s:
    case BinaryJavaExpression::Type::shr_u:
      return CompileShift(error_stream);
    default:
      // Compiler should catch any missing enums. We should never get here.
      return E_FAIL;
  }
}

HRESULT BinaryExpressionEvaluator::CompileArithmetical(
    std::ostream *err_stream) {
  // TODO(quoct): implement concatenation for strings
  CorElementType result;
  if (!NumericCompilerHelper::BinaryNumericalPromotion(
          arg1_->GetStaticType().cor_type, arg2_->GetStaticType().cor_type,
          &result, err_stream)) {
    *err_stream << kTypeMismatch;
    return E_FAIL;
  }

  result_type_ = {result};
  switch (result) {
    case CorElementType::ELEMENT_TYPE_I4: {
      computer_ = &BinaryExpressionEvaluator::ArithmeticComputer<int32_t>;
      return S_OK;
    }
    case CorElementType::ELEMENT_TYPE_U4: {
      computer_ = &BinaryExpressionEvaluator::ArithmeticComputer<uint32_t>;
      return S_OK;
    }
    case CorElementType::ELEMENT_TYPE_I8: {
      computer_ = &BinaryExpressionEvaluator::ArithmeticComputer<int64_t>;
      return S_OK;
    }
    case CorElementType::ELEMENT_TYPE_U8: {
      computer_ = &BinaryExpressionEvaluator::ArithmeticComputer<uint64_t>;
      return S_OK;
    }
    case CorElementType::ELEMENT_TYPE_R4: {
      computer_ = &BinaryExpressionEvaluator::ArithmeticComputer<float_t>;
      return S_OK;
    }
    case CorElementType::ELEMENT_TYPE_R8: {
      computer_ = &BinaryExpressionEvaluator::ArithmeticComputer<double_t>;
      return S_OK;
    }
    default: {
      *err_stream << kTypeMismatch;
      return E_FAIL;
    }
  }
}

HRESULT BinaryExpressionEvaluator::CompileRelational(std::ostream *err_stream) {
  const TypeSignature &signature1 = arg1_->GetStaticType();
  const TypeSignature &signature2 = arg2_->GetStaticType();
  result_type_ = {CorElementType::ELEMENT_TYPE_BOOLEAN};

  // If both items are numerical type, perform numeric promotion.
  if (TypeCompilerHelper::IsNumericalType(signature1.cor_type) &&
      TypeCompilerHelper::IsNumericalType(signature2.cor_type)) {
    CorElementType result;
    if (!NumericCompilerHelper::BinaryNumericalPromotion(
            signature1.cor_type, signature1.cor_type, &result, err_stream)) {
      *err_stream << kTypeMismatch;
      return E_FAIL;
    }

    switch (result) {
      case CorElementType::ELEMENT_TYPE_I4: {
        computer_ =
            &BinaryExpressionEvaluator::NumericalComparisonComputer<int32_t>;
        return S_OK;
      }
      case CorElementType::ELEMENT_TYPE_U4: {
        computer_ =
            &BinaryExpressionEvaluator::NumericalComparisonComputer<uint32_t>;
        return S_OK;
      }
      case CorElementType::ELEMENT_TYPE_I8: {
        computer_ =
            &BinaryExpressionEvaluator::NumericalComparisonComputer<int64_t>;
        return S_OK;
      }
      case CorElementType::ELEMENT_TYPE_U8: {
        computer_ =
            &BinaryExpressionEvaluator::NumericalComparisonComputer<uint64_t>;
        return S_OK;
      }
      case CorElementType::ELEMENT_TYPE_R4: {
        computer_ =
            &BinaryExpressionEvaluator::NumericalComparisonComputer<float_t>;
        return S_OK;
      }
      case CorElementType::ELEMENT_TYPE_R8: {
        computer_ =
            &BinaryExpressionEvaluator::NumericalComparisonComputer<double_t>;
        return S_OK;
      }
      default: {
        *err_stream << kTypeMismatch;
        return E_FAIL;
      }
    }
  }

  // We don't support the other expressions if the types are not numeric.
  if (type_ != BinaryJavaExpression::Type::eq &&
      type_ != BinaryJavaExpression::Type::ne) {
    *err_stream << kExpressionNotSupported;
    return E_NOTIMPL;
  }

  if (signature1.cor_type == CorElementType::ELEMENT_TYPE_BOOLEAN &&
      signature2.cor_type == CorElementType::ELEMENT_TYPE_BOOLEAN) {
    return CompileBooleanConditional(err_stream);
  }

  // Conditional operations applied to objects.
  if (signature1.type_name.compare(kStringClassName) == 0 &&
      signature2.type_name.compare(kStringClassName) == 0) {
    computer_ = &BinaryExpressionEvaluator::ConditionalStringComputer;
    return S_OK;
  } else if (!TypeCompilerHelper::IsNumericalType(signature1.cor_type) &&
             !TypeCompilerHelper::IsNumericalType(signature2.cor_type)) {
    // Compares address (only applies for non-numeric types.
    computer_ = &BinaryExpressionEvaluator::ConditionalObjectComputer;
    return S_OK;
  }

  *err_stream << kTypeMismatch;
  return E_FAIL;
}

HRESULT BinaryExpressionEvaluator::CompileBooleanConditional(
    std::ostream *err_stream) {
  // Conditional operations that apply to boolean arguments.
  if (arg1_->GetStaticType().cor_type == CorElementType::ELEMENT_TYPE_BOOLEAN &&
      arg2_->GetStaticType().cor_type == CorElementType::ELEMENT_TYPE_BOOLEAN) {
    computer_ = &BinaryExpressionEvaluator::ConditionalBooleanComputer;
    result_type_ = {CorElementType::ELEMENT_TYPE_BOOLEAN};
    return S_OK;
  }

  *err_stream << kTypeMismatch;
  return E_FAIL;
}

HRESULT BinaryExpressionEvaluator::CompileLogical(std::ostream *err_stream) {
  const CorElementType &arg1_type = arg1_->GetStaticType().cor_type;
  const CorElementType &arg2_type = arg2_->GetStaticType().cor_type;

  // We support 2 cases, either both arguments are integers
  // or if both arguments are boolean.
  if (TypeCompilerHelper::IsIntegralType(arg1_type) &&
      TypeCompilerHelper::IsIntegralType(arg2_type)) {
    // For numerical type, perform binary numerical promotion.
    CorElementType result;
    if (!NumericCompilerHelper::BinaryNumericalPromotion(
            arg1_->GetStaticType().cor_type, arg2_->GetStaticType().cor_type,
            &result, err_stream)) {
      *err_stream << kTypeMismatch;
      return E_FAIL;
    }

    result_type_ = { result };
    switch (result) {
      case CorElementType::ELEMENT_TYPE_I4: {
        computer_ = &BinaryExpressionEvaluator::BitwiseComputer<int32_t>;
        return S_OK;
      }
      case CorElementType::ELEMENT_TYPE_U4: {
        computer_ = &BinaryExpressionEvaluator::BitwiseComputer<uint32_t>;
        return S_OK;
      }
      case CorElementType::ELEMENT_TYPE_I8: {
        computer_ = &BinaryExpressionEvaluator::BitwiseComputer<int64_t>;
        return S_OK;
      }
      case CorElementType::ELEMENT_TYPE_U8: {
        computer_ = &BinaryExpressionEvaluator::BitwiseComputer<uint64_t>;
        return S_OK;
      }
      default: {
        *err_stream << kTypeMismatch;
        return E_FAIL;
      }
    }
  }

  // Otherwise, try to compile them as boolean.
  return CompileBooleanConditional(err_stream);
}

HRESULT BinaryExpressionEvaluator::CompileShift(std::ostream *err_stream) {
  CorElementType arg1_type = arg1_->GetStaticType().cor_type;
  CorElementType arg2_type = arg2_->GetStaticType().cor_type;
  if (!TypeCompilerHelper::IsIntegralType(arg1_type) &&
      !TypeCompilerHelper::IsIntegralType(arg2_type)) {
    *err_stream << kTypeMismatch;
    return E_FAIL;
  }

  // Arg2 has to be an int or numerically promotable to an int.
  if (!NumericCompilerHelper::IsNumericallyPromotedToInt(arg2_type) &&
      arg2_type != CorElementType::ELEMENT_TYPE_I4) {
    *err_stream << kTypeMismatch;
    return E_FAIL;
  }

  if (NumericCompilerHelper::IsNumericallyPromotedToInt(arg1_type)) {
    arg1_type = CorElementType::ELEMENT_TYPE_I4;
  }

  result_type_ = {arg1_type};
  switch (arg1_type) {
    case CorElementType::ELEMENT_TYPE_I4: {
      computer_ = &BinaryExpressionEvaluator::ShiftComputer<int32_t, 0x1f>;
      return S_OK;
    }
    case CorElementType::ELEMENT_TYPE_U4: {
      computer_ = &BinaryExpressionEvaluator::ShiftComputer<uint32_t, 0x1f>;
      return S_OK;
    }
    case CorElementType::ELEMENT_TYPE_I8: {
      computer_ = &BinaryExpressionEvaluator::ShiftComputer<int64_t, 0x3f>;
      return S_OK;
    }
    case CorElementType::ELEMENT_TYPE_U8: {
      computer_ = &BinaryExpressionEvaluator::ShiftComputer<uint64_t, 0x3f>;
      return S_OK;
    }
    default: {
      *err_stream << kTypeMismatch;
      return false;
    }
  }
}

HRESULT BinaryExpressionEvaluator::Evaluate(
    std::shared_ptr<DbgObject> *dbg_object, IEvalCoordinator *eval_coordinator,
    std::ostream *err_stream) const {
  std::shared_ptr<DbgObject> arg1_obj;
  HRESULT hr = arg1_->Evaluate(&arg1_obj, eval_coordinator, err_stream);
  if (FAILED(hr)) {
    *err_stream << kFailedToEvalFirstSubExpr;
    return hr;
  }

  // For this special case, don't evaluate the second obj.
  if (type_ == BinaryJavaExpression::Type::conditional_and) {
    bool boolean1;
    HRESULT hr = NumericCompilerHelper::ExtractPrimitiveValue<bool>(
        arg1_obj.get(), &boolean1);
    if (FAILED(hr)) {
      return hr;
    }

    // If arg1 in 'arg1 && arg2' is false, expression is false.
    if (!boolean1) {
      *dbg_object = std::shared_ptr<DbgObject>(new DbgPrimitive<bool>(false));
      return S_OK;
    }
    // Otherwise, proceeds to evaluate the second operand.
  }
  else if (type_ == BinaryJavaExpression::Type::conditional_or) {
    bool boolean1;
    HRESULT hr = NumericCompilerHelper::ExtractPrimitiveValue<bool>(
        arg1_obj.get(), &boolean1);
    if (FAILED(hr)) {
      return hr;
    }

    // If arg1 in 'arg1 || arg2' is true, expression is true.
    if (boolean1) {
      *dbg_object = std::shared_ptr<DbgObject>(new DbgPrimitive<bool>(true));
      return S_OK;
    }
    // Otherwise, proceeds to evaluate the second operand.
  }

  std::shared_ptr<DbgObject> arg2_obj;
  HRESULT hr = arg2_->Evaluate(&arg2_obj, eval_coordinator, err_stream);
  if (FAILED(hr)) {
    *err_stream << kFailedToEvalSecondSubExpr;
    return hr;
  }

  return (this->*computer_)(arg1_obj, arg2_obj, dbg_object);
}

template <typename T>
HRESULT BinaryExpressionEvaluator::ArithmeticComputer(
    std::shared_ptr<DbgObject> arg1, std::shared_ptr<DbgObject> arg2,
    std::shared_ptr<DbgObject> *result) const {
  T value1;
  HRESULT hr = NumericCompilerHelper::ExtractPrimitiveValue<T>(
      arg1.get(), &value1);
  if (FAILED(hr)) {
    return hr;
  }

  T value2;
  HRESULT hr = NumericCompilerHelper::ExtractPrimitiveValue<T>(
      arg2.get(), &value2);
  if (FAILED(hr)) {
    return hr;
  }

  switch (type_) {
    case BinaryJavaExpression::Type::add: {
      *result = std::shared_ptr<DbgObject>(new DbgPrimitive<T>(value1 + value2));
      return S_OK;
    }

    case BinaryJavaExpression::Type::sub: {
      *result = std::shared_ptr<DbgObject>(new DbgPrimitive<T>(value1 - value2));
      return S_OK;
    }

    case BinaryJavaExpression::Type::mul: {
      *result = std::shared_ptr<DbgObject>(new DbgPrimitive<T>(value1 * value2));
      return S_OK;
    }

    case BinaryJavaExpression::Type::mod:
    case BinaryJavaExpression::Type::div:
      if (IsDivisionByZero(value2)) {
        return E_INVALIDARG;
      }

      if (IsDivisionOverflow(value1, value2)) {
        return E_INVALIDARG;
      }

      if (type_ == BinaryJavaExpression::Type::div) {
        *result = std::shared_ptr<DbgObject>(new DbgPrimitive<T>(value1 / value2));
        return S_OK;
      } else {
        *result = std::shared_ptr<DbgObject>(new DbgPrimitive<T>(ComputeModule(value1, value2)));
        return S_OK;
      }

    default:
      return E_NOTIMPL;
  }
}

template <typename T>
HRESULT BinaryExpressionEvaluator::BitwiseComputer(
    std::shared_ptr<DbgObject> arg1, std::shared_ptr<DbgObject> arg2,
    std::shared_ptr<DbgObject> *result) const {
  T value1;
  HRESULT hr = NumericCompilerHelper::ExtractPrimitiveValue<T>(
      arg1.get(), &value1);
  if (FAILED(hr)) {
    return hr;
  }

  T value2;
  HRESULT hr = NumericCompilerHelper::ExtractPrimitiveValue<T>(
      arg2.get(), &value2);
  if (FAILED(hr)) {
    return hr;
  }

  switch (type_) {
    case BinaryJavaExpression::Type::bitwise_and: {
      *result = std::shared_ptr<DbgObject>(new DbgPrimitive<T>(value1 & value2));
      return S_OK;
    }

    case BinaryJavaExpression::Type::bitwise_or: {
      *result = std::shared_ptr<DbgObject>(new DbgPrimitive<T>(value1 | value2));
      return S_OK;
    }

    case BinaryJavaExpression::Type::bitwise_xor: {
      *result = std::shared_ptr<DbgObject>(new DbgPrimitive<T>(value1 ^ value2));
      return S_OK;
    }

    default:
      return E_NOTIMPL;
  }
}

template <typename T, uint16 Bitmask>
HRESULT BinaryExpressionEvaluator::ShiftComputer(
    std::shared_ptr<DbgObject> arg1, std::shared_ptr<DbgObject> arg2,
    std::shared_ptr<DbgObject> *result) const {
  T value1;
  HRESULT hr =
      NumericCompilerHelper::ExtractPrimitiveValue<T>(arg_object.get(), &value);
  if (FAILED(hr)) {
    return hr;
  }

  int32_t value2 = 0;
  HRESULT hr = NumericCompilerHelper::ExtractPrimitiveValue<int32_t>(
      arg_object.get(), &value);
  if (FAILED(hr)) {
    return hr;
  }

  // For the predefined operators, the number of bits to
  // shift is computed as follows:
  //   1. When the type of x is int or uint,
  // the shift count is given by the low-order five bits of count.
  // In other words, the shift count is computed from count & 0x1F.
  //   2. When the type of x is long or ulong, the shift count
  // is given by the low-order six bits of count.
  // In other words, the shift count is computed from count & 0x3F.
  // Bitmask represents either 0x1F or 0x3F.
  value2 &= Bitmask;

  switch (type_) {
    case BinaryJavaExpression::Type::shl: {
      value1 = value1 << value2;
      break;
    }

    case BinaryJavaExpression::Type::shr_s:
    case BinaryJavaExpression::Type::shr_u: {
      value1 = value1 >> value2;
      break;
    }

    default:
      return E_NOTIMPL;
  }

  *result = std::shared_ptr<DbgObject>(new DbgPrimitive<T>(value1));
  return S_OK;
}

HRESULT BinaryExpressionEvaluator::ConditionalObjectComputer(
    std::shared_ptr<DbgObject> arg1, std::shared_ptr<DbgObject> arg2,
    std::shared_ptr<DbgObject> *result) const {
  bool has_same_address = arg1->GetAddress() == arg2->GetAddress();

  switch (type_) {
    case BinaryJavaExpression::Type::eq: {
      *result = std::shared_ptr<DbgObject>(new DbgPrimitive<bool>(has_same_address));
      return S_OK;
    }

    case BinaryJavaExpression::Type::ne: {
      *result = std::shared_ptr<DbgObject>(new DbgPrimitive<bool>(has_same_address));
      return S_OK;
    }

    default:
      return E_NOTIMPL;
  }
}

HRESULT BinaryExpressionEvaluator::ConditionalStringComputer(
    std::shared_ptr<DbgObject> arg1, std::shared_ptr<DbgObject> arg2,
    std::shared_ptr<DbgObject> *result) const {
  // Extracts out the 2 strings and compare them.
  std::string first_string;
  HRESULT hr = DbgString::GetString(arg1.get(), &first_string);
  if (FAILED(hr)) {
    return hr;
  }

  std::string second_string;
  HRESULT hr = DbgString::GetString(arg2.get(), &second_string);
  if (FAILED(hr)) {
    return hr;
  }

  const bool is_equal = first_string.compare(second_string) == 0;

  switch (type_) {
    case BinaryJavaExpression::Type::eq: {
      *result = std::shared_ptr<DbgObject>(new DbgPrimitive<bool>(is_equal));
      return S_OK;
    }

    case BinaryJavaExpression::Type::ne: {
      *result = std::shared_ptr<DbgObject>(new DbgPrimitive<bool>(!is_equal));
      return S_OK;
    }

    default:
      return E_NOTIMPL;
  }
}

HRESULT BinaryExpressionEvaluator::ConditionalBooleanComputer(
    std::shared_ptr<DbgObject> arg1, std::shared_ptr<DbgObject> arg2,
    std::shared_ptr<DbgObject> *result) const {
  // Extracts out the booleans and perform the binary operator.
  bool boolean1;
  HRESULT hr = NumericCompilerHelper::ExtractPrimitiveValue<bool>(
      arg1.get(), &boolean1);
  if (FAILED(hr)) {
    return hr;
  }

  bool boolean2;
  HRESULT hr = NumericCompilerHelper::ExtractPrimitiveValue<bool>(
      arg2.get(), &boolean2);
  if (FAILED(hr)) {
    return hr;
  }

  switch (type_) {
    case BinaryJavaExpression::Type::conditional_and:
    case BinaryJavaExpression::Type::bitwise_and: {
      *result = std::shared_ptr<DbgObject>(new DbgPrimitive<bool>(boolean1 && boolean2));
      return S_OK;
    }

    case BinaryJavaExpression::Type::conditional_or:
    case BinaryJavaExpression::Type::bitwise_or: {
      *result = std::shared_ptr<DbgObject>(new DbgPrimitive<bool>(boolean1 || boolean2));
      return S_OK;
    }

    case BinaryJavaExpression::Type::eq: {
      *result = std::shared_ptr<DbgObject>(new DbgPrimitive<bool>(boolean1 == boolean2));
      return S_OK;
    }

    case BinaryJavaExpression::Type::ne:
    case BinaryJavaExpression::Type::bitwise_xor: {
      *result = std::shared_ptr<DbgObject>(new DbgPrimitive<bool>(boolean1 != boolean2));
      return S_OK;
    }

    default:
      return E_NOTIMPL;
  }
}

template <typename T>
HRESULT BinaryExpressionEvaluator::NumericalComparisonComputer(
    std::shared_ptr<DbgObject> arg1, std::shared_ptr<DbgObject> arg2,
    std::shared_ptr<DbgObject> *result) const {
  T value1;
  HRESULT hr = NumericCompilerHelper::ExtractPrimitiveValue<T>(
      arg1.get(), &value1);
  if (FAILED(hr)) {
    return hr;
  }

  T value2;
  HRESULT hr = NumericCompilerHelper::ExtractPrimitiveValue<T>(
      arg2.get(), &value2);
  if (FAILED(hr)) {
    return hr;
  }

  switch (type_) {
    case BinaryJavaExpression::Type::eq:
      return JVariant::Boolean(value1 == value2);

    case BinaryJavaExpression::Type::ne:
    case BinaryJavaExpression::Type::bitwise_xor:
      return JVariant::Boolean(value1 != value2);

    case BinaryJavaExpression::Type::le:
      return JVariant::Boolean(value1 <= value2);

    case BinaryJavaExpression::Type::ge:
      return JVariant::Boolean(value1 >= value2);

    case BinaryJavaExpression::Type::lt:
      return JVariant::Boolean(value1 < value2);

    case BinaryJavaExpression::Type::gt:
      return JVariant::Boolean(value1 > value2);

    default:
      DCHECK(false);  // Any other operations are not supported for booleans.
      return INTERNAL_ERROR_MESSAGE;
  }
}

}  // namespace google_cloud_debugger
