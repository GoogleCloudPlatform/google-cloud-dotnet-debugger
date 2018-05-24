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
#include "dbg_array.h"
#include "dbg_class.h"
#include "dbg_class_property.h"
#include "dbg_object_factory.h"
#include "dbg_reference_object.h"
#include "error_messages.h"
#include "i_cor_debug_helper.h"
#include "i_dbg_stack_frame.h"
#include "i_eval_coordinator.h"

namespace google_cloud_debugger {

// Maximum depth of inner classes we support to follow the implicit
// chain of references.
static constexpr int kMaxInnerClassesDepth = 10;

FieldEvaluator::FieldEvaluator(
    std::unique_ptr<ExpressionEvaluator> instance_source,
    std::string identifier_name, std::string possible_class_name,
    std::string field_name, std::shared_ptr<ICorDebugHelper> debug_helper)
    : instance_source_(std::move(instance_source)),
      identifier_name_(std::move(identifier_name)),
      possible_class_name_(std::move(possible_class_name)),
      field_name_(std::move(field_name)),
      debug_helper_(debug_helper) {}

HRESULT FieldEvaluator::Compile(IDbgStackFrame *stack_frame,
                                ICorDebugILFrame *debug_frame,
                                std::ostream *err_stream) {
  HRESULT hr = CompileUsingInstanceSource(stack_frame, debug_frame, err_stream);
  if (SUCCEEDED(hr)) {
    compiled_using_instance_source_ = true;
    return hr;
  }

  hr = CompileUsingClassName(stack_frame, debug_frame, err_stream);
  if (SUCCEEDED(hr)) {
    return hr;
  }

  return E_FAIL;
}

HRESULT FieldEvaluator::CompileUsingInstanceSource(
    IDbgStackFrame *stack_frame, ICorDebugILFrame *debug_frame,
    std::ostream *err_stream) {
  HRESULT hr = instance_source_->Compile(stack_frame, debug_frame, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  const TypeSignature &instance_source_signature =
      instance_source_->GetStaticType();

  if (instance_source_signature.type_name.empty()) {
    std::cerr << kTypeNameNotAvailable;
    return false;
  }

  return CompileClassMemberHelper(instance_source_signature, field_name_,
                                  stack_frame, debug_frame, err_stream);
}

HRESULT FieldEvaluator::CompileUsingClassName(IDbgStackFrame *stack_frame,
                                              ICorDebugILFrame *debug_frame,
                                              std::ostream *err_stream) {
  if (possible_class_name_.empty()) {
    return E_FAIL;
  }

  return CompileClassMemberHelper(
      TypeSignature{CorElementType::ELEMENT_TYPE_CLASS, possible_class_name_},
      field_name_, stack_frame, debug_frame, err_stream);
}

HRESULT FieldEvaluator::CompileClassMemberHelper(
    const TypeSignature &class_signature, const std::string &member_name,
    IDbgStackFrame *stack_frame, ICorDebugILFrame *debug_frame,
    std::ostream *err_stream) {
  // If class_signature is an array, we can only support "Length" property.
  if (class_signature.is_array) {
    if (member_name.compare("Length") != 0) {
      *err_stream << "Only Length property is supported for array.";
      return E_NOTIMPL;
    }

    if (!instance_source_) {
      *err_stream << "Cannot get length of null array.";
      return E_FAIL;
    }

    result_type_ = {CorElementType::ELEMENT_TYPE_I4, kInt32ClassName};
    is_static_ = false;
    is_array_length_ = true;
    return S_OK;
  }

  // To find member of a class, we need the corresponding
  // ICorDebugModule and IMetaDataImport of the module that class
  // is in. We will also need the metadata token associated with
  // that class.
  HRESULT hr = stack_frame->GetClassTokenAndModule(
      class_signature.type_name, &class_token_, &debug_module_,
      &metadata_import_);
  if (FAILED(hr)) {
    return hr;
  }

  // If we cannot find the class, there is nothing we can do.
  if (hr == S_FALSE) {
    return E_FAIL;
  }

  hr = stack_frame->GetFieldFromClass(
      class_token_, member_name, &field_def_, &is_static_, &result_type_,
      class_signature.generic_types, metadata_import_, &std::cerr);
  if (SUCCEEDED(hr) && hr != S_FALSE) {
    return hr;
  }

  hr = stack_frame->GetPropertyFromClass(
      class_token_, member_name, &class_property_,
      class_signature.generic_types, metadata_import_, &std::cerr);
  if (FAILED(hr)) {
    return hr;
  }

  if (hr == S_FALSE) {
    return E_FAIL;
  }

  is_static_ = class_property_->IsStatic();
  return class_property_->GetTypeSignature(&result_type_);
}

HRESULT FieldEvaluator::EvaluateStaticMember(
    std::shared_ptr<DbgObject> *result_object,
    IEvalCoordinator *eval_coordinator, IDbgObjectFactory *obj_factory,
    std::vector<CComPtr<ICorDebugType>> *generic_class_types,
    std::ostream *err_stream) const {
  HRESULT hr;
  CComPtr<ICorDebugValue> debug_field_value;
  std::unique_ptr<DbgObject> field_value_obj;

  // Class_property_ is null if this is a field or auto-impl property.
  // Hence, we can get them directly without function evaluation.
  if (class_property_ == nullptr) {
    if (eval_coordinator == nullptr || obj_factory == nullptr) {
      return E_INVALIDARG;
    }

    CComPtr<ICorDebugClass> debug_class;
    hr = debug_module_->GetClassFromToken(class_token_, &debug_class);
    if (FAILED(hr)) {
      return hr;
    }

    CComPtr<ICorDebugType> class_type;
    hr = debug_helper_->GetInstantiatedClassType(
        debug_class, generic_class_types, &class_type, &std::cerr);
    if (FAILED(hr)) {
      std::cerr << "Failed to get instantiated type from ICorDebugClass.";
      return hr;
    }

    CComPtr<ICorDebugILFrame> debug_frame;
    hr = eval_coordinator->GetActiveDebugFrame(&debug_frame);
    if (FAILED(hr)) {
      return hr;
    }

    hr = class_type->GetStaticFieldValue(field_def_, debug_frame,
                                         &debug_field_value);
    if (FAILED(hr)) {
      return hr;
    }

    hr =
        obj_factory->CreateDbgObject(debug_field_value, kDefaultObjectEvalDepth,
                                     &field_value_obj, &std::cerr);
    if (FAILED(hr)) {
      return hr;
    }

    *result_object = std::move(field_value_obj);
    return S_OK;
  }

  hr =
      class_property_->Evaluate(nullptr, eval_coordinator, generic_class_types);
  if (FAILED(hr)) {
    *err_stream << "Failed to evaluate property "
                << class_property_->GetMemberName();
    return hr;
  }

  *result_object = class_property_->GetMemberValue();
  return S_OK;
}

HRESULT FieldEvaluator::EvaluateNonStaticMember(
    std::shared_ptr<DbgObject> *result_object,
    IEvalCoordinator *eval_coordinator, IDbgObjectFactory *obj_factory,
    std::shared_ptr<DbgObject> source_obj,
    std::vector<CComPtr<ICorDebugType>> *generic_class_types,
    std::ostream *err_stream) const {
  if (source_obj->GetIsNull()) {
    return E_FAIL;
  }

  if (is_array_length_) {
    DbgArray *dbg_array = dynamic_cast<DbgArray *>(source_obj.get());
    if (!dbg_array) {
      return E_FAIL;
    }

    int result = dbg_array->GetArraySize();
    *result_object =
        std::shared_ptr<DbgObject>(new DbgPrimitive<int32_t>(result));
    return S_OK;
  }

  DbgReferenceObject *reference_object =
      dynamic_cast<DbgReferenceObject *>(source_obj.get());
  if (!reference_object) {
    return E_FAIL;
  }

  // We can directly get the field/non-auto property without function
  // evaluation.
  if (class_property_ == nullptr) {
    return reference_object->GetNonStaticField(field_name_, result_object);
  }

  CComPtr<ICorDebugHandleValue> source_object_handle;
  HRESULT hr = reference_object->GetDebugHandle(&source_object_handle);
  if (FAILED(hr)) {
    return hr;
  }

  hr = class_property_->Evaluate(
      source_object_handle, eval_coordinator,
      const_cast<std::vector<CComPtr<ICorDebugType>> *>(generic_class_types));
  if (FAILED(hr)) {
    *err_stream << "Failed to evaluate property "
                << class_property_->GetMemberName();
    return hr;
  }

  *result_object = class_property_->GetMemberValue();
  return S_OK;
}

HRESULT FieldEvaluator::Evaluate(std::shared_ptr<DbgObject> *dbg_object,
                                 IEvalCoordinator *eval_coordinator,
                                 IDbgObjectFactory *obj_factory,
                                 std::ostream *err_stream) const {
  HRESULT hr;
  CComPtr<ICorDebugValue> debug_field_value;
  std::unique_ptr<DbgObject> field_value_obj;
  std::shared_ptr<DbgObject> source_obj;

  std::vector<CComPtr<ICorDebugType>> generic_class_types;

  // If we compile with the source, we may have to evaluate the source
  // to get generic types.
  if (compiled_using_instance_source_ && !is_array_length_) {
    const TypeSignature &source_signature = instance_source_->GetStaticType();
    if (source_signature.generic_types.size() != 0) {
      hr = instance_source_->Evaluate(&source_obj, eval_coordinator,
                                      obj_factory, err_stream);
      if (FAILED(hr)) {
        return hr;
      }

      DbgClass *class_obj = dynamic_cast<DbgClass *>(source_obj.get());
      if (!class_obj) {
        return E_FAIL;
      }

      hr = class_obj->GetGenericTypes(&generic_class_types);
      if (FAILED(hr)) {
        std::cerr << "Failed to get generic types of class.";
      }
    }
  }

  if (is_static_) {
    return EvaluateStaticMember(dbg_object, eval_coordinator, obj_factory,
                                &generic_class_types, err_stream);
  }

  // Moving on to non-static field/property so we need to get the source.
  if (!source_obj) {
    hr = instance_source_->Evaluate(&source_obj, eval_coordinator, obj_factory,
                                    err_stream);
    if (FAILED(hr)) {
      return hr;
    }
  }

  return EvaluateNonStaticMember(dbg_object, eval_coordinator, obj_factory,
                                 source_obj, &generic_class_types, err_stream);
}

}  // namespace google_cloud_debugger
