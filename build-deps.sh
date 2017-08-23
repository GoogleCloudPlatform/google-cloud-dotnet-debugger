# The script builds the submodule dependencies of this repository.
# TODO(talarico): Only rebuild submodules if needed.
# TODO(talarico): Allow configuration of platform and config

SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

# Build the gtest library
cd $ROOT_DIR/googletest/googletest/make
make gtest.a

# Build the gmock library
cd $ROOT_DIR/googletest/googlemock/make
make gmock.a

# Build the protobuf library
cd $ROOT_DIR/protobuf
./autogen.sh
./configure
make
sudo make install

# Build the coreclr library
cd $ROOT_DIR/coreclr
./build.sh skiptests skipnuget skipmscorlib

# Add the the core clr library path to LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$ROOT_DIR/coreclr/bin/Product/Linux.x64.Debug
sudo ldconfig
