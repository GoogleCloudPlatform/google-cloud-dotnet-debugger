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

#include <string>

#include "named_pipe_client_windows.h"

using std::string;

namespace google_cloud_debugger {

NamedPipeClient::~NamedPipeClient() {
  if (pipe_ == INVALID_HANDLE_VALUE) {
    return;
  }
  if (!CloseHandle(pipe_)) {
    std::cerr << "CloseHandle error: " << HRESULT_FROM_WIN32(GetLastError())
              << std::endl;
  }
}

HRESULT NamedPipeClient::Initialize() { return S_OK; }

HRESULT NamedPipeClient::WaitForConnection() {
  int timeout = kConnectionWaitTimeoutMs;
  bool file_found = false;

  while (timeout > 0) {
    BOOL available =
        WaitNamedPipeW(pipe_name_.c_str(), kConnectionWaitTimeoutMs);
    if (available) {
      file_found = true;
      break;
    }

    if (!available && GetLastError() != ERROR_FILE_NOT_FOUND) {
      std::cerr << "WaitNamedPipe error: " << HRESULT_FROM_WIN32(GetLastError())
                << std::endl;
      return HRESULT_FROM_WIN32(GetLastError());
    }

    timeout -= kConnectionSleepTimeoutMs;
    Sleep(kConnectionSleepTimeoutMs);
  }

  if (!file_found) {
    std::cerr << "WaitNamedPipe error: " << HRESULT_FROM_WIN32(GetLastError())
              << std::endl;
    return HRESULT_FROM_WIN32(GetLastError());
  }

  pipe_ = CreateFileW(pipe_name_.c_str(), GENERIC_READ | GENERIC_WRITE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL, NULL);

  if (pipe_ == INVALID_HANDLE_VALUE) {
    std::cerr << "CreateFile error: " << HRESULT_FROM_WIN32(GetLastError())
              << std::endl;
    return HRESULT_FROM_WIN32(GetLastError());
  }
  return S_OK;
}

HRESULT NamedPipeClient::Read(string *message) {
  if (message == nullptr) {
    return E_POINTER;
  }
  CHAR buf[kBufferSize];
  DWORD read = 0;
  BOOL success = ReadFile(pipe_, buf, kBufferSize - 1, &read, NULL);

  if (!success) {
    std::cerr << "ReadFile error: " << HRESULT_FROM_WIN32(GetLastError())
              << std::endl;
    return HRESULT_FROM_WIN32(GetLastError());
  }

  message->assign(buf, buf + read);
  return S_OK;
}

HRESULT NamedPipeClient::Write(const string &message) {
  const CHAR *buf = message.c_str();
  DWORD bytes_left = message.size();

  while (bytes_left > 0) {
    DWORD written = 0;
    int write = bytes_left > kBufferSize ? kBufferSize : bytes_left;

    BOOL success = WriteFile(pipe_, buf, write, &written, NULL);
    if (!success) {
      std::cerr << "WriteFile error: " << HRESULT_FROM_WIN32(GetLastError())
                << std::endl;
      return HRESULT_FROM_WIN32(GetLastError());
    }

    bytes_left -= written;
    buf += written;
  }
  return S_OK;
}

HRESULT NamedPipeClient::ShutDown() {
  if (!pipe_) {
    return S_FALSE;
  }

  if (!CancelIoEx(pipe_, nullptr)) {
    std::cerr << "Canceling IO error: " << HRESULT_FROM_WIN32(GetLastError())
              << std::endl;
    return HRESULT_FROM_WIN32(GetLastError());
  }

  if (!CloseHandle(pipe_)) {
    std::cerr << "CloseHandle error: " << HRESULT_FROM_WIN32(GetLastError())
              << std::endl;
    return HRESULT_FROM_WIN32(GetLastError());
  }

  pipe_ = INVALID_HANDLE_VALUE;

  return S_OK;
}

}  // namespace google_cloud_debugger

#endif  //  _WIN32
