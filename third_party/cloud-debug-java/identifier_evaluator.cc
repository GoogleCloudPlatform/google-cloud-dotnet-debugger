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
#include "dbg_stack_frame.h"
#include "dbg_object.h"

namespace google_cloud_debugger {

IdentifierEvaluator::IdentifierEvaluator(std::string identifier_name)
    : identifier_name_(identifier_name) {
}

HRESULT IdentifierEvaluator::Compile(
    DbgStackFrame *stack_frame,
    std::ostream *err_stream) {
  // Case 1: this is a local variable.
  std::unique_ptr<DbgObject> identifier_obj;
  HRESULT hr = stack_frame->GetLocalVariable(identifier_name_,
    &identifier_obj, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  // S_FALSE means there is no match.
  if (SUCCEEDED(hr) && hr != S_FALSE) {
    identifier_object_ = std::move(identifier_obj);
    return identifier_object_->GetTypeSignature(&result_type_);
  }

  // Case 2: static and non-static fields and auto-implemented properties.
  hr = stack_frame->GetFieldAndAutoPropFromFrame(identifier_name_,
    &identifier_obj, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  // S_FALSE means there is no match.
  if (SUCCEEDED(hr) && hr != S_FALSE) {
    identifier_object_ = std::move(identifier_obj);
    return identifier_object_->GetTypeSignature(&result_type_);
  }

  // Case 3: static and non-static properties with getter.
  hr = stack_frame->GetPropertyFromFrame(identifier_name_,
    &class_property_, &result_type_, err_stream);
  if (hr == S_FALSE) {
    hr = E_FAIL;
  }

  if (FAILED(hr)) {
    return hr;
  }

  return S_OK;
}

HRESULT IdentifierEvaluator::Evaluate(
    std::shared_ptr<DbgObject> *dbg_object,
    IEvalCoordinator *eval_coordinator,
    std::ostream * err_stream) const {
  if (class_property_ != nullptr) {
    *dbg_object = identifier_object_;
    return S_OK;
  }

  // TODO(quoct): Evaluates the property.
}

}  // namespace google_cloud_debugger
