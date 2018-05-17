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

#include "expression_evaluator.h"
#include "ccomptr.h"

namespace google_cloud_debugger {

class ICorDebugHelper;

// Evaluates indexer access expressions.
// For example: a[b] where a is a Dictionary, Array or List.
class IndexerAccessExpressionEvaluator : public ExpressionEvaluator {
 public:
  // Stores the subexpression for the collection and the index.
  IndexerAccessExpressionEvaluator(
      std::unique_ptr<ExpressionEvaluator> source_collection,
      std::unique_ptr<ExpressionEvaluator> source_index,
      std::shared_ptr<ICorDebugHelper> debug_helper);

  // Compiles the expression to determine its return type.
  // TODO(quoct): We are only compiling the expression if
  // the source_collection_ expression is an array. We need
  // to do this for any expression that has get_Item() function.
  HRESULT Compile(
      IDbgStackFrame *stack_frame,
      ICorDebugILFrame *debug_frame,
      std::ostream *err_stream) override;

  // Returns the static type that the expression compiles to.
  // This will be the type of the item in the collection.
  // For example, if the expression is a[3] where a is System.Int32[],
  // then this will return System.Int32.
  const TypeSignature& GetStaticType() const override { return return_type_; }

  // Evaluates the expression and returns the result in dbg_object.
  HRESULT Evaluate(
      std::shared_ptr<DbgObject> *dbg_object,
      IEvalCoordinator *eval_coordinator,
      IDbgObjectFactory *obj_factory,
      std::ostream *err_stream) const override;

 private:
  // Evaluates the expression when the source is an array.
  HRESULT EvaluateArrayIndex(
      std::shared_ptr<DbgObject> source_obj,
      std::shared_ptr<DbgObject> index_obj,
      std::shared_ptr<DbgObject> *dbg_object,
      IEvalCoordinator *eval_coordinator,
      IDbgObjectFactory *obj_factory,
      std::ostream *err_stream) const;

  // Evaluates the expression when the source is a class with
  // get_Item method.
  HRESULT EvaluateGetItemIndex(
      std::shared_ptr<DbgObject> source_obj,
      std::shared_ptr<DbgObject> index_obj,
      std::shared_ptr<DbgObject> *dbg_object,
      IEvalCoordinator *eval_coordinator,
      IDbgObjectFactory *obj_factory,
      std::ostream *err_stream) const;

  // Subexpression that computes the actual collection.
  std::unique_ptr<ExpressionEvaluator> source_collection_;

  // Subexpression that computes the key of the object in the collection
  // that we are accessing.
  std::unique_ptr<ExpressionEvaluator> source_index_;

  // Type of the objects in the collection.
  TypeSignature return_type_;

  // ICorDebugFunction for get_Item method.
  CComPtr<ICorDebugFunction> get_item_method_;

  // Helper for dealing with ICorDebug objects.
  std::shared_ptr<ICorDebugHelper> debug_helper_;

  DISALLOW_COPY_AND_ASSIGN(IndexerAccessExpressionEvaluator);
};


}  // namespace google_cloud_debugger

#endif  // ARRAY_EXPRESSION_EVALUATOR_H_
