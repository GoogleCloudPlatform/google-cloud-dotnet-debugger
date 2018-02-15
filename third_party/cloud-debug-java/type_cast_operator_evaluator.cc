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

#include "type_cast_operator_evaluator.h"

#include "common.h"
#include "compiler_helpers.h"

namespace google_cloud_debugger {

TypeCastOperatorEvaluator::TypeCastOperatorEvaluator(
    std::unique_ptr<ExpressionEvaluator> source,
    const std::string& target_type)
    : source_(std::move(source)),
      target_class_(nullptr),
      computer_(nullptr) {
  result_type_.type = TypeSignature::Object;
  CorElementType target_cor_type =
    TypeCompilerHelper::ConvertStringToCorElementType(target_type);
  target_type_ = TypeSignature{
    target_cor_type,
    target_type
  };
}

HRESULT TypeCastOperatorEvaluator::Compile(
    DbgStackFrame *stack_frame,
    std::ostream *err_stream) {
  HRESULT hr = source_->Compile(stack_frame, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  result_type_ = source_->GetStaticType();

  // Checks if only one of the source and target is boolean.
  if (IsInvalidPrimitiveBooleanTypeConversion(result_type_.cor_type_,
    target_type_.cor_type)) {
    *err_stream << "Invalid type cast.";
    return E_FAIL;
  }

  if (result_type_.cor_type_ == CorElementType::ELEMENT_TYPE_BOOLEAN
    && target_type_.cor_type == CorElementType::ELEMENT_TYPE_BOOLEAN) {
    computer_ = &TypeCastOperatorEvaluator::DoNothingComputer;
    return S_OK;
  }

  // TODO(quoct): Test unbox scenario.
  if (TypeCompilerHelper::IsNumericalType(result_type_.type) &&
      TypeCompilerHelper::IsNumericalType(target_type_.cor_type)) {
    // In this case, we cast to target_type_.
    result_type_ = target_type_;
    switch (result_type_.type) {
      case CorElementType::ELEMENT_TYPE_CHAR: {
        computer_ = &TypeCastOperatorEvaluator::NumericalCastComputer<char>;
        return S_OK;
      }
      case CorElementType::ELEMENT_TYPE_U1: {
        computer_ = &TypeCastOperatorEvaluator::NumericalCastComputer<uint8_t>;
        return S_OK;
      }
      case CorElementType::ELEMENT_TYPE_I1: {
        computer_ = &TypeCastOperatorEvaluator::NumericalCastComputer<int8_t>;
        return S_OK;
      }
      case CorElementType::ELEMENT_TYPE_U2: {
        computer_ = &TypeCastOperatorEvaluator::NumericalCastComputer<uint16_t>;
        return S_OK;
      }
      case CorElementType::ELEMENT_TYPE_I2: {
        computer_ = &TypeCastOperatorEvaluator::NumericalCastComputer<int16_t>;
        return S_OK;
      }
      case CorElementType::ELEMENT_TYPE_U4: {
        computer_ = &TypeCastOperatorEvaluator::NumericalCastComputer<uint32_t>;
        return S_OK;
      }
      case CorElementType::ELEMENT_TYPE_I4: {
        computer_ = &TypeCastOperatorEvaluator::NumericalCastComputer<int32_t>;
        return S_OK;
      }
      case CorElementType::ELEMENT_TYPE_U8: {
        computer_ = &TypeCastOperatorEvaluator::NumericalCastComputer<uint64_t>;
        return S_OK;
      }
      case CorElementType::ELEMENT_TYPE_I8: {
        computer_ = &TypeCastOperatorEvaluator::NumericalCastComputer<int64_t>;
        return S_OK;
      }
      case CorElementType::ELEMENT_TYPE_R4: {
        computer_ = &TypeCastOperatorEvaluator::NumericalCastComputer<float_t>;
        return S_OK;
      }
      case CorElementType::ELEMENT_TYPE_R8: {
        computer_ = &TypeCastOperatorEvaluator::NumericalCastComputer<double_t>;
        return S_OK;
      }
      default:
        *err_stream << "Unknown Numeric type.";
        return E_NOTIMPL;
    }
  }

  if (!IsNumericTypeName(target_type_) &&
      (source_->GetStaticType().type == JType::Object)) {
    // Both source and target are Objects.
    computer_ = &TypeCastOperatorEvaluator::ObjectTypeComputer;

    JniLocalRef target_class_local_ref =
        readers_factory->FindClassByName(target_type_, error_message);
    if (target_class_local_ref == nullptr) {
      return false;
    }

    target_class_ = jni()->NewGlobalRef(target_class_local_ref.get());

    JvmtiBuffer<char> signature;
    if (jvmti()->GetClassSignature(
          static_cast<jclass>(target_class_),
          signature.ref(),
          nullptr) != JVMTI_ERROR_NONE) {
      *error_message = INTERNAL_ERROR_MESSAGE;
      return false;
    }

    result_type_.object_signature = signature.get();

    if (IsEitherTypeObjectArray()) {
      *error_message = {
          TypeCastUnsupported,
          { TypeNameFromSignature(source_->GetStaticType()), target_type_ }
      };
      return false;
    }

    return true;
  }

  *error_message = {
      TypeCastUnsupported,
      { TypeNameFromSignature(source_->GetStaticType()), target_type_ }
  };
  return false;
}


ErrorOr<JVariant> TypeCastOperatorEvaluator::Evaluate(
    const EvaluationContext& evaluation_context) const {
  ErrorOr<JVariant> source_result = source_->Evaluate(evaluation_context);
  if (source_result.is_error()) {
    return source_result;
  }

  return (this->*computer_)(source_result.value());
}


bool TypeCastOperatorEvaluator::IsInvalidPrimitiveBooleanTypeConversion(
    const CorElementType &source,
    const CorElementType &target)
  const {
  if (target == CorElementType::ELEMENT_TYPE_BOOLEAN &&
      TypeCompilerHelper::IsNumericalType(source)) {
    return true;
  }

  if ((TypeCompilerHelper::IsNumericalType(target)) &&
     (source == CorElementType::ELEMENT_TYPE_BOOLEAN)) {
      return true;
  }
  return false;
}

bool TypeCastOperatorEvaluator::IsEitherTypeObjectArray() {
  return IsArrayObjectType(result_type_) ||
      IsArrayObjectType(source_->GetStaticType());
}


ErrorOr<JVariant> TypeCastOperatorEvaluator::DoNothingComputer(
    const JVariant& source) const {
  return JVariant(source);
}


ErrorOr<JVariant> TypeCastOperatorEvaluator::ObjectTypeComputer(
    const JVariant& source) const {
  jobject source_value = nullptr;
  if (!source.get<jobject>(&source_value)) {
    LOG(ERROR) << "Couldn't extract the source value as an Object: "
               << source.ToString(false);
    return INTERNAL_ERROR_MESSAGE;
  }

  if (!jni()->IsInstanceOf(source_value,
                           static_cast<jclass>(target_class_))) {
    return FormatMessageModel {
        TypeCastEvaluateInvalid,
        { TypeNameFromSignature(source_->GetStaticType()), target_type_ }
    };
  }

  JVariant result;
  result.assign_new_ref(JVariant::ReferenceKind::Local, source_value);

  return std::move(result);
}

}  // namespace google_cloud_debugger
