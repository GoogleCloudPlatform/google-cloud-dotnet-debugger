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

#ifndef IDENTIFIER_EVALUATOR_H_
#define IDENTIFIER_EVALUATOR_H_

#include <vector>

#include "expression_evaluator.h"

namespace google_cloud_debugger {

class DbgObject;
class DbgClassProperty;

// Evaluates local variables, static variables and member variables encountered
// in CSharp expression.
class IdentifierEvaluator : public ExpressionEvaluator {
 public:
  explicit IdentifierEvaluator(std::string identifier_name);

  virtual HRESULT Compile(
      IDbgStackFrame *stack_frame,
      ICorDebugILFrame *debug_frame,
      std::ostream *err_stream) override;

  const TypeSignature& GetStaticType() const override { return result_type_; }

  HRESULT Evaluate(
      std::shared_ptr<DbgObject> *dbg_object,
      IEvalCoordinator *eval_coordinator,
      IDbgObjectFactory *obj_factory,
      std::ostream *err_stream) const override;

 private:
  // Name of the identifier (whether it is local variable or something else).
  std::string identifier_name_;

  std::shared_ptr<DbgObject> identifier_object_;

  std::unique_ptr<DbgClassProperty> class_property_;

  // Generic type parameters for the class that the method is in.
  std::vector<CComPtr<ICorDebugType>> generic_class_types_;

  // Statically computed resulting type of the expression. This is what
  // computer_ is supposed to produce.
  TypeSignature result_type_;

  DISALLOW_COPY_AND_ASSIGN(IdentifierEvaluator);
};

}  // namespace google_cloud_debugger

#endif  // IDENTIFIER_EVALUATOR_H_
