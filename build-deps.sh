#!/bin/bash

# The script builds the submodule dependencies of this repository.

set -e

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

CONFIG=debug
CONFIG_CAP=Debug
PROTOBUF_CXXFLAGS=-g
REBUILD_PROTOBUF=false
while (( "$#" )); do
  if [[ "$1" == "--release" ]]
  then
    CONFIG=release
    CONFIG_CAP=Release
    PROTOBUF_CXXFLAGS=
  # We have the option to rebuild protobuf as it installs
  # itself and will need to be relinked it switching between
  # debug and release.gi
  elif [[ "$1" == "--rebuild-protobuf" ]]
  then 
    REBUILD_PROTOBUF=true
  fi
  shift
done

THIRD_PARTY_DIR=$ROOT_DIR/third_party
GTEST_DIR=$THIRD_PARTY_DIR/googletest/googletest/make
GMOCK_DIR=$THIRD_PARTY_DIR/googletest/googlemock/make
PROTOBUF_DIR=$THIRD_PARTY_DIR/protobuf
CORECLR_DIR=$THIRD_PARTY_DIR/coreclr
CORECLR_BIN=$CORECLR_DIR/bin/Product/Linux.x64.$CONFIG_CAP
LIB_DBGSHIM=$CORECLR_BIN/libdbgshim.so

# Build the gtest library
if [[ ! -f $GTEST_DIR/gtest.a ]]
then
  make -C $GTEST_DIR gtest.a
else
  echo "Skipping gtest, it was already built."
fi

# Build the gmock library
if [[ ! -f $GMOCK_DIR/gmock.a ]]
then
  make -C $GMOCK_DIR gmock.a
else
  echo "Skipping gmock, it was already built."
fi

# Build the protobuf library
if [[ ! $(ldconfig -p | grep libprotobuf.so ) || $REBUILD_PROTOBUF = true ]]
then
  # We need to run in the protobuf directory per comments in autogen.sh
  cd $PROTOBUF_DIR
  ./autogen.sh
  ./configure CXXFLAGS=$PROTOBUF_CXXFLAGS
  sudo make install
else
  echo "Skipping protobuf, it was already built."
fi

# Build the coreclr library
if [[ ! -f $LIB_DBGSHIM ]]
then
  cd $CORECLR_DIR
  ./build.sh -$CONFIG skiptests skipnuget
else
  echo "Skipping coreclr, it was already built."
fi


export LD_LIBRARY_PATH=$CORECLR_BIN
sudo ldconfig
