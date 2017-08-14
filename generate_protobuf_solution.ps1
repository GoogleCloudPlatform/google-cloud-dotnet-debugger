cd .\protobuf\cmake\
mkdir build\solution
cd .\build\solution\
cmake -G "Visual Studio 15 2017 Win64" -Dprotobuf_BUILD_TESTS=OFF ../..