:: See documentation in type-shell-output.bat

cd %~dp0
cd ..

call .\build-deps.cmd
call .\build.cmd

bash -c ./run_unit_tests.sh
