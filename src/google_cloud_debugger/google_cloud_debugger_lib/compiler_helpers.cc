// Copyright 2018 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <map>

#include "class_names.h"
#include "compiler_helpers.h"
#include "dbg_primitive.h"
#include "i_cor_debug_helper.h"
#include "type_signature.h"

namespace google_cloud_debugger {

bool NumericCompilerHelper::IsImplicitNumericConversionable(
    const TypeSignature &source, const TypeSignature &target) {
  const CorElementType &source_type = source.cor_type;
  const CorElementType &target_type = target.cor_type;
  // Most of the logic here is from implicit numeric conversions:
  // https://docs.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/conversions#implicit-conversions
  switch (source_type) {
    case CorElementType::ELEMENT_TYPE_I1: {
      switch (target_type) {
        case CorElementType::ELEMENT_TYPE_I1:
        case CorElementType::ELEMENT_TYPE_I2:
        case CorElementType::ELEMENT_TYPE_I4:
        case CorElementType::ELEMENT_TYPE_I8:
        case CorElementType::ELEMENT_TYPE_R4:
        case CorElementType::ELEMENT_TYPE_R8:
          return true;
        default:
          return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_U1: {
      switch (target_type) {
        case CorElementType::ELEMENT_TYPE_U1:
        case CorElementType::ELEMENT_TYPE_I2:
        case CorElementType::ELEMENT_TYPE_U2:
        case CorElementType::ELEMENT_TYPE_I4:
        case CorElementType::ELEMENT_TYPE_U4:
        case CorElementType::ELEMENT_TYPE_I8:
        case CorElementType::ELEMENT_TYPE_U8:
        case CorElementType::ELEMENT_TYPE_R4:
        case CorElementType::ELEMENT_TYPE_R8:
          return true;
        default:
          return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_I2: {
      switch (target_type) {
        case CorElementType::ELEMENT_TYPE_I2:
        case CorElementType::ELEMENT_TYPE_I4:
        case CorElementType::ELEMENT_TYPE_I8:
        case CorElementType::ELEMENT_TYPE_R4:
        case CorElementType::ELEMENT_TYPE_R8:
          return true;
        default:
          return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_U2: {
      switch (target_type) {
        case CorElementType::ELEMENT_TYPE_U2:
        case CorElementType::ELEMENT_TYPE_I4:
        case CorElementType::ELEMENT_TYPE_U4:
        case CorElementType::ELEMENT_TYPE_I8:
        case CorElementType::ELEMENT_TYPE_U8:
        case CorElementType::ELEMENT_TYPE_R4:
        case CorElementType::ELEMENT_TYPE_R8:
          return true;
        default:
          return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_I4: {
      switch (target_type) {
        case CorElementType::ELEMENT_TYPE_I4:
        case CorElementType::ELEMENT_TYPE_I8:
        case CorElementType::ELEMENT_TYPE_R4:
        case CorElementType::ELEMENT_TYPE_R8:
          return true;
        default:
          return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_U4: {
      switch (target_type) {
        case CorElementType::ELEMENT_TYPE_U4:
        case CorElementType::ELEMENT_TYPE_I8:
        case CorElementType::ELEMENT_TYPE_U8:
        case CorElementType::ELEMENT_TYPE_R4:
        case CorElementType::ELEMENT_TYPE_R8:
          return true;
        default:
          return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_I8: {
      switch (target_type) {
        case CorElementType::ELEMENT_TYPE_I8:
        case CorElementType::ELEMENT_TYPE_R4:
        case CorElementType::ELEMENT_TYPE_R8:
          return true;
        default:
          return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_U8: {
      switch (target_type) {
        case CorElementType::ELEMENT_TYPE_U8:
        case CorElementType::ELEMENT_TYPE_R4:
        case CorElementType::ELEMENT_TYPE_R8:
          return true;
        default:
          return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_CHAR: {
      switch (target_type) {
        case CorElementType::ELEMENT_TYPE_CHAR:
        case CorElementType::ELEMENT_TYPE_U2:
        case CorElementType::ELEMENT_TYPE_I4:
        case CorElementType::ELEMENT_TYPE_U4:
        case CorElementType::ELEMENT_TYPE_I8:
        case CorElementType::ELEMENT_TYPE_U8:
        case CorElementType::ELEMENT_TYPE_R4:
        case CorElementType::ELEMENT_TYPE_R8:
          return true;
        default:
          return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_R4: {
      switch (target_type) {
        case CorElementType::ELEMENT_TYPE_R4:
        case CorElementType::ELEMENT_TYPE_R8:
          return true;
        default:
          return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_R8: {
      switch (target_type) {
        case CorElementType::ELEMENT_TYPE_R8:
          return true;
        default:
          return false;
      }
    }
    default:
      return false;
  }
}

bool NumericCompilerHelper::IsNumericallyPromotedToInt(
    const CorElementType &source) {
  return source == CorElementType::ELEMENT_TYPE_CHAR ||
         source == CorElementType::ELEMENT_TYPE_I1 ||
         source == CorElementType::ELEMENT_TYPE_U1 ||
         source == CorElementType::ELEMENT_TYPE_I2;
}

// Returns true for I1, I2, I4 and I8 types.
inline bool IsSignedIntegralType(const CorElementType &element_type) {
  return element_type == CorElementType::ELEMENT_TYPE_I1 ||
         element_type == CorElementType::ELEMENT_TYPE_I2 ||
         element_type == CorElementType::ELEMENT_TYPE_I4 ||
         element_type == CorElementType::ELEMENT_TYPE_I8;
}

bool NumericCompilerHelper::BinaryNumericalPromotion(const CorElementType &arg1,
                                                     const CorElementType &arg2,
                                                     CorElementType *result,
                                                     std::ostream *err_stream) {
  if (!TypeCompilerHelper::IsNumericalType(arg1) &&
      !TypeCompilerHelper::IsNumericalType(arg2)) {
    *err_stream << "Both arguments has to be of numerical types.";
    return false;
  }

  // TODO(quoct): Add support for Decimal.
  if (arg1 == CorElementType::ELEMENT_TYPE_R8 ||
      arg2 == CorElementType::ELEMENT_TYPE_R8) {
    *result = CorElementType::ELEMENT_TYPE_R8;
    return true;
  } else if (arg1 == CorElementType::ELEMENT_TYPE_R4 ||
             arg2 == CorElementType::ELEMENT_TYPE_R4) {
    *result = CorElementType::ELEMENT_TYPE_R4;
    return true;
  } else if (arg1 == CorElementType::ELEMENT_TYPE_U8 ||
             arg2 == CorElementType::ELEMENT_TYPE_U8) {
    // If the other operand is of type sbyte, short, int or long,
    // an error will occur. This is because no integral type exists that can
    // represent the full range of ulong as well as the signed integral types.
    if (IsSignedIntegralType(arg1) || IsSignedIntegralType(arg2)) {
      *err_stream << "If one of the argument is an unsigned long, "
                  << "the other cannot be a signed integral type.";
      return false;
    }
    *result = CorElementType::ELEMENT_TYPE_U8;
    return true;
  } else if (arg1 == CorElementType::ELEMENT_TYPE_I8 ||
             arg2 == CorElementType::ELEMENT_TYPE_I8) {
    *result = CorElementType::ELEMENT_TYPE_I8;
    return true;
  } else if (arg1 == CorElementType::ELEMENT_TYPE_U4 ||
             arg2 == CorElementType::ELEMENT_TYPE_U4) {
    // If 1 of the operand is sbyte, short or int,
    // the result will be long. We can simply call
    // IsSignedIntegralType without worrying about arg1
    // being an I8 type because that is already checked above.
    if (IsSignedIntegralType(arg1)) {
      *result = CorElementType::ELEMENT_TYPE_I8;
      return true;
    }

    if (IsSignedIntegralType(arg2)) {
      *result = CorElementType::ELEMENT_TYPE_I8;
      return true;
    }

    *result = CorElementType::ELEMENT_TYPE_U4;
    return true;
  } else {
    // Otherwise, both operands are converted to int.
    *result = CorElementType::ELEMENT_TYPE_I4;
    return true;
  }
}

bool TypeCompilerHelper::IsNumericalType(const CorElementType &cor_type) {
  return IsIntegralType(cor_type) ||
         cor_type == CorElementType::ELEMENT_TYPE_R4 ||
         cor_type == CorElementType::ELEMENT_TYPE_R8;
}

bool TypeCompilerHelper::IsIntegralType(const CorElementType &cor_type) {
  return cor_type == CorElementType::ELEMENT_TYPE_I1 ||
         cor_type == CorElementType::ELEMENT_TYPE_U1 ||
         cor_type == CorElementType::ELEMENT_TYPE_I2 ||
         cor_type == CorElementType::ELEMENT_TYPE_U2 ||
         cor_type == CorElementType::ELEMENT_TYPE_I4 ||
         cor_type == CorElementType::ELEMENT_TYPE_U4 ||
         cor_type == CorElementType::ELEMENT_TYPE_I8 ||
         cor_type == CorElementType::ELEMENT_TYPE_U8 ||
         cor_type == CorElementType::ELEMENT_TYPE_CHAR;
}

bool TypeCompilerHelper::IsArrayType(const CorElementType &array_type) {
  return array_type == CorElementType::ELEMENT_TYPE_ARRAY ||
         array_type == CorElementType::ELEMENT_TYPE_SZARRAY;
}

bool TypeCompilerHelper::IsObjectType(const CorElementType &cor_type) {
  return cor_type == CorElementType::ELEMENT_TYPE_OBJECT ||
         cor_type == CorElementType::ELEMENT_TYPE_CLASS ||
         cor_type == CorElementType::ELEMENT_TYPE_VALUETYPE;
}

CorElementType TypeCompilerHelper::ConvertStringToCorElementType(
    const std::string &type_string) {
  static std::map<std::string, CorElementType> string_to_cor_type{
      {kCharClassName, CorElementType::ELEMENT_TYPE_BOOLEAN},
      {kSByteClassName, CorElementType::ELEMENT_TYPE_I1},
      {kCharClassName, CorElementType::ELEMENT_TYPE_CHAR},
      {kByteClassName, CorElementType::ELEMENT_TYPE_U1},
      {kInt16ClassName, CorElementType::ELEMENT_TYPE_I2},
      {kUInt16ClassName, CorElementType::ELEMENT_TYPE_U2},
      {kInt32ClassName, CorElementType::ELEMENT_TYPE_I4},
      {kUInt32ClassName, CorElementType::ELEMENT_TYPE_U4},
      {kInt64ClassName, CorElementType::ELEMENT_TYPE_I8},
      {kUInt64ClassName, CorElementType::ELEMENT_TYPE_U8},
      {kStringClassName, CorElementType::ELEMENT_TYPE_STRING},
      {kObjectClassName, CorElementType::ELEMENT_TYPE_OBJECT}};

  if (string_to_cor_type.find(type_string) != string_to_cor_type.end()) {
    return string_to_cor_type[type_string];
  } else {
    auto open_bracket = type_string.find("[");
    if (open_bracket == std::string::npos) {
      return CorElementType::ELEMENT_TYPE_OBJECT;
    }

    auto closing_bracket = type_string.find("]");
    if (closing_bracket == std::string::npos ||
        closing_bracket < open_bracket) {
      return CorElementType::ELEMENT_TYPE_OBJECT;
    }

    // This means the type is something like System.Int32[],
    // which represents a simple array.
    if (open_bracket + 1 == closing_bracket) {
      return CorElementType::ELEMENT_TYPE_SZARRAY;
    }

    // This means the type is something like System.Int32[,], which is
    // a multidimensional array.
    return CorElementType::ELEMENT_TYPE_ARRAY;
  }
}

HRESULT TypeCompilerHelper::ConvertCorElementTypeToString(
    const CorElementType &cor_type, std::string *result) {
  static std::map<CorElementType, std::string> cor_type_to_string{
      {CorElementType::ELEMENT_TYPE_BOOLEAN, kBooleanClassName},
      {CorElementType::ELEMENT_TYPE_I1, kSByteClassName},
      {CorElementType::ELEMENT_TYPE_CHAR, kCharClassName},
      {CorElementType::ELEMENT_TYPE_U1, kByteClassName},
      {CorElementType::ELEMENT_TYPE_I2, kInt16ClassName},
      {CorElementType::ELEMENT_TYPE_U2, kUInt16ClassName},
      {CorElementType::ELEMENT_TYPE_I4, kInt32ClassName},
      {CorElementType::ELEMENT_TYPE_U4, kUInt32ClassName},
      {CorElementType::ELEMENT_TYPE_I8, kInt64ClassName},
      {CorElementType::ELEMENT_TYPE_U8, kUInt64ClassName},
      {CorElementType::ELEMENT_TYPE_R4, kSingleClassName},
      {CorElementType::ELEMENT_TYPE_R8, kDoubleClassName},
      {CorElementType::ELEMENT_TYPE_I, kIntPtrClassName},
      {CorElementType::ELEMENT_TYPE_U, kUIntPtrClassName},
      {CorElementType::ELEMENT_TYPE_STRING, kStringClassName},
      {CorElementType::ELEMENT_TYPE_OBJECT, kObjectClassName}};

  if (result == nullptr) {
    return E_INVALIDARG;
  }

  if (cor_type_to_string.find(cor_type) != cor_type_to_string.end()) {
    *result = cor_type_to_string[cor_type];
    return S_OK;
  }

  return E_FAIL;
}

HRESULT TypeCompilerHelper::IsBaseClass(
    mdTypeDef source_class, IMetaDataImport *source_class_metadata,
    const std::vector<CComPtr<ICorDebugAssembly>> &loaded_assemblies,
    const std::string &target_class, ICorDebugHelper *debug_helper,
    std::ostream *err_stream) {
  HRESULT hr;
  mdTypeDef current_class_token = source_class;
  CComPtr<IMetaDataImport> current_metadata_import;
  std::string current_class_name;
  mdToken current_base_class_token = source_class;

  current_metadata_import = source_class_metadata;
  int max_class_to_traverse = 10;
  while (max_class_to_traverse >= 0) {
    max_class_to_traverse -= 1;
    hr = debug_helper->GetTypeNameFromMdTypeDef(
        current_class_token, current_metadata_import, &current_class_name,
        &current_base_class_token, err_stream);
    if (FAILED(hr)) {
      return hr;
    }

    if (current_class_name.compare(target_class) == 0) {
      return S_OK;
    }

    if (current_class_name.compare(kObjectClassName)) {
      return E_FAIL;
    }

    // Retrieves the base class token and metadata import.
    CorTokenType token_type =
        (CorTokenType)(TypeFromToken(current_base_class_token));
    if (token_type == CorTokenType::mdtTypeDef) {
      // No need to change the IMetaDataImport since we are still in the same
      // module.
      current_class_token = current_base_class_token;
    } else if (token_type == CorTokenType::mdtTypeRef) {
      CComPtr<IMetaDataImport> resolved_metadata_import;
      mdTypeDef resolved_class_token;
      hr = debug_helper->GetMdTypeDefAndMetaDataFromTypeRef(
          current_base_class_token, loaded_assemblies, current_metadata_import,
          &resolved_class_token, &resolved_metadata_import, err_stream);
      if (FAILED(hr)) {
        return hr;
      }

      current_class_token = resolved_class_token;
      current_metadata_import = resolved_metadata_import;
    } else {
      // Not supported.
      *err_stream << "Only mdTypeDef and mdTypeRef tokens are supported.";
      return E_NOTIMPL;
    }
  }

  return E_FAIL;
}

}  //  namespace google_cloud_debugger
