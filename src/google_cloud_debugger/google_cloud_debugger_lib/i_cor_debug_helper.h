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

#ifndef I_CORDEBUG_HELPER_H_
#define I_CORDEBUG_HELPER_H_

#include <memory>
#include <ostream>
#include <vector>

#include "cor.h"
#include "cordebug.h"

namespace google_cloud_debugger {

class DbgClassProperty;

// Helper methods for various ICorDebug objects.

// Given an ICorDebugClass, extracts out IMetaDataImport and stores it in
// metadata_import.
HRESULT GetMetadataImportFromICorDebugClass(ICorDebugClass *debug_class,
                                            IMetaDataImport **metadata_import,
                                            std::ostream *err_stream);

// Given an ICorDebugModule, extracts out IMetaDataImport and stores it in
// metadata_import.
HRESULT GetMetadataImportFromICorDebugModule(ICorDebugModule *debug_module,
                                             IMetaDataImport **metadata_import,
                                             std::ostream *err_stream);

// Extracts module name from ICorDebugModule.
HRESULT GetModuleNameFromICorDebugModule(ICorDebugModule *debug_module,
                                         std::vector<WCHAR> *module_name,
                                         std::ostream *err_stream);

// Extracts ICorDebugType from ICorDebug.
HRESULT GetICorDebugType(ICorDebugValue *debug_value,
                         ICorDebugType **debug_type, std::ostream *err_stream);

// Given an ICorDebugValue, keep trying to dereference it until we cannot
// anymore. This function will set is_null to true if this is a null
// reference. If debug_value is not a reference, this function will simply set
// dereferenced_value to debug_value. dereferenced_value cannot be null.
HRESULT Dereference(ICorDebugValue *debug_value,
                    ICorDebugValue **dereferenced_value, BOOL *is_null,
                    std::ostream *err_stream);

// The depth at which we will stop dereferencing.
const int kReferenceDepth = 10;

// Given an ICorDebugValue, try to unbox it if possible.
// If debug_value is not a boxed value, this function will simply set
// unboxed_value to debug_value.
HRESULT Unbox(ICorDebugValue *debug_value, ICorDebugValue **unboxed_value,
              std::ostream *err_stream);

// Given an ICorDebugValue, dereference and unbox it to get the underlying
// object. Set is_null to true if the object is null.
HRESULT DereferenceAndUnbox(ICorDebugValue *debug_value,
                            ICorDebugValue **dereferenced_and_unboxed_value,
                            BOOL *is_null, std::ostream *err_stream);

// Given an ICorDebugValue, creates a strong handle to the underlying
// object. ICorDebugValue must represents an object type that can
// be stored on the heap.
HRESULT CreateStrongHandle(ICorDebugValue *debug_value,
                           ICorDebugHandleValue **handle,
                           std::ostream *err_stream);

// Extracts out a string from ICorDebugStringValue.
HRESULT ExtractStringFromICorDebugStringValue(
    ICorDebugStringValue *debug_string, std::string *returned_string,
    std::ostream *err_stream);

// Given a metadata token for the parameter param_token,
// extracts out the parameter name.
// metadata_import is the MetaDataImport of the module
// that contains the method with the param_token.
HRESULT ExtractParamName(IMetaDataImport *metadata_import,
                         mdParamDef param_token, std::string *param_name,
                         std::ostream *err_stream);

// Extracts out ICorDebugModule from ICorDebugFrame.
HRESULT GetICorDebugModuleFromICorDebugFrame(ICorDebugFrame *debug_frame,
                                             ICorDebugModule **debug_module,
                                             std::ostream *err_stream);

// Parsed compressed bytes (1 to 4 bytes) from PCCOR_SIGNATURE signature.
// This will also set sig_len to appropriate length.
// The uncompressed bytes will bee stored in result.
HRESULT ParseCompressedBytes(PCCOR_SIGNATURE *signature, ULONG *sig_len,
                             ULONG *result);

// Parses the metadata signature of a field and retrieves
// the field's type.
HRESULT ParseFieldSig(PCCOR_SIGNATURE *signature, ULONG *sig_len,
                      IMetaDataImport *metadata_import,
                      std::string *field_type_name);

// Parses the metadata signature of a property and retrieves
// the property's type.
HRESULT ParsePropertySig(PCCOR_SIGNATURE *signature, ULONG *sig_len,
                         IMetaDataImport *metadata_import,
                         std::string *property_type_name);

// Given a PCCOR_SIGNATURE signature, parses the type
// and stores the result in type_name. Also update the sig_len.
HRESULT ParseTypeFromSig(PCCOR_SIGNATURE *signature, ULONG *sig_len,
                         IMetaDataImport *metadata_import,
                         std::string *type_name);

// Given a PCCOR_SIGNATURE signature, parses the next byte A.
// Then, parses and skips the next A bytes.
HRESULT ParseAndSkipBasedOnFirstByteSignature(PCCOR_SIGNATURE *signature,
                                              ULONG *sig_len);

// Given a PCCOR_SIGNATURE, parses the first byte
// and checks that with calling_convention.
HRESULT ParseAndCheckFirstByte(PCCOR_SIGNATURE *signature, ULONG *sig_len,
                               CorCallingConvention calling_convention);

// Extracts out the metadata for field field_name
// in class with metadata token class_token.
// This method will also set *is_static based
// on whether the field is static or not.
// It will also return the signature of the field and its length.
HRESULT GetFieldInfo(IMetaDataImport *metadata_import, mdTypeDef class_token,
                     const std::string &field_name, mdFieldDef *field_def,
                     bool *is_static, PCCOR_SIGNATURE *field_sig,
                     ULONG *signature_len, std::ostream *err_stream);

// Creates a DbgClassProperty object that corresponds with
// the property property_name in class with metadata token class_token.
// This method will also set *is_static based
// on whether the property is static or not.
HRESULT GetPropertyInfo(IMetaDataImport *metadata_import,
                        mdProperty class_token, const std::string &prop_name,
                        std::unique_ptr<DbgClassProperty> *result,
                        std::ostream *err_stream);

// Gets name from mdTypeDef token type_token.
HRESULT GetTypeNameFromMdTypeDef(mdTypeDef type_token,
                                 IMetaDataImport *metadata_import,
                                 std::string *type_name, mdToken *base_token,
                                 std::ostream *err_stream);

// Gets name from mdTypeRef token type_token.
HRESULT GetTypeNameFromMdTypeRef(mdTypeRef type_token,
                                 IMetaDataImport *metadata_import,
                                 std::string *type_name,
                                 std::ostream *err_stream);

// Given a TypeRef token and its MetaDataImport, this function
// converts it into a TypeDef token. The function will also return
// the corresponding MetaDataImport for that token.
HRESULT GetMdTypeDefAndMetaDataFromTypeRef(
    mdTypeRef type_ref_token, IMetaDataImport *type_ref_token_metadata,
    mdTypeDef *result_type_def, IMetaDataImport **result_type_def_metadata);

// Retrieves ICorDebugAppDomain from ICorDebugFrame.
HRESULT GetAppDomainFromICorDebugFrame(ICorDebugFrame *debug_frame,
                                       ICorDebugAppDomain **app_domain,
                                       std::ostream *err_stream);

// Count generic params of method/class referred to by mdToken.
HRESULT CountGenericParams(IMetaDataImport *metadata_import,
                           const mdToken &token, uint32_t *result);

}  // namespace google_cloud_debugger

#endif  //  I_CORDEBUG_HELPER_H_
