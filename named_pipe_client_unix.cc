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

#ifdef PLATFORM_UNIX

#include <errno.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>

#include "named_pipe_client_unix.h"

using std::cerr;
using std::string;

namespace google_cloud_debugger {

NamedPipeClient::~NamedPipeClient() {
  if (pipe_ == -1) {
    return;
  }
  if (close(pipe_) == -1) {
    cerr << "close error: " << strerror(errno) << std::endl;
  }
}

HRESULT NamedPipeClient::Initialize() {
  pipe_ = socket(PF_UNIX, SOCK_STREAM, 0);
  if (pipe_ == -1) {
    cerr << "socket error: " << strerror(errno) << std::endl;
    return E_FAIL;
  }
  return S_OK;
}

HRESULT NamedPipeClient::WaitForConnection() {
  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, pipe_name_, sizeof(addr.sun_path) - 1);

  int timeout = kConnectionWaitTimeoutMs;
  bool file_found = false;

  while (timeout > 0) {
    int conn = connect(pipe_, (struct sockaddr *)&addr, sizeof(addr));
    if (conn == 0) {
      file_found = true;
      break;
    }

    if (conn == -1 && errno != ENOENT) {
      cerr << "connect error: " << strerror(errno) << std::endl;
      return E_FAIL;
    }

    timeout -= kConnectionSleepTimeoutMs;
    usleep(kConnectionSleepTimeoutMs * 1000);
  }

  if (!file_found) {
    cerr << "connect error: " << strerror(errno) << std::endl;
    return E_FAIL;
  }

  return S_OK;
}

HRESULT NamedPipeClient::Write(string *message) {
  if (message == nullptr) {
    return E_POINTER;
  }

  char buff[kBufferSize];
  int read = recv(pipe_, buff, kBufferSize, 0);

  if (read == -1) {
    cerr << "recv error: " << strerror(errno) << std::endl;
    return E_FAIL;
  }

  message->assign(buf, buf + read);
  return S_OK;
}

HRESULT NamedPipeClient::Write(const string &message) {
  const char *buf = message.c_str();
  DWORD bytes_left = message.length();

  while (bytes_left > 0) {
    int write = bytes_left > kBufferSize ? kBufferSize : bytes_left;

    int written = send(pipe_, buf, write, 0);
    if (written == -1) {
      cerr << "send error: " << strerror(errno) << std::endl;
      return E_FAIL;
    }

    bytes_left -= written;
    buf += written;
  }
  return S_OK;
}

}  // namespace google_cloud_debugger

#endif  //  PLATFORM_UNIX
