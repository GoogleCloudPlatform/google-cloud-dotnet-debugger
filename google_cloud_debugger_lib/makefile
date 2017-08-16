# Directory that contains this makefile.
ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

# .NET Core headers.
PREBUILT_PAL_INC = $(ROOT_DIR)/../coreclr/src/pal/prebuilt/inc/
PAL_RT_INC = $(ROOT_DIR)/../coreclr/src/pal/inc/rt/
PAL_INC = $(ROOT_DIR)/../coreclr/src/pal/inc/
CORE_CLR_INC = $(ROOT_DIR)/../coreclr/src/inc/
DBGSHIM_INC = $(ROOT_DIR)/../third_party/coreclr-subset/

INCDIRS = -I${PREBUILT_PAL_INC} -I${PAL_RT_INC} -I${PAL_INC} -I${CORE_CLR_INC} -I${DBGSHIM_INC} `pkg-config --cflags protobuf`

DBG_OBJECTS = dbgobject.o dbgstring.o dbgarray.o dbgclass.o dbgclassfield.o dbgclassproperty.o
PDB_PARSERS = metadataheaders.o metadatatables.o documentindex.o custombinaryreader.o portablepdbfile.o
BREAKPOINTS = dbgbreakpoint.o breakpointcollection.o breakpoint.o breakpoint_client.o
ALL_O_FILES = variablemanager.o evalcoordinator.o debuggercallback.o debugger.o  namedpiped.o ${BREAKPOINTS} ${DBG_OBJECTS} ${PDB_PARSERS}
CC_FLAGS = -x c++ -std=c++11 -fPIC -fms-extensions -fsigned-char -fwrapv -DFEATURE_PAL -DPAL_STDCPP_COMPAT -DBIT64 -DPLATFORM_UNIX -g

namedpiped.o: named_pipe_client_unix.h named_pipe_client_unix.cc
	clang-3.5 named_pipe_client_unix.cc ${INCDIRS} ${CC_FLAGS} -c -o namedpiped.o

breakpoint.o: breakpoint.pb.h breakpoint.pb.cc
	clang-3.5 breakpoint.pb.cc ${INCDIRS} ${CC_FLAGS} -c -o breakpoint.o

breakpoint_client.o: breakpoint_client.h breakpoint_client.cc
	clang-3.5 breakpoint_client.cc ${INCDIRS} ${CC_FLAGS} -c -o breakpoint_client.o

dbgobject.o: dbgobject.h dbgobject.cc
	clang-3.5 dbgobject.cc ${INCDIRS} ${CC_FLAGS} -c -o dbgobject.o

dbgstring.o: dbgstring.h dbgstring.cc
	clang-3.5 dbgstring.cc ${INCDIRS} ${CC_FLAGS} -c -o dbgstring.o

dbgarray.o: dbgarray.h dbgarray.cc
	clang-3.5 dbgarray.cc ${INCDIRS} ${CC_FLAGS} -c -o dbgarray.o

dbgclass.o: dbgclass.h dbgclass.cc
	clang-3.5 dbgclass.cc ${INCDIRS} ${CC_FLAGS} -c -o dbgclass.o

dbgclassfield.o: dbgclassfield.h dbgclassfield.cc
	clang-3.5 dbgclassfield.cc ${INCDIRS} ${CC_FLAGS} -c -o dbgclassfield.o

dbgclassproperty.o: dbgclassproperty.h dbgclassproperty.cc
	clang-3.5 dbgclassproperty.cc ${INCDIRS} ${CC_FLAGS} -c -o dbgclassproperty.o

evalcoordinator.o: evalcoordinator.h evalcoordinator.cc
	clang-3.5 evalcoordinator.cc ${INCDIRS} ${CC_FLAGS} -c -o evalcoordinator.o

variablemanager.o: variablemanager.h variablemanager.cc
	clang-3.5 variablemanager.cc ${INCDIRS} ${CC_FLAGS} -c -o variablemanager.o

debuggercallback.o: debuggercallback.h debuggercallback.cc
	clang-3.5 debuggercallback.cc ${INCDIRS} ${CC_FLAGS} -c -o debuggercallback.o

dbgbreakpoint.o: dbgbreakpoint.h dbgbreakpoint.cc
	clang-3.5 dbgbreakpoint.cc ${INCDIRS} ${CC_FLAGS} -c -o dbgbreakpoint.o

breakpointcollection.o: breakpointcollection.h breakpointcollection.cc
	clang-3.5 breakpointcollection.cc ${INCDIRS} ${CC_FLAGS} -c -o breakpointcollection.o

debugger.o: debugger.h debugger.cc
	clang-3.5 debugger.cc ${INCDIRS} ${CC_FLAGS} -c -o debugger.o

metadataheaders.o: metadataheaders.h metadataheaders.cc
	clang-3.5 metadataheaders.cc ${INCDIRS} ${CC_FLAGS} -c -o metadataheaders.o

metadatatables.o: metadatatables.h metadatatables.cc
	clang-3.5 metadatatables.cc ${INCDIRS} ${CC_FLAGS} -c -o metadatatables.o

documentindex.o: documentindex.h documentindex.cc
	clang-3.5 documentindex.cc ${INCDIRS} ${CC_FLAGS} -c -o documentindex.o

custombinaryreader.o: custombinaryreader.h custombinaryreader.cc
	clang-3.5 custombinaryreader.cc ${INCDIRS} ${CC_FLAGS} -c -o custombinaryreader.o

portablepdbfile.o: portablepdbfile.h portablepdbfile.cc
	clang-3.5 portablepdbfile.cc ${INCDIRS} ${CC_FLAGS} -c -o portablepdbfile.o

google_cloud_debugger: ${ALL_O_FILES}
	ar rcs libgoogle_cloud_debugger.a ${ALL_O_FILES}

clean:
	rm -f *.o *.a
