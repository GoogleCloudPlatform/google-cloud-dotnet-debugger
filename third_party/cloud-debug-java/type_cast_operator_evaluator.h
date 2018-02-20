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

#ifndef DEVTOOLS_CDBG_DEBUGLETS_JAVA_TYPE_CAST_OPERATOR_EVALUATOR_H_
#define DEVTOOLS_CDBG_DEBUGLETS_JAVA_TYPE_CAST_OPERATOR_EVALUATOR_H_

#include "common.h"
#include "expression_evaluator.h"
#include "java_expression.h"
#include "type_signature.h"

namespace google_cloud_debugger {

// Implements Type cast C# operator.
// Logic is based on:
// https://docs.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/expressions#cast-expressions
class TypeCastOperatorEvaluator : public ExpressionEvaluator {
 public:
  // Class constructor. The instance will own "source".
  TypeCastOperatorEvaluator(std::unique_ptr<ExpressionEvaluator> source,
                            const std::string &target_type);

  // Compiles the expression.
  // If both source and target are boolean, this will set result_type_
  // to target_type_ and do nothing.
  // If both source and target are numerical types, this will set
  // the computer_ function to NumericalCastComputer.
  // If both source and target are object types, this will check whether
  // either of them is a base class of the other. If not, this function will
  // fail. Otherwise, sets result_type_ to target_type_ and do nothing.
  HRESULT Compile(DbgStackFrame *stack_frame,
                  std::ostream *err_stream) override;

  // Returns the static type of the expression.
  const TypeSignature &GetStaticType() const override { return result_type_; }

  // Evalutes the expression by calling the appropriate computer_.
  HRESULT Evaluate(std::shared_ptr<DbgObject> *dbg_object,
                   IEvalCoordinator *eval_coordinator,
                   std::ostream *err_stream) const override;

 private:
  // Compiles type cast expression when both the source
  // and target are numeric types.
  HRESULT CompileNumericalCast(const CorElementType &source_type,
                               const CorElementType &target_type,
                               std::ostream *err_stream);

  // Returns S_OK if source_type is a child class of target_type.
  HRESULT IsBaseType(DbgStackFrame *stack_frame, const std::string &source_type,
                     const std::string &target_type,
                     std::ostream *err_stream) const;

  // No-op Computer.
  HRESULT DoNothingComputer(std::shared_ptr<DbgObject> source,
                            std::shared_ptr<DbgObject> *result) const;

  // Numerical-cast Computer.
  // Gets T value from source and creates a DbgPrimitive object
  // of type T with that value.
  template <typename T>
  HRESULT NumericalCastComputer(std::shared_ptr<DbgObject> source,
                                std::shared_ptr<DbgObject> *result) const {
    if (!source || !result) {
      return E_INVALIDARG;
    }

    T value;
    HRESULT hr =
        NumericCompilerHelper::ExtractPrimitiveValue<T>(source.get(), &value);

    if (FAILED(hr)) {
      return hr;
    }

    *result = std::shared_ptr<DbgObject>(new DbgPrimitive<T>(value));
    return S_OK;
  }

  // Returns true if this expression is a valid boolean type conversion from boolean
  // to primitive numeric type and vice versa.
  bool IsValidPrimitiveBooleanTypeConversion(
      const CorElementType &source, const CorElementType &target) const;

  // Compiled expression corresponding to the source.
  std::unique_ptr<ExpressionEvaluator> source_;

  // Statically computed resulting type of the expression.
  TypeSignature result_type_;

  // Target type of the expression.
  TypeSignature target_type_;

  // Pointer to a member function of this class to do the actual evaluation.
  HRESULT (TypeCastOperatorEvaluator::*computer_)
  (std::shared_ptr<DbgObject> source, std::shared_ptr<DbgObject> *result) const;

  DISALLOW_COPY_AND_ASSIGN(TypeCastOperatorEvaluator);
};

}  // namespace google_cloud_debugger

#endif  // DEVTOOLS_CDBG_DEBUGLETS_JAVA_TYPE_CAST_OPERATOR_EVALUATOR_H_
