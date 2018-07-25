:: See documentation in type-shell-output.bat

cd %~dp0
cd ..

.\build-deps.cmd
.\build.cmd

%~dp0type-shell-output.bat run_integration_tests