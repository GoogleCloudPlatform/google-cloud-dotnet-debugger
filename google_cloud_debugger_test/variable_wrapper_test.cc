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
#include <cstdlib>
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
// This class contains its own variable proto.
class FakeDbgObjectBase : public DbgObject {
 public:
  FakeDbgObjectBase(ICorDebugType *debug_type, int depth)
      : DbgObject(debug_type, depth) {}

  virtual void Initialize(ICorDebugValue *debug_value, BOOL is_null) override {}

  virtual HRESULT PopulateType(Variable *variable) override {
    variable->set_type(type_);
    return S_OK;
  }

  // Type of the object.
  std::string type_ =  "Type" + std::to_string(rand());
 
  // Proto of the object.
  Variable variable_proto_;
};

// Helper class that implements DbgObject.
// This class returns no members.
class FakeDbgObjectValue : public FakeDbgObjectBase {
 public:
  FakeDbgObjectValue(ICorDebugType *debug_type, int depth)
      : FakeDbgObjectBase(debug_type, depth) {}

  virtual HRESULT PopulateValue(Variable *variable) override {
    variable->set_value(value_);
    return S_OK;
  }

  virtual HRESULT PopulateMembers(Variable *variable_proto,
                                  std::vector<VariableWrapper> *members,
                                  IEvalCoordinator *eval_coordinator) override {
    return S_FALSE;
  }

  // Creates a variable wrapper. The proto of the wrapper
  // will be variable_proto_ field of the object.
  static VariableWrapper GetVariableWrapper() {
    shared_ptr<FakeDbgObjectValue> object(new FakeDbgObjectValue(nullptr, 0));
    return VariableWrapper(&object->variable_proto_, object);
  }
  // Value of the object.
  std::string value_ =  "Value" + std::to_string(rand()); 
};

// Helper class that implements DbgObject.
// This class returns members but no value.
class FakeDbgObjectMembers : public FakeDbgObjectBase {
 public:
  FakeDbgObjectMembers(ICorDebugType *debug_type, int depth)
      : FakeDbgObjectBase(debug_type, depth) {}

  virtual HRESULT PopulateValue(Variable *variable) override { return S_FALSE; }

  virtual HRESULT PopulateMembers(Variable *variable_proto,
                                  std::vector<VariableWrapper> *members,
                                  IEvalCoordinator *eval_coordinator) override {
    members->insert(members->begin(), members_.begin(), members_.end());
    return S_OK;
  }

  // Creates a variable wrapper. The proto of the wrapper
  // will be variable_proto_ field of the object.
  static VariableWrapper GetVariableWrapper() {
    shared_ptr<FakeDbgObjectMembers> object(new FakeDbgObjectMembers(nullptr, 0));
    return VariableWrapper(&object->variable_proto_, object);
  }

  // Members of the object.
  vector<VariableWrapper> members_;
};

// Test Fixture for DbgClass.
// Contains various ICorDebug mock objects needed.
class VariableWrapperTest : public ::testing::Test {
 protected:
  VariableWrapper value_wrapper_ = FakeDbgObjectValue::GetVariableWrapper();
  VariableWrapper value_wrapper_2_ = FakeDbgObjectValue::GetVariableWrapper();
  VariableWrapper value_wrapper_3_ = FakeDbgObjectValue::GetVariableWrapper();
  VariableWrapper value_wrapper_4_ = FakeDbgObjectValue::GetVariableWrapper();

  VariableWrapper members_wrapper_ = FakeDbgObjectMembers::GetVariableWrapper();
  VariableWrapper members_wrapper_2_ = FakeDbgObjectMembers::GetVariableWrapper();
  VariableWrapper members_wrapper_3_ = FakeDbgObjectMembers::GetVariableWrapper();

  IEvalCoordinatorMock eval_coordinator_;
};

// Helper function to check the value populated in the variable proto
// of wrapper matches the value of the FakeDbgObjectValue in VariableWrapper.
void CheckValue(VariableWrapper *wrapper) {
  FakeDbgObjectValue *fake_dbg_object = (FakeDbgObjectValue *)wrapper->GetVariableValue().get();
  EXPECT_EQ(wrapper->GetVariableProto()->value(), fake_dbg_object->value_);
}

// Helper function to check the type populated in the variable proto
// of wrapper matches the value of the FakeDbgObjectBase in VariableWrapper.
void CheckType(VariableWrapper *wrapper) {
  FakeDbgObjectBase *fake_dbg_object = (FakeDbgObjectBase *)wrapper->GetVariableValue().get();
  EXPECT_EQ(wrapper->GetVariableProto()->type(), fake_dbg_object->type_);
}

// Helper function to add variable wrapper item to the FakeDbgObjectMembers
// in wrapper container.
void AddMembers(VariableWrapper *container, const VariableWrapper &item) {
  FakeDbgObjectMembers *fake_members_ = (FakeDbgObjectMembers *)container->GetVariableValue().get();
  fake_members_->members_.push_back(item);
}

// Tests the PopulateValue function of VariableWrapper.
TEST_F(VariableWrapperTest, TestPopulateValue) {
  HRESULT hr = value_wrapper_.PopulateValue();
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
  CheckValue(&value_wrapper_);

  // This should return S_FALSE because variable_value_2 returns S_FALSE
  // when PopulateValue is called.
  EXPECT_EQ(members_wrapper_.PopulateValue(), S_FALSE);
}

// Tests the error cases for PopulateValue function of VariableWrapper.
TEST_F(VariableWrapperTest, TestPopulateValueError) {
  VariableWrapper wrapper(nullptr, value_wrapper_.GetVariableValue());
  EXPECT_EQ(wrapper.PopulateValue(), E_INVALIDARG);

  VariableWrapper wrapper_2(value_wrapper_.GetVariableProto(), nullptr);
  EXPECT_EQ(wrapper_2.PopulateValue(), E_INVALIDARG);
}

// Tests the PopulateType function of VariableWrapper.
TEST_F(VariableWrapperTest, TestPopulateType) {
  HRESULT hr = value_wrapper_.PopulateType();
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
  CheckType(&value_wrapper_);
}

// Tests the error cases for PopulateType function of VariableWrapper.
TEST_F(VariableWrapperTest, TestPopulateTypeError) {
  VariableWrapper wrapper(nullptr, value_wrapper_.GetVariableValue());
  EXPECT_EQ(wrapper.PopulateType(), E_INVALIDARG);

  VariableWrapper wrapper_2(value_wrapper_.GetVariableProto(), nullptr);
  EXPECT_EQ(wrapper_2.PopulateType(), E_INVALIDARG);
}

// Tests the PopulateMember function of VariableWrapper.
TEST_F(VariableWrapperTest, TestPopulateMembers) {
  AddMembers(&members_wrapper_, value_wrapper_);
  AddMembers(&members_wrapper_, value_wrapper_2_);

  vector<VariableWrapper> members;
  HRESULT hr = members_wrapper_.PopulateMembers(&members, &eval_coordinator_);

  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;
  EXPECT_EQ(members.size(), 2);
  EXPECT_EQ(members[0].GetVariableProto(), value_wrapper_.GetVariableProto());
  EXPECT_EQ(members[1].GetVariableProto(), value_wrapper_2_.GetVariableProto());
}

// Tests the error cases for PopulateMember function of VariableWrapper.
TEST_F(VariableWrapperTest, TestPopulateMembersError) {
  vector<VariableWrapper> members;

  EXPECT_EQ(members_wrapper_.PopulateMembers(nullptr, &eval_coordinator_), E_INVALIDARG);
  EXPECT_EQ(members_wrapper_.PopulateMembers(&members, nullptr), E_INVALIDARG);
}

// Tests PerformBFS method when there is only 1 item.
TEST_F(VariableWrapperTest, TestBFSOneItem) {
  queue<VariableWrapper> bfs_queue;
  bfs_queue.push(value_wrapper_);
  HRESULT hr = VariableWrapper::PerformBFS(&bfs_queue, &eval_coordinator_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  // BFS should fill up the proto with both value and type.
  CheckType(&value_wrapper_);
  CheckValue(&value_wrapper_);
}

// Tests PerformBFS method when there is 1 item with 2 children.
TEST_F(VariableWrapperTest, TestBFSTwoChildren) {
  AddMembers(&members_wrapper_, value_wrapper_);
  AddMembers(&members_wrapper_, value_wrapper_2_);

  queue<VariableWrapper> bfs_queue;
  bfs_queue.push(members_wrapper_);
  HRESULT hr = VariableWrapper::PerformBFS(&bfs_queue, &eval_coordinator_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  // BFS should fill up the proto with correct type.
  CheckType(&members_wrapper_);

  // Checks that the children are filled up with correct information.
  // BFS should fill up the proto with both value and type.
  CheckType(&value_wrapper_);
  CheckValue(&value_wrapper_);
  CheckType(&value_wrapper_2_);
  CheckValue(&value_wrapper_2_);
}

// Tests PerformBFS method when there is 1 item with 2 children
// and 1 of the children has another 2 children. We will, however,
// sets the BFS level so that the last 2 children won't be evaluated.
TEST_F(VariableWrapperTest, TestBFSTwoLevels) {
  AddMembers(&members_wrapper_, value_wrapper_);
  AddMembers(&members_wrapper_, members_wrapper_2_);
  AddMembers(&members_wrapper_2_, value_wrapper_2_);
  AddMembers(&members_wrapper_2_, value_wrapper_3_);
  members_wrapper_.SetBFSLevel(google_cloud_debugger::kDefaultObjectEvalDepth - 2);

  queue<VariableWrapper> bfs_queue;
  bfs_queue.push(members_wrapper_);
  HRESULT hr = VariableWrapper::PerformBFS(&bfs_queue, &eval_coordinator_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  // BFS should fill up the proto with correct type.
  CheckType(&members_wrapper_);

  // Checks that the children are filled up with correct information.
  // BFS should fill up the proto with both value and type.
  CheckType(&value_wrapper_);
  CheckValue(&value_wrapper_);
  CheckType(&members_wrapper_);

  // The other children should have errors set.
  EXPECT_TRUE(value_wrapper_2_.GetVariableProto()->status().iserror());
  EXPECT_EQ(value_wrapper_2_.GetVariableProto()->status().message(),
      "Object evaluation limit reached");
  EXPECT_TRUE(value_wrapper_3_.GetVariableProto()->status().iserror());
  EXPECT_EQ(value_wrapper_3_.GetVariableProto()->status().message(),
      "Object evaluation limit reached");
}

// Tests PerformBFS method when there is 1 item with 2 children
// and each chilren has 2 children.
TEST_F(VariableWrapperTest, TestBFSThreeLevels) {
  AddMembers(&members_wrapper_, value_wrapper_);
  AddMembers(&members_wrapper_, members_wrapper_2_);
  AddMembers(&members_wrapper_, members_wrapper_3_);
  AddMembers(&members_wrapper_2_, value_wrapper_);
  AddMembers(&members_wrapper_2_, value_wrapper_2_);
  AddMembers(&members_wrapper_3_, value_wrapper_3_);
  AddMembers(&members_wrapper_3_, value_wrapper_4_);

  queue<VariableWrapper> bfs_queue;
  bfs_queue.push(members_wrapper_);
  HRESULT hr = VariableWrapper::PerformBFS(&bfs_queue, &eval_coordinator_);
  EXPECT_TRUE(SUCCEEDED(hr)) << "Failed with hr: " << hr;

  // Checks that BFS fill up everything correctly.
  CheckType(&members_wrapper_);
  CheckType(&members_wrapper_2_);
  CheckType(&members_wrapper_3_);

  CheckType(&value_wrapper_);
  CheckValue(&value_wrapper_);
  CheckType(&value_wrapper_2_);
  CheckValue(&value_wrapper_2_);
  CheckType(&value_wrapper_3_);
  CheckValue(&value_wrapper_3_);
  CheckType(&value_wrapper_4_);
  CheckValue(&value_wrapper_4_);
}

}  // namespace google_cloud_debugger_test
