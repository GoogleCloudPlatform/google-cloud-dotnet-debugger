:: See documentation in type-shell-output.bat

cd %~dp0
cd ..

set GOOGLE_APPLICATION_CREDENTIALS="$KOKORO_KEYSTORE_DIR\73609_cloud-sharp-jenkins-compute-service-account"

git submodule init
git submodule update

call .\build-deps.cmd
call .\build.cmd

"C:\Program Files\Git\bin\bash.exe" run_integration_tests.sh
