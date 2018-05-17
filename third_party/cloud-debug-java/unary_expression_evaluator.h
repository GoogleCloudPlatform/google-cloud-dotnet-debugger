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

#ifndef UNARY_EXPRESSION_EVALUATOR_H_
#define UNARY_EXPRESSION_EVALUATOR_H_

#include "expression_evaluator.h"
#include "csharp_expression.h"

namespace google_cloud_debugger {

// Implements all CSharp unary operators.
class UnaryExpressionEvaluator : public ExpressionEvaluator {
 public:
  // Class constructor. The instance will own "arg". "arg" is expected to be
  // uninitialized at this point.
  UnaryExpressionEvaluator(
      UnaryCSharpExpression::Type type,
      std::unique_ptr<ExpressionEvaluator> arg);

  // Compiles and extracts the static type of the expression into result_type_.
  // Will return an failed HRESULT if the expression cannot be compiled.
  // This function will also select the appropriate member function to
  // evaluate the expressoin and assign that to computer_.
  HRESULT Compile(
      IDbgStackFrame *stack_frame,
      ICorDebugILFrame *debug_frame,
      std::ostream *err_stream) override;

  // Returns the static type of the expression.
  const TypeSignature& GetStaticType() const override {
    return result_type_;
  }

  // Evaluates the expression and stores the result in dbg_object.
  // This simply calls computer_, which should have been assigned in Compile call. 
  HRESULT Evaluate(
      std::shared_ptr<DbgObject> *dbg_object,
      IEvalCoordinator *eval_coordinator,
      IDbgObjectFactory *obj_factory,
      std::ostream *err_stream) const override;

 private:
  // Tries to compile the expression for unary plus and minus operators.
  // Returns E_FAIL if the argument is not suitable.
  // The logic here is based on:
  // https://docs.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/expressions#unary-operators
  HRESULT CompilePlusMinusOperators(std::ostream *err_stream);

  // Tries to compile the expression for bitwise complement operator (!).
  // Returns E_FAIL if the argument is not suitable.
  // The logic here is based on:
  // https://docs.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/expressions#unary-operators
  HRESULT CompileBitwiseComplement(std::ostream *err_stream);

  // Tries to compile the expression for logical complement operator (!).
  // Returns E_FAIL if the argument is not suitable.
  // The logic here is based on:
  // https://docs.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/expressions#unary-operators
  HRESULT CompileLogicalComplement(std::ostream *err_stream);

  // Computes the logical complement of a boolean argument (operator !).
  // This extracts out value in arg_object and creates a DbgObject
  // with value !value. Will returns failed HRESULT if arg_object is not
  // a boolean.
  static HRESULT LogicalComplementComputer(std::shared_ptr<DbgObject> arg_object,
      std::shared_ptr<DbgObject> *dbg_object);

  // NOP computer used for unary plus operator (+) that does nothing beyond.
  // numeric promotion.
  static HRESULT DoNothingComputer(std::shared_ptr<DbgObject> arg_object,
      std::shared_ptr<DbgObject> *dbg_object);

  // This extracts out value in arg_object and creates a DbgObject
  // with value -value.
  template <typename T>
  static HRESULT MinusOperatorComputer(std::shared_ptr<DbgObject> arg_object,
      std::shared_ptr<DbgObject> *dbg_object);

  // This extracts out value in arg_object and creates a DbgObject
  // with value ~value.
  template <typename T>
  static HRESULT BitwiseComplementComputer(std::shared_ptr<DbgObject> arg_object,
      std::shared_ptr<DbgObject> *dbg_object);

 private:
  // Binary expression type (e.g. +, -, ~, !).
  const UnaryCSharpExpression::Type type_;

  // Compiled expression corresponding to the unary operator argument.
  std::unique_ptr<ExpressionEvaluator> arg_;

  // Pointer to a member function of this class to do the actual evaluation
  // of the unary expression.
  HRESULT (*computer_)(std::shared_ptr<DbgObject> arg_object,
      std::shared_ptr<DbgObject> *result_object);

  // Statically computed resulting type of the expression. This is what
  // computer_ is supposed product.
  TypeSignature result_type_;

  DISALLOW_COPY_AND_ASSIGN(UnaryExpressionEvaluator);
};


}  // namespace google_cloud_debugger

#endif  // UNARY_EXPRESSION_EVALUATOR_H_
