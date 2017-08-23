# Builds all source in the repo.
# TODO(talarico): Allow configuration of platform and config

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

cd $ROOT_DIR/google_cloud_debugger_lib
make google_cloud_debugger

cd $ROOT_DIR/google_cloud_debugger
make google_cloud_debugger

cd $ROOT_DIR/google_cloud_debugger_test
make google_cloud_debugger_test

dotnet restore $ROOT_DIR/Google.Cloud.Diagnostics.Debug
dotnet build $ROOT_DIR/Google.Cloud.Diagnostics.Debug
