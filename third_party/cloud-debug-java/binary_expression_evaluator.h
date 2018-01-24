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

#ifndef DEVTOOLS_CDBG_DEBUGLETS_JAVA_BINARY_EXPRESSION_EVALUATOR_H_
#define DEVTOOLS_CDBG_DEBUGLETS_JAVA_BINARY_EXPRESSION_EVALUATOR_H_

#include "common.h"
#include "expression_evaluator.h"
#include "java_expression.h"

namespace google_cloud_debugger {

// Implements all Java binary operators.
class BinaryExpressionEvaluator : public ExpressionEvaluator {
 public:
  // Class constructor. The instance will own "arg1" and "arg2". These are
  // expected to be uninitialized at this point.
  BinaryExpressionEvaluator(
      BinaryJavaExpression::Type type,
      std::unique_ptr<ExpressionEvaluator> arg1,
      std::unique_ptr<ExpressionEvaluator> arg2);

  // Compiles the subexpressions. Based on the static type of the subexpressions
  // and the binary operator, compiles and sets the static type of this expression.
  HRESULT Compile(
      DbgStackFrame* readers_factory,
      std::ostream* err_stream) override;

  // Returns the static type of the expression.
  const TypeSignature& GetStaticType() const override {
    return result_type_;
  }

  // Evaluates the binary expression.
  // This will first evaluate the first subexpression.
  // If the operator is either && or ||, this function may skip
  // evaluating the second subexpression (short-circuiting).
  // Otherwise, evaluates the second expression and perform
  // the binary function computer_ on both of them.
  HRESULT Evaluate(
    std::shared_ptr<DbgObject> *dbg_object,
    IEvalCoordinator *eval_coordinator,
    std::ostream *err_stream) const override;

 private:
  // Implements "Compile" for arithmetical operators (+, -, *, /, %).
  HRESULT CompileArithmetical(std::ostream* err_stream);

  // Implements "Compile" for conditional operators (e.g. &&, ==, <=).
  HRESULT CompileRelational(std::ostream* err_stream);

  // Implements "Compile" for boolean conditional operators
  // (e.g. &, |, &&, ==, <=).
  HRESULT CompileBooleanConditional(std::ostream* err_stream);

  // Implements "Compile" for bitwise operators (&, |, ^).
  HRESULT CompileLogical(std::ostream* err_stream);

  // Implements "Compile" for shoft operators (<<, >>, >>>).
  HRESULT CompileShift(std::ostream* err_stream);

  // Computes the value of the expression for arithmetical operators. The
  // template type "T" is the type that both arguments were promoted into.
  template <typename T>
  HRESULT ArithmeticComputer(
      std::shared_ptr<DbgObject> arg1,
      std::shared_ptr<DbgObject> arg2,
      std::shared_ptr<DbgObject> *result) const;

  // Computes the value of the expression for bitwise operators. This does not
  // include bitwise operators applied on booleans (which become conditional
  // operators). The template type "T" can be either any integral types.
  // "T" is the type that both arguments were promoted too.
  template <typename T>
  HRESULT BitwiseComputer(
      std::shared_ptr<DbgObject> arg1,
      std::shared_ptr<DbgObject> arg2,
      std::shared_ptr<DbgObject> *result) const;

  // Computes the value of shift expression. The template type "T" denotes the
  // type of the first argument (the shifted number). The type of
  // the second argument must be int. "Bitmask" is applied to the
  // second argument as per specifications:
  // https://docs.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/expressions#shift-operators
  template <typename T, uint16 Bitmask>
  HRESULT ShiftComputer(
      std::shared_ptr<DbgObject> arg1,
      std::shared_ptr<DbgObject> arg2,
      std::shared_ptr<DbgObject> *result) const;

  // Implements comparison operator on .NET objects.
  // Objects are equal if they have the same address.
  HRESULT ConditionalObjectComputer(
      std::shared_ptr<DbgObject> arg1,
      std::shared_ptr<DbgObject> arg2,
      std::shared_ptr<DbgObject> *result) const;

  // Compares two stringgs.
  HRESULT ConditionalStringComputer(
      std::shared_ptr<DbgObject> arg1,
      std::shared_ptr<DbgObject> arg2,
      std::shared_ptr<DbgObject> *result) const;

  // Implements conditional operators. This method will extract out
  // the boolean value from arg1 and arg2 and perform the binary operators
  // on them. The result will be stored in result.
  HRESULT ConditionalBooleanComputer(
      std::shared_ptr<DbgObject> arg1,
      std::shared_ptr<DbgObject> arg2,
      std::shared_ptr<DbgObject> *result) const;

  // Implements comparison operators for numerical types (i.e. not booleans).
  // The two arguments are promoted to the same type and compared against each other.
  template <typename T>
  HRESULT NumericalComparisonComputer(
      std::shared_ptr<DbgObject> arg1,
      std::shared_ptr<DbgObject> arg2,
      std::shared_ptr<DbgObject> *result) const;

 private:
  // Binary expression type (e.g. + or <<).
  const BinaryJavaExpression::Type type_;

  // Compiled expression corresponding to the first operand.
  std::unique_ptr<ExpressionEvaluator> arg1_;

  // Compiled expression corresponding to the second operand.
  std::unique_ptr<ExpressionEvaluator> arg2_;

  // Pointer to a member function of this class to do the actual evaluation
  // of the binary expression.
  HRESULT (BinaryExpressionEvaluator::*computer_)(
      std::shared_ptr<DbgObject> arg1,
      std::shared_ptr<DbgObject> arg2,
      std::shared_ptr<DbgObject> *result) const;

  // Statically computed resulting type of the expression. This is what
  // computer_ is supposed product.
  TypeSignature result_type_;

  DISALLOW_COPY_AND_ASSIGN(BinaryExpressionEvaluator);
};


}  // namespace google_cloud_debugger

#endif  // DEVTOOLS_CDBG_DEBUGLETS_JAVA_BINARY_EXPRESSION_EVALUATOR_H_
