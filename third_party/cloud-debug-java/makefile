# Directory that contains this makefile.
ANTLR_CPP_LIB = ../antlr/lib/cpp
ANTLR_CPP_LIB_SRC = $(ANTLR_CPP_LIB)/src
ANTLR_CPP_LIB_INCLUDE = $(ANTLR_CPP_LIB)

ANTLR_RUNTIME_SOURCES = \
	$(ANTLR_CPP_LIB_SRC)/ANTLRException.cpp \
	$(ANTLR_CPP_LIB_SRC)/ANTLRUtil.cpp \
	$(ANTLR_CPP_LIB_SRC)/ASTFactory.cpp \
	$(ANTLR_CPP_LIB_SRC)/ASTNULLType.cpp \
	$(ANTLR_CPP_LIB_SRC)/ASTRefCount.cpp \
	$(ANTLR_CPP_LIB_SRC)/BaseAST.cpp \
	$(ANTLR_CPP_LIB_SRC)/BitSet.cpp \
	$(ANTLR_CPP_LIB_SRC)/CharBuffer.cpp \
	$(ANTLR_CPP_LIB_SRC)/CharScanner.cpp \
	$(ANTLR_CPP_LIB_SRC)/CharStreamException.cpp \
	$(ANTLR_CPP_LIB_SRC)/CharStreamIOException.cpp \
	$(ANTLR_CPP_LIB_SRC)/CommonAST.cpp \
	$(ANTLR_CPP_LIB_SRC)/CommonASTWithHiddenTokens.cpp \
	$(ANTLR_CPP_LIB_SRC)/CommonHiddenStreamToken.cpp \
	$(ANTLR_CPP_LIB_SRC)/CommonToken.cpp \
	$(ANTLR_CPP_LIB_SRC)/ExceptionSlot.cpp \
	$(ANTLR_CPP_LIB_SRC)/IOException.cpp \
	$(ANTLR_CPP_LIB_SRC)/InputBuffer.cpp \
	$(ANTLR_CPP_LIB_SRC)/LLkParser.cpp \
	$(ANTLR_CPP_LIB_SRC)/MismatchedCharException.cpp \
	$(ANTLR_CPP_LIB_SRC)/MismatchedTokenException.cpp \
	$(ANTLR_CPP_LIB_SRC)/NoViableAltException.cpp \
	$(ANTLR_CPP_LIB_SRC)/NoViableAltForCharException.cpp \
	$(ANTLR_CPP_LIB_SRC)/Parser.cpp \
	$(ANTLR_CPP_LIB_SRC)/RecognitionException.cpp \
	$(ANTLR_CPP_LIB_SRC)/SemanticException.cpp \
	$(ANTLR_CPP_LIB_SRC)/String.cpp \
	$(ANTLR_CPP_LIB_SRC)/Token.cpp \
	$(ANTLR_CPP_LIB_SRC)/TokenBuffer.cpp \
	$(ANTLR_CPP_LIB_SRC)/TokenStreamBasicFilter.cpp \
	$(ANTLR_CPP_LIB_SRC)/TokenStreamException.cpp \
	$(ANTLR_CPP_LIB_SRC)/TokenStreamHiddenTokenFilter.cpp \
	$(ANTLR_CPP_LIB_SRC)/TokenStreamIOException.cpp \
	$(ANTLR_CPP_LIB_SRC)/TokenStreamRecognitionException.cpp \
	$(ANTLR_CPP_LIB_SRC)/TokenStreamRetryException.cpp \
	$(ANTLR_CPP_LIB_SRC)/TokenStreamSelector.cpp \
	$(ANTLR_CPP_LIB_SRC)/TreeParser.cpp \

JAVA_EXPRESSION_GRAMMAR_FILE = java_expression.g

antlrgen: $(JAVA_EXPRESSION_GRAMMAR_FILE)
	mkdir antlrgen
	java -classpath ../antlr/libantlr.jar antlr.Tool -o antlrgen $(JAVA_EXPRESSION_GRAMMAR_FILE)

clean:
	rm -rf antlrgen
