# This script runs all unit tests.
# This script assumes that build.sh and build-deps.sh have been run.
#
# TODO(talarico): Make sure this works with debug and release builds

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

dotnet test $ROOT_DIR/Google.Cloud.Diagnostics.Debug.Tests

if [[ "$OS" == "Windows_NT" ]]
then
  $ROOT_DIR/x64/Debug/google_cloud_debugger_test
else
  $ROOT_DIR/google_cloud_debugger_test/google_cloud_debugger_test
fi
