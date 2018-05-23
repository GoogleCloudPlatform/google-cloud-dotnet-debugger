#!/bin/bash

# The script builds and packages the debugger and agent into a minimum set
# to be used alone.

set -e

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

AGENT_DIR=$ROOT_DIR/src/Google.Cloud.Diagnostics.Debug
DEBUGGER_DIR=$ROOT_DIR/src/google_cloud_debugger
THIRD_PARTY_DIR=$ROOT_DIR/third_party

# Create 
COMMIT_HASH=$(git rev-parse --short HEAD)
TEMP_DIR=package-"$COMMIT_HASH"
TEMP_AGENT_DIR=$TEMP_DIR/agent
TEMP_DEBUGGER_DIR=$TEMP_DIR/debugger

mkdir -p $TEMP_DIR
mkdir -p $TEMP_AGENT_DIR
mkdir -p $TEMP_DEBUGGER_DIR

# Rebuild everything from scratch.
$ROOT_DIR/build-deps.sh --release --rebuild-protobuf
$ROOT_DIR/build.sh --release --rebuild

# Run unit and integration tests.
$ROOT_DIR/run_unit_tests.sh --release 
$ROOT_DIR/run_integration_tests.sh --release 

# Publish the agent.
dotnet restore -r debian.8-x64 $AGENT_DIR 
dotnet publish -c Release -f netcoreapp2.0 -r ubuntu.16.04-x64 $AGENT_DIR/Google.Cloud.Diagnostics.Debug/Google.Cloud.Diagnostics.Debug.csproj
cp -r $AGENT_DIR/Google.Cloud.Diagnostics.Debug/bin/Release/netcoreapp2.0/ubuntu.16.04-x64/publish/* $TEMP_AGENT_DIR

# Copy over the debugger.
cp $DEBUGGER_DIR/google_cloud_debugger/google_cloud_debugger $TEMP_DEBUGGER_DIR

# Copy the needed so files.
cp $THIRD_PARTY_DIR/protobuf/src/.libs/libprotobuf.so.13.0.2 $TEMP_DEBUGGER_DIR/libprotobuf.so.13
# TODO(talarico): Figure out which exactly which .so files we need.
cp $THIRD_PARTY_DIR/coreclr/bin/Product/Linux.x64.Release/*.so $TEMP_DEBUGGER_DIR

# Package everyting into a tar. 
cd $TEMP_DIR
tar -czvf $TEMP_DIR.tar.gz *
mv $TEMP_DIR.tar.gz $ROOT_DIR
