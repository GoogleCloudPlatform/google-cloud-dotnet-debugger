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

#ifndef I_NAMED_PIPE_H_
#define I_NAMED_PIPE_H_

#include <string>

namespace google_cloud_debugger {

// Functionality of a named pipe.
class INamedPipe {
 public:
  virtual ~INamedPipe() {};

  // Initializes the pipe and returns an HRESULT.
  virtual HRESULT Initialize() = 0;

  // Waits for the pipe to connect.  This function will
  // block until a connection is made or until a timeout
  // occurs.
  virtual HRESULT WaitForConnection() = 0;

  // Reads up to the maximum buffer size from
  // the pipe and returns an HRESULT.  This function will
  // block until there is a message to read.
  // Note: strings are used only as containers.
  virtual HRESULT Read(std::string *message) = 0;

  // Write a message up to the buffer in chunks of the maximum
  // buffer size from the pipe and returns an HRESULT.
  // Note: strings are used only as containers.
  virtual HRESULT Write(const std::string &message) = 0;
};

}  // namespace google_cloud_debugger

#endif  //  I_NAMED_PIPE_H_
