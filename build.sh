# Builds all source in the repo.
# TODO(talarico): Allow configuration of platform 

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

CONFIG="Debug"
if [ "$1" != "" ]; then
  CONFIG=$1
fi

if [ "$CONFIG" != "Debug" -a "$CONFIG" != "Release" ]; then
  echo "Invlaid configuration '$CONFIG'"
  exit
fi
    

cd $ROOT_DIR/google_cloud_debugger_lib
make google_cloud_debugger CONFIG=$CONFIG

cd $ROOT_DIR/google_cloud_debugger
make google_cloud_debugger CONFIG=$CONFIG

cd $ROOT_DIR/google_cloud_debugger_test
make google_cloud_debugger_test CONFIG=$CONFIG

dotnet restore $ROOT_DIR/Google.Cloud.Diagnostics.Debug
dotnet build $ROOT_DIR/Google.Cloud.Diagnostics.Debug -c $CONFIG
