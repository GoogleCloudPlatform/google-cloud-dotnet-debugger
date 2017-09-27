# Directory that contains this makefile.
ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

# Directories of Google Test and Google Mock.
GTEST_DIR = $(ROOT_DIR)/../googletest/googletest/
GMOCK_DIR = $(ROOT_DIR)/../googletest/googlemock/

# .NET Core headers.
PREBUILT_PAL_INC = $(ROOT_DIR)/../coreclr/src/pal/prebuilt/inc/
BUILT_PAL_INC = $(ROOT_DIR)/../coreclr/bin/Product/Linux.x64.Debug/inc/
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
GMOCK_LIB = $(GMOCK_DIR)make/
GTEST_LIB = $(GTEST_DIR)make/

# .NET Core libraries.
CORE_CLR_LIB = $(ROOT_DIR)/../coreclr/bin/Product/Linux.x64.Debug/lib/
CORE_CLR_LIB2 = $(ROOT_DIR)/../coreclr/bin/Product/Linux.x64.Debug/

INCDIRS = -I${PREBUILT_PAL_INC} -I${BUILT_PAL_INC} -I${PAL_RT_INC} -I${PAL_INC} -I${CORE_CLR_INC} -I${DBGSHIM_INC} -I${GCLOUD_DEBUGGER} -I${GMOCK_INC} -I${GTEST_INC} `pkg-config --cflags protobuf`
INCLIBS = -L${CORE_CLR_LIB} -L${CORE_CLR_LIB2} -L${GCLOUD_DEBUGGER} -L${GMOCK_LIB} -L${GTEST_LIB} -lcorguids -lcoreclrpal -lpalrt -leventprovider -lpthread -ldl -luuid -lunwind-x86_64 -lstdc++ -ldbgshim `pkg-config --libs protobuf` -l:gtest.a -l:gmock.a -lgoogle_cloud_debugger_lib
CC_FLAGS = -x c++ -std=c++11 -fPIC -fms-extensions -fsigned-char -fwrapv -DFEATURE_PAL -DPAL_STDCPP_COMPAT -DBIT64 -DPLATFORM_UNIX -g -Wmacro-redefined

TESTS = unit_test_main.o breakpoint_client_test.o dbgstring_test.o dbgclassfield_test.o dbgclassproperty_test.o dbgarray_test.o custom_binary_stream_test.o common_action_mocks.o

common_action_mocks.o: common_action_mocks.h common_action_mocks.cc
	clang-3.5 common_action_mocks.cc ${INCDIRS} ${CC_FLAGS} -c -o common_action_mocks.o

breakpoint_client_test.o: breakpoint_client_test.cc
	clang-3.5 breakpoint_client_test.cc ${INCDIRS} ${CC_FLAGS} -c -o breakpoint_client_test.o

dbgbreakpoint_test.o: dbgbreakpoint_test.cc
	clang-3.5 dbgbreakpoint_test.cc ${INCDIRS} ${CC_FLAGS} -c -o dbgbreakpoint_test.o

dbgstring_test.o: dbgstring_test.cc
	clang-3.5 dbgstring_test.cc ${INCDIRS} ${CC_FLAGS} -c -o dbgstring_test.o

dbgclassfield_test.o: dbgclassfield_test.cc
	clang-3.5 dbgclassfield_test.cc ${INCDIRS} ${CC_FLAGS} -c -o dbgclassfield_test.o

dbgclassproperty_test.o: dbgclassproperty_test.cc
	clang-3.5 dbgclassproperty_test.cc ${INCDIRS} ${CC_FLAGS} -c -o dbgclassproperty_test.o

dbgclass_test.o: dbgclass_test.cc
	clang-3.5 dbgclass_test.cc ${INCDIRS} ${CC_FLAGS} -c -o dbgclass_test.o

dbgarray_test.o: dbgarray_test.cc
	clang-3.5 dbgarray_test.cc ${INCDIRS} ${CC_FLAGS} -c -o dbgarray_test.o

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

