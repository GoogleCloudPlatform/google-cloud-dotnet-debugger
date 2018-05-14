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

#include "unary_expression_evaluator.h"

#include "compiler_helpers.h"
#include "dbg_object.h"
#include "dbg_primitive.h"
#include "error_messages.h"
#include "type_signature.h"

namespace google_cloud_debugger {

UnaryExpressionEvaluator::UnaryExpressionEvaluator(
    UnaryCSharpExpression::Type type, std::unique_ptr<ExpressionEvaluator> arg)
    : type_(type), arg_(std::move(arg)), computer_(nullptr) {
  result_type_ = TypeSignature::Object;
}

HRESULT UnaryExpressionEvaluator::Compile(IDbgStackFrame *stack_frame,
                                          ICorDebugILFrame *debug_frame,
                                          std::ostream *err_stream) {
  HRESULT hr = arg_->Compile(stack_frame, debug_frame, err_stream);
  if (FAILED(hr)) {
    *err_stream << kFailedToCompileFirstSubExpr;
    return hr;
  }

  switch (type_) {
    case UnaryCSharpExpression::Type::plus:
    case UnaryCSharpExpression::Type::minus:
      return CompilePlusMinusOperators(err_stream);

    case UnaryCSharpExpression::Type::bitwise_complement:
      return CompileBitwiseComplement(err_stream);

    case UnaryCSharpExpression::Type::logical_complement:
      return CompileLogicalComplement(err_stream);
  }

  return E_FAIL;
}

HRESULT UnaryExpressionEvaluator::CompilePlusMinusOperators(
    std::ostream *err_stream) {
  assert((type_ == UnaryCSharpExpression::Type::plus) ||
         (type_ == UnaryCSharpExpression::Type::minus));
  const bool is_plus = (type_ == UnaryCSharpExpression::Type::plus);
  CorElementType cor_type = arg_->GetStaticType().cor_type;

  // Promote sbyte, byte, short, ushort or char to int.
  if (NumericCompilerHelper::IsNumericallyPromotedToInt(cor_type)) {
    cor_type = CorElementType::ELEMENT_TYPE_I4;
  }

  // For - operator, we also promote uint to long.
  if (!is_plus && cor_type == CorElementType::ELEMENT_TYPE_U4) {
    cor_type = CorElementType::ELEMENT_TYPE_I8;
  }

  result_type_ = {cor_type};
  HRESULT hr = TypeCompilerHelper::ConvertCorElementTypeToString(
      cor_type, &result_type_.type_name);
  if (FAILED(hr)) {
    return hr;
  }

  // For + operator, we do nothing.
  // TODO(quoct): Add support for Decimal.
  if (is_plus) {
    if (cor_type == CorElementType::ELEMENT_TYPE_I4 ||
        cor_type == CorElementType::ELEMENT_TYPE_U4 ||
        cor_type == CorElementType::ELEMENT_TYPE_I8 ||
        cor_type == CorElementType::ELEMENT_TYPE_U8 ||
        cor_type == CorElementType::ELEMENT_TYPE_R4 ||
        cor_type == CorElementType::ELEMENT_TYPE_R8) {
      computer_ = DoNothingComputer;
      return S_OK;
    }
    return E_FAIL;
  }

  // - operator case.
  switch (cor_type) {
    case CorElementType::ELEMENT_TYPE_I4: {
      computer_ = MinusOperatorComputer<int32_t>;
      return S_OK;
    }
    // uint promoted to long.
    case CorElementType::ELEMENT_TYPE_I8: {
      computer_ = MinusOperatorComputer<int64_t>;
      return S_OK;
    }
    case CorElementType::ELEMENT_TYPE_R4: {
      computer_ = MinusOperatorComputer<float_t>;
      return S_OK;
    }
    case CorElementType::ELEMENT_TYPE_R8: {
      computer_ = MinusOperatorComputer<double_t>;
      return S_OK;
    }
    default: {
      // Unary plus and minus operators only apply to primitive numeric type.
      *err_stream << kTypeMismatch;
      return E_FAIL;
    }
  }
}

HRESULT UnaryExpressionEvaluator::CompileBitwiseComplement(
    std::ostream *err_stream) {
  assert(type_ == UnaryCSharpExpression::Type::bitwise_complement);
  CorElementType cor_type = arg_->GetStaticType().cor_type;

  // Promote sbyte, byte, short, ushort or char to int.
  if (NumericCompilerHelper::IsNumericallyPromotedToInt(cor_type)) {
    cor_type = CorElementType::ELEMENT_TYPE_I4;
  }

  result_type_ = { cor_type };
  HRESULT hr = TypeCompilerHelper::ConvertCorElementTypeToString(
      cor_type, &result_type_.type_name);
  if (FAILED(hr)) {
    return hr;
  }

  switch (cor_type) {
    case CorElementType::ELEMENT_TYPE_I4: {
      computer_ = BitwiseComplementComputer<int32_t>;
      return S_OK;
    }
    case CorElementType::ELEMENT_TYPE_U4: {
      computer_ = BitwiseComplementComputer<uint32_t>;
      return S_OK;
    }
    case CorElementType::ELEMENT_TYPE_I8: {
      computer_ = BitwiseComplementComputer<int64_t>;
      return S_OK;
    }
    case CorElementType::ELEMENT_TYPE_U8: {
      computer_ = BitwiseComplementComputer<uint64_t>;
      return S_OK;
    }
    default:
      // Bitwise complement only applies to primitive integral types.
      *err_stream << kTypeMismatch;
      return E_FAIL;
  }
}

HRESULT UnaryExpressionEvaluator::CompileLogicalComplement(
    std::ostream *err_stream) {
  assert(type_ == UnaryCSharpExpression::Type::logical_complement);
  if (arg_->GetStaticType().cor_type == CorElementType::ELEMENT_TYPE_BOOLEAN) {
    computer_ = LogicalComplementComputer;
    result_type_ = { CorElementType::ELEMENT_TYPE_BOOLEAN, kBooleanClassName };
    return S_OK;
  }

  *err_stream << kTypeMismatch;
  return E_FAIL;
}

HRESULT UnaryExpressionEvaluator::Evaluate(
      std::shared_ptr<DbgObject> *dbg_object,
      IEvalCoordinator *eval_coordinator,
      IDbgObjectFactory *obj_factory,
      std::ostream *err_stream) const {
  std::shared_ptr<DbgObject> arg_obj;
  HRESULT hr = arg_->Evaluate(&arg_obj, eval_coordinator,
                              obj_factory, err_stream);
  if (FAILED(hr)) {
    *err_stream << kFailedToEvalFirstSubExpr;
    return hr;
  }

  return computer_(arg_obj, dbg_object);
}

HRESULT UnaryExpressionEvaluator::LogicalComplementComputer(
    std::shared_ptr<DbgObject> arg_object,
    std::shared_ptr<DbgObject> *dbg_object) {
  if (!dbg_object) {
    return E_INVALIDARG;
  }

  bool value;
  HRESULT hr = NumericCompilerHelper::ExtractPrimitiveValue<bool>(
      arg_object.get(), &value);
  if (FAILED(hr)) {
    return hr;
  }

  *dbg_object = std::shared_ptr<DbgObject>(new DbgPrimitive<bool>(!value));
  return S_OK;
}

HRESULT UnaryExpressionEvaluator::DoNothingComputer(
    std::shared_ptr<DbgObject> arg_object,
    std::shared_ptr<DbgObject> *dbg_object) {
  *dbg_object = arg_object;
  return S_OK;
}

template <typename T>
HRESULT UnaryExpressionEvaluator::MinusOperatorComputer(
    std::shared_ptr<DbgObject> arg_object,
    std::shared_ptr<DbgObject> *dbg_object) {
  if (!dbg_object) {
    return E_INVALIDARG;
  }

  T value;
  HRESULT hr = NumericCompilerHelper::ExtractPrimitiveValue<T>(
      arg_object.get(), &value);
  if (FAILED(hr)) {
    return hr;
  }

  *dbg_object = std::shared_ptr<DbgObject>(new DbgPrimitive<T>(-value));
  return S_OK;
}

template <typename T>
HRESULT UnaryExpressionEvaluator::BitwiseComplementComputer(
    std::shared_ptr<DbgObject> arg_object,
    std::shared_ptr<DbgObject> *dbg_object) {
  if (!dbg_object) {
    return E_INVALIDARG;
  }

  T value;
  HRESULT hr = NumericCompilerHelper::ExtractPrimitiveValue<T>(
      arg_object.get(), &value);
  if (FAILED(hr)) {
    return hr;
  }

  *dbg_object = std::shared_ptr<DbgObject>(new DbgPrimitive<T>(~value));
  return S_OK;
}

}  // namespace google_cloud_debugger
