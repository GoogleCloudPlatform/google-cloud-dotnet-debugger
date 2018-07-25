:: See documentation in type-shell-output.bat

cd %~dp0
cd ..

set GOOGLE_APPLICATION_CREDENTIALS="$KOKORO_KEYSTORE_DIR/73609_cloud-sharp-jenkins-compute-service-account"

call .\build-deps.cmd
call .\build.cmd

call %~dp0type-shell-output.bat run_unit_tests
