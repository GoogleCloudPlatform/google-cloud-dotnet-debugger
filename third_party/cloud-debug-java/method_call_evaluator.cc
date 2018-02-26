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

#include "method_call_evaluator.h"

#include <string>
#include <sstream>

#include "ccomptr.h"
#include "cordebug.h"
#include "dbg_reference_object.h"
#include "dbg_stack_frame.h"
#include "debugger_callback.h"
#include "i_eval_coordinator.h"
#include "i_cor_debug_helper.h"
#include "method_info.h"

namespace google_cloud_debugger {

MethodCallEvaluator::MethodCallEvaluator(
    string method_name, std::unique_ptr<ExpressionEvaluator> instance_source,
    string possible_class_name,
    std::vector<std::unique_ptr<ExpressionEvaluator>> arguments)
    : method_name_(std::move(method_name)),
      instance_source_(std::move(instance_source)),
      possible_class_name_(std::move(possible_class_name)),
      arguments_(std::move(arguments)) {}

HRESULT MethodCallEvaluator::Compile(DbgStackFrame* stack_frame,
                                     std::ostream* err_stream) {
  HRESULT hr;
  method_info_.method_name = method_name_;
  std::vector<std::string> argument_types;

  // Don't support method call with more than 10 arguments.
  const int max_argument_supported = 10;

  if (arguments_.size() > max_argument_supported) {
    *err_stream << "Method call with more than 10 arguments are not supported.";
    return E_FAIL;
  }

  // Compile argument expressions.
  for (auto& argument : arguments_) {
    hr = argument->Compile(stack_frame, err_stream);
    if (FAILED(hr)) {
      return hr;
    }

    method_info_.argument_types.push_back(argument->GetStaticType().type_name);
  }

  if ((instance_source_ == nullptr) && possible_class_name_.empty()) {
    // No source and no class name so this has to be interpreted
    // as a method in the current class.
    hr = stack_frame->GetDebugFunctionFromCurrentClass(&method_info_,
      &matched_method_);
    if (FAILED(hr)) {
      return hr;
    }

    if (matched_method_) {
      // Retrieves the generic types for the class.
      // TOOD(quoct): Need to find a way to do this for
      // fully qualified class name. Probably have to update ANTLR
      // grammar file to support that.
      hr = stack_frame->GetClassGenericTypeParameters(&generic_class_types_);
      if (FAILED(hr)) {
        *err_stream << "Failed to retrieve generic type parameters for class.";
        return hr;
      }
    }
  }

  if (!matched_method_ && instance_source_ != nullptr) {
    // Calling method on a result of prior expression (for example:
    // "a.b.startsWith(...)").
    hr = instance_source_->Compile(stack_frame, err_stream);
    if (FAILED(hr)) {
      return hr;
    }

    std::string source_class_name = instance_source_->GetStaticType().type_name;
    hr = GetMethodFromClassNameHelper(source_class_name, stack_frame, err_stream);
    if (FAILED(hr)) {
      *err_stream << "Failed to retrieve ICorDebugFunction "
        << method_info_.method_name << " from the current class.";
      return hr;
    }

    if (matched_method_) {
      instance_source_is_invoking_obj_ = true;
    }
  }

  if (!matched_method_ && !possible_class_name_.empty()) {
    hr = GetMethodFromClassNameHelper(possible_class_name_,
        stack_frame, err_stream);
    if (FAILED(hr)) {
      return hr;
    }
  }

  if (!matched_method_) {
    return E_FAIL;
  }

  // We need to store the frame to get the "this" object later.
  hr = stack_frame->GetFrame(&debug_frame_);
  if (FAILED(hr)) {
    return hr;
  }

  // TODO(quoct): Generic methods are not supported yet.
  if (method_info_.has_generic_types) {
    return E_NOTIMPL;
  }

  if (stack_frame->IsStaticMethod() && !method_info_.is_static) {
    return E_FAIL;
  }

  return S_OK;
}

HRESULT MethodCallEvaluator::Evaluate(
      std::shared_ptr<DbgObject> *dbg_object,
      IEvalCoordinator *eval_coordinator, std::ostream *err_stream) const {
  // Create ICorDebugEval with the ICorDebugFunction.
  CComPtr<ICorDebugEval> debug_eval;
  HRESULT hr = eval_coordinator->CreateEval(&debug_eval);
  if (FAILED(hr)) {
    *err_stream << "Failed to create ICorDebugEval.";
    return hr;
  }

  CComPtr<ICorDebugEval2> debug_eval_2;
  hr = debug_eval->QueryInterface(__uuidof(ICorDebugEval2),
                                  reinterpret_cast<void **>(&debug_eval_2));

  if (FAILED(hr)) {
    return hr;
  }

  CComPtr<ICorDebugValue> invoking_object;
  // We need the invoking object in case if this is a non-static call.
  // In addition, if this expression is A.B where A is the instance_source_
  // (so instance_source_is_invoking_obj_ is true), we would also need
  // to retrieve A to get the instantiated generic class types of A.
  if (instance_source_is_invoking_obj_ || !method_info_.is_static) {
    hr = GetInvokingObject(&invoking_object, eval_coordinator, err_stream);
    if (FAILED(hr)) {
      *err_stream << "Failed to get invoking object for method call "
        << method_name_;
      return hr;
    }
  }

  std::vector<std::shared_ptr<DbgObject>> arguments_objs;
  
  for (auto& argument : arguments_) {
    std::shared_ptr<DbgObject> arg_obj;
    hr = argument->Evaluate(&arg_obj, eval_coordinator, err_stream);
    if (FAILED(hr)) {
      return hr;
    }
    arguments_objs.push_back(std::move(arg_obj));
  }

  std::vector<ICorDebugValue *> arg_debug_values;
  if (method_info_.is_static) {
    arg_debug_values.push_back(invoking_object);
  }

  for (auto &argument_obj : arguments_objs) {
    CComPtr<ICorDebugValue> arg_debug_value;
    hr = argument_obj->GetICorDebugValue(&arg_debug_value, debug_eval);
    if (FAILED(hr)) {
      return hr;
    }

    // Add reference as debug_value will be deleted out of this scope.
    arg_debug_value->AddRef();
    arg_debug_values.push_back(arg_debug_value);
  }

  std::vector<ICorDebugType *> eval_generic_types;

  // If instance source is the invoking object, we can
  // also retrieve the class generic types here as it is now
  // instantiated.
  if (instance_source_is_invoking_obj_) {
    std::vector<CComPtr<ICorDebugType>> generic_class_types;
    hr = PopulateGenericClassTypesFromClassObject(
        invoking_object, &generic_class_types, err_stream);
    if (FAILED(hr)) {
      return hr;
    }

    // TODO(quoct): Refactor this code and the code below.
    // Was having some trouble with const'ness when refactoring
    // since this Evaluate function is const and it is not supposed
    // to change the vector generic_class_types_.
    eval_generic_types.reserve(generic_class_types.size());

    for (const auto &item : generic_class_types) {
      eval_generic_types.push_back(item);
    }
  }
  else {
    eval_generic_types.reserve(generic_class_types_.size());

    for (const auto &item : generic_class_types_) {
      eval_generic_types.push_back(item);
    }
  }

  hr = debug_eval_2->CallParameterizedFunction(
      matched_method_, eval_generic_types.size(), eval_generic_types.data(),
      arg_debug_values.size(), arg_debug_values.data());

  if (FAILED(hr)) {
    return hr;
  }

  CComPtr<ICorDebugValue> eval_result;
  BOOL exception_occurred = FALSE;
  hr = eval_coordinator->WaitForEval(
    &exception_occurred, debug_eval, &eval_result);

  if (FAILED(hr)) {
    return hr;
  }

  std::unique_ptr<DbgObject> eval_obj_result;
  hr = DbgObject::CreateDbgObject(eval_result, kDefaultObjectEvalDepth, &eval_obj_result,
    &std::cerr);
  if (FAILED(hr)) {
    return hr;
  }

  *dbg_object = std::move(eval_obj_result);
  return S_OK;
}

HRESULT MethodCallEvaluator::GetMethodFromClassNameHelper(
    const std::string &class_name,
    DbgStackFrame *stack_frame,
    std::ostream *err_stream) {
  HRESULT hr;
  CComPtr<ICorDebugModule> debug_module;
  CComPtr<IMetaDataImport> metadata_import;
  mdTypeDef class_token;

  hr = stack_frame->GetClassTokenAndModule(possible_class_name_,
    &class_token, &debug_module, &metadata_import);
  if (FAILED(hr)) {
    *err_stream << "Failed to retrieve class token from class "
      << possible_class_name_;
    return hr;
  }
    
  // Try to find method in the class with name possible_class_name_.
  hr = stack_frame->GetDebugFunctionFromClass(
    metadata_import, class_token, &method_info_, &matched_method_);
  if (FAILED(hr)) {
    *err_stream << "Failed to retrieve ICorDebugFunction "
      << method_info_.method_name << " from class " << possible_class_name_;
    return hr;
  }

  if (!method_info_.is_static) {
    *err_stream << "Method call from fully qualified class name has to be static.";
    return E_FAIL;
  }

  return S_OK;
}

HRESULT MethodCallEvaluator::GetInvokingObject(
    ICorDebugValue **invoking_object,
    IEvalCoordinator *eval_coordinator,
    std::ostream *err_stream) const {
  HRESULT hr = E_FAIL;

  if (instance_source_is_invoking_obj_) {
    std::shared_ptr<DbgObject> source_obj;
    hr = instance_source_->Evaluate(&source_obj, eval_coordinator, err_stream);
    if (FAILED(hr)) {
      *err_stream << "Failed to evaluate source object.";
      return hr;
    }

    // Only supports method call on anything that has a debug handle.
    // Also, don't support calling on null object.
    DbgReferenceObject *reference_obj = dynamic_cast<DbgReferenceObject *>(source_obj.get());
    if (reference_obj == nullptr) {
      *err_stream << "Currently, only method call from a reference object is supported.";
      return E_NOTIMPL;
    }

    CComPtr<ICorDebugHandleValue> source_object_handle;
    hr = reference_obj->GetDebugHandle(&source_object_handle);
    if (FAILED(hr)) {
      return hr;
    }

    *invoking_object = source_object_handle;
    source_object_handle->AddRef();
    return S_OK;
  }

  return debug_frame_->GetArgument(0, invoking_object);
}

HRESULT MethodCallEvaluator::PopulateGenericClassTypesFromClassObject(
    ICorDebugValue *class_object,
    std::vector<CComPtr<ICorDebugType>> *generic_types,
    std::ostream *err_stream) const {
  HRESULT hr;
  CComPtr<ICorDebugType> debug_type;
  hr = GetICorDebugType(class_object, &debug_type, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  CComPtr<ICorDebugTypeEnum> type_enum;
  hr = debug_type->EnumerateTypeParameters(&type_enum);
  if (FAILED(hr)) {
    return hr;
  }

  return DebuggerCallback::EnumerateICorDebugSpecifiedType<ICorDebugTypeEnum, ICorDebugType>(
    type_enum, generic_types);
}

}  // namespace google_cloud_debugger
