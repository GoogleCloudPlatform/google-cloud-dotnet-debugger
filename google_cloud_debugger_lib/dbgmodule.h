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

#ifndef DBG_MODULE_COLLECTION_H_
#define DBG_MODULE_COLLECTION_H_

#include "cordebug.h"
#include <vector>

namespace google_cloud_debugger {
class DbgModule {
public:
  std::string module_name;

  static DbgModule *GetModule(ICorDebugModule *debug_module) {

  }

private:
  std::vector<DbgModule> module_collection_;

};
}  // namespace google_cloud_debugger

#endif
