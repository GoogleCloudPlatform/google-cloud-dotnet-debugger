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
#include "class_names.h"
#include "dbg_object.h"

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

  DbgPrimitive(T value) : DbgObject(nullptr, 0) {
    value_ = value;
    SetCorElementType(value);
  }

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
    if (!debug_value) {
      return E_INVALIDARG;
    }

    HRESULT hr;
    CComPtr<ICorDebugGenericValue> generic_value;

    // Tease out the generic value interface so derived class can use it.
    hr = debug_value->QueryInterface(__uuidof(ICorDebugGenericValue),
                                     reinterpret_cast<void **>(&generic_value));
    if (FAILED(hr)) {
      WriteError("Failed to extract generic value from ICorDebugValue.");
      return hr;
    }

    SetCorElementType(value_);
    return generic_value->GetValue(&value_);
  }

  // Returns the primitive value stored.
  T GetValue() { return value_; }

  // No evaluation is needed!
  HRESULT PopulateValue(
      google::cloud::diagnostics::debug::Variable *variable) override {
    if (FAILED(initialize_hr_)) {
      return initialize_hr_;
    }

    if (!variable) {
      return E_INVALIDARG;
    }

    variable->set_value(std::to_string(value_));
    return S_OK;
  }

  HRESULT GetTypeString(
      std::string *type_string) override {
    if (!type_string) {
      return E_INVALIDARG;
    }

    if (cor_element_type_ == ELEMENT_TYPE_I) {
      *type_string = kIntPtrClassName;
    } else if (cor_element_type_ == ELEMENT_TYPE_U) {
      *type_string = kUIntPtrClassName;
    } else {
      *type_string = GetTypeCore(value_);
    }
    return S_OK;
  }

  // Creates a value in memory that corresponds with value_
  // and returns an ICorDebugValue for that value.
  HRESULT GetICorDebugValue(ICorDebugValue **debug_value,
    ICorDebugEval *debug_eval) override {
    if (debug_eval == nullptr) {
      return E_INVALIDARG;
    }

    HRESULT hr = debug_eval->CreateValue(element_type_, nullptr,
        debug_value);
    if (FAILED(hr)) {
      return hr;
    }

    CComPtr<ICorDebugGenericValue> generic_value;
    hr = (*debug_value)->QueryInterface(__uuidof(ICorDebugGenericValue),
      reinterpret_cast<void **>(&generic_value));
    if (FAILED(hr)) {
      return hr;
    }

    return generic_value->SetValue(&value_);
  }

 private:
  const std::string GetTypeCore(char value) {
    return kCharClassName;
  }
  const std::string GetTypeCore(bool value) {
    return kBooleanClassName;
  }
  const std::string GetTypeCore(std::int8_t) {
    return kSByteClassName;
  }
  const std::string GetTypeCore(std::uint8_t) {
    return kByteClassName;
  }
  const std::string GetTypeCore(std::int16_t) {
    return kInt16ClassName;
  }
  const std::string GetTypeCore(std::uint16_t) {
    return kUInt16ClassName;
  }
  const std::string GetTypeCore(std::int32_t) {
    return kInt32ClassName;
  }
  const std::string GetTypeCore(std::uint32_t) {
    return kUInt32ClassName;
  }
  const std::string GetTypeCore(std::int64_t) {
    return kInt64ClassName;
  }
  const std::string GetTypeCore(std::uint64_t) {
    return kUInt64ClassName;
  }
  const std::string GetTypeCore(std::float_t) {
    return kSingleClassName;
  }
  const std::string GetTypeCore(std::double_t) {
    return kDoubleClassName;
  }

  void SetCorElementType(char value) {
    cor_element_type_ = CorElementType::ELEMENT_TYPE_CHAR;
  }
  void SetCorElementType(bool value) {
    cor_element_type_ = CorElementType::ELEMENT_TYPE_BOOLEAN;
  }
  void SetCorElementType(std::int8_t value) {
    cor_element_type_ = CorElementType::ELEMENT_TYPE_I1;
  }
  void SetCorElementType(std::uint8_t value) {
    cor_element_type_ = CorElementType::ELEMENT_TYPE_U1;
  }
  void SetCorElementType(std::int16_t value) {
    cor_element_type_ = CorElementType::ELEMENT_TYPE_I2;
  }
  void SetCorElementType(std::uint16_t) {
    cor_element_type_ = CorElementType::ELEMENT_TYPE_U2;
  }
  void SetCorElementType(std::int32_t value) {
    cor_element_type_ = CorElementType::ELEMENT_TYPE_I4;
  }
  void SetCorElementType(std::uint32_t) {
    cor_element_type_ = CorElementType::ELEMENT_TYPE_U4;
  }
  void SetCorElementType(std::int64_t value) {
    cor_element_type_ = CorElementType::ELEMENT_TYPE_I8;
  }
  void SetCorElementType(std::uint64_t value) {
    cor_element_type_ = CorElementType::ELEMENT_TYPE_U8;
  }
  void SetCorElementType(std::float_t value) {
    cor_element_type_ = CorElementType::ELEMENT_TYPE_R4;
  }
  void SetCorElementType(std::double_t value) {
    cor_element_type_ = CorElementType::ELEMENT_TYPE_R8;
  }

  T value_;
};

}  //  namespace google_cloud_debugger

#endif  // DBG_PRIMITIVE_H_
