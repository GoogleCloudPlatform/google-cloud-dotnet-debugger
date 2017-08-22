:: The script builds the submodule dependencies of this repository if they have not been built yet.
:: This script is called during the Visual Studio build process and may not work in other places.

:: TODO(talarico): Allow building with multiple Visual Studio versions. 

:: Check that the user passed in a valid parameter.
if "%3" == "" (
  echo "Parameters should be: repository root, configuration, platform"
  exit
)

SET ROOT_DIR=%1
SET CONFIG=%2
SET PLATFORM=%3
SET PLATFORM_STR=""

if "%3" == "x64" (
  SET PLATFORM_STR= Win64
)

:: Ensure the submodule are up to date.
git submodule init
git submodule update

:: Build the protobuf repository.
if not exist "%ROOT_DIR%protobuf\%PLATFORM%\%CONFIG%\libprotobuf.lib" (
  :: Clone the extra repository protobuf needs.
  if not exist "%ROOT_DIR%protobuf\gmock" git clone -b release-1.7.0 https://github.com/google/googlemock.git %ROOT_DIR%protobuf\gmock
  if not exist "%ROOT_DIR%protobuf\gmock\gtest" git clone -b release-1.7.0 https://github.com/google/googletest.git %ROOT_DIR%protobuf\gmock\gtest
  if not exist "%ROOT_DIR%protobuf\install" mkdir %ROOT_DIR%protobuf\install
  if not exist "%ROOT_DIR%protobuf\cmake\build\solution" mkdir %ROOT_DIR%protobuf\cmake\build\solution

  if not exist "%ROOT_DIR%protobuf\%PLATFORM%\%CONFIG%" mkdir %ROOT_DIR%protobuf\%PLATFORM%\%CONFIG%
  cd %ROOT_DIR%protobuf\cmake\build\solution
  cmake -G "Visual Studio 15 2017%PLATFORM_STR%" -DCMAKE_INSTALL_PREFIX=%ROOT_DIR%protobuf %ROOT_DIR%protobuf\cmake
  msbuild %ROOT_DIR%protobuf\cmake\build\solution\ALL_BUILD.vcxproj /p:Configuration=%CONFIG%
  
  cd %ROOT_DIR%protobuf\cmake\build\solution\%CONFIG%
  if "%CONFIG%" == "Debug" (
    ren libprotobufd.lib libprotobuf.lib
  )
  copy libprotobuf.lib %ROOT_DIR%protobuf\%PLATFORM%\%CONFIG%
)

:: Build the googlemock repository.
if not exist "%ROOT_DIR%googletest\googlemock\%PLATFORM%\%CONFIG%\gmock.lib" (
  if not exist "%ROOT_DIR%googletest\googlemock\%PLATFORM%\%CONFIG%" mkdir %ROOT_DIR%googletest\googlemock\%PLATFORM%\%CONFIG%
  cd %ROOT_DIR%googletest\googlemock
  cmake -G "Visual Studio 15 2017%PLATFORM_STR%" %ROOT_DIR%googletest\googlemock
  msbuild %ROOT_DIR%googletest\googlemock\gmock.sln /p:Configuration=%CONFIG%
  
  cd %ROOT_DIR%\googletest\googlemock\%CONFIG%
  copy gmock.lib %ROOT_DIR%googletest\googlemock\%PLATFORM%\%CONFIG%
)

:: Build the googletest repository.
if not exist "%ROOT_DIR%googletest\googletest\%PLATFORM%\%CONFIG%\gtest.lib" (
  if not exist "%ROOT_DIR%googletest\googletest\%PLATFORM%\%CONFIG%" mkdir %ROOT_DIR%googletest\googletest\%PLATFORM%\%CONFIG%
  cd %ROOT_DIR%googletest\googletest
  cmake -G "Visual Studio 15 2017%PLATFORM_STR%" %ROOT_DIR%googletest\googletest
  msbuild %ROOT_DIR%googletest\googletest\gtest.sln /p:Configuration=%CONFIG%
  
  cd %ROOT_DIR%\googletest\googletest\%CONFIG%
  copy gtest.lib %ROOT_DIR%googletest\googletest\%PLATFORM%\%CONFIG%
)

:: Build the coreclr repository without tests.
if not exist "%ROOT_DIR%coreclr\bin\obj\Windows_NT.%PLATFORM%.%CONFIG%\src\dlls\dbgshim\%CONFIG%" (
    %ROOT_DIR%coreclr\build.cmd skiptests %PLATFORM% %CONFIG%
    %ROOT_DIR%coreclr\build-packages.cmd -buildType=%CONFIG% -buildArch=%PLATFORM%
)

cd %ROOT_DIR%