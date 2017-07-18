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

#ifndef DBG_PRIMITIVE_H_
#define DBG_PRIMITIVE_H_

#include <iostream>
#include <type_traits>

#include "ccomptr.h"
#include "dbgobject.h"

namespace google_cloud_debugger {
// Template class for DbgObject of primitive type.
// Primitive type here is defined as pointers and arithmetic types.
// This class will store a copy of the underlying object as value_.
template <typename T>
class DbgPrimitive : public DbgObject {
  // Supported types are char, bool, int8_t, uint8_t,
  // int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t,
  // float, double, intptr_t, uintptr_t.
  static_assert(std::is_fundamental<T>::value, "Fundamental types required.");

 public:
  DbgPrimitive(ICorDebugType *debug_type) : DbgObject(debug_type, 0) {}

  // debug_value can be a null pointer, in which case value_ is just the default
  // value for the type.
  void Initialize(ICorDebugValue *debug_value, BOOL is_null) override {
    CComPtr<ICorDebugType> debug_type;

    debug_type = GetDebugType();
    if (debug_type) {
      initialize_hr_ = debug_type->GetType(&cor_element_type_);

      if (FAILED(initialize_hr_)) {
        WriteError("Failed to extract type from ICorDebugType.");
        return;
      }
    }

    if (debug_value) {
      initialize_hr_ = SetValue(debug_value);
    }
  }

  // Tease out the generic value interface from ICorDebugValue object
  // and get the value from it.
  HRESULT SetValue(ICorDebugValue *debug_value) {
    HRESULT hr;
    CComPtr<ICorDebugGenericValue> generic_value;

    // Tease out the generic value interface so derived class can use it.
    hr = debug_value->QueryInterface(__uuidof(ICorDebugGenericValue),
                                     reinterpret_cast<void **>(&generic_value));
    if (FAILED(hr)) {
      WriteError("Failed to extract generic value from ICorDebugValue.");
      return hr;
    }

    return generic_value->GetValue(&value_);
  }

  // No evaluation is needed!
  HRESULT OutputValue() override {
    if (FAILED(initialize_hr_)) {
      return initialize_hr_;
    }

    WriteOutput("\"" + std::to_string(value_) + "\"");
    return S_OK;
  }

  HRESULT OutputType() override {
    if (cor_element_type_ == ELEMENT_TYPE_I) {
      WriteOutput("System.IntPtr");
    } else if (cor_element_type_ == ELEMENT_TYPE_U) {
      WriteOutput("System.UIntPtr");
    } else {
      PrintTypeCore(value_);
    }
    return S_OK;
  }

 private:
  void PrintTypeCore(char) { WriteOutput("System.Char"); }
  void PrintTypeCore(bool) { WriteOutput("System.Boolean"); }
  void PrintTypeCore(int8_t) { WriteOutput("System.SByte"); }
  void PrintTypeCore(uint8_t) { WriteOutput("System.Byte"); }
  void PrintTypeCore(int16_t) { WriteOutput("System.Int16"); }
  void PrintTypeCore(uint16_t) { WriteOutput("System.UInt16"); }
  void PrintTypeCore(int32_t) { WriteOutput("System.Int32"); }
  void PrintTypeCore(uint32_t) { WriteOutput("System.UInt32"); }
  void PrintTypeCore(int64_t) { WriteOutput("System.Int64"); }
  void PrintTypeCore(uint64_t) { WriteOutput("System.UInt64"); }
  void PrintTypeCore(float) { WriteOutput("System.Single"); }
  void PrintTypeCore(double) { WriteOutput("System.Double"); }

  T value_;
  CorElementType cor_element_type_;
};

}  //  namespace google_cloud_debugger

#endif  // DBG_PRIMITIVE_H_
