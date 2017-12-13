# The script builds the submodule dependencies of this repository.
# TODO(talarico): Allow configuration of platform and config

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

GTEST_DIR=$ROOT_DIR/googletest/googletest/make
GMOCK_DIR=$ROOT_DIR/googletest/googlemock/make
PROTOBUF_DIR=$ROOT_DIR/protobuf
CORECLR_DIR=$ROOT_DIR/coreclr
CORECLR_BIN=$CORECLR_DIR/bin/Product/Linux.x64.Debug

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
if [[ ! $(ldconfig -p | grep libprotobuf.so ) ]]
then
  # We need to run in the protobuf directory per comments in autogen.sh
  cd $PROTOBUF_DIR
  ./autogen.sh
  ./configure
  make
  sudo make install
else
  echo "Skipping protobuf, it was already built."
fi

# Build the coreclr library
if [[ ! -d $CORECLR_BIN ]]
then
  $CORECLR_DIR/build.sh skiptests skipnuget skipmscorlib
else
  echo "Skipping coreclr, it was already built."
fi

export LD_LIBRARY_PATH=$CORECLR_BIN
sudo ldconfig
