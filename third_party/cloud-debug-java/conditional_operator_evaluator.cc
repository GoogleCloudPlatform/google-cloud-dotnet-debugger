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

#include "conditional_operator_evaluator.h"

#include "numeric_cast_evaluator.h"
#include "../../google_cloud_debugger_lib/dbg_object.h"
#include "../../google_cloud_debugger_lib/class_names.h"
#include "../../google_cloud_debugger_lib/error_messages.h"

namespace google_cloud_debugger {

ConditionalOperatorEvaluator::ConditionalOperatorEvaluator(
    std::unique_ptr<ExpressionEvaluator> condition,
    std::unique_ptr<ExpressionEvaluator> if_true,
    std::unique_ptr<ExpressionEvaluator> if_false)
    : condition_(std::move(condition)),
      if_true_(std::move(if_true)),
      if_false_(std::move(if_false)) {
  result_type_ = TypeSignature {
    CorElementType::ELEMENT_TYPE_OBJECT,
    kObjectClassName
  };
}


HRESULT ConditionalOperatorEvaluator::Compile(
    DbgStackFrame* stack_frame, std::ostream *err_stream) {
  HRESULT hr = condition_->Compile(stack_frame);
  if (FAILED(hr)) {
    return hr;
  }

  hr = if_true_->Compile(stack_frame, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  hr = if_false_->Compile(stack_frame, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  // All conditional operators must have "condition_" of a boolean type.
  if (condition_->GetStaticType().cor_type != CorElementType::ELEMENT_TYPE_BOOLEAN) {
    *err_stream << kConditionHasToBeBoolean;
    return E_FAIL;
  }

  // Case 1: both "if_true_" and "if_false_" are of a boolean type.
  if (CompileBoolean()) {
    return true;
  }

  // Case 2: both "if_true_" and "if_false_" are numeric.
  if (CompileNumeric()) {
    return true;
  }

  // Case 3: both "if_true_" and "if_false_" are objects.
  if (CompileObjects()) {
    return true;
  }

  *err_stream << kTypeMisMatch;
  return E_FAIL;
}


bool ConditionalOperatorEvaluator::CompileBoolean() {
  // TODO(vlif): unbox "if_true_" and "if_false_" from Boolean to boolean.
  if ((if_true_->GetStaticType().cor_type == CorElementType::ELEMENT_TYPE_BOOLEAN) &&
      (if_false_->GetStaticType().cor_type == CorElementType::ELEMENT_TYPE_BOOLEAN)) {
    result_type_ = { CorElementType::ELEMENT_TYPE_BOOLEAN };
    return true;
  }

  return false;
}


bool ConditionalOperatorEvaluator::CompileNumeric() {
  // According to C# spec, if an implicit conversion exists from X to Y
  // but not from Y to X then Y is the type of the expression.
  // If an implicit conversion exists from Y to X but not from X to Y
  // then X is the type of the exppression.
  const CorElementType &true_type = if_true_->GetStaticType().cor_type;
  const CorElementType &false_type = if_false_->GetStaticType().cor_type;
  switch (true_type) {
    case CorElementType::ELEMENT_TYPE_I1:
    {
      switch (false_type) {
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
    MORE CASES HERE.
    default:
  }

  if (IsEitherType(CorElementType::ELEMENT_TYPE_R8)) {
    if (!ApplyNumericPromotions<jdouble>(&unused_error_message)) {
      return false;
    }

    result_type_ = { JType::Double };
    return true;

  } else if (IsEitherType(JType::Float)) {
    if (!ApplyNumericPromotions<jfloat>(&unused_error_message)) {
      return false;
    }

    result_type_ = { JType::Float };
    return true;

  } else if (IsEitherType(JType::Long)) {
    if (!ApplyNumericPromotions<jlong>(&unused_error_message)) {
      return false;
    }

    result_type_ = { JType::Long };
    return true;

  } else {
    if (!ApplyNumericPromotions<jint>(&unused_error_message)) {
      return false;
    }

    result_type_ = { JType::Int };
    return true;
  }
}


bool ConditionalOperatorEvaluator::CompileObjects() {
  // TODO(vlif): this is a super-simplistic implementation that doesn't cover
  // a lot of cases. For more details please refer to Java Language
  // Specification section 15.25.3 and tables in section 15.25.

  const JSignature& true_signature = if_true_->GetStaticType();
  const JSignature& false_signature = if_false_->GetStaticType();
  if ((true_signature.type == JType::Object) &&
      (false_signature.type == JType::Object)) {
    if (true_signature.object_signature == false_signature.object_signature) {
      result_type_ = true_signature;
    } else {
      result_type_ = { JType::Object };  // Note the lost signature.
    }
    return true;
  }

  return false;
}


bool ConditionalOperatorEvaluator::IsEitherType(const CorElementType &type) const {
  return (if_true_->GetStaticType().cor_type == type) ||
         (if_false_->GetStaticType().cor_type == type);
}


template <typename TTargetType>
bool ConditionalOperatorEvaluator::ApplyNumericPromotions(
    FormatMessageModel* error_message) {
  return ApplyNumericCast<TTargetType>(&if_true_, error_message) &&
         ApplyNumericCast<TTargetType>(&if_false_, error_message);
}


ErrorOr<JVariant> ConditionalOperatorEvaluator::Evaluate(
    const EvaluationContext& evaluation_context) const {
  ErrorOr<JVariant> evaluated_condition =
      condition_->Evaluate(evaluation_context);
  if (evaluated_condition.is_error()) {
    return evaluated_condition;
  }

  jboolean condition_value = false;
  if (!evaluated_condition.value().get<jboolean>(&condition_value)) {
    return INTERNAL_ERROR_MESSAGE;
  }

  const ExpressionEvaluator& target_expression =
      (condition_value ? *if_true_ : *if_false_);

  return target_expression.Evaluate(evaluation_context);
}

}  // namespace google_cloud_debugger
