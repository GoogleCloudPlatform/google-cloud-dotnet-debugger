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
#include <queue>
#include <vector>

#include "breakpoint.pb.h"
#include "constants.h"
#include "cor.h"
#include "cordebug.h"
#include "dbg_object.h"
#include "i_eval_coordinator_mock.h"
#include "variable_wrapper.h"
#include "winerror.h"

using google::cloud::diagnostics::debug::Variable;
using google_cloud_debugger::DbgObject;
using google_cloud_debugger::IEvalCoordinator;
using google_cloud_debugger::VariableWrapper;
using std::queue;
using std::shared_ptr;
using std::string;
using std::vector;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Mock;
using ::testing::Return;
using ::testing::SetArgPointee;

namespace google_cloud_debugger_test {

// Helper class that implements DbgObject.
// This class returns no members.
class DbgObjectValue : public DbgObject {
 public:
  DbgObjectValue(ICorDebugType *debug_type, int depth)
      : DbgObject(debug_type, depth) {}

  virtual void Initialize(ICorDebugValue *debug_value, BOOL is_null) override {}

  virtual HRESULT PopulateType(Variable *variable) override {
    variable->set_type(type_);
    return S_OK;
  }

  virtual HRESULT PopulateValue(Variable *variable) override {
    variable->set_value(value_);
    return S_OK;
  }

  virtual HRESULT PopulateMembers(Variable *variable_proto,
                                  std::vector<VariableWrapper> *members,
                                  IEvalCoordinator *eval_coordinator) override {
    return S_FALSE;
  }

  // Type of the object.
  std::string type_;

  // Value of the object.
  std::string value_;
};

// Helper class that implements DbgObject.
// This class returns members but no value.
class DbgObjectMembers : public DbgObject {
 public:
  DbgObjectMembers(ICorDebugType *debug_type, int depth)
      : DbgObject(debug_type, depth) {}

  virtual void Initialize(ICorDebugValue *debug_value, BOOL is_null) override {}

  virtual HRESULT PopulateType(Variable *variable) override {
    variable->set_type(type_);
    return S_OK;
  }

  virtual HRESULT PopulateValue(Variable *variable) override { return S_FALSE; }

  //
  virtual HRESULT PopulateMembers(Variable *variable_proto,
                                  std::vector<VariableWrapper> *members,
                                  IEvalCoordinator *eval_coordinator) override {
    members->insert(members->begin(), members_.begin(), members_.end());
    return S_OK;
  }

  // Type of the object.
  std::string type_;

  // Members of the object.
  vector<VariableWrapper> members_;
};

// Test Fixture for DbgClass.
// Contains various ICorDebug mock objects needed.
class VariableWrapperTest : public ::testing::Test {
 protected:
  void SetUp() {
    value_object_->value_ = value_object_value_;
    value_object_->type_ = value_object_type_;
    value_object_2_->value_ = value_object_2_value_;
    value_object_2_->type_ = value_object_2_type_;
    value_object_3_->value_ = value_object_3_value_;
    value_object_3_->type_ = value_object_3_type_;
    member_object_->type_ = member_object_type_;
    member_object_2_->type_ = member_object_2_type_;
  }

  // DbgObject that returns value.
  shared_ptr<DbgObjectValue> value_object_ =
      shared_ptr<DbgObjectValue>(new DbgObjectValue(nullptr, 0));

  // Variable Proto that accompanies value_object_.
  Variable value_object_proto_;

  // Value of the value_object_.
  string value_object_value_ = "Value";

  // Type of the value_object_.
  string value_object_type_ = "Type";

  // DbgObject that returns value.
  shared_ptr<DbgObjectValue> value_object_2_ =
      shared_ptr<DbgObjectValue>(new DbgObjectValue(nullptr, 0));

  // Variable Proto that accompanies value_object_2.
  Variable value_object_2_proto_;

  // Value of the value_object_2_.
  string value_object_2_value_ = "Value2";

  // Type of the value_object_2_.
  string value_object_2_type_ = "Type2";

  // DbgObject that returns value.
  shared_ptr<DbgObjectValue> value_object_3_ =
      shared_ptr<DbgObjectValue>(new DbgObjectValue(nullptr, 0));

  // Variable Proto that accompanies value_object_3_.
  Variable value_object_3_proto_;

  // Value of the value_object_3_.
  string value_object_3_value_ = "Value3";

  // Type of the value_object_3_.
  string value_object_3_type_ = "Type3";

  // DbgObject that returns members.
  shared_ptr<DbgObjectMembers> member_object_ =
      shared_ptr<DbgObjectMembers>(new DbgObjectMembers(nullptr, 0));

  // Variable Proto that accompanies member_object_.
  Variable member_object_proto_;

  // Type of the member_object_.
  string member_object_type_ = "Type4";

  // DbgObject that returns members.
  shared_ptr<DbgObjectMembers> member_object_2_ =
      shared_ptr<DbgObjectMembers>(new DbgObjectMembers(nullptr, 0));

  // Variable Proto that accompanies member_object_2_.
  Variable member_object_2_proto_;

  // Type of the member_object_2_.
  string member_object_2_type_ = "Type5";

  IEvalCoordinatorMock eval_coordinator_;
};

// Tests the PopulateValue function of VariableWrapper.
TEST_F(VariableWrapperTest, TestPopulateValue) {
  VariableWrapper wrapper(&value_object_proto_, value_object_);

  HRESULT hr = wrapper.PopulateValue();
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
  EXPECT_EQ(value_object_proto_.value(), value_object_value_);

  VariableWrapper wrapper_2(&member_object_proto_, member_object_);

  // This should return S_FALSE because variable_value_2 returns S_FALSE
  // when PopulateValue is called.
  EXPECT_EQ(wrapper_2.PopulateValue(), S_FALSE);
}

// Tests the error cases for PopulateValue function of VariableWrapper.
TEST_F(VariableWrapperTest, TestPopulateValueError) {
  VariableWrapper wrapper(nullptr, value_object_);
  EXPECT_EQ(wrapper.PopulateValue(), E_INVALIDARG);

  VariableWrapper wrapper_2(&value_object_proto_, nullptr);
  EXPECT_EQ(wrapper_2.PopulateValue(), E_INVALIDARG);
}

// Tests the PopulateType function of VariableWrapper.
TEST_F(VariableWrapperTest, TestPopulateType) {
  VariableWrapper wrapper(&value_object_proto_, value_object_);

  HRESULT hr = wrapper.PopulateType();
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
  EXPECT_EQ(value_object_proto_.type(), value_object_type_);
}

// Tests the error cases for PopulateValue function of VariableWrapper.
TEST_F(VariableWrapperTest, TestPopulateTypeError) {
  VariableWrapper wrapper(nullptr, value_object_);
  EXPECT_EQ(wrapper.PopulateType(), E_INVALIDARG);

  VariableWrapper wrapper_2(&value_object_proto_, nullptr);
  EXPECT_EQ(wrapper_2.PopulateType(), E_INVALIDARG);
}

// Tests the error cases for PopulateValue function of VariableWrapper.
TEST_F(VariableWrapperTest, TestPopulateMembers) {
  member_object_->members_.push_back(
      VariableWrapper(&value_object_proto_, value_object_));
  member_object_->members_.push_back(
      VariableWrapper(&value_object_2_proto_, value_object_2_));
  VariableWrapper member_wrapper(&member_object_proto_, member_object_);

  vector<VariableWrapper> members;
  HRESULT hr = member_wrapper.PopulateMembers(&members, &eval_coordinator_);

  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
  EXPECT_EQ(members.size(), 2);
  EXPECT_EQ(members[0].GetVariableProto(), &value_object_proto_);
  EXPECT_EQ(members[1].GetVariableProto(), &value_object_2_proto_);
}

// Tests the error cases for PopulateValue function of VariableWrapper.
TEST_F(VariableWrapperTest, TestPopulateMemberseError) {
  VariableWrapper wrapper(&member_object_proto_, member_object_);
  vector<VariableWrapper> members;

  EXPECT_EQ(wrapper.PopulateMembers(nullptr, &eval_coordinator_), E_INVALIDARG);
  EXPECT_EQ(wrapper.PopulateMembers(&members, nullptr), E_INVALIDARG);
}

// Tests PerformBFS method when there is only 1 item.
TEST_F(VariableWrapperTest, TestBFSOneItem) {
  VariableWrapper wrapper(&value_object_proto_, value_object_);

  queue<VariableWrapper> bfs_queue;
  bfs_queue.push(wrapper);
  HRESULT hr = VariableWrapper::PerformBFS(&bfs_queue, &eval_coordinator_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  // BFS should fill up the proto with both value and type.
  EXPECT_EQ(value_object_proto_.value(), value_object_value_);
  EXPECT_EQ(value_object_proto_.type(), value_object_type_);
}

// Tests PerformBFS method when there is 1 item with 2 children.
TEST_F(VariableWrapperTest, TestBFSTwoChildren) {
  member_object_->members_.push_back(
      VariableWrapper(&value_object_proto_, value_object_));
  member_object_->members_.push_back(
      VariableWrapper(&value_object_2_proto_, value_object_2_));
  VariableWrapper member_wrapper(&member_object_proto_, member_object_);

  queue<VariableWrapper> bfs_queue;
  bfs_queue.push(member_wrapper);
  HRESULT hr = VariableWrapper::PerformBFS(&bfs_queue, &eval_coordinator_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  // BFS should fill up the proto with correct type.
  EXPECT_EQ(member_object_proto_.type(), member_object_type_);

  // Checks that the children are filled up with correct information.
  // BFS should fill up the proto with both value and type.
  EXPECT_EQ(value_object_proto_.value(), value_object_value_);
  EXPECT_EQ(value_object_proto_.type(), value_object_type_);
  EXPECT_EQ(value_object_2_proto_.value(), value_object_2_value_);
  EXPECT_EQ(value_object_2_proto_.type(), value_object_2_type_);
}

// Tests PerformBFS method when there is 1 item with 2 children
// and 1 of the children has another 2 children. We will, however,
// sets the BFS level so that the last 2 children won't be evaluated.
TEST_F(VariableWrapperTest, TestBFSTwoLevels) {
  member_object_->members_.push_back(
      VariableWrapper(&value_object_proto_, value_object_));
  member_object_->members_.push_back(
      VariableWrapper(&member_object_2_proto_, member_object_2_));
  member_object_2_->members_.push_back(
      VariableWrapper(&value_object_2_proto_, value_object_2_));
  member_object_2_->members_.push_back(
      VariableWrapper(&value_object_3_proto_, value_object_3_));
  VariableWrapper member_wrapper(&member_object_proto_, member_object_,
      google_cloud_debugger::kDefaultObjectEvalDepth - 2);

  queue<VariableWrapper> bfs_queue;
  bfs_queue.push(member_wrapper);
  HRESULT hr = VariableWrapper::PerformBFS(&bfs_queue, &eval_coordinator_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  // BFS should fill up the proto with correct type.
  EXPECT_EQ(member_object_proto_.type(), member_object_type_);

  // Checks that the children are filled up with correct information.
  // BFS should fill up the proto with both value and type.
  EXPECT_EQ(value_object_proto_.value(), value_object_value_);
  EXPECT_EQ(value_object_proto_.type(), value_object_type_);
  EXPECT_EQ(member_object_2_proto_.type(), member_object_2_type_);

  // The other children should have errors set.
  EXPECT_TRUE(value_object_2_proto_.status().iserror(), true);
  EXPECT_EQ(value_object_2_proto_.status().message(),
      "Object evaluation limit reached");
  EXPECT_TRUE(value_object_3_proto_.status().iserror(), true);
  EXPECT_EQ(value_object_3_proto_.status().message(),
      "Object evaluation limit reached");
}

}  // namespace google_cloud_debugger_test
