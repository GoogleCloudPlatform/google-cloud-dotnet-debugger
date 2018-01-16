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

#ifndef COMPILER_HELPERS_H_
#define COMPILER_HELPERS_H_

#include "cor.h"

// Various helper functions for compiling such as numeric conversions,
// numeric promotions, etc.
namespace google_cloud_debugger {

class TypeSignature;

// Returns true if source can be implicitly converted to target.
// If source and target are not numeric type, the function
// return false.
// If source and target are the same numeric type, the function
// will also return true.
// TODO(quoct): Add support for decimal type.
bool IsImplicitNumericConversionable(const TypeSignature &source,
  const TypeSignature &target);

}  //  namespace google_cloud_debugger

#endif  //  COMPILER_HELPERS_H_
