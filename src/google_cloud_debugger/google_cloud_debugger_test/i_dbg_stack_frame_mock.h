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

#ifndef I_DBG_STACK_FRAME_MOCK_H_
#define I_DBG_STACK_FRAME_MOCK_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

#include "dbg_class_property.h"
#include "i_dbg_stack_frame.h"

namespace google_cloud_debugger_test {

// Mock for IPortablePdbFile
class IDbgStackFrameMock : public google_cloud_debugger::IDbgStackFrame {
 public:
  HRESULT GetPropertyFromFrame(
      const std::string &property_name,
      std::unique_ptr<google_cloud_debugger::DbgClassProperty> *result_obj,
      std::ostream *err_stream) override {
    google_cloud_debugger::DbgClassProperty *class_property;
    HRESULT hr =
        GetPropertyFromFrameHelper(property_name, &class_property, err_stream);
    if (FAILED(hr)) {
      return hr;
    }

    *result_obj = std::unique_ptr<google_cloud_debugger::DbgClassProperty>(
        class_property);
    return hr;
  };

  HRESULT GetPropertyFromClass(
      const mdTypeDef &class_token,
      const std::string &property_name,
      std::unique_ptr<google_cloud_debugger::DbgClassProperty>
          *result_obj,
      const std::vector<google_cloud_debugger::TypeSignature>
          &generic_signatures,
      IMetaDataImport *metadata_import,
      std::ostream *err_stream) override {
    google_cloud_debugger::DbgClassProperty *class_property;
    HRESULT hr =
        GetPropertyFromClassHelper(class_token, property_name, &class_property,
          generic_signatures, metadata_import, err_stream);
    if (FAILED(hr)) {
      return hr;
    }

    *result_obj = std::unique_ptr<google_cloud_debugger::DbgClassProperty>(
        class_property);
    return hr;
  };

  MOCK_METHOD3(
      GetLocalVariable,
      HRESULT(const std::string &variable_name,
              std::shared_ptr<google_cloud_debugger::DbgObject> *dbg_object,
              std::ostream *err_stream));
  MOCK_METHOD4(
      GetFieldAndAutoPropFromFrame,
      HRESULT(const std::string &member_name,
              std::shared_ptr<google_cloud_debugger::DbgObject> *dbg_object,
              ICorDebugILFrame *debug_frame, std::ostream *err_stream));
  MOCK_METHOD3(
      GetPropertyFromFrameHelper,
      HRESULT(const std::string &property_name,
              google_cloud_debugger::DbgClassProperty **property_object,
              std::ostream *err_stream));
  MOCK_METHOD8(GetFieldFromClass,
               HRESULT(const mdTypeDef &class_token,
                       const std::string &field_name, mdFieldDef *field_def,
                       bool *is_static,
                       google_cloud_debugger::TypeSignature *type_signature,
                       const std::vector<google_cloud_debugger::TypeSignature>
                           &generic_signatures,
                       IMetaDataImport *metadata_import,
                       std::ostream *err_stream));
  MOCK_METHOD6(GetPropertyFromClassHelper,
               HRESULT(const mdTypeDef &class_token,
                       const std::string &property_name,
                       google_cloud_debugger::DbgClassProperty **class_property,
                       const std::vector<google_cloud_debugger::TypeSignature>
                           &generic_signatures,
                       IMetaDataImport *metadata_import,
                       std::ostream *err_stream));
  MOCK_METHOD4(GetClassTokenAndModule,
               HRESULT(const std::string &class_name, mdTypeDef *class_token,
                       ICorDebugModule **debug_module,
                       IMetaDataImport **metadata_import));
  MOCK_METHOD3(IsBaseType,
               HRESULT(const google_cloud_debugger::TypeSignature &source_type,
                       const google_cloud_debugger::TypeSignature &target_type,
                       std::ostream *err_stream));
  MOCK_METHOD6(GetDebugFunctionFromClass,
               HRESULT(IMetaDataImport *metadata_import,
                       ICorDebugModule *debug_module,
                       const mdTypeDef &class_token,
                       google_cloud_debugger::MethodInfo *method_info,
                       const std::vector<google_cloud_debugger::TypeSignature> &generic_types,
                       ICorDebugFunction **debug_function));
  MOCK_METHOD2(GetDebugFunctionFromCurrentClass,
               HRESULT(google_cloud_debugger::MethodInfo *method_info,
                       ICorDebugFunction **debug_function));
  MOCK_METHOD1(
      GetCurrentClassTypeParameters,
      HRESULT(std::vector<google_cloud_debugger::CComPtr<ICorDebugType>>
                  *debug_types));
  MOCK_METHOD0(IsStaticMethod, bool());
  MOCK_METHOD0(IsAsyncMethod, bool());
  MOCK_METHOD0(GetThisObject,
               std::shared_ptr<google_cloud_debugger::DbgObject>());
};

}  // namespace google_cloud_debugger_test

#endif  //  I_DBG_STACK_FRAME_MOCK_H_
