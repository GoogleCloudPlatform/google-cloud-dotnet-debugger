:: See documentation in type-shell-output.bat

cd %~dp0
cd ..

git submodule init
git submodule update

call .\build-deps.cmd
call .\build.cmd

"C:\Program Files\Git\bin\bash.exe" run_unit_tests.sh
