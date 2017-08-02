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

#ifdef _WIN32
#ifndef NAMED_PIPE_CLIENT_H_
#define NAMED_PIPE_CLIENT_H_

#include <iostream>
#include <cor.h>
#include <string>
#include <windef.h>

#include "constants.h"
#include "i_named_pipe.h"

namespace google_cloud_debugger {

// A named pipe client for windows.
class NamedPipeClient : public INamedPipe {
public:
	~NamedPipeClient();
	HRESULT Initialize() override;
	HRESULT WaitForConnection() override;
	HRESULT Read(std::string *message) override;
	HRESULT Write(const std::string &message) override;

private:
	// The name of the pipe.
	const std::wstring pipe_name_ = std::wstring(L"\\\\.\\pipe\\") + std::wstring(kPipeNameW);

	// A handle to the open pipe.
	HANDLE pipe_ = INVALID_HANDLE_VALUE;
};

}  // namespace google_cloud_debugger

#endif  //  NAMED_PIPE_CLIENT_H_
#endif  //  _WIN32
