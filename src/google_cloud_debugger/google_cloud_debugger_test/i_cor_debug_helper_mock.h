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

#ifndef I_COR_DEBUG_HELPER_MOCK_H_
#define I_COR_DEBUG_HELPER_MOCK_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "i_cor_debug_helper.h"

namespace google_cloud_debugger_test {

class ICorDebugHelperMock : public google_cloud_debugger::ICorDebugHelper {
 public:
  HRESULT GetMdTypeDefAndMetaDataFromTypeRef(
      mdTypeRef type_ref_token,
      const std::vector<google_cloud_debugger::CComPtr<ICorDebugAssembly>>
          &loaded_assemblies,
      IMetaDataImport *type_ref_token_metadata, mdTypeDef *result_type_def,
      IMetaDataImport **result_type_def_metadata,
      std::ostream *err_stream) override {
    return GetMdTypeDefAndMetaDataFromTypeRefHelper(
        type_ref_token, type_ref_token_metadata, result_type_def,
        result_type_def_metadata, err_stream);
  }

  MOCK_METHOD3(GetMetadataImportFromICorDebugClass,
               HRESULT(ICorDebugClass *debug_class,
                       IMetaDataImport **metadata_import,
                       std::ostream *err_stream));
  MOCK_METHOD3(GetMetadataImportFromICorDebugModule,
               HRESULT(ICorDebugModule *debug_module,
                       IMetaDataImport **metadata_import,
                       std::ostream *err_stream));
  MOCK_METHOD3(GetModuleNameFromICorDebugModule,
               HRESULT(ICorDebugModule *debug_module,
                       std::vector<WCHAR> *module_name,
                       std::ostream *err_stream));
  MOCK_METHOD3(GetICorDebugType,
               HRESULT(ICorDebugValue *debug_value, ICorDebugType **debug_type,
                       std::ostream *err_stream));
  MOCK_METHOD4(Dereference, HRESULT(ICorDebugValue *debug_value,
                                    ICorDebugValue **dereferenced_value,
                                    BOOL *is_null, std::ostream *err_stream));
  MOCK_METHOD3(Unbox, HRESULT(ICorDebugValue *debug_value,
                              ICorDebugValue **unboxed_value,
                              std::ostream *err_stream));
  MOCK_METHOD4(DereferenceAndUnbox,
               HRESULT(ICorDebugValue *debug_value,
                       ICorDebugValue **dereferenced_and_unboxed_value,
                       BOOL *is_null, std::ostream *err_stream));
  MOCK_METHOD3(CreateStrongHandle, HRESULT(ICorDebugValue *debug_value,
                                           ICorDebugHandleValue **handle,
                                           std::ostream *err_stream));
  MOCK_METHOD3(ExtractStringFromICorDebugStringValue,
               HRESULT(ICorDebugStringValue *debug_string,
                       std::string *returned_string, std::ostream *err_stream));
  MOCK_METHOD4(ExtractParamName,
               HRESULT(IMetaDataImport *metadata_import, mdParamDef param_token,
                       std::string *param_name, std::ostream *err_stream));
  MOCK_METHOD3(GetICorDebugModuleFromICorDebugFrame,
               HRESULT(ICorDebugFrame *debug_frame,
                       ICorDebugModule **debug_module,
                       std::ostream *err_stream));
  MOCK_METHOD3(ParseCompressedBytes, HRESULT(PCCOR_SIGNATURE *signature,
                                             ULONG *sig_len, ULONG *result));
  MOCK_METHOD5(ParseFieldSig,
               HRESULT(PCCOR_SIGNATURE *signature, ULONG *sig_len,
                       IMetaDataImport *metadata_import,
                       const std::vector<google_cloud_debugger::TypeSignature>
                           &generic_class_types,
                       google_cloud_debugger::TypeSignature *type_signature));
  MOCK_METHOD5(ParsePropertySig,
               HRESULT(PCCOR_SIGNATURE *signature, ULONG *sig_len,
                       IMetaDataImport *metadata_import,
                       const std::vector<google_cloud_debugger::TypeSignature>
                           &generic_class_types,
                       google_cloud_debugger::TypeSignature *type_signature));
  MOCK_METHOD5(ParseTypeFromSig,
               HRESULT(PCCOR_SIGNATURE *signature, ULONG *sig_len,
                       IMetaDataImport *metadata_import,
                       const std::vector<google_cloud_debugger::TypeSignature>
                           &generic_class_types,
                       google_cloud_debugger::TypeSignature *type_signature));
  MOCK_METHOD8(GetFieldInfo,
               HRESULT(IMetaDataImport *metadata_import, mdTypeDef class_token,
                       const std::string &field_name, mdFieldDef *field_def,
                       bool *is_static, PCCOR_SIGNATURE *field_sig,
                       ULONG *signature_len, std::ostream *err_stream));
  MOCK_METHOD6(
      GetPropertyInfo,
      HRESULT(IMetaDataImport *metadata_import, mdProperty class_token,
              const std::string &prop_name,
              std::unique_ptr<google_cloud_debugger::DbgClassProperty> *result,
              ICorDebugModule *debug_module, std::ostream *err_stream));
  MOCK_METHOD5(GetTypeNameFromMdTypeDef,
               HRESULT(mdTypeDef type_token, IMetaDataImport *metadata_import,
                       std::string *type_name, mdToken *base_token,
                       std::ostream *err_stream));
  MOCK_METHOD4(GetTypeNameFromMdTypeRef,
               HRESULT(mdTypeRef type_token, IMetaDataImport *metadata_import,
                       std::string *type_name, std::ostream *err_stream));
  MOCK_METHOD5(GetMdTypeDefAndMetaDataFromTypeRefHelper,
               HRESULT(mdTypeRef type_ref_token,
                       IMetaDataImport *type_ref_token_metadata,
                       mdTypeDef *result_type_def,
                       IMetaDataImport **result_type_def_metadata,
                       std::ostream *err_stream));
  MOCK_METHOD3(GetAppDomainFromICorDebugFrame,
               HRESULT(ICorDebugFrame *debug_frame,
                       ICorDebugAppDomain **app_domain,
                       std::ostream *err_stream));
  MOCK_METHOD3(CountGenericParams,
               HRESULT(IMetaDataImport *metadata_import, const mdToken &token,
                       uint32_t *result));
  MOCK_METHOD4(
      GetInstantiatedClassType,
      HRESULT(ICorDebugClass *debug_class,
              std::vector<google_cloud_debugger::CComPtr<ICorDebugType>>
                  *parameter_types,
              ICorDebugType **result_type, std::ostream *err_stream));
  MOCK_METHOD3(
      PopulateGenericClassTypesFromClassObject,
      HRESULT(ICorDebugValue *class_object,
              std::vector<google_cloud_debugger::CComPtr<ICorDebugType>>
                  *generic_types,
              std::ostream *err_stream));
};

}  // namespace google_cloud_debugger_test

#endif  //  I_COR_DEBUG_HELPER_MOCK_H_
