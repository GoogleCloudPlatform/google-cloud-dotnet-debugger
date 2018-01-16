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

#ifndef DEVTOOLS_CDBG_DEBUGLETS_JAVA_CONDITIONAL_OPERATOR_EVALUATOR_H_
#define DEVTOOLS_CDBG_DEBUGLETS_JAVA_CONDITIONAL_OPERATOR_EVALUATOR_H_

#include "common.h"
#include "expression_evaluator.h"

namespace google_cloud_debugger {

// Implements conditional C# operator (i.e. a ? b : c).
class ConditionalOperatorEvaluator : public ExpressionEvaluator {
 public:
  // Class constructor. The instance will own "condition", "if_true" and
  // "if_false". These are expected to be uninitialized at this point.
  ConditionalOperatorEvaluator(
      std::unique_ptr<ExpressionEvaluator> condition,
      std::unique_ptr<ExpressionEvaluator> if_true,
      std::unique_ptr<ExpressionEvaluator> if_false);

  // Determines the type of the expression using
  // the helper functions CompileBoolean, CompileNumeric
  // and CompileObjects.
  HRESULT Compile(
      DbgStackFrame *stack_frame, std::ostream *err_stream) override;

  // Returns static type of this expression, determined by
  // the true and false expression.
  virtual const TypeSignature& GetStaticType() const override {
    return result_type_;
  }

  // To evaluate the expression, the condition will be evaluated first.
  // If the condition is not a boolean object, return an error.
  // If condition is true, call Evaluate on the true expression.
  // If condition is false, call Evaluate on the false expression.
  HRESULT Evaluate(
      std::shared_ptr<google_cloud_debugger::DbgObject> *dbg_object,
      IEvalCoordinator *eval_coordinator,
      std::ostream *err_stream) const override;

 private:
  // Compiles the conditional operator if both "if_true_" and "if_false_"
  // are boolean. Returns false if arguments are of other types.
  bool CompileBoolean();

  // Compiles the conditional operator if both "if_true_" and "if_false_"
  // are numeric. Returns true only if both of them are of the same numeric
  // type or 1 can be implicitly converted to the other.
  bool CompileNumeric();

  // Compiles the conditional operator if both "if_true_" and "if_false_"
  // are references to objects. If the expressions
  // have the same type name, then gives result_type_ that type name.
  // Otherwise, sets result_type_ to object type.
  // TODO(quoct): Support the case where 1 type name can be
  // implicitly converted to the other. For example, if
  // if_true_ is a child class of if_false_.
  bool CompileObjects();

 private:
  // Compiled expression corresponding to the condition.
  std::unique_ptr<ExpressionEvaluator> condition_;

  // Expression applied if "condition_" evaluates to true.
  std::unique_ptr<ExpressionEvaluator> if_true_;

  // Expression applied if "condition_" evaluates to false.
  std::unique_ptr<ExpressionEvaluator> if_false_;

  // Statically computed resulting type of the expression. This is what
  // computer_ is supposed to produce.
  TypeSignature result_type_;

  DISALLOW_COPY_AND_ASSIGN(ConditionalOperatorEvaluator);
};


}  // namespace google_cloud_debugger

#endif  // DEVTOOLS_CDBG_DEBUGLETS_JAVA_CONDITIONAL_OPERATOR_EVALUATOR_H_
