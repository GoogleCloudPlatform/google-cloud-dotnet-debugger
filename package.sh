# The script builds and packages the debugger and agent into a minium set
# to be used alone.
#
# TODO(talarico): This script currently assumes build.sh and build-deps.sh
#     have already been run.
# TODO(talarico): Allow configuration of platform and config


SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

# Create 
COMMIT_HASH=$(git rev-parse --short HEAD)
TEMP_DIR=package-"$COMMIT_HASH"

mkdir -p $TEMP_DIR

# Publish the agent.
dotnet restore -r debian.8-x64
dotnet publish -c Debug -f netcoreapp1.1 -r debian.8-x64 $ROOT_DIR/Google.Cloud.Diagnostics.Debug/Google.Cloud.Diagnostics.Debug/Google.Cloud.Diagnostics.Debug.csproj
cp -r $ROOT_DIR/Google.Cloud.Diagnostics.Debug/Google.Cloud.Diagnostics.Debug/bin/Debug/netcoreapp1.1/debian.8-x64/publish/* $TEMP_DIR

# Copy over the debugger.
cp $ROOT_DIR/google_cloud_debugger/google_cloud_debugger $TEMP_DIR

# Copy the needed so files.
cp $ROOT_DIR/protobuf/src/.libs/libprotobuf.so.13.0.2 $TEMP_DIR/libprotobuf.so.13
# TODO(talarico): Figure out which exactly which .so files we need.
cp $ROOT_DIR/coreclr/bin/Product/Linux.x64.Debug/*.so $TEMP_DIR

# Package everyting into a tar. 
cd $TEMP_DIR
tar -czvf $TEMP_DIR.tar.gz *
mv $TEMP_DIR.tar.gz $ROOT_DIR
