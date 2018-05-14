/**
 * Copyright 2018 Google Inc. All Rights Reserved.
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

#include "class_names.h"
#include "type_signature.h"

namespace google_cloud_debugger {

TypeSignature TypeSignature::Object =
    TypeSignature{CorElementType::ELEMENT_TYPE_OBJECT, kObjectClassName};

int TypeSignature::compare(const TypeSignature &other) const {
  if (cor_type != other.cor_type || type_name.compare(other.type_name) != 0) {
    return 1;
  }

  if (is_array != other.is_array && array_rank != other.array_rank) {
    return 1;
  }

  if (generic_types.size() != other.generic_types.size()) {
    return 1;
  }

  for (size_t i = 0; i < generic_types.size(); ++i) {
    if (generic_types[i].compare(other.generic_types[i]) != 0) {
      return 1;
    }
  }

  return 0;
}

}  // namespace google_cloud_debugger
