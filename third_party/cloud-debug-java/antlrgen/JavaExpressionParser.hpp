#ifndef INC_JavaExpressionParser_hpp_
#define INC_JavaExpressionParser_hpp_

#include "third_party/antlr/lib/cpp/antlr/config.hpp"
/* $ANTLR 2.7.2: "csharp_expression.g" -> "JavaExpressionParser.hpp"$ */
#include "third_party/antlr/lib/cpp/antlr/TokenStream.hpp"
#include "third_party/antlr/lib/cpp/antlr/TokenBuffer.hpp"
#include "JavaExpressionParserTokenTypes.hpp"
#include "third_party/antlr/lib/cpp/antlr/LLkParser.hpp"

#line 21 "csharp_expression.g"

  #include <iostream>
  #include <string>

  #include "../../cloud-debug-java/common.h"
  #include "../../cloud-debug-java/csharp_expression.h"
  #include "../../cloud-debug-java/messages.h"

#line 21 "JavaExpressionParser.hpp"
ANTLR_BEGIN_NAMESPACE(google_cloud_debugger)
class JavaExpressionParser : public ANTLR_USE_NAMESPACE(antlr)LLkParser, public JavaExpressionParserTokenTypes
{
#line 223 "csharp_expression.g"

 public:
  // Init or re-init the parser
  void Init() {
    errors_.clear();
    initializeASTFactory(fact_);
    setASTFactory(&fact_);
  }

  void reportError(const std::string& s) {
    errors_.push_back(s);
  }

  void reportError(const antlr::RecognitionException& ex) {
    errors_.push_back(ex.getMessage());
  }

  int num_errors() { return errors_.size(); }

  const std::vector<std::string>& errors() { return errors_; }

 private:
  std::vector<std::string> errors_;
  antlr::ASTFactory fact_;
#line 26 "JavaExpressionParser.hpp"
public:
	void initializeASTFactory( ANTLR_USE_NAMESPACE(antlr)ASTFactory& factory );
protected:
	JavaExpressionParser(ANTLR_USE_NAMESPACE(antlr)TokenBuffer& tokenBuf, int k);
public:
	JavaExpressionParser(ANTLR_USE_NAMESPACE(antlr)TokenBuffer& tokenBuf);
protected:
	JavaExpressionParser(ANTLR_USE_NAMESPACE(antlr)TokenStream& lexer, int k);
public:
	JavaExpressionParser(ANTLR_USE_NAMESPACE(antlr)TokenStream& lexer);
	JavaExpressionParser(const ANTLR_USE_NAMESPACE(antlr)ParserSharedInputState& state);
	int getNumTokens() const
	{
		return JavaExpressionParser::NUM_TOKENS;
	}
	const char* getTokenName( int type ) const
	{
		if( type > getNumTokens() ) return 0;
		return JavaExpressionParser::tokenNames[type];
	}
	const char* const* getTokenNames() const
	{
		return JavaExpressionParser::tokenNames;
	}
	//DBG genRuleHeader(statement)
	public: void statement();
	//DBG genRuleHeader(expression)
	public: void expression();
	//DBG genRuleHeader(conditionalExpression)
	public: void conditionalExpression();
	//DBG genRuleHeader(expressionList)
	public: void expressionList();
	//DBG genRuleHeader(conditionalOrExpression)
	public: void conditionalOrExpression();
	//DBG genRuleHeader(conditionalAndExpression)
	public: void conditionalAndExpression();
	//DBG genRuleHeader(inclusiveOrExpression)
	public: void inclusiveOrExpression();
	//DBG genRuleHeader(exclusiveOrExpression)
	public: void exclusiveOrExpression();
	//DBG genRuleHeader(andExpression)
	public: void andExpression();
	//DBG genRuleHeader(equalityExpression)
	public: void equalityExpression();
	//DBG genRuleHeader(relationalExpression)
	public: void relationalExpression();
	//DBG genRuleHeader(shiftExpression)
	public: void shiftExpression();
	//DBG genRuleHeader(additiveExpression)
	public: void additiveExpression();
	//DBG genRuleHeader(multiplicativeExpression)
	public: void multiplicativeExpression();
	//DBG genRuleHeader(unaryExpression)
	public: void unaryExpression();
	//DBG genRuleHeader(unaryExpressionNotPlusMinus)
	public: void unaryExpressionNotPlusMinus();
	//DBG genRuleHeader(castExpression)
	public: void castExpression();
	//DBG genRuleHeader(primary)
	public: void primary();
	//DBG genRuleHeader(selector)
	public: void selector();
	//DBG genRuleHeader(classOrInterfaceType)
	public: void classOrInterfaceType();
	//DBG genRuleHeader(arguments)
	public: void arguments();
	//DBG genRuleHeader(literal)
	public: void literal();
private:
	static const char* tokenNames[];
#ifndef NO_STATIC_CONSTS
	static const int NUM_TOKENS = 71;
#else
	enum {
		NUM_TOKENS = 71
	};
#endif
	
	static const unsigned long _tokenSet_0_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_0;
	static const unsigned long _tokenSet_1_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_1;
	static const unsigned long _tokenSet_2_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_2;
	static const unsigned long _tokenSet_3_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_3;
	static const unsigned long _tokenSet_4_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_4;
	static const unsigned long _tokenSet_5_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_5;
	static const unsigned long _tokenSet_6_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_6;
	static const unsigned long _tokenSet_7_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_7;
	static const unsigned long _tokenSet_8_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_8;
	static const unsigned long _tokenSet_9_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_9;
	static const unsigned long _tokenSet_10_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_10;
	static const unsigned long _tokenSet_11_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_11;
	static const unsigned long _tokenSet_12_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_12;
	static const unsigned long _tokenSet_13_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_13;
	static const unsigned long _tokenSet_14_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_14;
	static const unsigned long _tokenSet_15_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_15;
	static const unsigned long _tokenSet_16_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_16;
	static const unsigned long _tokenSet_17_data_[];
	static const ANTLR_USE_NAMESPACE(antlr)BitSet _tokenSet_17;
};

ANTLR_END_NAMESPACE
#endif /*INC_JavaExpressionParser_hpp_*/
