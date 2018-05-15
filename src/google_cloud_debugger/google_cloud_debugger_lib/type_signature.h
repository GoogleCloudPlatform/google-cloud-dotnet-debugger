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

#ifndef TYPE_SIGNATURE_H_
#define TYPE_SIGNATURE_H_

#include <string>
#include <vector>

#include "cor.h"

namespace google_cloud_debugger {

// This struct represents a .NET type. This is used to compare
// whether 2 objects have the same type.
struct TypeSignature {
  // CorElementType of the type signature.
  CorElementType cor_type;

  // This is useful if cor_type is not an integral or float type.
  // If this is a signature of a generic type, type_name
  // will be the name of the class without the generic types.
  std::string type_name;

  // Vector containing the generic types of the signatures.
  // If this is an array, the first generic_type is the type
  // of each array element.
  std::vector<TypeSignature> generic_types;

  // True if this is an array.
  bool is_array = false;

  // Rank of the array.
  ULONG array_rank = 0;

  static TypeSignature Object;

  // Returns 0 if the 2 TypeSignature are equal.
  // Two TypeSignatures are equal if they have
  // the same cor_type, type_name, is_array, array_rank
  // and all TypeSignatures in the generic_types vector
  // are equal.
  int compare(const TypeSignature &other) const;
};
  
}  // namespace google_cloud_debugger

#endif  // TYPE_SIGNATURE_H_
