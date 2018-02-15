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

#ifndef DEVTOOLS_CDBG_DEBUGLETS_JAVA_TYPE_CAST_OPERATOR_EVALUATOR_H_
#define DEVTOOLS_CDBG_DEBUGLETS_JAVA_TYPE_CAST_OPERATOR_EVALUATOR_H_

#include "common.h"
#include "expression_evaluator.h"
#include "java_expression.h"
#include "type_signature.h"

namespace google_cloud_debugger {

// Implements Type cast C# operator.
// Logic is based on:
// https://docs.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/expressions#cast-expressions
class TypeCastOperatorEvaluator : public ExpressionEvaluator {
 public:
  // Class constructor. The instance will own "source".
  TypeCastOperatorEvaluator(
      std::unique_ptr<ExpressionEvaluator> source,
      const std::string& target_type);

  HRESULT Compile(
      DbgStackFrame *stack_frame, std::ostream *err_stream) override;

  const TypeSignature& GetStaticType() const override {
    return result_type_;
  }

  Nullable<jvalue> GetStaticValue() const override {
    return nullptr;
  }

  ErrorOr<JVariant> Evaluate(
      const EvaluationContext& evaluation_context) const override;

 private:
  // No-op Computer.
   HRESULT DoNothingComputer(std::shared_ptr<DbgObject> source,
     std::shared_ptr<DbgObject> *result) const;

  // Numerical-cast Computer.
  template <typename T>
  HRESULT NumericalCastComputer(
    std::shared_ptr<DbgObject> source,
    std::shared_ptr<DbgObject> *result) const;

  // Evaluation method when the type cast is of Object type.
  ErrorOr<JVariant> ObjectTypeComputer(const JVariant& source) const;

  // Returns true if it is invalid boolean type conversion from boolean
  // to primitive numeric type and vice versa
  bool IsInvalidPrimitiveBooleanTypeConversion(const CorElementType &source,
    const CorElementType &target) const;

  // Returns true if both source and target types are primitive boolean.
  bool AreBothTypesPrimitiveBoolean() const;

  // Returns true if either source or target type is Object Array.
  bool IsEitherTypeObjectArray();

  // Compiled expression corresponding to the source.
  std::unique_ptr<ExpressionEvaluator> source_;

  // Statically computed resulting type of the expression.
  JSignature result_type_;

  // Target type of the expression.
  TypeSignature target_type_;

  // Target class derived by looking up the target_type_;
  jobject target_class_ = { nullptr };

  // Pointer to a member function of this class to do the actual evaluation.
  HRESULT (TypeCastOperatorEvaluator::*computer_)(
      std::shared_ptr<DbgObject> source,
      std::shared_ptr<DbgObject> *result) const;

  DISALLOW_COPY_AND_ASSIGN(TypeCastOperatorEvaluator);
};


}  // namespace google_cloud_debugger

#endif  // DEVTOOLS_CDBG_DEBUGLETS_JAVA_TYPE_CAST_OPERATOR_EVALUATOR_H_
