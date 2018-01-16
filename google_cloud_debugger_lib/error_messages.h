// Copyright 2017 Google Inc. All Rights Reserved.
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

#ifndef ERROR_MESSAGES_H_
#define ERROR_MESSAGES_H_

#include <string>

// This file contains error messages used by the Debugger.
namespace google_cloud_debugger {

static const std::string kFailedEvalCreation =
    "Failed to create ICorDebugEval.";

static const std::string kFailedToCreateString =
    "Failed to create a new string.";

static const std::string kFailedToCreateDbgObject =
    "Failed to create DbgObject.";

static const std::string kConditionHasToBeBoolean =
    "Condition must be a boolean.";

static const std::string kTypeMisMatch =
    "Actual types does not match expected types.";

}  // namespace google_cloud_debugger

#endif  //  ERROR_MESSAGES_H_
