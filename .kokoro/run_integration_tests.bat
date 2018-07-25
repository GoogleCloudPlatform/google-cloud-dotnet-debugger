:: See documentation in type-shell-output.bat

cd %~dp0
cd ..

call .\build-deps.cmd
call .\build.cmd

call %~dp0type-shell-output.bat run_integration_tests