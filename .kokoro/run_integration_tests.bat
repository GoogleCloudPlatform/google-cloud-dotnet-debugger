:: Build the repo's dependencies, the repo and run its integration tests.

set ROOT_DIR=%~dp0..
cd %ROOT_DIR%

set GOOGLE_APPLICATION_CREDENTIALS="$KOKORO_KEYSTORE_DIR\73609_cloud-sharp-jenkins-compute-service-account"

git submodule init
git submodule update

%ROOT_DIR%\build-deps.cmd && %ROOT_DIR%\build.cmd && "C:\Program Files\Git\bin\bash.exe" %ROOT_DIR%\run_integration_tests.sh
