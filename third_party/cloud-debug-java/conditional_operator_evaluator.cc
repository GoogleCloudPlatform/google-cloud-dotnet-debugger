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

#include "dbg_primitive.h"
#include "class_names.h"
#include "error_messages.h"
#include "compiler_helpers.h"

namespace google_cloud_debugger {

ConditionalOperatorEvaluator::ConditionalOperatorEvaluator(
    std::unique_ptr<ExpressionEvaluator> condition,
    std::unique_ptr<ExpressionEvaluator> if_true,
    std::unique_ptr<ExpressionEvaluator> if_false)
    : condition_(std::move(condition)),
      if_true_(std::move(if_true)),
      if_false_(std::move(if_false)) {
  result_type_ = TypeSignature::Object;
}


HRESULT ConditionalOperatorEvaluator::Compile(
    IDbgStackFrame* stack_frame, ICorDebugILFrame *debug_frame,
    std::ostream *err_stream) {
  HRESULT hr = condition_->Compile(stack_frame, debug_frame, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  hr = if_true_->Compile(stack_frame, debug_frame, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  hr = if_false_->Compile(stack_frame, debug_frame, err_stream);
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
    return S_OK;
  }

  // Case 2: both "if_true_" and "if_false_" are numeric.
  if (CompileNumeric()) {
    return S_OK;
  }

  // Case 3: both "if_true_" and "if_false_" are objects.
  if (CompileObjects()) {
    return S_OK;
  }

  *err_stream << kTypeMismatch;
  return E_FAIL;
}


bool ConditionalOperatorEvaluator::CompileBoolean() {
  if ((if_true_->GetStaticType().cor_type == CorElementType::ELEMENT_TYPE_BOOLEAN) &&
      (if_false_->GetStaticType().cor_type == CorElementType::ELEMENT_TYPE_BOOLEAN)) {
    result_type_ = { CorElementType::ELEMENT_TYPE_BOOLEAN, kBooleanClassName };
    return true;
  }

  return false;
}

bool ConditionalOperatorEvaluator::CompileNumeric() {
  const TypeSignature &true_type = if_true_->GetStaticType();
  const TypeSignature &false_type = if_false_->GetStaticType();
  if (NumericCompilerHelper::IsImplicitNumericConversionable(
      true_type, false_type)) {
    result_type_ = false_type;
    return true;
  }

  if (NumericCompilerHelper::IsImplicitNumericConversionable(
      false_type, true_type)) {
    result_type_ = true_type;
    return true;
  }

  return false;
}


bool ConditionalOperatorEvaluator::CompileObjects() {
  const TypeSignature &true_type = if_true_->GetStaticType();
  const TypeSignature &false_type = if_false_->GetStaticType();
  if (true_type.cor_type == false_type.cor_type &&
      (TypeCompilerHelper::IsObjectType(true_type.cor_type)
        || true_type.is_array
        || true_type.cor_type == CorElementType::ELEMENT_TYPE_STRING)) {
    if (true_type.compare(false_type) == 0) {
      result_type_ = true_type;
    } else {
      result_type_ = TypeSignature::Object;  // Note the lost signature.
    }
    return true;
  }

  return false;
}

HRESULT ConditionalOperatorEvaluator::Evaluate(
    std::shared_ptr<DbgObject> *dbg_object,
    IEvalCoordinator *eval_coordinator,
    IDbgObjectFactory *obj_factory,
    std::ostream *err_stream) const {
  std::shared_ptr<DbgObject> condition_obj;
  HRESULT hr =
      condition_->Evaluate(&condition_obj, eval_coordinator,
                           obj_factory, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  DbgPrimitive<bool> *condition_value = dynamic_cast<DbgPrimitive<bool> *>(condition_obj.get());
  if (!condition_value) {
    return E_FAIL;
  }

  if (condition_value->GetValue()) {
    return if_true_->Evaluate(dbg_object, eval_coordinator,
                              obj_factory, err_stream);
  }

  return if_false_->Evaluate(dbg_object, eval_coordinator,
                             obj_factory, err_stream);
}

}  // namespace google_cloud_debugger
