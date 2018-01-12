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

#ifndef VARIABLE_WRAPPER_H_
#define VARIABLE_WRAPPER_H_

#include <functional>
#include <memory>
#include <queue>
#include <sstream>
#include <string>

#include "breakpoint.pb.h"
#include "constants.h"
#include "cor.h"
#include "cordebug.h"
#include "dbg_object.h"

namespace google_cloud_debugger {

class IEvalCoordinator;

// This wrapper class contains pointers to a variable proto and
// its underlying object. It also contains the BFS level,
// which is used by PopulateStackFrame to stop the BFS when
// it reaches kDefaultObjectEvalDepth.
class VariableWrapper {
public:
  // Constructor that takes in variable proto, the underlying object
  // and the BFS level (default to 1).
  VariableWrapper(google::cloud::diagnostics::debug::Variable *variable_proto,
    std::shared_ptr<DbgObject> variable_value, int bfs_level = 1)
    : variable_proto_(variable_proto),
    variable_value_(variable_value),
    bfs_level_(bfs_level) {}

  // This method is used to process all VariableWrapper in the
  // queue by populating their variable_proto_ with the underlying
  // object variable_value_.
  // Until the queue is empty, this method:
  //  1. Checks if terminate_condition is true. If so, returns.
  //  2. Pops out an item X.
  //  3. If X is null, continues with the loop.
  //  4. If the BFS level of X is kDefaultObjectEvalDepth,
  // sets an error status on X saying that we cannot evaluate
  // its children and continues with the loop.
  //  5. Otherwise, tries to get members (children) of X.
  //  6. If there are members, pushes them into the queue. We
  // also set the BFS level of the members to be the BFS
  // level of the node X + 1. If not, call PopulateValue on X.
  static HRESULT PerformBFS(std::queue<VariableWrapper> *bfs_queue,
                            const std::function<bool()> &terminate_condition,
                            IEvalCoordinator *eval_coordinator);

  // Populates variable proto variable_proto_ with
  // variable_value_ object.
  HRESULT PopulateValue();

  // Populates variable proto variable_proto_ with
  // type of variable_value_ object.
  HRESULT PopulateType();

  // Calls PopulateMembers of variable_value_ object.
  // Pass in variable_proto_ as the parent proto.
  HRESULT PopulateMembers(std::vector<VariableWrapper> *members,
                          IEvalCoordinator *eval_coordinator);

  // Returns the variable proto of this wrapper.
  google::cloud::diagnostics::debug::Variable *GetVariableProto() {
    return variable_proto_;
  }

  // Returns the variable value of this wrapper.
  std::shared_ptr<DbgObject> GetVariableValue() {
    return variable_value_;
  }

  // Sets BFS level.
  void SetBFSLevel(std::int32_t level) {
    bfs_level_ = level;
  }

private:
  // The proto for this variable.
  google::cloud::diagnostics::debug::Variable *variable_proto_;

  // The underlying object that will be used to populate variable proto.
  std::shared_ptr<DbgObject> variable_value_;

  // The BFS level that this variable is at.
  std::int32_t bfs_level_;
};

}  //  namespace google_cloud_debugger

#endif  //  VARIABLE_WRAPPER_H_
 