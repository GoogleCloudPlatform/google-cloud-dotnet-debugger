// Copyright 2018 Google Inc. All Rights Reserved.
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

#ifndef METHOD_INFO_H_
#define METHOD_INFO_H_

#include <string>
#include <vector>

#include "cor.h"
#include "cordebug.h"

namespace google_cloud_debugger {

class DbgStackFrame;

// Utility class that contains the properties
// of a function needed to perform a method call.
struct MethodInfo {
 public:
  // The name of the function.
  std::string method_name;

  // The metadata token of the method.
  mdMethodDef method_token;

  // The types of the arguments.
  const std::vector<std::string> argument_types;

  // True if the function is static.
  bool is_static;

  // True if the method has generic types.
  bool has_generic_types;

  // Given a class represented by class_token and the
  // MetaDataImport of the module that this class is in, this function
  // will try to find in this class the method that has the name
  // method_name and arguments whose types fit argument_types.
  // Once the method is found, the function will also populate
  // is_static and has_generic_types field.
  HRESULT PopulateMethodDefFromNameAndArguments(
      IMetaDataImport *metadata_import, const mdTypeDef &class_token,
      DbgStackFrame *stack_frame);

 private:
  // Helper function to find all methods that matches the name
  // method_name.
  HRESULT GetMethodDefsFromName(IMetaDataImport *metadata_import,
                                const mdTypeDef &class_token,
                                std::vector<mdMethodDef> *methods_matched,
                                std::ostream *err_stream);
};

}  //  namespace google_cloud_debugger

#endif  //  METHOD_INFO_H_
