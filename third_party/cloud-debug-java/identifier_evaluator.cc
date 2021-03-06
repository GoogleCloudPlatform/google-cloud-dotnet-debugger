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

#include "identifier_evaluator.h"
#include "i_dbg_stack_frame.h"
#include "i_eval_coordinator.h"
#include "dbg_object.h"
#include "dbg_class_property.h"
#include "error_messages.h"

namespace google_cloud_debugger {

IdentifierEvaluator::IdentifierEvaluator(std::string identifier_name)
    : identifier_name_(identifier_name) {
}

HRESULT IdentifierEvaluator::Compile(
    IDbgStackFrame *stack_frame,
    ICorDebugILFrame *debug_frame,
    std::ostream *err_stream) {
  // Only needs to check null for stack frame because we don't
  // invoke the other arguments.
  if (!stack_frame) {
    return E_INVALIDARG;
  }

  // Case 1: this is a local variable.
  HRESULT hr = stack_frame->GetLocalVariable(identifier_name_,
    &identifier_object_, &std::cerr);
  if (FAILED(hr)) {
    return hr;
  }

  // S_FALSE means there is no match.
  if (SUCCEEDED(hr) && hr != S_FALSE) {
    return identifier_object_->GetTypeSignature(&result_type_);
  }

  // Case 2: static and non-static fields and auto-implemented properties.
  hr = stack_frame->GetFieldAndAutoPropFromFrame(identifier_name_,
    &identifier_object_, debug_frame, &std::cerr);
  if (FAILED(hr)) {
    return hr;
  }

  // S_FALSE means there is no match.
  if (SUCCEEDED(hr) && hr != S_FALSE) {
    return identifier_object_->GetTypeSignature(&result_type_);
  }

  // Case 3: static and non-static properties with getter.
  hr = stack_frame->GetPropertyFromFrame(identifier_name_,
    &class_property_, &std::cerr);
  if (hr == S_FALSE) {
    hr = E_FAIL;
  }

  if (FAILED(hr)) {
    return hr;
  }

  hr = class_property_->GetTypeSignature(&result_type_);
  if (FAILED(hr)) {
    return hr;
  }

  if (!class_property_->IsStatic() && stack_frame->IsStaticMethod()) {
    *err_stream << "Cannot get non-static property "
                << identifier_name_ << " in a static frame.";
    return E_FAIL;
  }

  this_object_ = stack_frame->GetThisObject();

  // Generic type parameters for the class that the method is in.
  return stack_frame->GetCurrentClassTypeParameters(&generic_class_types_);
}

HRESULT IdentifierEvaluator::Evaluate(
    std::shared_ptr<DbgObject> *dbg_object,
    IEvalCoordinator *eval_coordinator,
    IDbgObjectFactory *obj_factory,
    std::ostream * err_stream) const {
  if (class_property_ == nullptr) {
    *dbg_object = identifier_object_;
    return S_OK;
  }

  if (eval_coordinator == nullptr) {
    return E_INVALIDARG;
  }

  CComPtr<ICorDebugValue> invoking_object;
  HRESULT hr;
  // If this is a non-static property, we have to get the invoking object.
  if (!class_property_->IsStatic()) {
    if (!this_object_) {
      *err_stream << "Failed to get 'this' object.";
      return E_FAIL;
    }

    CComPtr<ICorDebugEval> debug_eval;
    hr = eval_coordinator->CreateEval(&debug_eval);
    if (FAILED(hr)) {
      std::cerr << "Failed to create ICorDebugEval.";
      return hr;
    }

    hr = this_object_->GetICorDebugValue(&invoking_object, debug_eval);
    if (FAILED(hr)) {
      *err_stream << "Failed to get 'this' object.";
      return hr;
    }
  }

  if (!eval_coordinator->MethodEvaluation()) {
    *err_stream << kConditionEvalNeeded;
    return E_FAIL;
  }

  hr = class_property_->Evaluate(invoking_object, eval_coordinator,
    const_cast<std::vector<CComPtr<ICorDebugType>> *>(&generic_class_types_));
  if (FAILED(hr)) {
    *err_stream << "Failed to evaluate property "
                << class_property_->GetMemberName();
    return hr;
  }

  *dbg_object = class_property_->GetMemberValue();
  return S_OK;
}

}  // namespace google_cloud_debugger
