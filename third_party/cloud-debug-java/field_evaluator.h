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

#ifndef DEVTOOLS_CDBG_DEBUGLETS_JAVA_FIELD_EVALUATOR_H_
#define DEVTOOLS_CDBG_DEBUGLETS_JAVA_FIELD_EVALUATOR_H_

#include "common.h"
#include "expression_evaluator.h"
#include "type_signature.h"

namespace google_cloud_debugger {

class DbgClassProperty;

// Evaluates class fields (either instance or static).
class FieldEvaluator : public ExpressionEvaluator {
 public:
  // Class constructor for "field" reader. It can handle two cases:
  // 1. Instance field of an object computed by "instance_source". The
  // "possible_class_name" is ignored in this case.
  // 2. Static variable of a "possible_class_name" class (if specified). The
  // name should be fully qualified (e.g. "com.my.Green"). "instance_source"
  // is ignored in this case.
  FieldEvaluator(
      std::unique_ptr<ExpressionEvaluator> instance_source,
      std::string identifier_name,
      std::string possible_class_name,
      std::string field_name);

  HRESULT Compile(
      DbgStackFrame *stack_frame,
      std::ostream *err_stream) override;

  const TypeSignature &GetStaticType() const override { return result_type_; }

  HRESULT Evaluate(
      std::shared_ptr<DbgObject> *dbg_object,
      IEvalCoordinator *eval_coordinator,
      std::ostream *err_stream) const override;

 private:
  // Tries to compile the subexpression as a reader of instance field.
  HRESULT CompileInstanceField(
      DbgStackFrame *stack_frame,
      std::ostream *err_stream);

  // Tries to compile the subexpression as a reader of a static field.
  HRESULT CompileStaticField(
      DbgStackFrame *stack_frame,
      std::ostream *err_stream);

 private:
  // Expression computing the source object to read field from.
  std::unique_ptr<ExpressionEvaluator> instance_source_;

  // Fully qualified identifier name we are trying to interpret. This should
  // be "possible_class_name_.identifier_name".
  std::string identifier_name_;

  // Fully qualified class name to try to interpret "field_name_" as static.
  std::string possible_class_name_;

  // Name of the instance field to read.
  std::string field_name_;

  // Statically computed resulting type of the expression. This is what
  // computer_ is supposed product.
  TypeSignature result_type_;

  // If the field is a non-autoimplemented property, this field will be set
  // to that property.
  std::unique_ptr<DbgClassProperty> class_property;

  DISALLOW_COPY_AND_ASSIGN(FieldEvaluator);
};

}  // namespace google_cloud_debugger

#endif  // DEVTOOLS_CDBG_DEBUGLETS_JAVA_FIELD_EVALUATOR_H_
