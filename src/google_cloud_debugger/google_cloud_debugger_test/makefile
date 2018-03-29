# Directory that contains this makefile.
ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
THIRD_PARTY_DIR:=$(realpath $(ROOT_DIR)/../../../third_party)

ifeq ($(COVERAGE),true)
  COVERAGE_ARG = -coverage
endif

# Directories of Google Test and Google Mock.
GTEST_DIR = $(THIRD_PARTY_DIR)/googletest/googletest/
GMOCK_DIR = $(THIRD_PARTY_DIR)/googletest/googlemock/

# .NET Core headers.
PREBUILT_PAL_INC = $(THIRD_PARTY_DIR)/coreclr/src/pal/prebuilt/inc/
BUILT_PAL_INC = $(THIRD_PARTY_DIR)/coreclr/bin/Product/Linux.x64.Debug/inc/
PAL_RT_INC = $(THIRD_PARTY_DIR)/coreclr/src/pal/inc/rt/
PAL_INC = $(THIRD_PARTY_DIR)/coreclr/src/pal/inc/
CORE_CLR_INC = $(THIRD_PARTY_DIR)/coreclr/src/inc/
DBGSHIM_INC = $(THIRD_PARTY_DIR)/coreclr-subset/

# Google Test and Google Mock headers.
GMOCK_INC = $(GMOCK_DIR)include/
GTEST_INC = $(GTEST_DIR)include/

# Google Cloud Debugger Library directory.
GCLOUD_DEBUGGER = $(ROOT_DIR)/../google_cloud_debugger_lib

# ANTLR Library directory.
ANTLR_LIB = $(THIRD_PARTY_DIR)/antlr/lib/cpp
ANTLR_INC = $(THIRD_PARTY_DIR)/antlr/lib/cpp/antlr

# Google Test and Google Mock libraries.
GMOCK_LIB = $(GMOCK_DIR)make/
GTEST_LIB = $(GTEST_DIR)make/

# Cloud Debug Java directory.
DEBUG_JAVA = $(THIRD_PARTY_DIR)/cloud-debug-java/

# .NET Core libraries.
CORE_CLR_LIB = $(THIRD_PARTY_DIR)/coreclr/bin/Product/Linux.x64.Debug/lib/
CORE_CLR_LIB2 = $(THIRD_PARTY_DIR)/coreclr/bin/Product/Linux.x64.Debug/

INCDIRS = -I${PREBUILT_PAL_INC} -I${BUILT_PAL_INC} -I${PAL_RT_INC} -I${PAL_INC} -I${CORE_CLR_INC} -I${DBGSHIM_INC} -I${GCLOUD_DEBUGGER} -I${DEBUG_JAVA} -I${GMOCK_INC} -I${GTEST_INC} `pkg-config --cflags protobuf`
INCLIBS = -L${CORE_CLR_LIB} -L${CORE_CLR_LIB2} -L${GCLOUD_DEBUGGER} -L${ANTLR_LIB} -L${GMOCK_LIB} -L${GTEST_LIB} -lcorguids -lcoreclrpal -lpalrt -lm -leventprovider -lpthread -ldl -luuid -lunwind-x86_64 -lstdc++ `pkg-config --libs protobuf` -l:gtest.a -l:gmock.a -lgoogle_cloud_debugger_lib -lantlr_lib
CC_FLAGS = -x c++ -std=c++11 -fPIC -fms-extensions -fsigned-char -fwrapv -DFEATURE_PAL -DPAL_STDCPP_COMPAT -DBIT64 -DPLATFORM_UNIX -g -Wmacro-redefined 

SRC_TEST_FILES := $(wildcard *_test.cc)
OBJ_TEST_FILES := $(patsubst %_test.cc,%_test.o,${SRC_TEST_FILES})

TESTS = unit_test_main.o ${OBJ_TEST_FILES} common_action_mocks.o i_portable_pdb_mocks.o i_dbg_object_factory_mock.o

google_cloud_debugger_test: ${TESTS}
	clang-3.9 -o google_cloud_debugger_test ${TESTS} ${INCDIRS} ${CC_FLAGS} ${COVERAGE_ARG} ${INCLIBS} -v

common_action_mocks.o: common_action_mocks.h common_action_mocks.cc
	clang-3.9 common_action_mocks.cc ${INCDIRS} ${CC_FLAGS} -c -o common_action_mocks.o

i_portable_pdb_mocks.o: i_portable_pdb_mocks.h i_portable_pdb_mocks.cc
	clang-3.9 i_portable_pdb_mocks.cc ${INCDIRS} ${CC_FLAGS} -c -o i_portable_pdb_mocks.o

i_dbg_object_factory_mock.o: i_dbg_object_factory_mock.h i_dbg_object_factory_mock.cc
	clang-3.9 i_dbg_object_factory_mock.cc ${INCDIRS} ${CC_FLAGS} -c -o i_dbg_object_factory_mock.o

%_test.o: %_test.cc
	clang-3.9 ${INCDIRS} ${CC_FLAGS} -c -o $@ $<

unit_test_main.o: unit_test_main.cc
	clang-3.9 unit_test_main.cc ${INCDIRS} ${CC_FLAGS} -c -o unit_test_main.o

clean:
	rm -f *.o *.a *.g* google_cloud_debugger_test

