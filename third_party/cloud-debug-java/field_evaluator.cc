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

#include "field_evaluator.h"

#include "compiler_helpers.h"
#include "dbg_class_property.h"
#include "dbg_reference_object.h"
#include "dbg_stack_frame.h"
#include "error_messages.h"

namespace google_cloud_debugger {

// Maximum depth of inner classes we support to follow the implicit
// chain of references.
static constexpr int kMaxInnerClassesDepth = 10;

FieldEvaluator::FieldEvaluator(
    std::unique_ptr<ExpressionEvaluator> instance_source,
    std::string identifier_name, std::string possible_class_name,
    std::string field_name)
    : instance_source_(std::move(instance_source)),
      identifier_name_(std::move(identifier_name)),
      possible_class_name_(std::move(possible_class_name)),
      field_name_(std::move(field_name)) {}

HRESULT FieldEvaluator::Compile(DbgStackFrame *stack_frame,
                                std::ostream *err_stream) {
  HRESULT hr = stack_frame->GetFrame(&debug_frame_);
  if (FAILED(hr)) {
    return hr;
  }

  hr = CompileUsingInstanceSource(stack_frame, err_stream);
  if (SUCCEEDED(hr)) {
    return hr;
  }

  hr = CompileUsingClassName(stack_frame, err_stream);
  if (SUCCEEDED(hr)) {
    return hr;
  }

  return E_FAIL;
}

HRESULT FieldEvaluator::CompileUsingInstanceSource(DbgStackFrame *stack_frame,
                                             std::ostream *err_stream) {
  HRESULT hr = instance_source_->Compile(stack_frame, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  const TypeSignature &instance_source_signature =
      instance_source_->GetStaticType();

  if (instance_source_signature.type_name.empty()) {
    *err_stream << kTypeNameNotAvailable;
    return false;
  }

  return CompileClassMemberHelper(instance_source_signature.type_name,
                                  field_name_, stack_frame, err_stream);
}

HRESULT FieldEvaluator::CompileUsingClassName(DbgStackFrame *stack_frame,
                                           std::ostream *err_stream) {
  if (possible_class_name_.empty()) {
    return E_FAIL;
  }

  return CompileClassMemberHelper(possible_class_name_, field_name_,
                                  stack_frame, err_stream);
}

HRESULT FieldEvaluator::CompileClassMemberHelper(const std::string &class_name,
                                                 const std::string &member_name,
                                                 DbgStackFrame *stack_frame,
                                                 std::ostream *err_stream) {
  // To find member of a class, we need the corresponding
  // ICorDebugModule and IMetaDataImport of the module that class
  // is in. We will also need the metadata token associated with
  // that class.
  HRESULT hr = stack_frame->GetClassTokenAndModule(
      class_name, &class_token_, &debug_module_, &metadata_import_);
  if (FAILED(hr) || hr == S_FALSE) {
    return E_FAIL;
  }

  hr = stack_frame->GetFieldFromClass(
      class_token_, member_name, &field_def_, &is_static_,
      &result_type_, metadata_import_, err_stream);
  if (SUCCEEDED(hr)) {
    return hr;
  }

  hr = stack_frame->GetPropertyFromClass(
      class_token_, member_name, &class_property_, metadata_import_, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  is_static_ = class_property_->IsStatic();

  return class_property_->GetTypeSignature(&result_type_);
}

HRESULT FieldEvaluator::Evaluate(std::shared_ptr<DbgObject> *dbg_object,
                                 IEvalCoordinator *eval_coordinator,
                                 std::ostream *err_stream) const {
  HRESULT hr;
  CComPtr<ICorDebugValue> debug_field_value;
  std::unique_ptr<DbgObject> field_value_obj;
  if (is_static_) {
    // Class_property_ is null if this is a field or auto-impl property.
    // Hence, we can get them directly without function evaluation.
    if (class_property_ == nullptr) {
      CComPtr<ICorDebugClass> debug_class;
      hr = debug_module_->GetClassFromToken(class_token_, &debug_class);
      if (FAILED(hr)) {
        return hr;
      }

      hr = debug_class->GetStaticFieldValue(field_def_, debug_frame_, &debug_field_value);
      if (FAILED(hr)) {
        return hr;
      }

      hr = DbgObject::CreateDbgObject(debug_field_value, kDefaultObjectEvalDepth,
          &field_value_obj, err_stream);
      if (FAILED(hr)) {
        return hr;
      }

      *dbg_object = std::move(field_value_obj);
      return S_OK;
    }

    // TODO(quoct): OTHERWISE, HANDLE FUNCTION EVALUATION FOR STATIC PROPERTY.
  }

  // Moving on to non-static field/property so we need to get the source.
  std::shared_ptr<DbgObject> source_obj;
  hr = instance_source_->Evaluate(&source_obj, eval_coordinator, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  if (source_obj->GetIsNull()) {
    return E_FAIL;
  }

  // We can directly get the field/non-auto property without function evaluation.
  if (class_property_ == nullptr) {
    DbgReferenceObject *reference_object = dynamic_cast<DbgReferenceObject *>(source_obj.get());
    if (!reference_object) {
      return E_FAIL;
    }

    return reference_object->GetNonStaticField(field_name_, dbg_object);
  }

  // TODO(quoct): OTHERWISE, HANDLE FUNCTION EVALUATION FOR STATIC PROPERTY.
  return E_NOTIMPL;
}

}  // namespace google_cloud_debugger
