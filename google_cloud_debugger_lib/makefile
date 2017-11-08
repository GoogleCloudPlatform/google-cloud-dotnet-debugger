# Directory that contains this makefile.
ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

# .NET Core headers.
PREBUILT_PAL_INC = $(ROOT_DIR)/../coreclr/src/pal/prebuilt/inc/
PAL_RT_INC = $(ROOT_DIR)/../coreclr/src/pal/inc/rt/
PAL_INC = $(ROOT_DIR)/../coreclr/src/pal/inc/
CORE_CLR_INC = $(ROOT_DIR)/../coreclr/src/inc/
DBGSHIM_INC = $(ROOT_DIR)/../third_party/coreclr-subset/

INCDIRS = -I${PREBUILT_PAL_INC} -I${PAL_RT_INC} -I${PAL_INC} -I${CORE_CLR_INC} -I${DBGSHIM_INC} `pkg-config --cflags protobuf`

DBG_OBJECTS = dbg_object.o dbg_string.o dbg_array.o dbg_class.o dbg_class_field.o dbg_class_property.o dbg_stack_frame.o dbg_enum.o dbg_builtin_collection.o
PDB_PARSERS = metadata_headers.o metadata_tables.o document_index.o custom_binary_reader.o portable_pdb_file.o
BREAKPOINTS = dbg_breakpoint.o breakpoint_collection.o breakpoint.o breakpoint_client.o
ALL_O_FILES = string_stream_wrapper.o stack_frame_collection.o eval_coordinator.o debugger_callback.o debugger.o namedpiped.o i_cor_debug_helper.o ${BREAKPOINTS} ${DBG_OBJECTS} ${PDB_PARSERS}
CC_FLAGS = -x c++ -std=c++11 -fPIC -fms-extensions -fsigned-char -fwrapv -DFEATURE_PAL -DPAL_STDCPP_COMPAT -DBIT64 -DPLATFORM_UNIX -g

namedpiped.o: named_pipe_client_unix.h named_pipe_client_unix.cc
	clang-3.5 named_pipe_client_unix.cc ${INCDIRS} ${CC_FLAGS} -c -o namedpiped.o

breakpoint.o: breakpoint.pb.h breakpoint.pb.cc
	clang-3.5 breakpoint.pb.cc ${INCDIRS} ${CC_FLAGS} -c -o breakpoint.o

breakpoint_client.o: breakpoint_client.h breakpoint_client.cc
	clang-3.5 breakpoint_client.cc ${INCDIRS} ${CC_FLAGS} -c -o breakpoint_client.o

dbg_object.o: dbg_object.h dbg_object.cc
	clang-3.5 dbg_object.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_object.o

dbg_string.o: dbg_string.h dbg_string.cc
	clang-3.5 dbg_string.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_string.o

dbg_array.o: dbg_array.h dbg_array.cc
	clang-3.5 dbg_array.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_array.o

dbg_class.o: dbg_class.h dbg_class.cc
	clang-3.5 dbg_class.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_class.o

dbg_enum.o: dbg_enum.h dbg_enum.cc
	clang-3.5 dbg_enum.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_enum.o

dbg_builtin_collection.o: dbg_builtin_collection.h dbg_builtin_collection.cc
	clang-3.5 dbg_builtin_collection.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_builtin_collection.o

dbg_class_field.o: dbg_class_field.h dbg_class_field.cc
	clang-3.5 dbg_class_field.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_class_field.o

dbg_class_property.o: dbg_class_property.h dbg_class_property.cc
	clang-3.5 dbg_class_property.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_class_property.o
	
dbg_stack_frame.o: dbg_stack_frame.h dbg_stack_frame.cc
	clang-3.5 dbg_stack_frame.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_stack_frame.o

eval_coordinator.o: i_eval_coordinator.h eval_coordinator.h eval_coordinator.cc
	clang-3.5 eval_coordinator.cc ${INCDIRS} ${CC_FLAGS} -c -o eval_coordinator.o

string_stream_wrapper.o: string_stream_wrapper.h string_stream_wrapper.h string_stream_wrapper.cc
	clang-3.5 string_stream_wrapper.cc ${INCDIRS} ${CC_FLAGS} -c -o string_stream_wrapper.o

stack_frame_collection.o: i_stack_frame_collection.h stack_frame_collection.h stack_frame_collection.cc
	clang-3.5 stack_frame_collection.cc ${INCDIRS} ${CC_FLAGS} -c -o stack_frame_collection.o

debugger_callback.o: debugger_callback.h debugger_callback.cc
	clang-3.5 debugger_callback.cc ${INCDIRS} ${CC_FLAGS} -c -o debugger_callback.o

dbg_breakpoint.o: dbg_breakpoint.h dbg_breakpoint.cc
	clang-3.5 dbg_breakpoint.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_breakpoint.o

breakpoint_collection.o: breakpoint_collection.h breakpoint_collection.cc
	clang-3.5 breakpoint_collection.cc ${INCDIRS} ${CC_FLAGS} -c -o breakpoint_collection.o

i_cor_debug_helper.o: i_cor_debug_helper.h i_cor_debug_helper.cc
	clang-3.5 i_cor_debug_helper.cc ${INCDIRS} ${CC_FLAGS} -c -o i_cor_debug_helper.o

debugger.o: debugger.h debugger.cc
	clang-3.5 debugger.cc ${INCDIRS} ${CC_FLAGS} -c -o debugger.o

metadata_headers.o: metadata_headers.h metadata_headers.cc
	clang-3.5 metadata_headers.cc ${INCDIRS} ${CC_FLAGS} -c -o metadata_headers.o

metadata_tables.o: metadata_tables.h metadata_tables.cc
	clang-3.5 metadata_tables.cc ${INCDIRS} ${CC_FLAGS} -c -o metadata_tables.o

document_index.o: document_index.h document_index.cc
	clang-3.5 document_index.cc ${INCDIRS} ${CC_FLAGS} -c -o document_index.o

custom_binary_reader.o: custom_binary_reader.h custom_binary_reader.cc
	clang-3.5 custom_binary_reader.cc ${INCDIRS} ${CC_FLAGS} -c -o custom_binary_reader.o

portable_pdb_file.o: i_portable_pdb_file.h portable_pdb_file.h portable_pdb_file.cc
	clang-3.5 portable_pdb_file.cc ${INCDIRS} ${CC_FLAGS} -c -o portable_pdb_file.o

google_cloud_debugger_lib: ${ALL_O_FILES}
	ar rcs libgoogle_cloud_debugger_lib.a ${ALL_O_FILES}

clean:
	rm -f *.o *.a
