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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstdint>
#include <string>

#include "ccomptr.h"
#include "common_action_mocks.h"
#include "i_cordebug_mocks.h"
#include "i_evalcoordinator_mock.h"
#include "i_metadataimport_mock.h"
#include "i_portablepdb_mocks.h"
#include "stackframecollection.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using google::cloud::diagnostics::debug::Breakpoint;
using google::cloud::diagnostics::debug::StackFrame;
using google_cloud_debugger::CComPtr;
using google_cloud_debugger::ConvertStringToWCharPtr;
using google_cloud_debugger::StackFrameCollection;
using google_cloud_debugger_portable_pdb::LocalVariableInfo;
using google_cloud_debugger_portable_pdb::MethodInfo;
using google_cloud_debugger_portable_pdb::Scope;
using google_cloud_debugger_portable_pdb::SequencePoint;
using std::string;
using std::unique_ptr;
using std::vector;

namespace google_cloud_debugger_test {

class FrameFixture {
 public:
  // Sets up the various mock calls in the frame based on the
  // argument provided. See function's body comments.
  void SetUpFrame(ICorDebugModuleMock *debug_module,
                  IMetaDataImportMock *metadata_import,
                  ULONG function_virtual_addr, mdMethodDef function_token,
                  const string &function_name, mdTypeDef class_token,
                  const string &class_name) {
    // Sets up the function and the class at this frame.
    frame_func_virtual_addr_ = function_virtual_addr;
    frame_function_token_ = function_token;
    frame_function_name_ = function_name;
    frame_class_name_ = class_name;
    frame_class_token_ = class_token;

    // Sets up mock call so the frame will return the frame_function_
    // object that we set up.
    EXPECT_CALL(frame_, GetFunction(_))
        .WillRepeatedly(
            DoAll(SetArgPointee<0>(&frame_function_), Return(S_OK)));

    // Sets up mock call for the frame function to return debug_module.
    EXPECT_CALL(frame_function_, GetModule(_))
        .WillRepeatedly(DoAll(SetArgPointee<0>(debug_module), Return(S_OK)));

    // Returns frame_function_token_ if GetToken is called.
    EXPECT_CALL(frame_function_, GetToken(_))
        .WillRepeatedly(
            DoAll(SetArgPointee<0>(frame_function_token_), Return(S_OK)));

    // Sets up metadata import for the function at this frame.
    wchar_function_name_ = ConvertStringToWCharPtr(frame_function_name_);
    size_t frame_function_name_len = wchar_function_name_.size();
    EXPECT_CALL(*metadata_import, GetMethodProps(frame_function_token_, _,
                                                 nullptr, 0, _, _, _, _, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(frame_class_token_),
                              SetArgPointee<4>(frame_function_name_len),
                              SetArgPointee<8>(frame_func_virtual_addr_),
                              Return(S_OK)));

    EXPECT_CALL(*metadata_import,
                GetMethodProps(frame_function_token_, _, _,
                               frame_function_name_len, _, _, _, _, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(frame_class_token_),
                              SetArg2ToWcharArray(wchar_function_name_.data(),
                                                  frame_function_name_len),
                              SetArgPointee<4>(frame_function_name_len),
                              SetArgPointee<8>(frame_func_virtual_addr_),
                              Return(S_OK)));

    // Sets up metadata import for the class at this frame.
    wchar_frame_class_name_ = ConvertStringToWCharPtr(frame_class_name_);
    size_t frame_class_name_len = wchar_frame_class_name_.size();
    EXPECT_CALL(*metadata_import,
                GetTypeDefProps(frame_class_token_, nullptr, 0, _, _, _))
        .WillRepeatedly(
            DoAll(SetArgPointee<3>(frame_class_name_len), Return(S_OK)));

    EXPECT_CALL(
        *metadata_import,
        GetTypeDefProps(frame_class_token_, _, frame_class_name_len, _, _, _))
        .WillRepeatedly(
            DoAll(SetArg1ToWcharArray(wchar_frame_class_name_.data(),
                                      frame_class_name_len),
                  SetArgPointee<3>(frame_class_name_len), Return(S_OK)));
  }

  // If is_il_frame is true, sets up this frame as an IL frame
  // with IP offset ip_offset.
  void SetUpILFrame(bool is_il_frame, ULONG32 ip_offset) {
    if (!is_il_frame) {
      ON_CALL(frame_, QueryInterface(__uuidof(ICorDebugILFrame), _))
          .WillByDefault(Return(E_NOINTERFACE));
      return;
    }

    ip_offset_ = ip_offset;
    // Extracts out IL frame from frame.
    ON_CALL(frame_, QueryInterface(__uuidof(ICorDebugILFrame), _))
        .WillByDefault(DoAll(SetArgPointee<1>(&il_frame_), Return(S_OK)));

    // Sets up the instruction pointer (IP) for this IL frame.
    ON_CALL(il_frame_, GetIP(_, _))
        .WillByDefault(DoAll(SetArgPointee<0>(ip_offset_),
                             SetArgPointee<1>(mapping_result_), Return(S_OK)));

    // Sets up the local variables and method arguments enum to return
    // no variables and method arguments to simplify the tests.
    // TODO(quoct): Test those cases in a separate test file for DbgStackFrame.
    ON_CALL(il_frame_, EnumerateLocalVariables(_))
        .WillByDefault(DoAll(SetArgPointee<0>(&local_var_enum_), Return(S_OK)));

    ON_CALL(il_frame_, EnumerateArguments(_))
        .WillByDefault(
            DoAll(SetArgPointee<0>(&method_arg_enum_), Return(S_OK)));

    // Makes the Next function sets arg2 to 0 (which means no value is
    // returned).
    ON_CALL(local_var_enum_, Next(_, _, _))
        .WillByDefault(DoAll(SetArgPointee<2>(0), Return(S_OK)));

    ON_CALL(method_arg_enum_, Next(_, _, _))
        .WillByDefault(DoAll(SetArgPointee<2>(0), Return(S_OK)));
  }

  // Returns full method name (assuming this frame comes from a module
  // with name module_name).
  string GetFullMethodName(const string &module_name) const {
    return module_name + "!" + frame_class_name_ + "." + frame_function_name_;
  }

  // The ICorDebug object that represents the frame.
  ICorDebugFrameMock frame_;

  // The ICorDebug object that represents the IL frame (queried from frame_).
  ICorDebugILFrameMock il_frame_;

  // Function in frame.
  ICorDebugFunctionMock frame_function_;

  // Token of the function the frame is in.
  mdMethodDef frame_function_token_;

  // Virtual address of the frame function.
  ULONG frame_func_virtual_addr_;

  // Name of the frame function.
  string frame_function_name_;

  // Vector of WCHAR of the name of the frame function.
  vector<WCHAR> wchar_function_name_;

  // Token of the class the frame is in.
  mdTypeDef frame_class_token_;

  // String of the class the frame is in.
  string frame_class_name_;

  // Name of the file the frame is in.
  string file_name_;

  // Vector of WCHAR of the name of the class.
  vector<WCHAR> wchar_frame_class_name_;

  // IP Offset in the function that the stack frame is in.
  ULONG32 ip_offset_;

  // Mapping result returned by GetIP function of the IL frame.
  CorDebugMappingResult mapping_result_ = CorDebugMappingResult::MAPPING_EXACT;

  // Local variables at this stack frame.
  ICorDebugValueEnumMock local_var_enum_;

  // Method arguments at this stack frame.
  ICorDebugValueEnumMock method_arg_enum_;
};

// Test Fixture for StackFrameCollection tests.
class StackFrameCollectionTest : public ::testing::Test {
 protected:
  virtual void SetUp() {}

  // Sets up the StackFrameCollection to return 3 frames.
  // Also sets up the ICorDebugModule debug_module_ to be
  // the module that the 3 frames are in.
  virtual void SetUpStackWalk() {
    // Sets up the stack walk to return 3 frames.
    EXPECT_CALL(debug_stack_walk_, GetFrame(_))
        .WillOnce(DoAll(SetArgPointee<0>(&first_frame_.frame_), Return(S_OK)))
        .WillOnce(DoAll(SetArgPointee<0>(&second_frame_.frame_), Return(S_OK)))
        .WillOnce(DoAll(SetArgPointee<0>(&third_frame_.frame_), Return(S_OK)))
        .WillOnce(Return(S_FALSE));

    ULONG func_virtual_addr = 1000;
    mdMethodDef func_token = 2000;
    string func_name = "MyFunction";
    mdTypeDef class_token = 3000;
    string class_name = "MyClass";

    first_frame_.SetUpFrame(&debug_module_, &metadata_import_,
                            func_virtual_addr, func_token, func_name,
                            class_token, class_name);
    first_frame_.SetUpILFrame(true, 500);
    second_frame_.SetUpFrame(
        &debug_module_, &metadata_import_, func_virtual_addr + 1,
        func_token + 1, func_name + "1", class_token + 1, class_name + "1");
    second_frame_.SetUpILFrame(false, 0);
    third_frame_.SetUpFrame(&debug_module_, &metadata_import_,
                            func_virtual_addr + 2, func_token + 2,
                            func_name + "2", class_token + 2, class_name + "2");
    third_frame_.SetUpILFrame(false, 0);

    // Sets up debug_module_ so it will return module_name_
    // when queried.
    ON_CALL(debug_module_, GetMetaDataInterface(IID_IMetaDataImport, _))
        .WillByDefault(
            DoAll(SetArgPointee<1>(&metadata_import_), Return(S_OK)));

    ON_CALL(metadata_import_, QueryInterface(_, _))
        .WillByDefault(
            DoAll(SetArgPointee<1>(&metadata_import_), Return(S_OK)));

    wchar_module_name_ = ConvertStringToWCharPtr(module_name_);
    uint32_t module_name_len = wchar_module_name_.size();

    ON_CALL(debug_module_, GetName(0, _, nullptr))
        .WillByDefault(DoAll(SetArgPointee<1>(module_name_len), Return(S_OK)));

    ON_CALL(debug_module_, GetName(module_name_len, _, _))
        .WillByDefault(DoAll(
            SetArgPointee<1>(module_name_len),
            SetArg2ToWcharArray(wchar_module_name_.data(), module_name_len),
            Return(S_OK)));
  }

  virtual void SetUpPDBFile() {
    // Creates a Portable PDB file, sets up mock calls
    // and pushes it into the pdb_files_.
    unique_ptr<IPortablePdbFileMock> pdb_file =
        unique_ptr<IPortablePdbFileMock>(new IPortablePdbFileMock());
    pdb_file_fixture_.module_name_ = module_name_;
    pdb_file_fixture_.SetUpIPortablePDBFile(pdb_file.get());

    pdb_files_.push_back(std::move(pdb_file));

    MethodInfo method;
    // Method def can just be some random number, not important here.
    method.method_def = 4000;

    // Makes this method the same as the first frame's method by giving
    // them the same function name and virtual address.
    EXPECT_CALL(metadata_import_, GetMethodProps(method.method_def, _, nullptr,
                                                 0, _, _, _, _, _, _))
        .WillRepeatedly(
            DoAll(SetArgPointee<1>(method.method_def),
                  SetArgPointee<4>(first_frame_.wchar_function_name_.size()),
                  SetArgPointee<8>(first_frame_.frame_func_virtual_addr_),
                  Return(S_OK)));

    // Gives method a sequence point that matches the IP Offset of the first
    // frame.
    SequencePoint seq_point;
    seq_point.start_line = 30;
    seq_point.il_offset = first_frame_.ip_offset_;

    method.sequence_points.push_back(seq_point);
    pdb_file_fixture_.first_doc_.methods_.push_back(method);

    // Sets up the name of the file for the first doc.
    first_frame_.file_name_ = "First file";
    pdb_file_fixture_.first_doc_.file_name_ = first_frame_.file_name_;
  }

  // Vector of PDB files that will be fed to the Initialize function
  // of StackFrameCollection.
  std::vector<unique_ptr<google_cloud_debugger_portable_pdb::IPortablePdbFile>>
      pdb_files_;

  // The PDB file fixture for the first PDB file in pdb_files_ vector.
  PortablePDBFileFixture pdb_file_fixture_;

  // Stack walk used by the stack frame collection.
  ICorDebugStackWalkMock debug_stack_walk_;

  // Debug module for the frames.
  ICorDebugModuleMock debug_module_;

  // MetaData from the module above.
  IMetaDataImportMock metadata_import_;

  // Name of the module above.
  string module_name_ = "MyModule";

  // Name of the module in WCHAR.
  vector<WCHAR> wchar_module_name_;

  FrameFixture first_frame_;
  FrameFixture second_frame_;
  FrameFixture third_frame_;
};

// Tests the Initialize function of stack frame collection
// no PDB file matches the module.
TEST_F(StackFrameCollectionTest, TestInitializeWithoutPDBFile) {
  StackFrameCollection stack_frame_collection;
  SetUpStackWalk();
  HRESULT hr =
      stack_frame_collection.Initialize(&debug_stack_walk_, pdb_files_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
}

// Tests the Initialize function of stack frame collection
// when we have a PDB that matches the module.
TEST_F(StackFrameCollectionTest, TestInitializeWithPDBFile) {
  StackFrameCollection stack_frame_collection;
  SetUpStackWalk();
  SetUpPDBFile();
  HRESULT hr =
      stack_frame_collection.Initialize(&debug_stack_walk_, pdb_files_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
}

// Tests the Initialize function of stack frame collection
// when there is an error.
TEST_F(StackFrameCollectionTest, TestInitializeError) {
  // Makes the stack walk returns error.
  {
    EXPECT_CALL(debug_stack_walk_, GetFrame(_))
        .WillRepeatedly(Return(E_ACCESSDENIED));
    StackFrameCollection stack_frame_collection;
    EXPECT_EQ(stack_frame_collection.Initialize(&debug_stack_walk_, pdb_files_),
              E_ACCESSDENIED);
  }

  // Null tests.
  {
    StackFrameCollection stack_frame_collection;
    EXPECT_EQ(stack_frame_collection.Initialize(nullptr, pdb_files_),
              E_INVALIDARG);
  }
}

// Tests the PopulateStackFrames function of stack frame collection.
TEST_F(StackFrameCollectionTest, TestPopulateStackFrames) {
  StackFrameCollection stack_frame_collection;
  SetUpStackWalk();
  SetUpPDBFile();
  HRESULT hr =
      stack_frame_collection.Initialize(&debug_stack_walk_, pdb_files_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  Breakpoint breakpoint;
  IEvalCoordinatorMock eval_coordinator;
  hr = stack_frame_collection.PopulateStackFrames(&breakpoint,
                                                  &eval_coordinator);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  // Should have 3 frames.
  EXPECT_EQ(breakpoint.stack_frames_size(), 3);

  // We set up the stack frame so that the first frame will have line number
  // and file name set (but not the second or third frame).
  StackFrame first_proto_frame = breakpoint.stack_frames(0);
  EXPECT_EQ(first_proto_frame.method_name(),
            first_frame_.GetFullMethodName(module_name_));
  // Path is set based on the first document index's file name.
  EXPECT_EQ(first_proto_frame.location().path(),
            pdb_file_fixture_.first_doc_.file_name_);
  // First frame function corresponds to the first sequence point of the first
  // method of the first document index.
  EXPECT_EQ(
      first_proto_frame.location().line(),
      pdb_file_fixture_.first_doc_.methods_[0].sequence_points[0].start_line);

  // No path or line number set for the second and third frames.
  StackFrame second_proto_frame = breakpoint.stack_frames(1);
  EXPECT_EQ(second_proto_frame.method_name(),
            second_frame_.GetFullMethodName(module_name_));
  EXPECT_EQ(second_proto_frame.location().path(), "");
  EXPECT_EQ(second_proto_frame.location().line(), 0);

  StackFrame third_proto_frame = breakpoint.stack_frames(2);
  EXPECT_EQ(third_proto_frame.method_name(),
            third_frame_.GetFullMethodName(module_name_));
  EXPECT_EQ(third_proto_frame.location().path(), "");
  EXPECT_EQ(third_proto_frame.location().line(), 0);
}

// Tests the error case for PopulateStackFrames function of stack frame
// collection.
TEST_F(StackFrameCollectionTest, TestPopulateStackFramesError) {
  StackFrameCollection stack_frame_collection;
  SetUpStackWalk();
  SetUpPDBFile();
  HRESULT hr =
      stack_frame_collection.Initialize(&debug_stack_walk_, pdb_files_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  Breakpoint breakpoint;
  IEvalCoordinatorMock eval_coordinator;
  EXPECT_EQ(
      stack_frame_collection.PopulateStackFrames(nullptr, &eval_coordinator),
      E_INVALIDARG);
  EXPECT_EQ(stack_frame_collection.PopulateStackFrames(&breakpoint, nullptr),
            E_INVALIDARG);
}

}  // namespace google_cloud_debugger_test
