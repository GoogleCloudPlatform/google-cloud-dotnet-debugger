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

#ifndef I_METADATA_IMPORT_MOCK_H_
#define I_METADATA_IMPORT_MOCK_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "cor.h"

namespace google_cloud_debugger_test {

// We have to break up methods that have more than 10 parameters since google
// mock cannot mock them.
class PartialMetaDataImportMock : public IMetaDataImport {
 public:
  virtual HRESULT GetEventProps(mdEvent ev, mdTypeDef *pClass, LPCWSTR szEvent,
                                ULONG cchEvent, ULONG *pchEvent,
                                DWORD *pdwEventFlags, mdToken *ptkEventType,
                                mdMethodDef *pmdAddOn, mdMethodDef *pmdRemoveOn,
                                mdMethodDef *pmdFire,
                                mdMethodDef rmdOtherMethod[], ULONG cMax,
                                ULONG *pcOtherMethod) override {
    return GetEventPropsFirst(ev, pClass, szEvent, cchEvent, pchEvent,
                              pdwEventFlags, ptkEventType) &
           GetEventPropsSecond(ev, pmdAddOn, pmdRemoveOn, pmdFire,
                               rmdOtherMethod, cMax, pcOtherMethod);
  }

  virtual HRESULT GetEventPropsFirst(mdEvent ev, mdTypeDef *pClass,
                                     LPCWSTR szEvent, ULONG cchEvent,
                                     ULONG *pchEvent, DWORD *pdwEventFlags,
                                     mdToken *ptkEventType) = 0;

  virtual HRESULT GetEventPropsSecond(mdEvent ev, mdMethodDef *pmdAddOn,
                                      mdMethodDef *pmdRemoveOn,
                                      mdMethodDef *pmdFire,
                                      mdMethodDef rmdOtherMethod[], ULONG cMax,
                                      ULONG *pcOtherMethod) = 0;

  virtual HRESULT GetMemberProps(mdToken mb, mdTypeDef *pClass, LPWSTR szMember,
                                 ULONG cchMember, ULONG *pchMember,
                                 DWORD *pdwAttr, PCCOR_SIGNATURE *ppvSigBlob,
                                 ULONG *pcbSigBlob, ULONG *pulCodeRVA,
                                 DWORD *pdwImplFlags, DWORD *pdwCPlusTypeFlag,
                                 UVCP_CONSTANT *ppValue,
                                 ULONG *pcchValue) override {
    return GetMemberPropsFirst(mb, pClass, szMember, cchMember, pchMember,
                               pdwAttr) &
           GetMemberPropsSecond(mb, ppvSigBlob, pcbSigBlob, pulCodeRVA,
                                pdwImplFlags, pdwCPlusTypeFlag, ppValue,
                                pcchValue);
  }

  virtual HRESULT GetMemberPropsFirst(mdToken mb, mdTypeDef *pClass,
                                      LPWSTR szMember, ULONG cchMember,
                                      ULONG *pchMember, DWORD *pdwAttr) = 0;

  virtual HRESULT GetMemberPropsSecond(mdToken mb, PCCOR_SIGNATURE *ppvSigBlob,
                                       ULONG *pcbSigBlob, ULONG *pulCodeRVA,
                                       DWORD *pdwImplFlags,
                                       DWORD *pdwCPlusTypeFlag,
                                       UVCP_CONSTANT *ppValue,
                                       ULONG *pcchValue) = 0;

  virtual HRESULT GetFieldProps(mdFieldDef mb, mdTypeDef *pClass,
                                LPWSTR szField, ULONG cchField, ULONG *pchField,
                                DWORD *pdwAttr, PCCOR_SIGNATURE *ppvSigBlob,
                                ULONG *pcbSigBlob, DWORD *pdwCPlusTypeFlag,
                                UVCP_CONSTANT *ppValue,
                                ULONG *pcchValue) override {
    return GetFieldPropsFirst(mb, pClass, szField, cchField, pchField,
                              pdwAttr) &
           GetFieldPropsSecond(mb, ppvSigBlob, pcbSigBlob, pdwCPlusTypeFlag,
                               ppValue, pcchValue);
  }

  virtual HRESULT GetFieldPropsFirst(mdFieldDef mb, mdTypeDef *pClass,
                                     LPWSTR szField, ULONG cchField,
                                     ULONG *pchField, DWORD *pdwAttr) = 0;

  virtual HRESULT GetFieldPropsSecond(
      mdFieldDef mb, PCCOR_SIGNATURE *ppvSigBlob, ULONG *pcbSigBlob,
      DWORD *pdwCPlusTypeFlag, UVCP_CONSTANT *ppValue, ULONG *pcchValue) = 0;

  virtual HRESULT GetPropertyProps(
      mdProperty prop, mdTypeDef *pClass, LPCWSTR szProperty, ULONG cchProperty,
      ULONG *pchProperty, DWORD *pdwPropFlags, PCCOR_SIGNATURE *ppvSig,
      ULONG *pbSig, DWORD *pdwCPlusTypeFlag, UVCP_CONSTANT *ppDefaultValue,
      ULONG *pcchDefaultValue, mdMethodDef *pmdSetter, mdMethodDef *pmdGetter,
      mdMethodDef rmdOtherMethod[], ULONG cMax, ULONG *pcOtherMethod) override {
    return GetPropertyPropsFirst(prop, pClass, szProperty, cchProperty,
                                 pchProperty, pdwPropFlags, ppvSig, pbSig,
                                 pdwCPlusTypeFlag) &
           GetPropertyPropsSecond(prop, ppDefaultValue, pcchDefaultValue,
                                  pmdSetter, pmdGetter, rmdOtherMethod, cMax,
                                  pcOtherMethod);
  }

  virtual HRESULT GetPropertyPropsFirst(mdProperty prop, mdTypeDef *pClass,
                                        LPCWSTR szProperty, ULONG cchProperty,
                                        ULONG *pchProperty, DWORD *pdwPropFlags,
                                        PCCOR_SIGNATURE *ppvSig, ULONG *pbSig,
                                        DWORD *pdwCPlusTypeFlag) = 0;

  virtual HRESULT GetPropertyPropsSecond(
      mdProperty prop, UVCP_CONSTANT *ppDefaultValue, ULONG *pcchDefaultValue,
      mdMethodDef *pmdSetter, mdMethodDef *pmdGetter,
      mdMethodDef rmdOtherMethod[], ULONG cMax, ULONG *pcOtherMethod) = 0;
};

class IMetaDataImportMock : public PartialMetaDataImportMock {
 public:
  MOCK_METHOD2(QueryInterface, HRESULT(REFIID riid, void **ppvObject));
  MOCK_METHOD0(AddRef, ULONG(void));
  MOCK_METHOD0(Release, ULONG(void));
  MOCK_METHOD1(CloseEnum, void(HCORENUM hEnum));
  MOCK_METHOD2(CountEnum, HRESULT(HCORENUM hEnum, ULONG *pulCount));
  MOCK_METHOD2(ResetEnum, HRESULT(HCORENUM hEnum, ULONG ulPos));
  MOCK_METHOD4(EnumTypeDefs, HRESULT(HCORENUM *phEnum, mdTypeDef rTypeDefs[],
                                     ULONG cMax, ULONG *pcTypeDefs));
  MOCK_METHOD5(EnumInterfaceImpls,
               HRESULT(HCORENUM *phEnum, mdTypeDef td, mdInterfaceImpl rImpls[],
                       ULONG cMax, ULONG *pcImpls));
  MOCK_METHOD4(EnumTypeRefs, HRESULT(HCORENUM *phEnum, mdTypeRef rTypeRefs[],
                                     ULONG cMax, ULONG *pcTypeRefs));
  MOCK_METHOD3(FindTypeDefByName,
               HRESULT(LPCWSTR szTypeDef, mdToken tkEnclosingClass,
                       mdTypeDef *ptd));
  MOCK_METHOD4(GetScopeProps, HRESULT(LPWSTR szName, ULONG cchName,
                                      ULONG *pchName, GUID *pmvid));
  MOCK_METHOD1(GetModuleFromScope, HRESULT(mdModule *pmd));
  MOCK_METHOD6(GetTypeDefProps,
               HRESULT(mdTypeDef td, LPWSTR szTypeDef, ULONG cchTypeDef,
                       ULONG *pchTypeDef, DWORD *pdwTypeDefFlags,
                       mdToken *ptkExtends));
  MOCK_METHOD3(GetInterfaceImplProps,
               HRESULT(mdInterfaceImpl iiImpl, mdTypeDef *pClass,
                       mdToken *ptkIface));
  MOCK_METHOD5(GetTypeRefProps,
               HRESULT(mdTypeRef tr, mdToken *ptkResolutionScope, LPWSTR szName,
                       ULONG cchName, ULONG *pchName));
  MOCK_METHOD4(ResolveTypeRef, HRESULT(mdTypeRef tr, REFIID riid,
                                       IUnknown **ppIScope, mdTypeDef *ptd));
  MOCK_METHOD5(EnumMembers,
               HRESULT(HCORENUM *phEnum, mdTypeDef cl, mdToken rMembers[],
                       ULONG cMax, ULONG *pcTokens));
  MOCK_METHOD6(EnumMembersWithName,
               HRESULT(HCORENUM *phEnum, mdTypeDef cl, LPCWSTR szName,
                       mdToken rMembers[], ULONG cMax, ULONG *pcTokens));
  MOCK_METHOD5(EnumMethods,
               HRESULT(HCORENUM *phEnum, mdTypeDef cl, mdMethodDef rMethods[],
                       ULONG cMax, ULONG *pcTokens));
  MOCK_METHOD6(EnumMethodsWithName,
               HRESULT(HCORENUM *phEnum, mdTypeDef cl, LPCWSTR szName,
                       mdMethodDef rMethods[], ULONG cMax, ULONG *pcTokens));
  MOCK_METHOD5(EnumFields,
               HRESULT(HCORENUM *phEnum, mdTypeDef cl, mdFieldDef rFields[],
                       ULONG cMax, ULONG *pcTokens));
  MOCK_METHOD6(EnumFieldsWithName,
               HRESULT(HCORENUM *phEnum, mdTypeDef cl, LPCWSTR szName,
                       mdFieldDef rFields[], ULONG cMax, ULONG *pcTokens));
  MOCK_METHOD5(EnumParams,
               HRESULT(HCORENUM *phEnum, mdMethodDef mb, mdParamDef rParams[],
                       ULONG cMax, ULONG *pcTokens));
  MOCK_METHOD5(EnumMemberRefs,
               HRESULT(HCORENUM *phEnum, mdToken tkParent,
                       mdMemberRef rMemberRefs[], ULONG cMax, ULONG *pcTokens));
  MOCK_METHOD6(EnumMethodImpls,
               HRESULT(HCORENUM *phEnum, mdTypeDef td, mdToken rMethodBody[],
                       mdToken rMethodDecl[], ULONG cMax, ULONG *pcTokens));
  MOCK_METHOD6(EnumPermissionSets,
               HRESULT(HCORENUM *phEnum, mdToken tk, DWORD dwActions,
                       mdPermission rPermission[], ULONG cMax,
                       ULONG *pcTokens));
  MOCK_METHOD5(FindMember,
               HRESULT(mdTypeDef td, LPCWSTR szName, PCCOR_SIGNATURE pvSigBlob,
                       ULONG cbSigBlob, mdToken *pmb));
  MOCK_METHOD5(FindMethod,
               HRESULT(mdTypeDef td, LPCWSTR szName, PCCOR_SIGNATURE pvSigBlob,
                       ULONG cbSigBlob, mdMethodDef *pmb));
  MOCK_METHOD5(FindField,
               HRESULT(mdTypeDef td, LPCWSTR szName, PCCOR_SIGNATURE pvSigBlob,
                       ULONG cbSigBlob, mdFieldDef *pmb));
  MOCK_METHOD5(FindMemberRef,
               HRESULT(mdTypeRef td, LPCWSTR szName, PCCOR_SIGNATURE pvSigBlob,
                       ULONG cbSigBlob, mdMemberRef *pmr));
  MOCK_METHOD10(GetMethodProps,
                HRESULT(mdMethodDef mb, mdTypeDef *pClass, LPWSTR szMethod,
                        ULONG cchMethod, ULONG *pchMethod, DWORD *pdwAttr,
                        PCCOR_SIGNATURE *ppvSigBlob, ULONG *pcbSigBlob,
                        ULONG *pulCodeRVA, DWORD *pdwImplFlags));
  MOCK_METHOD7(GetMemberRefProps,
               HRESULT(mdMemberRef mr, mdToken *ptk, LPWSTR szMember,
                       ULONG cchMember, ULONG *pchMember,
                       PCCOR_SIGNATURE *ppvSigBlob, ULONG *pbSig));
  MOCK_METHOD5(EnumProperties,
               HRESULT(HCORENUM *phEnum, mdTypeDef td, mdProperty rProperties[],
                       ULONG cMax, ULONG *pcProperties));
  MOCK_METHOD5(EnumEvents,
               HRESULT(HCORENUM *phEnum, mdTypeDef td, mdEvent rEvents[],
                       ULONG cMax, ULONG *pcEvents));
  MOCK_METHOD5(EnumMethodSemantics,
               HRESULT(HCORENUM *phEnum, mdMethodDef mb, mdToken rEventProp[],
                       ULONG cMax, ULONG *pcEventProp));
  MOCK_METHOD3(GetMethodSemantics, HRESULT(mdMethodDef mb, mdToken tkEventProp,
                                           DWORD *pdwSemanticsFlags));
  MOCK_METHOD6(GetClassLayout,
               HRESULT(mdTypeDef td, DWORD *pdwPackSize,
                       COR_FIELD_OFFSET rFieldOffset[], ULONG cMax,
                       ULONG *pcFieldOffset, ULONG *pulClassSize));
  MOCK_METHOD3(GetFieldMarshal,
               HRESULT(mdToken tk, PCCOR_SIGNATURE *ppvNativeType,
                       ULONG *pcbNativeType));
  MOCK_METHOD3(GetRVA,
               HRESULT(mdToken tk, ULONG *pulCodeRVA, DWORD *pdwImplFlags));
  MOCK_METHOD4(GetPermissionSetProps,
               HRESULT(mdPermission pm, DWORD *pdwAction,
                       void const **ppvPermission, ULONG *pcbPermission));
  MOCK_METHOD3(GetSigFromToken,
               HRESULT(mdSignature mdSig, PCCOR_SIGNATURE *ppvSig,
                       ULONG *pcbSig));
  MOCK_METHOD4(GetModuleRefProps, HRESULT(mdModuleRef mur, LPWSTR szName,
                                          ULONG cchName, ULONG *pchName));
  MOCK_METHOD4(EnumModuleRefs,
               HRESULT(HCORENUM *phEnum, mdModuleRef rModuleRefs[], ULONG cmax,
                       ULONG *pcModuleRefs));
  MOCK_METHOD3(GetTypeSpecFromToken,
               HRESULT(mdTypeSpec typespec, PCCOR_SIGNATURE *ppvSig,
                       ULONG *pcbSig));
  MOCK_METHOD2(GetNameFromToken,
               HRESULT(mdToken tk, MDUTF8CSTR *pszUtf8NamePtr));
  MOCK_METHOD4(EnumUnresolvedMethods,
               HRESULT(HCORENUM *phEnum, mdToken rMethods[], ULONG cMax,
                       ULONG *pcTokens));
  MOCK_METHOD4(GetUserString, HRESULT(mdString stk, LPWSTR szString,
                                      ULONG cchString, ULONG *pchString));
  MOCK_METHOD6(GetPinvokeMap,
               HRESULT(mdToken tk, DWORD *pdwMappingFlags, LPWSTR szImportName,
                       ULONG cchImportName, ULONG *pchImportName,
                       mdModuleRef *pmrImportDLL));
  MOCK_METHOD4(EnumSignatures,
               HRESULT(HCORENUM *phEnum, mdSignature rSignatures[], ULONG cmax,
                       ULONG *pcSignatures));
  MOCK_METHOD4(EnumTypeSpecs, HRESULT(HCORENUM *phEnum, mdTypeSpec rTypeSpecs[],
                                      ULONG cmax, ULONG *pcTypeSpecs));
  MOCK_METHOD4(EnumUserStrings, HRESULT(HCORENUM *phEnum, mdString rStrings[],
                                        ULONG cmax, ULONG *pcStrings));
  MOCK_METHOD3(GetParamForMethodIndex,
               HRESULT(mdMethodDef md, ULONG ulParamSeq, mdParamDef *ppd));
  MOCK_METHOD6(EnumCustomAttributes,
               HRESULT(HCORENUM *phEnum, mdToken tk, mdToken tkType,
                       mdCustomAttribute rCustomAttributes[], ULONG cMax,
                       ULONG *pcCustomAttributes));
  MOCK_METHOD5(GetCustomAttributeProps,
               HRESULT(mdCustomAttribute cv, mdToken *ptkObj, mdToken *ptkType,
                       void const **ppBlob, ULONG *pcbSize));
  MOCK_METHOD3(FindTypeRef, HRESULT(mdToken tkResolutionScope, LPCWSTR szName,
                                    mdTypeRef *ptr));
  MOCK_METHOD10(GetParamProps,
                HRESULT(mdParamDef tk, mdMethodDef *pmd, ULONG *pulSequence,
                        LPWSTR szName, ULONG cchName, ULONG *pchName,
                        DWORD *pdwAttr, DWORD *pdwCPlusTypeFlag,
                        UVCP_CONSTANT *ppValue, ULONG *pcchValue));
  MOCK_METHOD4(GetCustomAttributeByName,
               HRESULT(mdToken tkObj, LPCWSTR szName, const void **ppData,
                       ULONG *pcbData));
  MOCK_METHOD1(IsValidToken, BOOL(mdToken tk));
  MOCK_METHOD2(GetNestedClassProps,
               HRESULT(mdTypeDef tdNestedClass, mdTypeDef *ptdEnclosingClass));
  MOCK_METHOD3(GetNativeCallConvFromSig,
               HRESULT(void const *pvSig, ULONG cbSig, ULONG *pCallConv));
  MOCK_METHOD2(IsGlobal, HRESULT(mdToken pd, int *pbGlobal));
  MOCK_METHOD7(GetEventPropsFirst,
               HRESULT(mdEvent ev, mdTypeDef *pClass, LPCWSTR szEvent,
                       ULONG cchEvent, ULONG *pchEvent, DWORD *pdwEventFlags,
                       mdToken *ptkEventType));
  MOCK_METHOD7(GetEventPropsSecond,
               HRESULT(mdEvent ev, mdMethodDef *pmdAddOn,
                       mdMethodDef *pmdRemoveOn, mdMethodDef *pmdFire,
                       mdMethodDef rmdOtherMethod[], ULONG cMax,
                       ULONG *pcOtherMethod));
  MOCK_METHOD6(GetMemberPropsFirst,
               HRESULT(mdToken mb, mdTypeDef *pClass, LPWSTR szMember,
                       ULONG cchMember, ULONG *pchMember, DWORD *pdwAttr));
  MOCK_METHOD8(GetMemberPropsSecond,
               HRESULT(mdToken mb, PCCOR_SIGNATURE *ppvSigBlob,
                       ULONG *pcbSigBlob, ULONG *pulCodeRVA,
                       DWORD *pdwImplFlags, DWORD *pdwCPlusTypeFlag,
                       UVCP_CONSTANT *ppValue, ULONG *pcchValue));
  MOCK_METHOD6(GetFieldPropsFirst,
               HRESULT(mdFieldDef mb, mdTypeDef *pClass, LPWSTR szField,
                       ULONG cchField, ULONG *pchField, DWORD *pdwAttr));
  MOCK_METHOD6(GetFieldPropsSecond,
               HRESULT(mdFieldDef mb, PCCOR_SIGNATURE *ppvSigBlob,
                       ULONG *pcbSigBlob, DWORD *pdwCPlusTypeFlag,
                       UVCP_CONSTANT *ppValue, ULONG *pcchValue));
  MOCK_METHOD9(GetPropertyPropsFirst,
               HRESULT(mdProperty prop, mdTypeDef *pClass, LPCWSTR szProperty,
                       ULONG cchProperty, ULONG *pchProperty,
                       DWORD *pdwPropFlags, PCCOR_SIGNATURE *ppvSig,
                       ULONG *pbSig, DWORD *pdwCPlusTypeFlag));
  MOCK_METHOD8(GetPropertyPropsSecond,
               HRESULT(mdProperty prop, UVCP_CONSTANT *ppDefaultValue,
                       ULONG *pcchDefaultValue, mdMethodDef *pmdSetter,
                       mdMethodDef *pmdGetter, mdMethodDef rmdOtherMethod[],
                       ULONG cMax, ULONG *pcOtherMethod));
};

}  // namespace google_cloud_debugger_test

#endif  //  I_METADATA_IMPORT_H_
