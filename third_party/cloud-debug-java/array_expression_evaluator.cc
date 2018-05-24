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

#include "array_expression_evaluator.h"

#include "compiler_helpers.h"
#include "constants.h"
#include "dbg_array.h"
#include "error_messages.h"
#include "i_cor_debug_helper.h"
#include "i_dbg_object_factory.h"
#include "i_dbg_stack_frame.h"
#include "i_eval_coordinator.h"
#include "method_info.h"

namespace google_cloud_debugger {

IndexerAccessExpressionEvaluator::IndexerAccessExpressionEvaluator(
    std::unique_ptr<ExpressionEvaluator> source_collection,
    std::unique_ptr<ExpressionEvaluator> source_index,
    std::shared_ptr<ICorDebugHelper> debug_helper)
    : source_collection_(std::move(source_collection)),
      source_index_(std::move(source_index)),
      debug_helper_(debug_helper) {}

HRESULT IndexerAccessExpressionEvaluator::Compile(IDbgStackFrame *stack_frame,
                                                  ICorDebugILFrame *debug_frame,
                                                  std::ostream *err_stream) {
  HRESULT hr =
      source_collection_->Compile(stack_frame, debug_frame, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  hr = source_index_->Compile(stack_frame, debug_frame, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  const TypeSignature &index_type = source_index_->GetStaticType();
  const TypeSignature &source_type = source_collection_->GetStaticType();
  if (source_type.is_array) {
    if (!TypeCompilerHelper::IsIntegralType(index_type.cor_type)) {
      *err_stream << "Index of the array must be of integral type.";
      return E_FAIL;
    }

    // Type of each element in the array.
    if (source_type.generic_types.size() != 1) {
      *err_stream << "Cannot find the type of each array element.";
      return E_FAIL;
    }

    return_type_ = source_type.generic_types[0];
    return S_OK;
  }

  // If this is not an array, we need to get the function get_Item().
  CComPtr<ICorDebugModule> debug_module;
  CComPtr<IMetaDataImport> metadata_import;
  mdTypeDef class_token;

  hr = stack_frame->GetClassTokenAndModule(source_type.type_name, &class_token,
                                           &debug_module, &metadata_import);
  if (FAILED(hr)) {
    std::cerr << "Failed to retrieve class token for class  "
              << source_type.type_name;
    return hr;
  }

  MethodInfo method_info;
  method_info.argument_types.push_back(index_type);
  method_info.method_name = "get_Item";
  // Retrieves get_Item method.
  hr = stack_frame->GetDebugFunctionFromClass(metadata_import, debug_module,
                                              class_token, &method_info,
                                              source_type.generic_types,
                                              &get_item_method_);
  if (hr == S_FALSE) {
    hr = E_FAIL;
  }

  if (FAILED(hr)) {
    std::cerr << "Failed to retrieve ICorDebugFunction for get_Item "
              << " from class " << source_type.type_name;
    return hr;
  }

  return_type_ = method_info.returned_type;
  return S_OK;
}

HRESULT IndexerAccessExpressionEvaluator::Evaluate(
    std::shared_ptr<DbgObject> *dbg_object, IEvalCoordinator *eval_coordinator,
    IDbgObjectFactory *obj_factory, std::ostream *err_stream) const {
  std::shared_ptr<DbgObject> source_obj;
  HRESULT hr = source_collection_->Evaluate(&source_obj, eval_coordinator,
                                            obj_factory, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  if (source_obj->GetIsNull()) {
    return E_FAIL;
  }

  std::shared_ptr<DbgObject> index_obj;
  hr = source_index_->Evaluate(&index_obj, eval_coordinator, obj_factory,
                               err_stream);
  if (FAILED(hr)) {
    *err_stream << "Failed to evaluate source of the indexer access.";
    return hr;
  }

  // In the case that the source is an array, we retrieve the index as a long
  // and use that index to access the item in the array.
  if (TypeCompilerHelper::IsArrayType(source_obj->GetCorElementType())) {
    return EvaluateArrayIndex(source_obj, index_obj, dbg_object,
                              eval_coordinator, obj_factory, err_stream);
  }

  hr = EvaluateGetItemIndex(source_obj, index_obj, dbg_object,
                            eval_coordinator, obj_factory, err_stream);
  if (FAILED(hr)) {
    *err_stream << "Failed to evaluate indexer access.";
  }
  return hr;
}

HRESULT IndexerAccessExpressionEvaluator::EvaluateArrayIndex(
    std::shared_ptr<DbgObject> source_obj, std::shared_ptr<DbgObject> index_obj,
    std::shared_ptr<DbgObject> *dbg_object, IEvalCoordinator *eval_coordinator,
    IDbgObjectFactory *obj_factory, std::ostream *err_stream) const {
  int64_t index;
  HRESULT hr = NumericCompilerHelper::ExtractPrimitiveValue<int64_t>(
      index_obj.get(), &index);
  if (FAILED(hr)) {
    return hr;
  }

  DbgArray *array_obj = dynamic_cast<DbgArray *>(source_obj.get());
  if (array_obj == nullptr) {
    return E_INVALIDARG;
  }

  CComPtr<ICorDebugValue> array_item;
  hr = array_obj->GetArrayItem(index, &array_item);
  if (FAILED(hr)) {
    return hr;
  }

  std::unique_ptr<DbgObject> target_object;
  hr = obj_factory->CreateDbgObject(array_item, kDefaultObjectEvalDepth,
                                    &target_object, &std::cerr);
  if (FAILED(hr)) {
    return hr;
  }

  *dbg_object = std::move(target_object);
  return hr;
}

HRESULT IndexerAccessExpressionEvaluator::EvaluateGetItemIndex(
    std::shared_ptr<DbgObject> source_obj, std::shared_ptr<DbgObject> index_obj,
    std::shared_ptr<DbgObject> *dbg_object, IEvalCoordinator *eval_coordinator,
    IDbgObjectFactory *obj_factory, std::ostream *err_stream) const {
  // TODO(quoct): Look into refactoring the code below in this class,
  // method_call_evaluator, field_evaluator, identifier_evaluator
  // and dbg_class_property.
  CComPtr<ICorDebugEval> debug_eval;
  HRESULT hr = eval_coordinator->CreateEval(&debug_eval);
  if (FAILED(hr)) {
    std::cerr << "Failed to create ICorDebugEval.";
    return hr;
  }

  // Gets ICorDebugValue representing the source and the index.
  CComPtr<ICorDebugValue> source_debug_value;
  hr = source_obj->GetICorDebugValue(&source_debug_value, debug_eval);
  if (FAILED(hr)) {
    return hr;
  }

  CComPtr<ICorDebugValue> index_debug_value;
  hr = index_obj->GetICorDebugValue(&index_debug_value, debug_eval);
  if (FAILED(hr)) {
    return hr;
  }

  std::vector<ICorDebugValue *> arg_debug_values;
  arg_debug_values.push_back(source_debug_value);
  arg_debug_values.push_back(index_debug_value);

  // Gets the generic type associated the the class.
  std::vector<CComPtr<ICorDebugType>> generic_class_types;
  hr = debug_helper_->PopulateGenericClassTypesFromClassObject(
      source_debug_value, &generic_class_types, &std::cerr);
  if (FAILED(hr)) {
    return hr;
  }

  std::vector<ICorDebugType *> eval_generic_types;
  eval_generic_types.assign(generic_class_types.begin(),
                            generic_class_types.end());

  std::unique_ptr<DbgObject> eval_obj_result;
  hr = obj_factory->EvaluateAndCreateDbgObject(
    std::move(eval_generic_types), std::move(arg_debug_values),
    get_item_method_, debug_eval, eval_coordinator,
    &eval_obj_result, &std::cerr);
  if (FAILED(hr)) {
    return hr;
  }

  *dbg_object = std::move(eval_obj_result);
  return S_OK;
}

}  // namespace google_cloud_debugger
