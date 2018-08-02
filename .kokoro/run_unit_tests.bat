:: See documentation in type-shell-output.bat

set ROOT_DIR=%~dp0..
cd %ROOT_DIR%

git submodule init
git submodule update

%ROOT_DIR%\build-deps.cmd & ^
%ROOT_DIR%\build.cmd & ^
"C:\Program Files\Git\bin\bash.exe" %ROOT_DIR%\run_unit_tests.sh
