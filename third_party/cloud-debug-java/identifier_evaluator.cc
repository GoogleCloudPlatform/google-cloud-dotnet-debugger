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
    &identifier_object_, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  // S_FALSE means there is no match.
  if (SUCCEEDED(hr) && hr != S_FALSE) {
    return identifier_object_->GetTypeSignature(&result_type_);
  }

  // Case 2: static and non-static fields and auto-implemented properties.
  hr = stack_frame->GetFieldAndAutoPropFromFrame(identifier_name_,
    &identifier_object_, debug_frame, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  // S_FALSE means there is no match.
  if (SUCCEEDED(hr) && hr != S_FALSE) {
    return identifier_object_->GetTypeSignature(&result_type_);
  }

  // Case 3: static and non-static properties with getter.
  hr = stack_frame->GetPropertyFromFrame(identifier_name_,
    &class_property_, err_stream);
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

  // Generic type parameters for the class that the method is in.
  return stack_frame->GetClassGenericTypeParameters(debug_frame,
                                                    &generic_class_types_);
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

  CComPtr<ICorDebugValue> invoking_object;

  HRESULT hr;
  // If this is a non-static property, we have to get the invoking object.
  if (!class_property_->IsStatic()) {
    CComPtr<ICorDebugILFrame> debug_frame;
    hr = eval_coordinator->GetActiveDebugFrame(&debug_frame);
    if (FAILED(hr)) {
      *err_stream << "Failed to get the active debug frame.";
      return hr;
    }

    // Returns this object.
    hr = debug_frame->GetArgument(0, &invoking_object);
    if (FAILED(hr)) {
      *err_stream << "Failed to get the invoking object.";
      return hr;
    }
  }

  hr = class_property_->Evaluate(invoking_object, eval_coordinator,
    const_cast<std::vector<CComPtr<ICorDebugType>> *>(&generic_class_types_));
  if (FAILED(hr)) {
    *err_stream << "Failed to evaluate property.";
    return hr;
  }

  *dbg_object = class_property_->GetMemberValue();
  return S_OK;
}

}  // namespace google_cloud_debugger
