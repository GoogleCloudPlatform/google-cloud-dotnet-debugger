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

#ifndef EXPRESSION_EVALUATOR_H_
#define EXPRESSION_EVALUATOR_H_

#include <memory>

#include "common_headers.h"

namespace google_cloud_debugger {

class DbgObject;
class IDbgStackFrame;
class IEvalCoordinator;
class ICorDebugHelper;
class IDbgObjectFactory;

// Interface representing compiled expression or subexpression.
class ExpressionEvaluator {
 public:
  virtual ~ExpressionEvaluator() { }

  // Prepares the expression for execution and implements all static type
  // checks. For example "true + 8" is not a valid expression (though the
  // parser can't tell that). This expression error will be detected by
  // "Compile". In such cases "Compile" will return false and describe the
  // problem in "error_message". The "context" is used to obtain local
  // variables at the location where the expression is going to be evaluated
  // later and access static variables (still not implemented). The lifetime
  // of "context" is limited to "Compile" function. If the expression embeds
  // inner instances of "ExpressionEvaluator", the "Compile" must be called
  // recursively. The initialization phase is separated from the evaluation
  // phase to improve performance of repeatedly evaluated expressions and to
  // minimize amount of time that the debugged thread is paused on breakpoint.
  virtual HRESULT Compile(
      IDbgStackFrame *stack_frame, ICorDebugILFrame *debug_frame,
      std::ostream *err_stream) = 0;

  // Gets the type of the expression as it is known at compile time. If the
  // code is correct, the runtime type will be the same as compile time type.
  virtual const TypeSignature& GetStaticType() const = 0;

  // Evaluates the current value of the expression. Returns error if expression
  // computation fails. Failure can happen due to null references, underlying
  // calls fail and mismatched runtime types. If successful, dbg_object will
  // point to the result.
  // eval_coordinator is used for method call evaluation.
  virtual HRESULT Evaluate(
      std::shared_ptr<DbgObject> *dbg_object,
      IEvalCoordinator *eval_coordinator,
      IDbgObjectFactory *obj_factory,
      std::ostream *err_stream) const = 0;
};

}  // namespace google_cloud_debugger

#endif  // EXPRESSION_EVALUATOR_H_


