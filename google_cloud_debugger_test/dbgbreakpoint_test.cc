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

#include <algorithm>
#include <custombinaryreader.h>
#include <gtest/gtest.h>
#include <cstdlib>
#include <string>

#include "ccomptr.h"
#include "dbgbreakpoint.h"
#include "dbgobject.h"
#include "i_cordebug_mocks.h"
#include "i_evalcoordinator_mock.h"
#include "i_portablepdb_mocks.h"
#include "i_stackframecollection_mock.h"

using ::testing::Const;
using ::testing::Return;
using ::testing::ReturnRef;
using google::cloud::diagnostics::debug::Breakpoint;
using google_cloud_debugger::CComPtr;
using google_cloud_debugger::DbgBreakpoint;
using google_cloud_debugger_portable_pdb::IDocumentIndex;
using google_cloud_debugger_portable_pdb::MethodInfo;
using google_cloud_debugger_portable_pdb::SequencePoint;
using std::string;
using std::unique_ptr;
using std::vector;

namespace google_cloud_debugger_test {

// Test Fixture for DbgBreakpoint
// Sets up a default DbgBreakpoint for the test.
class DbgBreakpointTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    breakpoint_.Initialize(file_name_, id_, line_, column_);
    // Gives the PDB file the same file name as this breakpoint's file name.
    pdb_file_fixture_.first_doc_.file_name_ = file_name_;
    pdb_file_fixture_.SetUpIPortablePDBFile(&file_mock_);
  }

  // ICorDebugBreakpoint associated with this breakpoint.
  ICorDebugBreakpointMock cordebug_breakpoint_;

  // Breakpoint.
  DbgBreakpoint breakpoint_;

  // File name of the breakpoint.
  string file_name_ = "My file";

  // Lower case of the file name of the breakpoint.
  string lower_case_file_name_ = "my file";

  // Id of the breakpoint.
  string id_ = "My ID";

  // Line number of breakpoint.
  uint32_t line_ = 32;

  // Column of the breakpoint.
  uint32_t column_ = 64;

  // Mock object for Portable PDB file.
  IPortablePdbFileMock file_mock_;

  // Fixture for the PDB File.
  PortablePDBFileFixture pdb_file_fixture_;
};

// Tests that the Initialize function sets up the correct fields.
TEST_F(DbgBreakpointTest, Initialize) {
  // The names stored should be lower case.
  EXPECT_EQ(breakpoint_.GetFileName(), lower_case_file_name_);
  EXPECT_EQ(breakpoint_.GetId(), id_);
  EXPECT_EQ(breakpoint_.GetLine(), line_);
  EXPECT_EQ(breakpoint_.GetColumn(), column_);

  // Now we use the other Initialize function,
  // file name, ID, line and column should be copied over.
  DbgBreakpoint breakpoint2;
  breakpoint2.Initialize(breakpoint_);

  EXPECT_EQ(breakpoint2.GetFileName(), breakpoint_.GetFileName());
  EXPECT_EQ(breakpoint2.GetId(), breakpoint_.GetId());
  EXPECT_EQ(breakpoint2.GetLine(), breakpoint_.GetLine());
  EXPECT_EQ(breakpoint2.GetColumn(), breakpoint_.GetColumn());
}

// Tests that the Set/GetMethodToken function sets up the correct fields.
TEST_F(DbgBreakpointTest, SetGetMethodToken) {
  mdMethodDef method_token = 10;
  breakpoint_.SetMethodToken(method_token);
  EXPECT_EQ(breakpoint_.GetMethodToken(), method_token);
}

// Tests that Set/GetICorDebugBreakpoint function works.
TEST_F(DbgBreakpointTest, SetGetICorDebugBreakpoint) {
  breakpoint_.SetCorDebugBreakpoint(&cordebug_breakpoint_);

  CComPtr<ICorDebugBreakpoint> cordebug_breakpoint2;
  HRESULT hr = breakpoint_.GetCorDebugBreakpoint(&cordebug_breakpoint2);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
}

// Tests the error cases of Set/GetICorDebugBreakpoint function.
TEST_F(DbgBreakpointTest, SetGetICorDebugBreakpointError) {
  // Null argument.
  EXPECT_EQ(breakpoint_.GetCorDebugBreakpoint(nullptr), E_INVALIDARG);

  // Calls without setting the breakpoint.
  CComPtr<ICorDebugBreakpoint> cordebug_breakpoint2;
  EXPECT_EQ(breakpoint_.GetCorDebugBreakpoint(&cordebug_breakpoint2), E_FAIL);
}

// Returns a method that contains a sequence point
// thats contains breakpoint_line. Also sets the method
// def to method_def and the IL Offset of the matching
// sequence point to il_offset.
MethodInfo MakeMatchingMethod(uint32_t breakpoint_line,
                              uint32_t method_first_line, uint32_t method_def,
                              uint32_t il_offset) {
  assert(method_first_line < breakpoint_line);

  // Minimum amount of lines from breakpoint to last line.
  uint32_t min_line = 10;

  // This method's first line is less than the breakpoint's line
  // and its last line is greater than the breakpoint's line,
  // so the TrySetBreakpoint method should be able to use this method.
  MethodInfo method;
  method.first_line = method_first_line;
  method.last_line = breakpoint_line + std::max(breakpoint_line, min_line);
  method.method_def = method_def;

  // Puts a sequence point that does not match the line of the breakpoint.
  SequencePoint seq_point;
  seq_point.start_line = method.first_line;
  // End line is between the start line of the method and breakpoint_line.
  seq_point.end_line =
      method.first_line + (rand() % (breakpoint_line - method.first_line));
  seq_point.il_offset = rand() % il_offset;
  method.sequence_points.push_back(seq_point);

  assert(seq_point.end_line < breakpoint_line);

  // Puts in another sequence point that matches the line of the breakpoint.
  SequencePoint seq_point2;
  seq_point2.start_line = breakpoint_line;
  seq_point2.end_line = breakpoint_line + 1;
  seq_point2.il_offset = il_offset;
  method.sequence_points.push_back(seq_point2);

  // Puts a sequence point that does not match the line of the breakpoint.
  SequencePoint seq_point3;
  seq_point3.start_line = seq_point2.end_line + 1;
  // End line is between the start line of the method and breakpoint_line.
  seq_point3.end_line = seq_point3.end_line + 2;
  seq_point3.il_offset = il_offset + 10;
  method.sequence_points.push_back(seq_point3);

  assert(seq_point3.end_line < method.last_line);

  return method;
}

// Test the TrySetBreakpoint function of DbgBreakpoint.
TEST_F(DbgBreakpointTest, TrySetBreakpoint) {
  // Gets a method that matches the breakpoint.
  uint32_t method_first_line = rand() % line_;
  uint32_t method_def = 100;
  uint32_t il_offset = 99;
  MethodInfo method =
      MakeMatchingMethod(line_, method_first_line, method_def, il_offset);

  // Gets another method that does not match the breakpoint.
  MethodInfo method2 = MakeMatchingMethod(line_ * 2, method_first_line * 2,
                                          method_def * 2, il_offset * 2);

  // Push the methods into the method vector that
  // the document index matching this Breakpoint will return.
  pdb_file_fixture_.first_doc_.methods_.push_back(method);
  pdb_file_fixture_.first_doc_.methods_.push_back(method2);

  EXPECT_TRUE(breakpoint_.TrySetBreakpoint(file_mock_));
  EXPECT_EQ(breakpoint_.GetILOffset(), il_offset);
  EXPECT_EQ(breakpoint_.GetMethodDef(), method_def);
  EXPECT_TRUE(breakpoint_.IsSet());
}

// Test the TrySetBreakpoint function of DbgBreakpoint
// when there are multiple methods in the Document Index.
TEST_F(DbgBreakpointTest, TrySetBreakpointWithMultipleMethods) {
  // Gets a method that matches the breakpoint.
  uint32_t method_first_line = line_ - 10;
  uint32_t method_def = 100;
  uint32_t il_offset = 99;
  MethodInfo method =
      MakeMatchingMethod(line_, method_first_line, method_def, il_offset);

  // Gets another method that does not match the breakpoint.
  uint32_t method2_first_line = line_ + 100;
  uint32_t method2_def = method_def * 2;
  uint32_t il_offset_2 = il_offset * 2;
  MethodInfo method2 = MakeMatchingMethod(line_ + 120, method2_first_line,
                                          method2_def, il_offset_2);

  // Makes another the method that match the breakpoint
  // but has start line greater than the first method (so
  // this method should be selected since it is a better match).
  uint32_t method3_first_line = line_ - 5;
  uint32_t method3_def = method_def * 3;
  uint32_t il_offset_3 = 130;
  MethodInfo method3 =
      MakeMatchingMethod(line_, method3_first_line, method3_def, il_offset_3);

  // Push the methods into the method vector that
  // the document index matching this Breakpoint will return.
  pdb_file_fixture_.first_doc_.methods_.push_back(method);
  pdb_file_fixture_.first_doc_.methods_.push_back(method2);
  pdb_file_fixture_.first_doc_.methods_.push_back(method3);

  EXPECT_TRUE(breakpoint_.TrySetBreakpoint(file_mock_));
  EXPECT_EQ(breakpoint_.GetILOffset(), il_offset_3);
  EXPECT_EQ(breakpoint_.GetMethodDef(), method3_def);
  EXPECT_TRUE(breakpoint_.IsSet());
}

// Tests the case where no matching methods are found.
TEST_F(DbgBreakpointTest, TrySetBreakpointWithNoMatching) {
  // Gets a method that does not match the breakpoint.
  uint32_t method_first_line = line_ + 5;
  uint32_t method_def = 100;
  uint32_t il_offset = 99;
  MethodInfo method =
      MakeMatchingMethod(line_ + 10, method_first_line, method_def, il_offset);

  // The method now contains sequence points that does not match
  // the breakpoint. We now change the method_first_line
  // so it will be less than the breakpoint_line.
  // Note that this still mean that this method does not
  // match the breakpoint since its sequence points do not.
  method.first_line = line_ - 10;

  // Gets another method that does not match the breakpoint.
  uint32_t method2_first_line = line_ + 100;
  uint32_t method2_def = method_def * 2;
  uint32_t il_offset_2 = il_offset * 2;
  MethodInfo method2 = MakeMatchingMethod(line_ + 120, method2_first_line,
                                          method2_def, il_offset_2);

  // Push the methods into the method vector that
  // the document index matching this Breakpoint will return.
  pdb_file_fixture_.first_doc_.methods_.push_back(method);
  pdb_file_fixture_.first_doc_.methods_.push_back(method2);

  EXPECT_FALSE(breakpoint_.TrySetBreakpoint(file_mock_));
}

// Tests the PopulateBreakpoint function of DbgBreakpoint.
TEST_F(DbgBreakpointTest, PopulateBreakpoint) {
  Breakpoint proto_breakpoint;
  IStackFrameCollectionMock stackframe_collection_mock;
  IEvalCoordinatorMock eval_coordinator_mock;

  EXPECT_CALL(stackframe_collection_mock,
              PopulateStackFrames(&proto_breakpoint, &eval_coordinator_mock))
      .Times(1)
      .WillRepeatedly(Return(S_OK));

  HRESULT hr = breakpoint_.PopulateBreakpoint(
      &proto_breakpoint, &stackframe_collection_mock, &eval_coordinator_mock);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  // Checks that the proto breakpoint's properties are set.
  EXPECT_EQ(proto_breakpoint.location().line(), line_);
  EXPECT_EQ(proto_breakpoint.location().path(), lower_case_file_name_);
  EXPECT_EQ(proto_breakpoint.id(), id_);
}

// Tests the error cases of PopulateBreakpoint function of DbgBreakpoint.
TEST_F(DbgBreakpointTest, PopulateBreakpointError) {
  Breakpoint proto_breakpoint;
  IStackFrameCollectionMock stackframe_collection_mock;
  IEvalCoordinatorMock eval_coordinator_mock;

  // Checks for null error.
  EXPECT_EQ(breakpoint_.PopulateBreakpoint(nullptr, &stackframe_collection_mock,
                                           &eval_coordinator_mock),
            E_INVALIDARG);
  EXPECT_EQ(breakpoint_.PopulateBreakpoint(&proto_breakpoint, nullptr,
                                           &eval_coordinator_mock),
            E_INVALIDARG);
  EXPECT_EQ(breakpoint_.PopulateBreakpoint(
                &proto_breakpoint, &stackframe_collection_mock, nullptr),
            E_INVALIDARG);

  // Makes PopulateStackFrames returns error.
  EXPECT_CALL(stackframe_collection_mock,
              PopulateStackFrames(&proto_breakpoint, &eval_coordinator_mock))
      .Times(1)
      .WillRepeatedly(Return(CORDBG_E_BAD_REFERENCE_VALUE));

  EXPECT_EQ(breakpoint_.PopulateBreakpoint(&proto_breakpoint,
                                           &stackframe_collection_mock,
                                           &eval_coordinator_mock),
            CORDBG_E_BAD_REFERENCE_VALUE);
}

}  // namespace google_cloud_debugger_test
