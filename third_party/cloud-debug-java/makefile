# Directory that contains this makefile.
ANTLR_CPP_LIB = ../antlr/lib/cpp
ANTLR_CPP_LIB_SRC = $(ANTLR_CPP_LIB)/src
ANTLR_CPP_LIB_INCLUDE = $(ANTLR_CPP_LIB)

ANTLR_RUNTIME_SOURCES = \
	$(ANTLR_CPP_LIB_SRC)/src/ANTLRException.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/ANTLRUtil.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/ASTFactory.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/ASTNULLType.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/ASTRefCount.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/BaseAST.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/BitSet.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/CharBuffer.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/CharScanner.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/CharStreamException.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/CharStreamIOException.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/CommonAST.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/CommonASTWithHiddenTokens.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/CommonHiddenStreamToken.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/CommonToken.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/ExceptionSlot.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/IOException.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/InputBuffer.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/LLkParser.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/MismatchedCharException.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/MismatchedTokenException.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/NoViableAltException.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/NoViableAltForCharException.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/Parser.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/RecognitionException.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/SemanticException.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/String.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/Token.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/TokenBuffer.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/TokenStreamBasicFilter.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/TokenStreamException.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/TokenStreamHiddenTokenFilter.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/TokenStreamIOException.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/TokenStreamRecognitionException.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/TokenStreamRetryException.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/TokenStreamSelector.cpp \
	$(ANTLR_CPP_LIB_SRC)/src/TreeParser.cpp \

JAVA_EXPRESSION_GRAMMAR_FILE = csharp_expression.g

antlrgen: $(JAVA_EXPRESSION_GRAMMAR_FILE)
	mkdir antlrgen
	java -classpath ../antlr/libantlr.jar antlr.Tool -o antlrgen $(JAVA_EXPRESSION_GRAMMAR_FILE)

clean:
	rm -rf antlrgen
