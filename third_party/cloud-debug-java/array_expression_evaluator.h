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

#ifndef ARRAY_EXPRESSION_EVALUATOR_H_
#define ARRAY_EXPRESSION_EVALUATOR_H_

#include "common.h"
#include "expression_evaluator.h"
#include "type_signature.h"

namespace google_cloud_debugger {

// Evaluates indexer access expressions.
// For example: a[b] where a is a Dictionary, Array or List.
class IndexerAccessExpressionEvaluator : public ExpressionEvaluator {
 public:
  // Stores the subexpression for the collection and the index.
  IndexerAccessExpressionEvaluator(
      std::unique_ptr<ExpressionEvaluator> source_collection,
      std::unique_ptr<ExpressionEvaluator> source_index);

  // Compiles the expression to determine its return type.
  // TODO(quoct): We are only compiling the expression if
  // the source_collection_ expression is an array. We need
  // to do this for any expression that has get_Item() function.
  HRESULT Compile(
      DbgStackFrame *stack_frame,
      std::ostream *err_stream) override;

  // Returns the static type that the expression compiles to.
  // This will be the type of the item in the collection.
  // For example, if the expression is a[3] where a is System.Int32[],
  // then this will return System.Int32.
  const TypeSignature& GetStaticType() const override { return return_type_; }

  // Evaluates the expression and returns the result in dbg_object.
  // TODO(quoct): Add logic for accessing item in Dictionary, List, etc.
  HRESULT Evaluate(
      std::shared_ptr<DbgObject> *dbg_object,
      IEvalCoordinator *eval_coordinator, std::ostream *err_stream) const override;

 private:
  // Subexpression that computes the actual collection.
  std::unique_ptr<ExpressionEvaluator> source_collection_;

  // Subexpression that computes the key of the object in the collection
  // that we are accessing.
  std::unique_ptr<ExpressionEvaluator> source_index_;

  // Type of the objects in the collection.
  TypeSignature return_type_;

  DISALLOW_COPY_AND_ASSIGN(IndexerAccessExpressionEvaluator);
};


}  // namespace google_cloud_debugger

#endif  // ARRAY_EXPRESSION_EVALUATOR_H_
