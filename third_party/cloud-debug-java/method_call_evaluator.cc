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
#include "dbg_stack_frame.h"
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
  MethodInfo method_info;
  method_info.method_name = possible_class_name_;
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

    method_info.argument_types.push_back(argument->GetStaticType().type_name);
  }

  if ((instance_source_ == nullptr) && possible_class_name_.empty()) {
    // No source and no class name so this has to be interpreted
    // as a method in the current class.
    hr = stack_frame->GetDebugFunctionFromCurrentClass(&method_info,
      &matched_method_);
    if (FAILED(hr)) {
      return hr;
    }
  }

  if (!matched_method_ && instance_source_ != nullptr) {
    // Calling method on a result of prior expression (for example:
    // "a.b.startsWith(...)").
    hr = instance_source_->Compile(stack_frame, err_stream);
    if (FAILED(hr)) {
      return hr;
    }

    method_info.method_name = instance_source_->GetStaticType().type_name;
    hr = stack_frame->GetDebugFunctionFromCurrentClass(&method_info,
      &matched_method_);
    if (FAILED(hr)) {
      *err_stream << "Failed to retrieve ICorDebugFunction "
        << method_info.method_name << " from the current class.";
      return hr;
    }
  }

  if (!matched_method_ && !possible_class_name_.empty()) {
    // Try to find method in the class with name possible_class_name_.
    method_info.method_name = possible_class_name_;
    hr = stack_frame->GetMethodFromStaticClass(
      possible_class_name_, &method_info, &matched_method_);
    if (FAILED(hr)) {
      *err_stream << "Failed to retrieve ICorDebugFunction "
        << method_info.method_name << " from class " << possible_class_name_;
      return hr;
    }

    if (!method_info.is_static) {
      *err_stream << "Method call from fully qualified class name has to be static.";
      return E_FAIL;
    }
  }

  if (!matched_method_) {
    return E_FAIL;
  }

  // TODO(quoct): Generic methods are not supported yet.
  if (method_info.has_generic_types) {
    return E_NOTIMPL;
  }

  if (stack_frame->IsStaticMethod() && !method_info.is_static) {
    return E_FAIL;
  }

  return S_OK;
}

HRESULT MethodCallEvaluator::Evaluate(
      std::shared_ptr<DbgObject> *dbg_object,
      IEvalCoordinator *eval_coordinator, std::ostream *err_stream) const {
  //TODO(quoct): Implement this.
}

}  // namespace google_cloud_debugger
