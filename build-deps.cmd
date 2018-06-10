:: The script builds the submodule dependencies of this repository if they have not been built yet.
:: This script is called during the Visual Studio build process and may not work in other places.

:: TODO(talarico): Allow building with multiple Visual Studio versions. 

:: The only optional parameter is the configuration which defaults to Debug.


SET ROOT_DIR=%~dp0
SET CONFIG=%1
SET PLATFORM=x64
SET PLATFORM_STR= Win64

SET THIRD_PARTY_DIR=%ROOT_DIR%\third_party
SET PROTOBUF_DIR=%THIRD_PARTY_DIR%\protobuf
SET GMOCK_DIR=%THIRD_PARTY_DIR%\googletest\googlemock
SET GTEST_DIR=%THIRD_PARTY_DIR%\googletest\googletest
SET CORECLR_DIR=%THIRD_PARTY_DIR%\coreclr

if "%1" == "" (
  SET CONFIG=Debug
)

:: Ensure the submodule are up to date.
git submodule init
git submodule update

:: Build the protobuf repository.
if not exist "%PROTOBUF_DIR%\%PLATFORM%\%CONFIG%\libprotobuf.lib" (
  :: Clone the extra repository protobuf needs.
  if not exist "%PROTOBUF_DIR%\gmock" git clone -b release-1.7.0 https://github.com/google/googlemock.git %PROTOBUF_DIR%\gmock
  if not exist "%PROTOBUF_DIR%\gmock\gtest" git clone -b release-1.7.0 https://github.com/google/googletest.git %PROTOBUF_DIR%\gmock\gtest
  if not exist "%PROTOBUF_DIR%\install" mkdir %PROTOBUF_DIR%\install
  if not exist "%PROTOBUF_DIR%\cmake\build\solution" mkdir %PROTOBUF_DIR%\cmake\build\solution

  if not exist "%PROTOBUF_DIR%\%PLATFORM%\%CONFIG%" mkdir %PROTOBUF_DIR%\%PLATFORM%\%CONFIG%  
  cd %PROTOBUF_DIR%\cmake\build\solution
  cmake -G "Visual Studio 15 2017%PLATFORM_STR%" -DCMAKE_INSTALL_PREFIX=%PROTOBUF_DIR% %PROTOBUF_DIR%\cmake
  msbuild %PROTOBUF_DIR%\cmake\build\solution\ALL_BUILD.vcxproj /p:Configuration=%CONFIG%
  
  cd %PROTOBUF_DIR%\cmake\build\solution\%CONFIG%
  if "%CONFIG%" == "Debug" (
    ren libprotobufd.lib libprotobuf.lib
  )
  copy libprotobuf.lib %PROTOBUF_DIR%\%PLATFORM%\%CONFIG%
)

:: Build the googlemock repository.
if not exist "%GMOCK_DIR%\%PLATFORM%\%CONFIG%\gmock.lib" (
  if not exist "%GMOCK_DIR%\%PLATFORM%\%CONFIG%" mkdir %GMOCK_DIR%\%PLATFORM%\%CONFIG%
  cd %GMOCK_DIR%
  cmake -G "Visual Studio 15 2017%PLATFORM_STR%" %GMOCK_DIR%
  msbuild %GMOCK_DIR%\gmock.sln /p:Configuration=%CONFIG%
  
  cd %GMOCK_DIR%\%CONFIG%
  if "%CONFIG%" == "Debug" (
    ren gmockd.lib gmock.lib
  )
  copy gmock.lib %GMOCK_DIR%\%PLATFORM%\%CONFIG%
)

:: Build the googletest repository.
if not exist "%GTEST_DIR%\%PLATFORM%\%CONFIG%\gtest.lib" (
  if not exist "%GTEST_DIR%\%PLATFORM%\%CONFIG%" mkdir %GTEST_DIR%\%PLATFORM%\%CONFIG%
  cd %GTEST_DIR%
  cmake -G "Visual Studio 15 2017%PLATFORM_STR%" %GTEST_DIR%
  msbuild %GTEST_DIR%\gtest.sln /p:Configuration=%CONFIG%
  
  cd %GTEST_DIR%\%CONFIG%
  if "%CONFIG%" == "Debug" (
    ren gtestd.lib gtest.lib
  )
  copy gtest.lib %GTEST_DIR%\%PLATFORM%\%CONFIG%
)

:: Build the coreclr repository without tests.
if not exist "%CORECLR_DIR%\bin\obj\Windows_NT.%PLATFORM%.%CONFIG%\src\dlls\dbgshim\%CONFIG%" (
    %CORECLR_DIR%\build.cmd skiptests %PLATFORM% %CONFIG%
    %CORECLR_DIR%\build-packages.cmd -buildType=%CONFIG% -buildArch=%PLATFORM%
)

cd %ROOT_DIR%