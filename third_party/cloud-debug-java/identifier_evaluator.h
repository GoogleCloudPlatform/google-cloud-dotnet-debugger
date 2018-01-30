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

#ifndef DEVTOOLS_CDBG_DEBUGLETS_JAVA_IDENTIFIER_EVALUATOR_H_
#define DEVTOOLS_CDBG_DEBUGLETS_JAVA_IDENTIFIER_EVALUATOR_H_

#include "common.h"
#include "expression_evaluator.h"
#include "../../src/google_cloud_debugger/google_cloud_debugger_lib/type_signature.h"

namespace google_cloud_debugger {

class DbgObject;
class DbgClassProperty;

// Evaluates local variables, static variables and member variables encountered
// in Java expression.
class IdentifierEvaluator : public ExpressionEvaluator {
 public:
  explicit IdentifierEvaluator(std::string identifier_name);

  virtual HRESULT Compile(
      DbgStackFrame *stack_frame,
      std::ostream *err_stream) override;

  const TypeSignature& GetStaticType() const override { return result_type_; }

  HRESULT Evaluate(
      std::shared_ptr<DbgObject> *dbg_object,
      IEvalCoordinator *eval_coordinator, std::ostream *err_stream) const override;

 private:
  // Name of the identifier (whether it is local variable or something else).
  std::string identifier_name_;

  std::shared_ptr<DbgObject> identifier_object_;

  std::unique_ptr<DbgClassProperty> class_property_;

  // Statically computed resulting type of the expression. This is what
  // computer_ is supposed to produce.
  TypeSignature result_type_;

  bool is_non_auto_property = false;

  DISALLOW_COPY_AND_ASSIGN(IdentifierEvaluator);
};

}  // namespace google_cloud_debugger

#endif  // DEVTOOLS_CDBG_DEBUGLETS_JAVA_IDENTIFIER_EVALUATOR_H_
