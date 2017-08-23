SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$ROOT_DIR/coreclr/bin/Product/Linux.x64.Debug
sudo ldconfig

cd $ROOT_DIR/google_cloud_debugger_lib
make google_cloud_debugger

cd $ROOT_DIR/google_cloud_debugger
make google_cloud_debugger

cd $ROOT_DIR/google_cloud_debugger_test
make google_cloud_debugger_test

dotnet restore $ROOT_DIR/Google.Cloud.Diagnostics.Debug
dotnet build $ROOT_DIR/Google.Cloud.Diagnostics.Debug
