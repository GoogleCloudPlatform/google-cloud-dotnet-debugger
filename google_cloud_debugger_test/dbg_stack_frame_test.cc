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
#include "dbg_stack_frame.h"
#include "document_index.h"
#include "i_cor_debug_mocks.h"
#include "i_eval_coordinator_mock.h"
#include "i_metadata_import_mock.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Mock;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using google::cloud::diagnostics::debug::StackFrame;
using google_cloud_debugger::CComPtr;
using google_cloud_debugger::ConvertStringToWCharPtr;
using google_cloud_debugger::DbgStackFrame;
using google_cloud_debugger_portable_pdb::LocalVariableInfo;
using std::string;
using std::vector;

namespace google_cloud_debugger_test {

// Helper class that contains information about
// a local variable or a method argument.
class VariableInfo {
 public:
  // The ICorDebug object that represents the value
  // of this variable.
  ICorDebugGenericValueMock cordebug_value_;

  // Value of this variable.
  int32_t value_;

  // Name of this variable.
  std::string name_;

  // Name of this variable in WCHAR.
  vector<WCHAR> wchar_name_;

  // The position of this local variable.
  uint16_t slot_;
};

// Test Fixture for DbgClass.
// Contains various ICorDebug mock objects needed.
class DbgStackFrameTest : public ::testing::Test {
 protected:
  virtual void SetUp() {}

  // Sets up mock calls to process local variables.
  virtual void SetUpLocalVariables() {
    // Sets up the name, position and value
    // of the variables.
    first_local_var_.slot_ = 0;
    first_local_var_.name_ = "FirstVariable";
    first_local_var_.value_ = 100;

    second_local_var_.slot_ = 1;
    second_local_var_.name_ = "SecondVariable";
    second_local_var_.value_ = 200;

    // Sets up LocalVariableInfo vector, which is used to retrieve
    // name of a variable based on its position.
    local_variables_info_.resize(2);
    local_variables_info_[0].name = first_local_var_.name_;
    local_variables_info_[0].slot = first_local_var_.slot_;
    local_variables_info_[1].name = second_local_var_.name_;
    local_variables_info_[1].slot = second_local_var_.slot_;

    // Sets up the ICorDebugValue that represents the variables.
    local_variables_.resize(2);
    local_variables_[0] = &(first_local_var_.cordebug_value_);
    local_variables_[1] = &(second_local_var_.cordebug_value_);
    SetUpMockGenericValue(&(first_local_var_.cordebug_value_),
                          first_local_var_.value_);
    SetUpMockGenericValue(&(second_local_var_.cordebug_value_),
                          second_local_var_.value_);

    // Makes the ICorDebugValueEnum returns the local variables that we set up.
    EXPECT_CALL(frame_mock_, EnumerateLocalVariables(_))
        .Times(1)
        .WillRepeatedly(
            DoAll(SetArgPointee<0>(&local_var_enum_mock_), Return(S_OK)));

    EXPECT_CALL(local_var_enum_mock_, Next(_, _, _))
        .Times(2)
        .WillOnce(DoAll(SetArrayArgument<1>(local_variables_.begin(),
                                            local_variables_.end()),
                        SetArgPointee<2>(2), Return(S_OK)));
  }

  // Sets up mock calls to process method arguments.
  virtual void SetUpMethodArguments() {
    first_method_arg_.value_ = 1000;
    second_method_arg_.value_ = 2000;

    // Sets up the ICorDebugValue that represents the method arguments.
    method_arguments.resize(2);
    method_arguments[0] = &(first_method_arg_.cordebug_value_);
    method_arguments[1] = &(second_method_arg_.cordebug_value_);
    SetUpMockGenericValue(&(first_method_arg_.cordebug_value_),
                          first_method_arg_.value_);
    SetUpMockGenericValue(&(second_method_arg_.cordebug_value_),
                          second_method_arg_.value_);

    // Makes the ICorDebugValueEnum returns the method arguments that we set up.
    EXPECT_CALL(frame_mock_, EnumerateArguments(_))
        .Times(1)
        .WillRepeatedly(
            DoAll(SetArgPointee<0>(&method_arg_enum_mock_), Return(S_OK)));

    EXPECT_CALL(method_arg_enum_mock_, Next(_, _, _))
        .Times(2)
        .WillOnce(DoAll(SetArrayArgument<1>(method_arguments.begin(),
                                            method_arguments.end()),
                        SetArgPointee<2>(2), Return(S_OK)));
  }

  // Sets up mock call for MetaDataImport.
  virtual void SetUpMetaDataImport() {
    // Makes this method static.
    EXPECT_CALL(metadata_import_,
                GetMethodProps(method_token_, _, nullptr, 0, _, _, _, _, _, _))
        .Times(1)
        .WillRepeatedly(
            DoAll(SetArgPointee<5>(CorMethodAttr::mdStatic), Return(S_OK)));

    // Makes sure the MetaDataImport enumerate the parameter tokens in this
    // fixture.
    EXPECT_CALL(metadata_import_, EnumParams(_, method_token_, _, _, _))
        .Times(2)
        .WillOnce(DoAll(SetArrayArgument<2>(params_tokens_, params_tokens_ + 2),
                        SetArgPointee<4>(2), Return(S_OK)));

    // Sets up the mock calls so the first parameter token will correspond with
    // the first method argument's name.
    first_method_arg_.name_ = "FirstMethodArg";
    first_method_arg_.wchar_name_ =
        ConvertStringToWCharPtr(first_method_arg_.name_);
    uint32_t first_method_arg_name_len = first_method_arg_.wchar_name_.size();

    ON_CALL(metadata_import_,
            GetParamProps(params_tokens_[0], _, _, nullptr, 0, _, _, _, _, _))
        .WillByDefault(
            DoAll(SetArgPointee<5>(first_method_arg_name_len), Return(S_OK)));

    ON_CALL(metadata_import_,
            GetParamProps(params_tokens_[0], _, _, _, first_method_arg_name_len,
                          _, _, _, _, _))
        .WillByDefault(
            DoAll(SetArg3ToWcharArray(first_method_arg_.wchar_name_.data(),
                                      first_method_arg_name_len),
                  SetArgPointee<5>(first_method_arg_name_len), Return(S_OK)));

    // Sets up the mock calls so the second parameter token will correspond with
    // the second method argument's name.
    second_method_arg_.name_ = "SecondMethodArg";
    second_method_arg_.wchar_name_ =
        ConvertStringToWCharPtr(second_method_arg_.name_);
    uint32_t second_method_arg_name_len = second_method_arg_.wchar_name_.size();

    ON_CALL(metadata_import_,
            GetParamProps(params_tokens_[1], _, _, nullptr, 0, _, _, _, _, _))
        .WillByDefault(
            DoAll(SetArgPointee<5>(second_method_arg_name_len), Return(S_OK)));

    ON_CALL(metadata_import_,
            GetParamProps(params_tokens_[1], _, _, _,
                          second_method_arg_name_len, _, _, _, _, _))
        .WillByDefault(
            DoAll(SetArg3ToWcharArray(second_method_arg_.wchar_name_.data(),
                                      second_method_arg_name_len),
                  SetArgPointee<5>(second_method_arg_name_len), Return(S_OK)));
  }

  // ICorDebugILFrame used by this test to extract variables and arguments.
  ICorDebugILFrameMock frame_mock_;

  // ICorDebugEnum of the local variables.
  ICorDebugValueEnumMock local_var_enum_mock_;

  // Variables helper objects that contain the necessary
  // information for the local variables and the method arguments.
  VariableInfo first_local_var_;
  VariableInfo second_local_var_;
  VariableInfo first_method_arg_;
  VariableInfo second_method_arg_;

  // Local variables and method arguments returned by the
  // ICorDebugValueEnum.
  std::vector<CComPtr<ICorDebugValue>> local_variables_;
  std::vector<CComPtr<ICorDebugValue>> method_arguments;

  // LocalVariableInfo vector that is used to extract local variables' names.
  std::vector<LocalVariableInfo> local_variables_info_;

  // MetaDataImport used by the test.
  IMetaDataImportMock metadata_import_;

  // Token that represents the method the frame is in.
  mdMethodDef method_token_ = 10;

  // ICorDebugEnum of the method arguments.
  ICorDebugValueEnumMock method_arg_enum_mock_;

  // Tokens that correspond with the method arguments of this stack frame.
  mdParamDef params_tokens_[2] = {50, 60};

  // EvalCoordinator used by the frame to evaluate local variables and method
  // arguments.
  IEvalCoordinatorMock eval_coordinator_;
};

// Tests the Initialize function of DbgStackFrame.
TEST_F(DbgStackFrameTest, TestInitialize) {
  DbgStackFrame stack_frame;

  SetUpLocalVariables();
  SetUpMethodArguments();
  SetUpMetaDataImport();

  HRESULT hr = stack_frame.Initialize(&frame_mock_, local_variables_info_,
                                      method_token_, &metadata_import_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
}

// Tests the error case of Initialize function of DbgStackFrame
// when the arguments are null.
TEST_F(DbgStackFrameTest, TestInitializeNullError) {
  DbgStackFrame stack_frame;

  HRESULT hr = stack_frame.Initialize(nullptr, local_variables_info_,
                                      method_token_, &metadata_import_);
  EXPECT_EQ(hr, E_INVALIDARG);

  hr = stack_frame.Initialize(&frame_mock_, local_variables_info_,
                              method_token_, nullptr);
  EXPECT_EQ(hr, E_INVALIDARG);
}

// Tests the error case of Initialize function of DbgStackFrame
// when we cannot enumerate local variables.
TEST_F(DbgStackFrameTest, TestInitializeEnumerateLocalVariableError) {
  DbgStackFrame stack_frame;

  EXPECT_CALL(frame_mock_, EnumerateLocalVariables(_))
      .WillRepeatedly(Return(CORDBG_E_ENC_METHOD_NO_LOCAL_SIG));

  HRESULT hr = stack_frame.Initialize(&frame_mock_, local_variables_info_,
                                      method_token_, &metadata_import_);
  EXPECT_EQ(hr, CORDBG_E_ENC_METHOD_NO_LOCAL_SIG);
}

// Tests the error case of Initialize function of DbgStackFrame
// when we cannot enumerate method arguments.
TEST_F(DbgStackFrameTest, TestInitializeEnumerateArgumentsError) {
  DbgStackFrame stack_frame;

  SetUpLocalVariables();
  EXPECT_CALL(frame_mock_, EnumerateArguments(_))
      .WillRepeatedly(Return(CORDBG_E_BAD_THREAD_STATE));

  HRESULT hr = stack_frame.Initialize(&frame_mock_, local_variables_info_,
                                      method_token_, &metadata_import_);
  EXPECT_EQ(hr, CORDBG_E_BAD_THREAD_STATE);
}

// Tests the PopulateStackFrame function of DbgStackFrame.
TEST_F(DbgStackFrameTest, TestPopulateStackFrame) {
  DbgStackFrame stack_frame;
  StackFrame proto_stack_frame;

  SetUpLocalVariables();
  SetUpMethodArguments();
  SetUpMetaDataImport();

  HRESULT hr = stack_frame.Initialize(&frame_mock_, local_variables_info_,
                                      method_token_, &metadata_import_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  hr = stack_frame.PopulateStackFrame(&proto_stack_frame, &eval_coordinator_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  // Checks that the frame has the correct local variables.
  EXPECT_EQ(proto_stack_frame.locals().size(), 2);
  EXPECT_EQ(proto_stack_frame.locals(0).name(), first_local_var_.name_);
  EXPECT_EQ(proto_stack_frame.locals(0).value(),
            std::to_string(first_local_var_.value_));
  EXPECT_EQ(proto_stack_frame.locals(1).name(), second_local_var_.name_);
  EXPECT_EQ(proto_stack_frame.locals(1).value(),
            std::to_string(second_local_var_.value_));

  // Checks that the frame has the correct method arguments.
  EXPECT_EQ(proto_stack_frame.arguments().size(), 2);
  EXPECT_EQ(proto_stack_frame.arguments(0).name(), first_method_arg_.name_);
  EXPECT_EQ(proto_stack_frame.arguments(0).value(),
            std::to_string(first_method_arg_.value_));
  EXPECT_EQ(proto_stack_frame.arguments(1).name(), second_method_arg_.name_);
  EXPECT_EQ(proto_stack_frame.arguments(1).value(),
            std::to_string(second_method_arg_.value_));
}

// Tests the PopulateStackFrame function of DbgStackFrame
// when we don't have a variable's information.
TEST_F(DbgStackFrameTest, TestPopulateStackFrameSparse) {
  DbgStackFrame stack_frame;
  StackFrame proto_stack_frame;

  SetUpLocalVariables();
  SetUpMethodArguments();
  SetUpMetaDataImport();

  // Pops out a variable info so we won't get name
  // for the second varaible.
  local_variables_info_.pop_back();

  HRESULT hr = stack_frame.Initialize(&frame_mock_, local_variables_info_,
                                      method_token_, &metadata_import_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  hr = stack_frame.PopulateStackFrame(&proto_stack_frame, &eval_coordinator_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  // Checks that the frame has the correct local variables.
  EXPECT_EQ(proto_stack_frame.locals().size(), 2);
  EXPECT_EQ(proto_stack_frame.locals(0).name(), first_local_var_.name_);
  EXPECT_EQ(proto_stack_frame.locals(0).value(),
            std::to_string(first_local_var_.value_));
  // Name of the second variable should be variable_1 since
  // there is no corresponding LocalVariableInfo for it.
  EXPECT_EQ(proto_stack_frame.locals(1).name(), "variable_1");
  EXPECT_EQ(proto_stack_frame.locals(1).value(),
            std::to_string(second_local_var_.value_));

  // Checks that the frame has the correct method arguments.
  EXPECT_EQ(proto_stack_frame.arguments().size(), 2);
  EXPECT_EQ(proto_stack_frame.arguments(0).name(), first_method_arg_.name_);
  EXPECT_EQ(proto_stack_frame.arguments(0).value(),
            std::to_string(first_method_arg_.value_));
  EXPECT_EQ(proto_stack_frame.arguments(1).name(), second_method_arg_.name_);
  EXPECT_EQ(proto_stack_frame.arguments(1).value(),
            std::to_string(second_method_arg_.value_));
}

// Tests the error case of DbgStackFrame when arguments are null.
TEST_F(DbgStackFrameTest, TestPopulateStackFrameError) {
  DbgStackFrame stack_frame;
  StackFrame proto_stack_frame;

  SetUpLocalVariables();
  SetUpMethodArguments();
  SetUpMetaDataImport();

  HRESULT hr = stack_frame.Initialize(&frame_mock_, local_variables_info_,
                                      method_token_, &metadata_import_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  EXPECT_EQ(stack_frame.PopulateStackFrame(nullptr, &eval_coordinator_),
            E_INVALIDARG);
  EXPECT_EQ(stack_frame.PopulateStackFrame(&proto_stack_frame, nullptr),
            E_INVALIDARG);
}

}  // namespace google_cloud_debugger_test
