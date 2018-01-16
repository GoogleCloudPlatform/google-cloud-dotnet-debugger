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

#include "compiler_helpers.h"
#include "../third_party/cloud-debug-java/expression_evaluator.h"

namespace google_cloud_debugger {

bool IsImplicitNumericConversionable(const TypeSignature &source,
    const TypeSignature &target) {
  const CorElementType &source_type = source.cor_type;
  const CorElementType &target_type = target.cor_type;
  switch (source_type) {
    case CorElementType::ELEMENT_TYPE_I1:
    {
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
    case CorElementType::ELEMENT_TYPE_U1:
    {
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
    case CorElementType::ELEMENT_TYPE_I2:
    {
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
    case CorElementType::ELEMENT_TYPE_U2:
    {
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
    case CorElementType::ELEMENT_TYPE_I4:
    {
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
    case CorElementType::ELEMENT_TYPE_U4:
    {
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
    case CorElementType::ELEMENT_TYPE_I8:
    {
      switch (target_type) {
      case CorElementType::ELEMENT_TYPE_I8:
      case CorElementType::ELEMENT_TYPE_R4:
      case CorElementType::ELEMENT_TYPE_R8:
        return true;
      default:
        return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_U8:
    {
      switch (target_type) {
      case CorElementType::ELEMENT_TYPE_U8:
      case CorElementType::ELEMENT_TYPE_R4:
      case CorElementType::ELEMENT_TYPE_R8:
        return true;
      default:
        return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_CHAR:
    {
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
    case CorElementType::ELEMENT_TYPE_R4:
    {
      switch (target_type) {
      case CorElementType::ELEMENT_TYPE_R4:
      case CorElementType::ELEMENT_TYPE_R8:
        return true;
      default:
        return false;
      }
    }
    case CorElementType::ELEMENT_TYPE_R8:
    {
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

}  //  namespace google_cloud_debugger
