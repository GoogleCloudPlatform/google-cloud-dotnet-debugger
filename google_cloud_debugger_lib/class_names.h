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

#ifndef CLASS_NAMES_H_
#define CLASS_NAMES_H_

#include <string>

// This file contains .NET class names.

namespace google_cloud_debugger {

// String that represents integral type.
static const std::string kCharClassName = "System.Char";
static const std::string kBooleanClassName = "System.Boolean";
static const std::string kSByteClassName = "System.SByte";
static const std::string kByteClassName = "System.Byte";
static const std::string kInt16ClassName = "System.Int16";
static const std::string kUInt16ClassName = "System.UInt16";
static const std::string kInt32ClassName = "System.Int32";
static const std::string kUInt32ClassName = "System.UInt32";
static const std::string kInt64ClassName = "System.Int64";
static const std::string kUInt64ClassName = "System.UInt64";
static const std::string kSingleClassName = "System.Single";
static const std::string kDoubleClassName = "System.Double";
static const std::string kIntPtrClassName = "System.IntPtr";
static const std::string kUIntPtrClassName = "System.UIntPtr";

// Enum base class.
static const std::string kEnumClassName = "System.Enum";

// String that represents collection classes.
static const std::string kListClassName = "System.Collections.Generic.List`1";
static const std::string kHashSetClassName =
    "System.Collections.Generic.HashSet`1";
static const std::string kDictionaryClassName =
    "System.Collections.Generic.Dictionary`2";

}  // namespace google_cloud_debugger

#endif  //  CLASS_NAMES_H_
