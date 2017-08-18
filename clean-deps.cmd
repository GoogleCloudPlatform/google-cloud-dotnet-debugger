:: The script cleans up the submodule repos built by build-deps.cmd.  This is a very cruid script that will 
:: completely wipe out the repos to head.

:: Check that the user passed in a valid parameter.
if "%1"=="" (
  echo "First parameter should be the repos root directory"
  exit
)

SET ROOT_DIR=%1

:: Clean the protobuf repo.
cd %ROOT_DIR%protobuf
git clean -f -d
git reset --hard origin/master

:: Clean the coreclr repo.
cd %ROOT_DIR%coreclr
git clean -f -d
git reset --hard origin/master

:: Clean the googletest repo.
cd %ROOT_DIR%googletest
git clean -f -d
git reset --hard origin/master