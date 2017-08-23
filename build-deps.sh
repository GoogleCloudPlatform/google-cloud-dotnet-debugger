SCRIPT=$(readlink -f "$0")
ROOT_DIR=$(dirname "$SCRIPT")

cd $ROOT_DIR/googletest/googletest/make
make gtest.a

cd $ROOT_DIR/googletest/googlemock/make
make gmock.a

cd $ROOT_DIR/protobuf
./autogen.sh
./configure
make
sudo make install
sudo ldconfig


cd $ROOT_DIR/coreclr
./build.sh skiptests

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$ROOT_DIR/coreclr/bin/Product/Linux.x64.Debug
sudo ldconfig
