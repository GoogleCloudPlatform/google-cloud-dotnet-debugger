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

#include "variable_wrapper.h"

#include <iostream>
#include <queue>
#include <vector>

#include "string_stream_wrapper.h"

using google::cloud::diagnostics::debug::Variable;
using std::function;
using std::queue;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;

namespace google_cloud_debugger {

HRESULT VariableWrapper::PerformBFS(queue<VariableWrapper>* bfs_queue,
                                    const function<bool()> &terminate_condition,
                                    IEvalCoordinator *eval_coordinator) {
  if (!bfs_queue) {
    return E_INVALIDARG;
  }

  HRESULT hr;
  // Until the queue is empty, we:
  //  1. Pop out an item X.
  //  2. If X is null, continue with the loop.
  //  3. If the BFS level of X is kDefaultObjectEvalDepth,
  // sets an error status on X saying that we cannot evaluate
  // its children and continue with the loop.
  //  4. Otherwise, try to get members (children) of X.
  //  5. If there are members, push them into the queue. We
  // also set the BFS level of the members to be the BFS
  // level of the node X + 1. If not, call PopulateValue on X.
  while (!bfs_queue->empty()) {
    if (terminate_condition()) {
      return S_OK;
    }

    VariableWrapper current_variable = bfs_queue->front();
    // Populates the type of the variable into the variable proto.
    hr = current_variable.PopulateType();

    if (FAILED(hr)) {
      SetErrorStatusMessage(current_variable.variable_proto_,
                            current_variable.variable_value_->GetErrorString());
      bfs_queue->pop();
      continue;
    }

    if (current_variable.bfs_level_ >= kDefaultObjectEvalDepth) {
      // We have reached a level that is more than the evaluation depth.
      SetErrorStatusMessage(current_variable.variable_proto_,
                            "Object evaluation limit reached");
      bfs_queue->pop();
      continue;
    }

    // If variable is null, moves on.
    if (current_variable.variable_value_->GetIsNull()) {
      bfs_queue->pop();
      continue;
    }

    // Tries to see whether we can get any members (children) from
    // this variable.
    vector<VariableWrapper> variable_members;
    hr = current_variable.PopulateMembers(&variable_members, eval_coordinator);

    // If hr is S_FALSE then there are no members so we simply
    // call PopulateValue.
    if (hr == S_FALSE) {
      hr = current_variable.PopulateValue();
    }
    // Otherwise, process and put the members in the queue.
    else if (SUCCEEDED(hr)) {
      for (auto &member_value : variable_members) {
        member_value.bfs_level_ = current_variable.bfs_level_ + 1;
        bfs_queue->push(member_value);
      }
    }

    if (FAILED(hr)) {
      SetErrorStatusMessage(current_variable.variable_proto_,
                            current_variable.variable_value_->GetErrorString());
    }
    bfs_queue->pop();
  }

  return S_OK;
}

// Populates variable proto variable_proto_ with
// variable_value_ object.
HRESULT VariableWrapper::PopulateValue() {
  if (!variable_proto_ || !variable_value_) {
    return E_INVALIDARG;
  }
  return variable_value_->PopulateValue(variable_proto_);
}

// Populates variable proto variable_proto_ with
// type of variable_value_ object.
HRESULT VariableWrapper::PopulateType() {
  if (!variable_proto_ || !variable_value_) {
    return E_INVALIDARG;
  }
  return variable_value_->PopulateType(variable_proto_);
}

// Calls PopulateMembers of variable_value_ object.
// Pass in variable_proto_ as the parent proto.
HRESULT VariableWrapper::PopulateMembers(std::vector<VariableWrapper> *members,
  IEvalCoordinator *eval_coordinator) {
  if (!variable_proto_ || !variable_value_
    || !members || !eval_coordinator) {
    return E_INVALIDARG;
  }
  return variable_value_->PopulateMembers(variable_proto_,
    members, eval_coordinator);
}

}  //  namespace google_cloud_debugger
