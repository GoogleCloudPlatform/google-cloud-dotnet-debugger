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

#include <custombinaryreader.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <memory>
#include <string>

#include "breakpoint_client.h"
#include "i_named_pipe_mock.h"

using ::testing::DoAll;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::_;
using google::cloud::diagnostics::debug::Breakpoint;
using google::cloud::diagnostics::debug::SourceLocation;
using google_cloud_debugger::BreakpointClient;
using std::string;
using std::unique_ptr;
using std::vector;

namespace google_cloud_debugger_test {

// This macro is to create test for BreakpointClient's functions
// that simply call a function of the underlying NamedPipe (of the same name).
// TestFunction is the name of the BreakpointClient's function we are testing.
// TestFunctionReturnValue is what the underlying NamedPipe's function will
// return.
#define BREAKPOINT_CLIENT_SIMPLE_TEST(TestFunction, TestFunctionReturnValue)  \
  unique_ptr<INamedPipeMock> named_pipe(new (std::nothrow) INamedPipeMock()); \
  EXPECT_CALL(*named_pipe.get(), TestFunction())                              \
      .Times(1)                                                               \
      .WillRepeatedly(Return(TestFunctionReturnValue));                       \
  BreakpointClient client(std::move(named_pipe));                             \
  EXPECT_EQ(client.TestFunction(), TestFunctionReturnValue);

// Tests Initialize function of BreakpointClient.
TEST(BreakpointClientTest, Initialize) {
  BREAKPOINT_CLIENT_SIMPLE_TEST(Initialize, S_OK);
}

// Tests error case for Initialize function of BreakpointClient.
TEST(BreakpointClientTest, InitializeError) {
  BREAKPOINT_CLIENT_SIMPLE_TEST(Initialize, E_APPLICATION_EXITING);
}

// Tests WaitForConnection function of BreakpointClient.
TEST(BreakpointClientTest, WaitForConnection) {
  BREAKPOINT_CLIENT_SIMPLE_TEST(WaitForConnection, S_OK);
}

// Tests error case for WaitForConnection function of BreakpointClient.
TEST(BreakpointClientTest, WaitForConnectionError) {
  BREAKPOINT_CLIENT_SIMPLE_TEST(WaitForConnection, E_ACCESSDENIED);
}

// Tests ShutDown function of BreakpointClient.
TEST(BreakpointClientTest, ShutDown) {
  BREAKPOINT_CLIENT_SIMPLE_TEST(ShutDown, S_OK);
}

// Tests error case for ShutDown function of BreakpointClient.
TEST(BreakpointClientTest, ShutDownError) {
  BREAKPOINT_CLIENT_SIMPLE_TEST(ShutDown, E_APPLICATION_EXITING);
}

// This action will pop the last string in string_vector
// and make arg0 points to that string.
ACTION_P(ReadFromStringVectorToArg0, string_vector) {
  // HAVE TO MAKE SURE TO REMOVE THE CHUNK READ.
  // MAY HAVE TO CAST.
  // vector<string> breakpoint_string_chunks = *reinterpret_cast<vector<string>
  // *>
  vector<string> *string_vector_ptr =
      static_cast<vector<string> *>(string_vector);
  if (!string_vector_ptr->empty()) {
    *static_cast<string *>(arg0) = string_vector_ptr->back();
    string_vector_ptr->pop_back();
  }
}

// Sets properties activated, line and path of Breakpoint breakpoint
// and serialize and returns that as a breakpoint message (with
// kStartBreakpointMessage and kEndBreakpointMessage at the front and
// end of the message respectively).
string SetBreakpointAndSerialize(Breakpoint *breakpoint, bool activated,
                                 int32_t line, const string &path) {
  breakpoint->set_activated(true);
  SourceLocation *source_location = breakpoint->mutable_location();
  source_location->set_line(line);
  source_location->set_path(path);

  string breakpoint_string;
  breakpoint->SerializeToString(&breakpoint_string);
  return google_cloud_debugger::kStartBreakpointMessage + breakpoint_string +
         google_cloud_debugger::kEndBreakpointMessage;
}

// Tests ReadBreakpoint function of BreakpointClient.
TEST(BreakpointClientTest, ReadBreakpoint) {
  Breakpoint breakpoint;
  string breakpoint_string =
      SetBreakpointAndSerialize(&breakpoint, true, 35, "My Path");

  // Breaks up the breakpoint_string into chunks.
  vector<string> breakpoint_string_chunks;
  int32_t chunk_size = 5;
  for (size_t i = 0; i < breakpoint_string.length(); i += chunk_size) {
    breakpoint_string_chunks.push_back(breakpoint_string.substr(i, chunk_size));
  }

  std::reverse(breakpoint_string_chunks.begin(),
               breakpoint_string_chunks.end());

  unique_ptr<INamedPipeMock> named_pipe(new (std::nothrow) INamedPipeMock());

  // Makes the Read function of named pipe returns chunks from
  // breakpoint_string_chunks.
  EXPECT_CALL(*named_pipe.get(), Read(_))
      .WillRepeatedly(DoAll(
          ReadFromStringVectorToArg0(&breakpoint_string_chunks), Return(S_OK)));
  BreakpointClient client(std::move(named_pipe));

  Breakpoint read_breakpoint;
  EXPECT_EQ(client.ReadBreakpoint(&read_breakpoint), S_OK);

  EXPECT_EQ(read_breakpoint.activated(), breakpoint.activated());
  EXPECT_EQ(read_breakpoint.location().line(), breakpoint.location().line());
  EXPECT_EQ(read_breakpoint.location().path(), breakpoint.location().path());
}

// Tests error case of ReadBreakpoint function of BreakpointClient.
TEST(BreakpointClientTest, ReadBreakpointError) {
  unique_ptr<INamedPipeMock> named_pipe(new (std::nothrow) INamedPipeMock());

  EXPECT_CALL(*named_pipe.get(), Read(_))
      .Times(1)
      .WillRepeatedly(Return(E_APPLICATION_EXITING));
  BreakpointClient client(std::move(named_pipe));

  Breakpoint read_breakpoint;
  EXPECT_EQ(client.ReadBreakpoint(&read_breakpoint), E_APPLICATION_EXITING);
}

// Tests WriteBreakpoint function of BreakpointClient.
TEST(BreakpointClientTest, WriteBreakpoint) {
  Breakpoint breakpoint;
  string breakpoint_string =
      SetBreakpointAndSerialize(&breakpoint, true, 35, "My Path");

  unique_ptr<INamedPipeMock> named_pipe(new (std::nothrow) INamedPipeMock());

  string breakpoint_to_write;
  EXPECT_CALL(*named_pipe.get(), Write(_))
      .WillRepeatedly(DoAll(SaveArg<0>(&breakpoint_to_write), Return(S_OK)));
  BreakpointClient client(std::move(named_pipe));

  EXPECT_EQ(client.WriteBreakpoint(breakpoint), S_OK);
  EXPECT_EQ(breakpoint_to_write, breakpoint_string);
}

// Tests error case for WriteBreakpoint function of BreakpointClient.
TEST(BreakpointClientTest, WriteBreakpointError) {
  Breakpoint breakpoint;
  string breakpoint_string =
      SetBreakpointAndSerialize(&breakpoint, true, 35, "My Path");

  unique_ptr<INamedPipeMock> named_pipe(new (std::nothrow) INamedPipeMock());

  EXPECT_CALL(*named_pipe.get(), Write(_)).WillRepeatedly(Return(E_ABORT));
  BreakpointClient client(std::move(named_pipe));

  EXPECT_EQ(client.WriteBreakpoint(breakpoint), E_ABORT);
}

}  // namespace google_cloud_debugger_test
