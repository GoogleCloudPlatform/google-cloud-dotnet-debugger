# Directory that contains this makefile.
ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
REPO_DIR:=$(realpath $(ROOT_DIR)/../../../)
THIRD_PARTY_DIR:=$(realpath $(ROOT_DIR)/../../../third_party)

ifeq ($(COVERAGE),true)
  COVERAGE_ARG = -coverage
endif

# .NET Core headers.
PREBUILT_PAL_INC = $(THIRD_PARTY_DIR)/coreclr/src/pal/prebuilt/inc/
PAL_RT_INC = $(THIRD_PARTY_DIR)/coreclr/src/pal/inc/rt/
PAL_INC = $(THIRD_PARTY_DIR)/coreclr/src/pal/inc/
CORE_CLR_INC = $(THIRD_PARTY_DIR)/coreclr/src/inc/
DBGSHIM_INC = $(THIRD_PARTY_DIR)/coreclr-subset/
JAVA_DBG_INC = $(THIRD_PARTY_DIR)/cloud-debug-java/
ANTLR_DIR = $(THIRD_PARTY_DIR)/antlr/lib/cpp/
ANTLR_GEN_DIR = $(JAVA_DBG_INC)/antlrgen/

INCDIRS = -I${PREBUILT_PAL_INC} -I${PAL_RT_INC} -I${PAL_INC} -I${CORE_CLR_INC} -I${DBGSHIM_INC} -I${JAVA_DBG_INC} -I${ROOT_DIR} -I${REPO_DIR} -I${ANTLR_DIR} `pkg-config --cflags protobuf`

DBG_OBJECTS = dbg_object.o dbg_string.o dbg_array.o dbg_class.o dbg_class_field.o dbg_class_property.o dbg_stack_frame.o dbg_enum.o dbg_builtin_collection.o dbg_reference_object.o dbg_object_factory.o
PDB_PARSERS = metadata_headers.o metadata_tables.o document_index.o custom_binary_reader.o portable_pdb_file.o
BREAKPOINTS = dbg_breakpoint.o breakpoint_collection.o breakpoint.o breakpoint_client.o variable_wrapper.o breakpoint_location_collection.o method_info.o
EXPRESSION_EVALUATORS = array_expression_evaluator.o binary_expression_evaluator.o conditional_operator_evaluator.o csharp_expression.o expression_util.o field_evaluator.o identifier_evaluator.o method_call_evaluator.o string_evaluator.o type_cast_operator_evaluator.o unary_expression_evaluator.o type_signature.o
ANTLR_GEN_FILES = csharp_expression_compiler.o csharp_expression_lexer.o csharp_expression_parser.o
ALL_O_FILES = string_stream_wrapper.o stack_frame_collection.o eval_coordinator.o debugger_callback.o debugger.o namedpiped.o cor_debug_helper.o compiler_helpers.o ${BREAKPOINTS} ${DBG_OBJECTS} ${PDB_PARSERS} ${EXPRESSION_EVALUATORS} ${ANTLR_GEN_FILES}
CC_FLAGS = -x c++ -std=c++11 -fPIC -fms-extensions -fsigned-char -fwrapv -DFEATURE_PAL -DPAL_STDCPP_COMPAT -DBIT64 -DPLATFORM_UNIX -Wignored-attributes -g ${COVERAGE_ARG}

google_cloud_debugger_lib: ${ALL_O_FILES}
	ar rcs libgoogle_cloud_debugger_lib.a ${ALL_O_FILES}

namedpiped.o: named_pipe_client_unix.h named_pipe_client_unix.cc
	clang-3.9 named_pipe_client_unix.cc ${INCDIRS} ${CC_FLAGS} -c -o namedpiped.o

breakpoint.o: breakpoint.pb.h breakpoint.pb.cc
	clang-3.9 breakpoint.pb.cc ${INCDIRS} ${CC_FLAGS} -c -o breakpoint.o

breakpoint_client.o: breakpoint_client.h breakpoint_client.cc
	clang-3.9 breakpoint_client.cc ${INCDIRS} ${CC_FLAGS} -c -o breakpoint_client.o

dbg_object.o: dbg_object.h dbg_object.cc
	clang-3.9 dbg_object.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_object.o

dbg_string.o: dbg_string.h dbg_string.cc
	clang-3.9 dbg_string.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_string.o

dbg_array.o: dbg_array.h dbg_array.cc
	clang-3.9 dbg_array.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_array.o

dbg_class.o: dbg_class.h dbg_class.cc
	clang-3.9 dbg_class.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_class.o

dbg_reference_object.o: dbg_reference_object.h dbg_reference_object.cc
	clang-3.9 dbg_reference_object.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_reference_object.o

dbg_enum.o: dbg_enum.h dbg_enum.cc
	clang-3.9 dbg_enum.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_enum.o

dbg_builtin_collection.o: dbg_builtin_collection.h dbg_builtin_collection.cc
	clang-3.9 dbg_builtin_collection.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_builtin_collection.o

dbg_class_field.o: dbg_class_field.h dbg_class_field.cc
	clang-3.9 dbg_class_field.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_class_field.o

dbg_class_property.o: dbg_class_property.h dbg_class_property.cc
	clang-3.9 dbg_class_property.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_class_property.o
	
dbg_stack_frame.o: dbg_stack_frame.h dbg_stack_frame.cc
	clang-3.9 dbg_stack_frame.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_stack_frame.o

eval_coordinator.o: i_eval_coordinator.h eval_coordinator.h eval_coordinator.cc
	clang-3.9 eval_coordinator.cc ${INCDIRS} ${CC_FLAGS} -c -o eval_coordinator.o

string_stream_wrapper.o: string_stream_wrapper.h string_stream_wrapper.h string_stream_wrapper.cc
	clang-3.9 string_stream_wrapper.cc ${INCDIRS} ${CC_FLAGS} -c -o string_stream_wrapper.o

stack_frame_collection.o: i_stack_frame_collection.h stack_frame_collection.h stack_frame_collection.cc
	clang-3.9 stack_frame_collection.cc ${INCDIRS} ${CC_FLAGS} -c -o stack_frame_collection.o

debugger_callback.o: debugger_callback.h debugger_callback.cc
	clang-3.9 debugger_callback.cc ${INCDIRS} ${CC_FLAGS} -c -o debugger_callback.o

dbg_breakpoint.o: dbg_breakpoint.h dbg_breakpoint.cc
	clang-3.9 dbg_breakpoint.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_breakpoint.o

breakpoint_collection.o: breakpoint_collection.h breakpoint_collection.cc
	clang-3.9 breakpoint_collection.cc ${INCDIRS} ${CC_FLAGS} -c -o breakpoint_collection.o

breakpoint_location_collection.o: breakpoint_location_collection.h breakpoint_location_collection.cc
	clang-3.9 breakpoint_location_collection.cc ${INCDIRS} ${CC_FLAGS} -c -o breakpoint_location_collection.o

method_info.o: method_info.h method_info.cc
	clang-3.9 method_info.cc ${INCDIRS} ${CC_FLAGS} -c -o method_info.o

compiler_helpers.o: compiler_helpers.cc compiler_helpers.h
	clang-3.9 compiler_helpers.cc ${INCDIRS} ${CC_FLAGS} -c -o compiler_helpers.o

cor_debug_helper.o: cor_debug_helper.h cor_debug_helper.cc
	clang-3.9 cor_debug_helper.cc ${INCDIRS} ${CC_FLAGS} -c -o cor_debug_helper.o

dbg_object_factory.o: dbg_object_factory.h dbg_object_factory.cc
	clang-3.9 dbg_object_factory.cc ${INCDIRS} ${CC_FLAGS} -c -o dbg_object_factory.o

debugger.o: debugger.h debugger.cc
	clang-3.9 debugger.cc ${INCDIRS} ${CC_FLAGS} -c -o debugger.o

metadata_headers.o: metadata_headers.h metadata_headers.cc
	clang-3.9 metadata_headers.cc ${INCDIRS} ${CC_FLAGS} -c -o metadata_headers.o

metadata_tables.o: metadata_tables.h metadata_tables.cc
	clang-3.9 metadata_tables.cc ${INCDIRS} ${CC_FLAGS} -c -o metadata_tables.o

document_index.o: document_index.h document_index.cc
	clang-3.9 document_index.cc ${INCDIRS} ${CC_FLAGS} -c -o document_index.o

custom_binary_reader.o: custom_binary_reader.h custom_binary_reader.cc
	clang-3.9 custom_binary_reader.cc ${INCDIRS} ${CC_FLAGS} -c -o custom_binary_reader.o

portable_pdb_file.o: i_portable_pdb_file.h portable_pdb_file.h portable_pdb_file.cc
	clang-3.9 portable_pdb_file.cc ${INCDIRS} ${CC_FLAGS} -c -o portable_pdb_file.o

variable_wrapper.o: variable_wrapper.h variable_wrapper.cc
	clang-3.9 variable_wrapper.cc ${INCDIRS} ${CC_FLAGS} -c -o variable_wrapper.o

type_signature.o: type_signature.h type_signature.cc
	clang-3.9 type_signature.cc ${INCDIRS} ${CC_FLAGS} -c -o type_signature.o

array_expression_evaluator.o: ${JAVA_DBG_INC}array_expression_evaluator.h ${JAVA_DBG_INC}array_expression_evaluator.cc
	clang-3.9 ${JAVA_DBG_INC}array_expression_evaluator.cc ${INCDIRS} ${CC_FLAGS} -c -o array_expression_evaluator.o

binary_expression_evaluator.o: ${JAVA_DBG_INC}binary_expression_evaluator.h ${JAVA_DBG_INC}binary_expression_evaluator.cc
	clang-3.9 ${JAVA_DBG_INC}binary_expression_evaluator.cc ${INCDIRS} ${CC_FLAGS} -c -o binary_expression_evaluator.o

conditional_operator_evaluator.o: ${JAVA_DBG_INC}conditional_operator_evaluator.h ${JAVA_DBG_INC}conditional_operator_evaluator.cc
	clang-3.9 ${JAVA_DBG_INC}conditional_operator_evaluator.cc ${INCDIRS} ${CC_FLAGS} -c -o conditional_operator_evaluator.o

csharp_expression.o: ${JAVA_DBG_INC}csharp_expression.h ${JAVA_DBG_INC}csharp_expression.cc
	clang-3.9 ${JAVA_DBG_INC}csharp_expression.cc ${INCDIRS} ${CC_FLAGS} -c -o csharp_expression.o

expression_util.o: ${JAVA_DBG_INC}expression_util.h ${JAVA_DBG_INC}expression_util.cc
	clang-3.9 ${JAVA_DBG_INC}expression_util.cc ${INCDIRS} ${CC_FLAGS} -c -o expression_util.o

field_evaluator.o: ${JAVA_DBG_INC}field_evaluator.h ${JAVA_DBG_INC}field_evaluator.cc
	clang-3.9 ${JAVA_DBG_INC}field_evaluator.cc ${INCDIRS} ${CC_FLAGS} -c -o field_evaluator.o

identifier_evaluator.o: ${JAVA_DBG_INC}identifier_evaluator.h ${JAVA_DBG_INC}identifier_evaluator.cc
	clang-3.9 ${JAVA_DBG_INC}identifier_evaluator.cc ${INCDIRS} ${CC_FLAGS} -c -o identifier_evaluator.o

method_call_evaluator.o: ${JAVA_DBG_INC}method_call_evaluator.h ${JAVA_DBG_INC}method_call_evaluator.cc
	clang-3.9 ${JAVA_DBG_INC}method_call_evaluator.cc ${INCDIRS} ${CC_FLAGS} -c -o method_call_evaluator.o

string_evaluator.o: ${JAVA_DBG_INC}string_evaluator.h ${JAVA_DBG_INC}string_evaluator.cc
	clang-3.9 ${JAVA_DBG_INC}string_evaluator.cc ${INCDIRS} ${CC_FLAGS} -c -o string_evaluator.o

type_cast_operator_evaluator.o: ${JAVA_DBG_INC}type_cast_operator_evaluator.h ${JAVA_DBG_INC}type_cast_operator_evaluator.cc
	clang-3.9 ${JAVA_DBG_INC}type_cast_operator_evaluator.cc ${INCDIRS} ${CC_FLAGS} -c -o type_cast_operator_evaluator.o

unary_expression_evaluator.o: ${JAVA_DBG_INC}unary_expression_evaluator.h ${JAVA_DBG_INC}unary_expression_evaluator.cc
	clang-3.9 ${JAVA_DBG_INC}unary_expression_evaluator.cc ${INCDIRS} ${CC_FLAGS} -c -o unary_expression_evaluator.o

csharp_expression_compiler.o: ${ANTLR_GEN_DIR}CSharpExpressionCompiler.hpp ${ANTLR_GEN_DIR}CSharpExpressionCompiler.cc
	clang-3.9 ${ANTLR_GEN_DIR}CSharpExpressionCompiler.cc ${INCDIRS} ${CC_FLAGS} -c -o csharp_expression_compiler.o

csharp_expression_lexer.o: ${ANTLR_GEN_DIR}CSharpExpressionLexer.hpp ${ANTLR_GEN_DIR}CSharpExpressionLexer.cc
	clang-3.9 ${ANTLR_GEN_DIR}CSharpExpressionLexer.cc ${INCDIRS} ${CC_FLAGS} -c -o csharp_expression_lexer.o

csharp_expression_parser.o: ${ANTLR_GEN_DIR}CSharpExpressionParser.hpp ${ANTLR_GEN_DIR}CSharpExpressionParser.cc
	clang-3.9 ${ANTLR_GEN_DIR}CSharpExpressionParser.cc ${INCDIRS} ${CC_FLAGS} -c -o csharp_expression_parser.o

clean:
	rm -f *.o *.a *.g*
