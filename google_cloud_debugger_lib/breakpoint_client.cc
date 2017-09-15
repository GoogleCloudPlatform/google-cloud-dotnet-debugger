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

#include "breakpoint_client.h"

#include <mutex>

#include "constants.h"

using std::cerr;
using std::string;
using namespace google::cloud::diagnostics::debug;

namespace google_cloud_debugger {

BreakpointClient::BreakpointClient(std::unique_ptr<INamedPipe> pipe)
    : pipe_(std::move(pipe)) {}

HRESULT BreakpointClient::Initialize() { return pipe_->Initialize(); }

HRESULT BreakpointClient::WaitForConnection() {
  return pipe_->WaitForConnection();
}

HRESULT BreakpointClient::ReadBreakpoint(Breakpoint *breakpoint) {
  std::lock_guard<std::mutex> lock(mutex_);
  string buffer;
  std::swap(buffer, buffer_);
  string str;

  // Check if we have a full breakpoint message in the buffer.
  // If so just use it and do not try and read another breakpoint.
  std::size_t found_end = buffer.find(kEndBreakpointMessage);
  while (found_end == string::npos) {
    HRESULT result = pipe_->Read(&str);
    if (FAILED(result)) {
      return result;
    }
    buffer += str;
    str.clear();
    found_end = buffer.find(kEndBreakpointMessage);
  }

  // Ensure we have a start to the breakpoint message.
  std::size_t found_start = buffer.find(kStartBreakpointMessage);
  if (found_end == string::npos) {
    cerr << "invalid breakpoint message" << std::endl;
    return E_FAIL;
  }

  string newStr =
      buffer.substr(found_start + kStartBreakpointMessage.size(),
                    found_end - found_start - kStartBreakpointMessage.size());
  buffer_.append(buffer, found_end + kEndBreakpointMessage.size(), string::npos);

  if (!breakpoint->ParseFromString(newStr)) {
    cerr << "failed to serialize from protobuf" << std::endl;
    return E_FAIL;
  }
  return S_OK;
}

HRESULT BreakpointClient::WriteBreakpoint(const Breakpoint &breakpoint) {
  string bp_str;
  if (!breakpoint.SerializeToString(&bp_str)) {
    cerr << "failed to serialize to protobuf" << std::endl;
    return E_FAIL;
  }
  bp_str.insert(0, kStartBreakpointMessage);
  bp_str.append(kEndBreakpointMessage);
  return pipe_->Write(bp_str);
}

HRESULT BreakpointClient::ShutDown() {
  if (pipe_) {
    return pipe_->ShutDown();
  }

  return S_OK;
}

}  // namespace google_cloud_debugger
