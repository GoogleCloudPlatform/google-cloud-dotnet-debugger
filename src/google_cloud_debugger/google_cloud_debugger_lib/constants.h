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

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include <string>

namespace google_cloud_debugger {

// The buffer size to read and write pipes.
static const int kBufferSize = 1024;

// The maximum amount of time to wait for a pipe connection in milliseconds.
static const int kConnectionWaitTimeoutMs = 60000;

// The amount of time to wait between checks for a pipe connection in milliseconds.
static const int kConnectionSleepTimeoutMs = 2000;

// The default evaluation depth for an object.
static const int kDefaultObjectEvalDepth = 5;

// The start of a breakpoint message.
static const std::string kStartBreakpointMessage = "START_DEBUG_MESSAGE";

// The end of a breakpoint message.
static const std::string kEndBreakpointMessage = "END_DEBUG_MESSAGE";

// File extension for dll file.
static const std::string kDllExtension = ".dll";

// File extension for pdb file.
static const std::string kPdbExtension = ".pdb";

// If a field is a backing field of a property, its name will
// end with this.
static const std::string kBackingField = ">k__BackingField";

// Default size of a vector that we use to retrieve objects from ICorDebugEnum.
static const std::uint32_t kDefaultVectorSize = 100;

}  // namespace google_cloud_debugger

#endif  //  CONSTANTS_H_
