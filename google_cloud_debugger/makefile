# Directory that contains this makefile.
ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

# .NET Core headers.
PREBUILT_PAL_INC = $(ROOT_DIR)/../coreclr/src/pal/prebuilt/inc/
PAL_RT_INC = $(ROOT_DIR)/../coreclr/src/pal/inc/rt/
PAL_INC = $(ROOT_DIR)/../coreclr/src/pal/inc/
CORE_CLR_INC = $(ROOT_DIR)/../coreclr/src/inc/
DBGSHIM_INC = $(ROOT_DIR)/../third_party/coreclr-subset/
GCLOUD_DEBUGGER = $(ROOT_DIR)/../google_cloud_debugger_lib/

# .NET Core libraries.
CORE_CLR_LIB = $(ROOT_DIR)/../third_party/coreclr/linux/x64/$(CONFIG)/

PROTO_LIB = $(ROOT_DIR)/../third_party/protobuf/linux/x64/$(CONFIG)/

CC_CONFIG = -g
ifeq ($(CONFIG), "Release")
  CC_CONFIG = -g0
endif

INCDIRS = -I${PREBUILT_PAL_INC} -I${PAL_RT_INC} -I${PAL_INC} -I${CORE_CLR_INC} -I${DBGSHIM_INC} -I${GCLOUD_DEBUGGER} `pkg-config --cflags protobuf`
INCLIBS = -L${GCLOUD_DEBUGGER} -L${CORE_CLR_LIB} -L${PROTO_LIB} -lcorguids -lcoreclrpal -lpalrt -leventprovider -lpthread -ldl -luuid -lunwind-x86_64 -lstdc++ -ldbgshim -lprotobuf -lgoogle_cloud_debugger
CC_FLAGS = -x c++ -std=c++11 -fPIC -fms-extensions -fsigned-char -fwrapv -DFEATURE_PAL -DPAL_STDCPP_COMPAT -DBIT64 -DPLATFORM_UNIX ${CC_CONFIG}

consoledebugger.o: consoledebugger.cc
	clang-3.5 consoledebugger.cc ${INCDIRS} ${CC_FLAGS} -c

google_cloud_debugger: consoledebugger.o
	clang-3.5 -o google_cloud_debugger consoledebugger.o ${INCDIRS} ${CC_FLAGS} ${INCLIBS} -v

clean:
	rm -f *.o *.a google_cloud_debugger
