DIR1 = /usr/local/google/home/quoct/coreclr/src/pal/prebuilt/inc/
DIR2 = /usr/local/google/home/quoct/coreclr/bin/Product/Linux.x64.Debug/inc/
DIR3 = /usr/local/google/home/quoct/coreclr/src/pal/inc/rt/
DIR4 = /usr/local/google/home/quoct/coreclr/src/pal/inc/
DIR5 = /usr/local/google/home/quoct/coreclr/src/inc/
DIR6 = /usr/local/google/home/quoct/Debugging/ConsoleApplication1/third_party/Linux/
DIR7 = /usr/local/google/home/quoct/coreclr/src/debug/inc/
LIB = /usr/local/google/home/quoct/coreclr/bin/Product/Linux.x64.Debug/lib/
INCDIRS = -I${DIR1} -I${DIR2} -I${DIR3} -I${DIR4} -I${DIR5} -I${DIR6} -I${DIR7}
INCLIBS = -L${LIB} -L${DIR6} -lcorguids -lcoreclrpal -lpalrt -leventprovider -lpthread -ldl -luuid -lunwind-x86_64 -lstdc++ -ldbgshim
DBGOBJECTS = dbgobject.o dbgstring.o dbgarray.o dbgclass.o dbgclassfield.o dbgclassproperty.o
PDBPARSERS = metadataheaders.o metadatatables.o documentindex.o custombinaryreader.o portablepdbfile.o
ALLFILES = consoledebugger.o variablemanager.o evalcoordinator.o debuggercallback.o debugger.o dbgbreakpoint.o breakpointcollection.o ${DBGOBJECTS} ${PDBPARSERS}
CC_FLAGS = -x c++ -std=c++11 -fPIC -fms-extensions -fsigned-char -fwrapv -DFEATURE_PAL -DPAL_STDCPP_COMPAT -DBIT64 -DPLATFORM_UNIX -g

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

consoledebugger.o: consoledebugger.cc
	clang-3.5 consoledebugger.cc ${INCDIRS} ${CC_FLAGS} -c

debugger: ${ALLFILES}
	clang-3.5 -o Debugger ${ALLFILES} ${INCDIRS} ${CC_FLAGS} ${INCLIBS} -v

clean:
	rm *.o
