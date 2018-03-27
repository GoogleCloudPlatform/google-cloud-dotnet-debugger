# Directory that contains this makefile.
ANTLR_CPP_LIB = ../antlr/lib/cpp
ANTLR_CPP_LIB_SRC = $(ANTLR_CPP_LIB)/src
ANTLR_CPP_LIB_INCLUDE = $(ANTLR_CPP_LIB)

ANTLR_RUNTIME_SOURCES = \
	$(ANTLR_CPP_LIB_SRC)/src/$(wildcard *.cpp)

JAVA_EXPRESSION_GRAMMAR_FILE = csharp_expression.g

antlrgen: $(JAVA_EXPRESSION_GRAMMAR_FILE)
	mkdir antlrgen
	java -classpath ../antlr/libantlr.jar antlr.Tool -o antlrgen $(JAVA_EXPRESSION_GRAMMAR_FILE)

clean:
	rm -rf antlrgen
