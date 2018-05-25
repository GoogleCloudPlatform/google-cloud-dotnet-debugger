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

#include <sstream>
#include <string>

#include "cordebug.h"
#include "dbg_object_factory.h"
#include "dbg_reference_object.h"
#include "error_messages.h"
#include "i_dbg_stack_frame.h"
#include "debugger_callback.h"
#include "i_cor_debug_helper.h"
#include "i_eval_coordinator.h"
#include "method_info.h"

using std::string;

namespace google_cloud_debugger {

MethodCallEvaluator::MethodCallEvaluator(
    string method_name, std::unique_ptr<ExpressionEvaluator> instance_source,
    string possible_class_name, std::shared_ptr<ICorDebugHelper> debug_helper,
    std::vector<std::unique_ptr<ExpressionEvaluator>> arguments)
    : method_name_(std::move(method_name)),
      instance_source_(std::move(instance_source)),
      possible_class_name_(std::move(possible_class_name)),
      arguments_(std::move(arguments)),
      debug_helper_(debug_helper) {}

HRESULT MethodCallEvaluator::Compile(IDbgStackFrame *stack_frame,
                                     ICorDebugILFrame *debug_frame,
                                     std::ostream *err_stream) {
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
  for (auto &argument : arguments_) {
    hr = argument->Compile(stack_frame, debug_frame, err_stream);
    if (FAILED(hr)) {
      return hr;
    }

    method_info_.argument_types.push_back(argument->GetStaticType());
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
      hr = stack_frame->GetCurrentClassTypeParameters(&current_class_generic_types_);
      if (FAILED(hr)) {
        std::cerr << "Failed to retrieve generic type parameters for class.";
        return hr;
      }

      if (stack_frame->IsStaticMethod() && !method_info_.is_static) {
        return E_FAIL;
      }
    }
  }

  if (!matched_method_ && instance_source_ != nullptr) {
    // Calling method on a result of prior expression (for example:
    // "a.b.startsWith(...)").
    hr = instance_source_->Compile(stack_frame, debug_frame, err_stream);
    if (FAILED(hr)) {
      return hr;
    }

    const TypeSignature &source_class_sig = instance_source_->GetStaticType();
    hr = GetDebugFunctionFromClassNameHelper(source_class_sig, stack_frame,
                                             &method_info_, &matched_method_);
    if (FAILED(hr)) {
      std::cerr << "Failed to retrieve ICorDebugFunction "
                  << method_info_.method_name << " from the current class.";
      return hr;
    }

    if (matched_method_) {
      instance_source_is_invoking_obj_ = true;
    }
  }

  if (!matched_method_ && !possible_class_name_.empty()) {
    TypeSignature class_sig;
    class_sig.type_name = possible_class_name_;
    hr = GetDebugFunctionFromClassNameHelper(class_sig, stack_frame,
                                             &method_info_, &matched_method_);
    if (FAILED(hr)) {
      return hr;
    }
  }

  if (!matched_method_) {
    return E_FAIL;
  }

  // TODO(quoct): Generic methods are not supported yet.
  if (method_info_.has_generic_types) {
    return E_NOTIMPL;
  }

  return S_OK;
}

HRESULT MethodCallEvaluator::Evaluate(std::shared_ptr<DbgObject> *dbg_object,
                                      IEvalCoordinator *eval_coordinator,
                                      IDbgObjectFactory *obj_factory,
                                      std::ostream *err_stream) const {
  if (!eval_coordinator->MethodEvaluation()) {
    *err_stream << kConditionEvalNeeded;
    return E_FAIL;
  }

  HRESULT hr;
  CComPtr<ICorDebugValue> invoking_object;
  // We need the invoking object in case this is a non-static call.
  // In addition, if this expression is A.B where A is the instance_source_
  // (so instance_source_is_invoking_obj_ is true), we would also need
  // to retrieve A to get the instantiated generic class types of A.
  if (instance_source_is_invoking_obj_ || !method_info_.is_static) {
    hr = GetInvokingObject(&invoking_object, eval_coordinator, obj_factory,
                           err_stream);
    if (FAILED(hr)) {
      *err_stream << "Failed to evaluate invoking object for method call "
                  << method_name_;
      return hr;
    }
  }

  std::vector<ICorDebugValue *> arg_debug_values;
  if (!method_info_.is_static) {
    arg_debug_values.push_back(invoking_object);
  }

  // Create ICorDebugEval for evaluation.
  CComPtr<ICorDebugEval> debug_eval;
  hr = eval_coordinator->CreateEval(&debug_eval);
  if (FAILED(hr)) {
    std::cerr << "Failed to create ICorDebugEval.";
    return hr;
  }

  hr = EvaluateArgumentsHelper(&arg_debug_values, debug_eval, eval_coordinator,
                               obj_factory, err_stream);
  if (FAILED(hr)) {
    *err_stream << "Failed to evaluate method arguments.";
    return hr;
  }

  std::vector<ICorDebugType *> eval_generic_types;

  // If instance source is the invoking object, we can
  // also retrieve the class generic types here as it is now
  // instantiated.
  if (instance_source_is_invoking_obj_) {
    std::vector<CComPtr<ICorDebugType>> generic_class_types;
    hr = debug_helper_->PopulateGenericClassTypesFromClassObject(
        invoking_object, &generic_class_types, &std::cerr);
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
  } else {
    eval_generic_types.reserve(current_class_generic_types_.size());

    for (const auto &item : current_class_generic_types_) {
      eval_generic_types.push_back(item);
    }
  }

  std::unique_ptr<DbgObject> eval_obj_result;
  hr = obj_factory->EvaluateAndCreateDbgObject(
    std::move(eval_generic_types), std::move(arg_debug_values),
    matched_method_, debug_eval, eval_coordinator,
    &eval_obj_result, &std::cerr);
  if (FAILED(hr)) {
    return hr;
  }

  *dbg_object = std::move(eval_obj_result);
  return S_OK;
}

HRESULT MethodCallEvaluator::EvaluateArgumentsHelper(
    std::vector<ICorDebugValue *> *arg_debug_values, ICorDebugEval *debug_eval,
    IEvalCoordinator *eval_coordinator, IDbgObjectFactory *obj_factory,
    std::ostream *err_stream) const {
  HRESULT hr;
  for (auto &argument : arguments_) {
    std::shared_ptr<DbgObject> arg_obj;
    hr =
        argument->Evaluate(&arg_obj, eval_coordinator, obj_factory, err_stream);
    if (FAILED(hr)) {
      return hr;
    }

    CComPtr<ICorDebugValue> arg_debug_value;
    hr = arg_obj->GetICorDebugValue(&arg_debug_value, debug_eval);
    if (FAILED(hr)) {
      return hr;
    }

    // Add reference as debug_value will be deleted out of this scope.
    arg_debug_value->AddRef();
    arg_debug_values->push_back(arg_debug_value);
  }

  return S_OK;
}

HRESULT MethodCallEvaluator::GetDebugFunctionFromClassNameHelper(
    const TypeSignature &class_signature, IDbgStackFrame *stack_frame,
    MethodInfo *method_info, ICorDebugFunction **result_method) {
  HRESULT hr;
  CComPtr<ICorDebugModule> debug_module;
  CComPtr<IMetaDataImport> metadata_import;
  mdTypeDef class_token;

  hr = stack_frame->GetClassTokenAndModule(
    class_signature.type_name, &class_token,
    &debug_module, &metadata_import);
  if (FAILED(hr)) {
    std::cerr << "Failed to retrieve class token from class "
              << class_signature.type_name;
    return hr;
  }

  // Try to find method in the class with name possible_class_name_.
  // Also populates method_info fields.
  hr = stack_frame->GetDebugFunctionFromClass(
      metadata_import, debug_module, class_token, method_info,
      class_signature.generic_types, result_method);
  if (hr == S_FALSE) {
    hr = E_FAIL;
  }

  if (FAILED(hr)) {
    std::cerr << "Failed to retrieve ICorDebugFunction "
              << method_info->method_name << " from class "
              << class_signature.type_name;
    return hr;
  }

  return S_OK;
}

HRESULT MethodCallEvaluator::GetInvokingObject(
    ICorDebugValue **invoking_object, IEvalCoordinator *eval_coordinator,
    IDbgObjectFactory *obj_factory, std::ostream *err_stream) const {
  HRESULT hr = E_FAIL;

  if (instance_source_is_invoking_obj_) {
    std::shared_ptr<DbgObject> source_obj;
    hr = instance_source_->Evaluate(&source_obj, eval_coordinator, obj_factory,
                                    err_stream);
    if (FAILED(hr)) {
      *err_stream << "Failed to evaluate source object.";
      return hr;
    }

    // Only supports method call on anything that has a debug handle.
    // Also, don't support calling on null object.
    DbgReferenceObject *reference_obj =
        dynamic_cast<DbgReferenceObject *>(source_obj.get());
    if (reference_obj == nullptr) {
      *err_stream << "Currently, only method call from a reference object is "
                     "supported.";
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

  CComPtr<ICorDebugILFrame> debug_frame;
  hr = eval_coordinator->GetActiveDebugFrame(&debug_frame);
  if (FAILED(hr)) {
    std::cerr << "Failed to get the active debug frame.";
    return hr;
  }

  // Returns this object.
  return debug_frame->GetArgument(0, invoking_object);
}

}  // namespace google_cloud_debugger
