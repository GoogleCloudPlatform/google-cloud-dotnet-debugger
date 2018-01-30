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

#ifndef DEVTOOLS_CDBG_DEBUGLETS_JAVA_LITERAL_EVALUATOR_H_
#define DEVTOOLS_CDBG_DEBUGLETS_JAVA_LITERAL_EVALUATOR_H_

#include "common.h"
#include "expression_evaluator.h"
#include "../../src/google_cloud_debugger/google_cloud_debugger_lib/dbg_object.h"
#include "../../src/google_cloud_debugger/google_cloud_debugger_lib/type_signature.h"

namespace google_cloud_debugger {

// Represents a constant of any type (other than a string).
class LiteralEvaluator : public ExpressionEvaluator {
 public:
  explicit LiteralEvaluator(std::shared_ptr<DbgObject> literal_obj)
      : result_type_({ literal_obj->GetCorElementType() }) {
    n_ = literal_obj;
  }

  virtual HRESULT Compile(
      DbgStackFrame* stack_frame, std::ostream *err_stream) override {
    return S_OK;
  }

  const TypeSignature& GetStaticType() const override { return result_type_; }

  HRESULT Evaluate(std::shared_ptr<DbgObject> *dbg_object,
      IEvalCoordinator *eval_coordinator, std::ostream *err_stream) const override {
    *dbg_object = n_;
    return S_OK;
  }

 private:
  // Literal value associated with this leaf.
  std::shared_ptr<google_cloud_debugger::DbgObject> n_;

  // Statically computed resulting type of the expression.
  const TypeSignature result_type_;

  DISALLOW_COPY_AND_ASSIGN(LiteralEvaluator);
};

}  // namespace google_cloud_debugger

#endif  // DEVTOOLS_CDBG_DEBUGLETS_JAVA_LITERAL_EVALUATOR_H_


