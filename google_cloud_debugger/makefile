# Directory that contains this makefile.
ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

ifeq ($(COVERAGE),true)
  COVERAGE_ARG = -coverage
endif

# .NET Core headers.
PREBUILT_PAL_INC = $(ROOT_DIR)/../coreclr/src/pal/prebuilt/inc/
PAL_RT_INC = $(ROOT_DIR)/../coreclr/src/pal/inc/rt/
PAL_INC = $(ROOT_DIR)/../coreclr/src/pal/inc/
CORE_CLR_INC = $(ROOT_DIR)/../coreclr/src/inc/
DBGSHIM_INC = $(ROOT_DIR)/../third_party/coreclr-subset/
OPTION_PARSER_INC = $(ROOT_DIR)/../third_party/option-parser/
GCLOUD_DEBUGGER = $(ROOT_DIR)/../google_cloud_debugger_lib/

# .NET Core libraries.
CORE_CLR_LIB = $(ROOT_DIR)/../coreclr/bin/Product/Linux.x64.Debug/lib/
CORE_CLR_LIB2 = $(ROOT_DIR)/../coreclr/bin/Product/Linux.x64.Debug/

INCDIRS = -I${PREBUILT_PAL_INC} -I${PAL_RT_INC} -I${PAL_INC} -I${CORE_CLR_INC} -I${DBGSHIM_INC} -I${GCLOUD_DEBUGGER} -I${OPTION_PARSER_INC} `pkg-config --cflags protobuf`
INCLIBS = -L${GCLOUD_DEBUGGER} -L${CORE_CLR_LIB} -L${CORE_CLR_LIB2} -lcorguids -lcoreclrpal -lpalrt -leventprovider -lpthread -ldl -luuid -lunwind-x86_64 -lstdc++ -ldbgshim `pkg-config --libs protobuf` -lgoogle_cloud_debugger_lib
CC_FLAGS = -x c++ -std=c++11 -fPIC -fms-extensions -fsigned-char -fwrapv -DFEATURE_PAL -DPAL_STDCPP_COMPAT -DBIT64 -DPLATFORM_UNIX -g ${COVERAGE_ARG}

google_cloud_debugger: consoledebugger.o
	clang-3.9 -o google_cloud_debugger consoledebugger.o ${INCDIRS} ${CC_FLAGS} ${INCLIBS} -v

consoledebugger.o: consoledebugger.cc
	clang-3.9 consoledebugger.cc ${INCDIRS} ${CC_FLAGS} -c

clean:
	rm -f *.o *.a *.g* google_cloud_debugger
