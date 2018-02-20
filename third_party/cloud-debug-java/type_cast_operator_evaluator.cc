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

#include "class_names.h"
#include "common.h"
#include "compiler_helpers.h"
#include "dbg_stack_frame.h"
#include "i_cor_debug_helper.h"

namespace google_cloud_debugger {

TypeCastOperatorEvaluator::TypeCastOperatorEvaluator(
    std::unique_ptr<ExpressionEvaluator> source, const std::string &target_type)
    : source_(std::move(source)), computer_(nullptr) {
  result_type_ = TypeSignature::Object;
  CorElementType target_cor_type =
      TypeCompilerHelper::ConvertStringToCorElementType(target_type);
  target_type_ = TypeSignature{target_cor_type, target_type};
}

HRESULT TypeCastOperatorEvaluator::Compile(DbgStackFrame *stack_frame,
                                           std::ostream *err_stream) {
  HRESULT hr = source_->Compile(stack_frame, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  TypeSignature source_type = source_->GetStaticType();
  result_type_ = source_type;

  // Checks that if only one of the source and target is boolean,
  // then the other cannot be a numerical type.
  if (!IsValidPrimitiveBooleanTypeConversion(source_type.cor_type,
                                             target_type_.cor_type)) {
    *err_stream << "Invalid type cast.";
    return E_FAIL;
  }

  if (source_type.cor_type == CorElementType::ELEMENT_TYPE_BOOLEAN &&
      target_type_.cor_type == CorElementType::ELEMENT_TYPE_BOOLEAN) {
    computer_ = &TypeCastOperatorEvaluator::DoNothingComputer;
    return S_OK;
  }

  // TODO(quoct): Test unbox scenario.
  if (TypeCompilerHelper::IsNumericalType(source_type.cor_type) &&
      TypeCompilerHelper::IsNumericalType(target_type_.cor_type)) {
    return CompileNumericalCast(source_type.cor_type,
                                target_type_.cor_type,
                                err_stream);
  }

  // TODO(quoct): Array type not supported.
  // Investigate how much work to support it.
  if (TypeCompilerHelper::IsArrayType(target_type_.cor_type)) {
    *err_stream << "Casting to array type is not supported.";
    return E_NOTIMPL;
  }

  // If both are object type, checks that either source_type is a base
  // class of target_type or target_type is a base class of source_type.
  if (TypeCompilerHelper::IsObjectType(target_type_.cor_type) &&
      TypeCompilerHelper::IsObjectType(source_type.cor_type)) {
    hr = IsBaseType(stack_frame, source_type.type_name, target_type_.type_name,
                    err_stream);
    if (FAILED(hr)) {
      hr = IsBaseType(stack_frame, target_type_.type_name,
                      source_type.type_name, err_stream);
      if (FAILED(hr)) {
        return hr;
      }
    }

    result_type_ = target_type_;
    computer_ = &TypeCastOperatorEvaluator::DoNothingComputer;
    return hr;
  }

  *err_stream << "Unsupported cast.";
  return E_NOTIMPL;
}

HRESULT TypeCastOperatorEvaluator::CompileNumericalCast(
    const CorElementType &source_type,
    const CorElementType &target_type,
    std::ostream *err_stream) {
  // In this case, we cast to target_type_.
  result_type_ = target_type_;
  switch (target_type_.cor_type) {
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

HRESULT TypeCastOperatorEvaluator::IsBaseType(DbgStackFrame *stack_frame,
                                              const std::string &source_type,
                                              const std::string &target_type,
                                              std::ostream *err_stream) const {
  HRESULT hr;
  CComPtr<ICorDebugModule> debug_module;
  CComPtr<IMetaDataImport> source_metadata_import;
  mdToken source_token;

  hr = stack_frame->GetClassTokenAndModule(
      source_type, &source_token, &debug_module, &source_metadata_import);
  if (FAILED(hr)) {
    return hr;
  }

  return TypeCompilerHelper::IsBaseClass(source_token, source_metadata_import,
                                         target_type, err_stream);
}

HRESULT TypeCastOperatorEvaluator::Evaluate(
    std::shared_ptr<DbgObject> *dbg_object, IEvalCoordinator *eval_coordinator,
    std::ostream *err_stream) const {
  std::shared_ptr<DbgObject> source_obj;
  HRESULT hr = source_->Evaluate(&source_obj, eval_coordinator, err_stream);
  if (FAILED(hr)) {
    return hr;
  }

  return (this->*computer_)(source_obj, dbg_object);
}

bool TypeCastOperatorEvaluator::IsValidPrimitiveBooleanTypeConversion(
    const CorElementType &source, const CorElementType &target) const {
  if (target == CorElementType::ELEMENT_TYPE_BOOLEAN &&
      TypeCompilerHelper::IsNumericalType(source)) {
    return false;
  }

  if ((TypeCompilerHelper::IsNumericalType(target)) &&
      (source == CorElementType::ELEMENT_TYPE_BOOLEAN)) {
    return false;
  }
  return true;
}

HRESULT TypeCastOperatorEvaluator::DoNothingComputer(
    std::shared_ptr<DbgObject> source,
    std::shared_ptr<DbgObject> *result) const {
  if (!source || !result) {
    return E_INVALIDARG;
  }

  *result = source;
  return S_OK;
}

}  // namespace google_cloud_debugger
