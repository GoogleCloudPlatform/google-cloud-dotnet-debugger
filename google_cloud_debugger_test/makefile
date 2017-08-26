# Directory that contains this makefile.
ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

# Directories of Google Test and Google Mock.
GTEST_DIR = $(ROOT_DIR)/../googletest/googletest/
GMOCK_DIR = $(ROOT_DIR)/../googletest/googlemock/

# .NET Core headers.
PREBUILT_PAL_INC = $(ROOT_DIR)/../coreclr/src/pal/prebuilt/inc/
PAL_RT_INC = $(ROOT_DIR)/../coreclr/src/pal/inc/rt/
PAL_INC = $(ROOT_DIR)/../coreclr/src/pal/inc/
CORE_CLR_INC = $(ROOT_DIR)/../coreclr/src/inc/
DBGSHIM_INC = $(ROOT_DIR)/../third_party/coreclr-subset/

# Google Test and Google Mock headers.
GMOCK_INC = $(GMOCK_DIR)include/
GTEST_INC = $(GTEST_DIR)include/

# Google Cloud Debugger Library directory.
GCLOUD_DEBUGGER = $(ROOT_DIR)/../google_cloud_debugger_lib/

# Google Test and Google Mock libraries.
GMOCK_LIB = $(ROOT_DIR)/../third_party/googlemock/linux/x64/$(CONFIG)/
GTEST_LIB = $(ROOT_DIR)/../third_party/googletest/linux/x64/$(CONFIG)/

# .NET Core libraries.
CORE_CLR_LIB = $(ROOT_DIR)/../third_party/coreclr/linux/x64/$(CONFIG)/

PROTO_LIB = $(ROOT_DIR)/../third_party/protobuf/linux/x64/$(CONFIG)/

CC_CONFIG = -g
ifeq ($(CONFIG), "Release")
  CC_CONFIG = -g0
endif

INCDIRS = -I${PREBUILT_PAL_INC} -I${PAL_RT_INC} -I${PAL_INC} -I${CORE_CLR_INC} -I${DBGSHIM_INC} -I${GCLOUD_DEBUGGER} -I${GMOCK_INC} -I${GTEST_INC} `pkg-config --cflags protobuf`
INCLIBS = -L${CORE_CLR_LIB} -L${PROTO_LIB} -L${GCLOUD_DEBUGGER} -L${GMOCK_LIB} -L${GTEST_LIB} -lcorguids -lcoreclrpal -lpalrt -leventprovider -lpthread -ldl -luuid -lunwind-x86_64 -lstdc++ -ldbgshim -lprotobuf -l:gtest.a -l:gmock.a -lgoogle_cloud_debugger
CC_FLAGS = -x c++ -std=c++11 -fPIC -fms-extensions -fsigned-char -fwrapv -DFEATURE_PAL -DPAL_STDCPP_COMPAT -DBIT64 -DPLATFORM_UNIX ${CC_CONFIG} -Wmacro-redefined

TESTS = unit_test_main.o dbgstring_test.o custom_binary_stream_test.o

dbgstring_test.o: dbgstring_test.cc
	clang-3.5 dbgstring_test.cc ${INCDIRS} ${CC_FLAGS} -c -o dbgstring_test.o

custom_binary_stream_test.o: custom_binary_stream_test.cc
	clang-3.5 custom_binary_stream_test.cc ${INCDIRS} ${CC_FLAGS} -c -o custom_binary_stream_test.o

unit_test_main.o: unit_test_main.cc
	clang-3.5 unit_test_main.cc ${INCDIRS} ${CC_FLAGS} -c -o unit_test_main.o

google_cloud_debugger_test: ${TESTS}
	clang-3.5 -o google_cloud_debugger_test ${TESTS} ${INCDIRS} ${CC_FLAGS} ${INCLIBS} -v

clean:
	rm -f *.o *.a google_cloud_debugger_test
