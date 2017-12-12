# This script runs all integration tests.
# This script assumes that build-deps.sh has already been run.
#
# TODO(talarico): Make sure this works with debug and release builds
# TODO(talarico): This should also build deps but other scrips need to be fixed first

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

performance-tests=false
while (( "$#" )); do
  if [[ "$1" == "--performance-tests" ]]
  then 
    performance-tests=true
  fi
  shift
done

export LD_LIBRARY_PATH=$ROOT_DIR/coreclr/bin/Product/Linux.x64.Debug

dotnet publish $ROOT_DIR/Google.Cloud.Diagnostics.Debug.TestApp
dotnet test $ROOT_DIR/Google.Cloud.Diagnostics.Debug.IntegrationTests

if [[ "$performance-tests" == true ]]
then
  dotnet test $ROOT_DIR/Google.Cloud.Diagnostics.Debug.PerformanceTests
fi
