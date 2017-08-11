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

// The name of the named pipe.
static const std::string kPipeName = "dotnet-debugger";

// The name of the named pipe as a wide string.
static const std::wstring kPipeNameW = L"dotnet-debugger";

// The buffer size to read and write pipes.
static const int kBufferSize = 1024;

// The maximum amount of time to wait for a pipe connection in milliseconds.
static const int kConnectionWaitTimeoutMs = 20000;

// The amount of time to wait between checks for a pipe connection in milliseconds.
static const int kConnectionSleepTimeoutMs = 2000;

// The start of a breakpoint message.
static const std::string kStartBreakpointMessage = "START_DEBUG_MESSAGE";

// The end of a breakpoint message.
static const std::string kEndBreakpointMessage = "END_DEBUG_MESSAGE";

}  // namespace google_cloud_debugger

#endif  //  CONSTANTS_H_