/* $ANTLR 2.7.2: "csharp_expression.g" -> "CSharpExpressionCompiler.cc"$ */
#include "CSharpExpressionCompiler.hpp"
#include "third_party/antlr/lib/cpp/antlr/Token.hpp"
#include "third_party/antlr/lib/cpp/antlr/AST.hpp"
#include "third_party/antlr/lib/cpp/antlr/NoViableAltException.hpp"
#include "third_party/antlr/lib/cpp/antlr/MismatchedTokenException.hpp"
#include "third_party/antlr/lib/cpp/antlr/SemanticException.hpp"
#include "third_party/antlr/lib/cpp/antlr/BitSet.hpp"
ANTLR_BEGIN_NAMESPACE(google_cloud_debugger)
#line 1 "csharp_expression.g"
#line 12 "CSharpExpressionCompiler.cc"
CSharpExpressionCompiler::CSharpExpressionCompiler()
	: ANTLR_USE_NAMESPACE(antlr)TreeParser() {
}

//DBG genRule(statement)
CSharpExpression*  CSharpExpressionCompiler::statement(ANTLR_USE_NAMESPACE(antlr)RefAST _t) {
#line 511 "csharp_expression.g"
	CSharpExpression* root;
#line 21 "CSharpExpressionCompiler.cc"
	ANTLR_USE_NAMESPACE(antlr)RefAST statement_AST_in = _t;
#line 511 "csharp_expression.g"
	
	root = nullptr;
	
#line 27 "CSharpExpressionCompiler.cc"
	
	#ifdef ANTLR_EXCEPTIONS
	try {      // for error handling
	#else
	{ //_catch0
	#endif
		ANTLR_USE_NAMESPACE(antlr)RefAST __t163 = _t;
		ANTLR_USE_NAMESPACE(antlr)RefAST tmp1_AST_in = _t;
		match(_t,STATEMENT);
		if (ActiveException()) goto _catch0;
		_t = _t->getFirstChild();
		//DBG RuleRefElement( expression)
		root=expression(_t);
		if (ActiveException()) goto _catch0;
		_t = _retTree;
		_t = __t163;
		_t = _t->getNextSibling();
	}
	#ifdef ANTLR_EXCEPTIONS
		catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
			reportError(ex);
			if ( _t != ANTLR_USE_NAMESPACE(antlr)nullAST )
				_t = _t->getNextSibling();
		}
	#endif
	_catch0:
	if(0){}
	else if ( ANTLR_USE_NAMESPACE(antlr)RecognitionException::DynamicCast(ActiveException()) ){
		ANTLR_USE_NAMESPACE(antlr)RecognitionException* pEx = ANTLR_USE_NAMESPACE(antlr)RecognitionException::DynamicCast(ActiveException());
		reportError(*pEx);
		ClearException();
		if ( _t != ANTLR_USE_NAMESPACE(antlr)nullAST )
			_t = _t->getNextSibling();
	}
	else if ( ActiveException() ){ // uncaught exception handling
		return root;
	}
	_retTree = _t;
	return root;
}

//DBG genRule(expression)
CSharpExpression*  CSharpExpressionCompiler::expression(ANTLR_USE_NAMESPACE(antlr)RefAST _t) {
#line 517 "csharp_expression.g"
	CSharpExpression* je;
#line 73 "CSharpExpressionCompiler.cc"
	ANTLR_USE_NAMESPACE(antlr)RefAST expression_AST_in = _t;
	ANTLR_USE_NAMESPACE(antlr)RefAST identifier_node = ANTLR_USE_NAMESPACE(antlr)nullAST;
	ANTLR_USE_NAMESPACE(antlr)RefAST hex_numeric_literal_node = ANTLR_USE_NAMESPACE(antlr)nullAST;
	ANTLR_USE_NAMESPACE(antlr)RefAST oct_numeric_literal_node = ANTLR_USE_NAMESPACE(antlr)nullAST;
	ANTLR_USE_NAMESPACE(antlr)RefAST fp_numeric_literal_node = ANTLR_USE_NAMESPACE(antlr)nullAST;
	ANTLR_USE_NAMESPACE(antlr)RefAST dec_numeric_literal_node = ANTLR_USE_NAMESPACE(antlr)nullAST;
	ANTLR_USE_NAMESPACE(antlr)RefAST character_literal_node = ANTLR_USE_NAMESPACE(antlr)nullAST;
	ANTLR_USE_NAMESPACE(antlr)RefAST string_literal_node = ANTLR_USE_NAMESPACE(antlr)nullAST;
	ANTLR_USE_NAMESPACE(antlr)RefAST method_node = ANTLR_USE_NAMESPACE(antlr)nullAST;
#line 517 "csharp_expression.g"
	
	CSharpExpression* a = nullptr;
	CSharpExpression* b = nullptr;
	CSharpExpression* c = nullptr;
	CSharpExpressionSelector* s = nullptr;
	MethodArguments* r = nullptr;
	BinaryCSharpExpression::Type binary_expression_type;
	UnaryCSharpExpression::Type unary_expression_type;
	std::list<std::vector<char>> string_sequence;
	std::string type;
	
	je = nullptr;
	
#line 97 "CSharpExpressionCompiler.cc"
	
	#ifdef ANTLR_EXCEPTIONS
	try {      // for error handling
	#else
	{ //_catch1
	#endif
		//DBG genCommonBlk(
		if (_t == ANTLR_USE_NAMESPACE(antlr)nullAST )
			_t = ASTNULL;
		switch ( _t->getType()) {
		//DBG genCases(BitSet)
		case QUESTION:
		{
			ANTLR_USE_NAMESPACE(antlr)RefAST __t165 = _t;
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp2_AST_in = _t;
			match(_t,QUESTION);
			if (ActiveException()) goto _catch1;
			_t = _t->getFirstChild();
			//DBG RuleRefElement( expression)
			a=expression(_t);
			if (ActiveException()) goto _catch1;
			_t = _retTree;
			//DBG RuleRefElement( expression)
			b=expression(_t);
			if (ActiveException()) goto _catch1;
			_t = _retTree;
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp3_AST_in = _t;
			match(_t,COLON);
			if (ActiveException()) goto _catch1;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG RuleRefElement( expression)
			c=expression(_t);
			if (ActiveException()) goto _catch1;
			_t = _retTree;
			_t = __t165;
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 530 "csharp_expression.g"
			
			if ((a == nullptr) || (b == nullptr) || (c == nullptr)) {
			reportError("NULL argument in conditional expression");
			delete a;
			delete b;
			delete c;
			je = nullptr;
			} else {
			je = new ConditionalCSharpExpression(a, b, c);
			}
			
#line 148 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case PARENTHESES_EXPRESSION:
		{
			ANTLR_USE_NAMESPACE(antlr)RefAST __t166 = _t;
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp4_AST_in = _t;
			match(_t,PARENTHESES_EXPRESSION);
			if (ActiveException()) goto _catch1;
			_t = _t->getFirstChild();
			//DBG RuleRefElement( expression)
			je=expression(_t);
			if (ActiveException()) goto _catch1;
			_t = _retTree;
			_t = __t166;
			_t = _t->getNextSibling();
			break;
		}
		//DBG genCases(BitSet)
		case BINARY_EXPRESSION:
		{
			ANTLR_USE_NAMESPACE(antlr)RefAST __t167 = _t;
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp5_AST_in = _t;
			match(_t,BINARY_EXPRESSION);
			if (ActiveException()) goto _catch1;
			_t = _t->getFirstChild();
			//DBG RuleRefElement( expression)
			a=expression(_t);
			if (ActiveException()) goto _catch1;
			_t = _retTree;
			//DBG RuleRefElement( binary_expression_token)
			binary_expression_type=binary_expression_token(_t);
			if (ActiveException()) goto _catch1;
			_t = _retTree;
			//DBG RuleRefElement( expression)
			b=expression(_t);
			if (ActiveException()) goto _catch1;
			_t = _retTree;
			_t = __t167;
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 542 "csharp_expression.g"
			
			if ((a == nullptr) || (b == nullptr)) {
			reportError("NULL argument in binary expression");
			delete a;
			delete b;
			je = nullptr;
			} else {
			je = new BinaryCSharpExpression(binary_expression_type, a, b);
			}
			
#line 201 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case UNARY_EXPRESSION:
		{
			ANTLR_USE_NAMESPACE(antlr)RefAST __t168 = _t;
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp6_AST_in = _t;
			match(_t,UNARY_EXPRESSION);
			if (ActiveException()) goto _catch1;
			_t = _t->getFirstChild();
			//DBG RuleRefElement( unary_expression_token)
			unary_expression_type=unary_expression_token(_t);
			if (ActiveException()) goto _catch1;
			_t = _retTree;
			//DBG RuleRefElement( expression)
			a=expression(_t);
			if (ActiveException()) goto _catch1;
			_t = _retTree;
			_t = __t168;
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 552 "csharp_expression.g"
			
			if (a == nullptr) {
			reportError("NULL argument in unary expression");
			je = nullptr;
			} else {
			je = new UnaryCSharpExpression(unary_expression_type, a);
			}
			
#line 232 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case Identifier:
		{
			//DBG genTokenRef(TokenRefElement)
			identifier_node = _t;
			match(_t,Identifier);
			if (ActiveException()) goto _catch1;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 560 "csharp_expression.g"
			
			je = new CSharpIdentifier(identifier_node->getText());
			
#line 248 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case HEX_NUMERIC_LITERAL:
		{
			//DBG genTokenRef(TokenRefElement)
			hex_numeric_literal_node = _t;
			match(_t,HEX_NUMERIC_LITERAL);
			if (ActiveException()) goto _catch1;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 563 "csharp_expression.g"
			
			CSharpIntLiteral* nl = new CSharpIntLiteral;
			if (!nl->ParseString(hex_numeric_literal_node->getText(), 16)) {
			reportError("Hex integer literal could not be parsed");
			SetErrorMessage(BadNumericLiteral);
			
			delete nl;
			je = nullptr;
			} else {
			je = nl;
			}
			
#line 273 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case OCT_NUMERIC_LITERAL:
		{
			//DBG genTokenRef(TokenRefElement)
			oct_numeric_literal_node = _t;
			match(_t,OCT_NUMERIC_LITERAL);
			if (ActiveException()) goto _catch1;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 575 "csharp_expression.g"
			
			CSharpIntLiteral* nl = new CSharpIntLiteral;
			if (!nl->ParseString(oct_numeric_literal_node->getText(), 8)) {
			reportError("Octal integer literal could not be parsed");
			SetErrorMessage(BadNumericLiteral);
			
			delete nl;
			je = nullptr;
			} else {
			je = nl;
			}
			
#line 298 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case FP_NUMERIC_LITERAL:
		{
			//DBG genTokenRef(TokenRefElement)
			fp_numeric_literal_node = _t;
			match(_t,FP_NUMERIC_LITERAL);
			if (ActiveException()) goto _catch1;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 587 "csharp_expression.g"
			
			CSharpFloatLiteral* nl = new CSharpFloatLiteral;
			if (!nl->ParseString(fp_numeric_literal_node->getText())) {
			reportError("Floating point literal could not be parsed");
			SetErrorMessage(BadNumericLiteral);
			
			delete nl;
			je = nullptr;
			} else {
			je = nl;
			}
			
#line 323 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case DEC_NUMERIC_LITERAL:
		{
			//DBG genTokenRef(TokenRefElement)
			dec_numeric_literal_node = _t;
			match(_t,DEC_NUMERIC_LITERAL);
			if (ActiveException()) goto _catch1;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 599 "csharp_expression.g"
			
			CSharpIntLiteral* nl = new CSharpIntLiteral;
			if (!nl->ParseString(dec_numeric_literal_node->getText(), 10)) {
			reportError("Decimal integer literal could not be parsed");
			SetErrorMessage(BadNumericLiteral);
			
			delete nl;
			je = nullptr;
			} else {
			je = nl;
			}
			
#line 348 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case CharacterLiteral:
		{
			//DBG genTokenRef(TokenRefElement)
			character_literal_node = _t;
			match(_t,CharacterLiteral);
			if (ActiveException()) goto _catch1;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 611 "csharp_expression.g"
			
			CSharpCharLiteral* cl = new CSharpCharLiteral;
			if (!cl->ParseString(character_literal_node->getText())) {
			reportError("Invalid character");
			delete cl;
			je = nullptr;
			} else {
			je = cl;
			}
			
#line 371 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case StringLiteral:
		{
			//DBG genTokenRef(TokenRefElement)
			string_literal_node = _t;
			match(_t,StringLiteral);
			if (ActiveException()) goto _catch1;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 621 "csharp_expression.g"
			
			CSharpStringLiteral* sl = new CSharpStringLiteral;
			if (!sl->ParseString(string_literal_node->getText())) {
			reportError("Invalid string");
			delete sl;
			je = nullptr;
			} else {
			je = sl;
			}
			
#line 394 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case JTRUE:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp7_AST_in = _t;
			match(_t,JTRUE);
			if (ActiveException()) goto _catch1;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 631 "csharp_expression.g"
			
			je = new CSharpBooleanLiteral(true);
			
#line 410 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case JFALSE:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp8_AST_in = _t;
			match(_t,JFALSE);
			if (ActiveException()) goto _catch1;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 634 "csharp_expression.g"
			
			je = new CSharpBooleanLiteral(false);
			
#line 426 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case JNULL:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp9_AST_in = _t;
			match(_t,JNULL);
			if (ActiveException()) goto _catch1;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 637 "csharp_expression.g"
			
			je = new CSharpNullLiteral;
			
#line 442 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case PRIMARY_SELECTOR:
		{
			ANTLR_USE_NAMESPACE(antlr)RefAST __t169 = _t;
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp10_AST_in = _t;
			match(_t,PRIMARY_SELECTOR);
			if (ActiveException()) goto _catch1;
			_t = _t->getFirstChild();
			//DBG RuleRefElement( expression)
			a=expression(_t);
			if (ActiveException()) goto _catch1;
			_t = _retTree;
			//DBG RuleRefElement( selector)
			s=selector(_t);
			if (ActiveException()) goto _catch1;
			_t = _retTree;
			_t = __t169;
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 640 "csharp_expression.g"
			
			if ((a == nullptr) || (s == nullptr)) {
			reportError("PRIMARY_SELECTED inner expressions failed to compile");
			delete a;
			delete s;
			je = nullptr;
			} else {
			s->set_source(a);
			je = s;
			}
			
#line 476 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case TYPE_CAST:
		{
			ANTLR_USE_NAMESPACE(antlr)RefAST __t170 = _t;
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp11_AST_in = _t;
			match(_t,TYPE_CAST);
			if (ActiveException()) goto _catch1;
			_t = _t->getFirstChild();
			//DBG RuleRefElement( type_name)
			type=type_name(_t);
			if (ActiveException()) goto _catch1;
			_t = _retTree;
			//DBG RuleRefElement( expression)
			a=expression(_t);
			if (ActiveException()) goto _catch1;
			_t = _retTree;
			_t = __t170;
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 651 "csharp_expression.g"
			
			if (a == nullptr) {
			reportError("NULL source in type cast expression");
			je = nullptr;
			} else {
			je = new TypeCastCSharpExpression(type, a);
			}
			
#line 507 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case METHOD_CALL:
		{
			ANTLR_USE_NAMESPACE(antlr)RefAST __t171 = _t;
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp12_AST_in = _t;
			match(_t,METHOD_CALL);
			if (ActiveException()) goto _catch1;
			_t = _t->getFirstChild();
			//DBG genTokenRef(TokenRefElement)
			method_node = _t;
			match(_t,Identifier);
			if (ActiveException()) goto _catch1;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG RuleRefElement( arguments)
			r=arguments(_t);
			if (ActiveException()) goto _catch1;
			_t = _retTree;
			_t = __t171;
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 659 "csharp_expression.g"
			
			if (r == nullptr) {
			reportError("NULL method arguments in expression METHOD_CALL");
			je = nullptr;
			} else {
			je = new MethodCallExpression(method_node->getText(), r);
			}
			
#line 539 "CSharpExpressionCompiler.cc"
			break;
		}
		default:
		{
			SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltException(_t)); goto _catch1;
		}
		}
	}
	#ifdef ANTLR_EXCEPTIONS
		catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
			reportError(ex);
			if ( _t != ANTLR_USE_NAMESPACE(antlr)nullAST )
				_t = _t->getNextSibling();
		}
	#endif
	_catch1:
	if(0){}
	else if ( ANTLR_USE_NAMESPACE(antlr)RecognitionException::DynamicCast(ActiveException()) ){
		ANTLR_USE_NAMESPACE(antlr)RecognitionException* pEx = ANTLR_USE_NAMESPACE(antlr)RecognitionException::DynamicCast(ActiveException());
		reportError(*pEx);
		ClearException();
		if ( _t != ANTLR_USE_NAMESPACE(antlr)nullAST )
			_t = _t->getNextSibling();
	}
	else if ( ActiveException() ){ // uncaught exception handling
		return je;
	}
	_retTree = _t;
	return je;
}

//DBG genRule(binary_expression_token)
BinaryCSharpExpression::Type  CSharpExpressionCompiler::binary_expression_token(ANTLR_USE_NAMESPACE(antlr)RefAST _t) {
#line 669 "csharp_expression.g"
	BinaryCSharpExpression::Type type;
#line 575 "CSharpExpressionCompiler.cc"
	ANTLR_USE_NAMESPACE(antlr)RefAST binary_expression_token_AST_in = _t;
#line 669 "csharp_expression.g"
	
	type = static_cast<BinaryCSharpExpression::Type>(-1);
	
#line 581 "CSharpExpressionCompiler.cc"
	
	#ifdef ANTLR_EXCEPTIONS
	try {      // for error handling
	#else
	{ //_catch2
	#endif
		//DBG genCommonBlk(
		if (_t == ANTLR_USE_NAMESPACE(antlr)nullAST )
			_t = ASTNULL;
		switch ( _t->getType()) {
		//DBG genCases(BitSet)
		case ADD:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp13_AST_in = _t;
			match(_t,ADD);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 672 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::add;
#line 603 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case SUB:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp14_AST_in = _t;
			match(_t,SUB);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 673 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::sub;
#line 617 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case MUL:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp15_AST_in = _t;
			match(_t,MUL);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 674 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::mul;
#line 631 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case DIV:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp16_AST_in = _t;
			match(_t,DIV);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 675 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::div;
#line 645 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case MOD:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp17_AST_in = _t;
			match(_t,MOD);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 676 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::mod;
#line 659 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case AND:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp18_AST_in = _t;
			match(_t,AND);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 677 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::conditional_and;
#line 673 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case OR:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp19_AST_in = _t;
			match(_t,OR);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 678 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::conditional_or;
#line 687 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case EQUAL:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp20_AST_in = _t;
			match(_t,EQUAL);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 679 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::eq;
#line 701 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case NOTEQUAL:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp21_AST_in = _t;
			match(_t,NOTEQUAL);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 680 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::ne;
#line 715 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case CMP_LE:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp22_AST_in = _t;
			match(_t,CMP_LE);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 681 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::le;
#line 729 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case CMP_GE:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp23_AST_in = _t;
			match(_t,CMP_GE);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 682 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::ge;
#line 743 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case CMP_LT:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp24_AST_in = _t;
			match(_t,CMP_LT);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 683 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::lt;
#line 757 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case CMP_GT:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp25_AST_in = _t;
			match(_t,CMP_GT);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 684 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::gt;
#line 771 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case BITAND:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp26_AST_in = _t;
			match(_t,BITAND);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 685 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::bitwise_and;
#line 785 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case BITOR:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp27_AST_in = _t;
			match(_t,BITOR);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 686 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::bitwise_or;
#line 799 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case CARET:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp28_AST_in = _t;
			match(_t,CARET);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 687 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::bitwise_xor;
#line 813 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case SHIFT_LEFT:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp29_AST_in = _t;
			match(_t,SHIFT_LEFT);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 688 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::shl;
#line 827 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case SHIFT_RIGHT_S:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp30_AST_in = _t;
			match(_t,SHIFT_RIGHT_S);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 689 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::shr_s;
#line 841 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case SHIFT_RIGHT_U:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp31_AST_in = _t;
			match(_t,SHIFT_RIGHT_U);
			if (ActiveException()) goto _catch2;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 690 "csharp_expression.g"
			type = BinaryCSharpExpression::Type::shr_u;
#line 855 "CSharpExpressionCompiler.cc"
			break;
		}
		default:
		{
			SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltException(_t)); goto _catch2;
		}
		}
	}
	#ifdef ANTLR_EXCEPTIONS
		catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
			reportError(ex);
			if ( _t != ANTLR_USE_NAMESPACE(antlr)nullAST )
				_t = _t->getNextSibling();
		}
	#endif
	_catch2:
	if(0){}
	else if ( ANTLR_USE_NAMESPACE(antlr)RecognitionException::DynamicCast(ActiveException()) ){
		ANTLR_USE_NAMESPACE(antlr)RecognitionException* pEx = ANTLR_USE_NAMESPACE(antlr)RecognitionException::DynamicCast(ActiveException());
		reportError(*pEx);
		ClearException();
		if ( _t != ANTLR_USE_NAMESPACE(antlr)nullAST )
			_t = _t->getNextSibling();
	}
	else if ( ActiveException() ){ // uncaught exception handling
		return type;
	}
	_retTree = _t;
	return type;
}

//DBG genRule(unary_expression_token)
UnaryCSharpExpression::Type  CSharpExpressionCompiler::unary_expression_token(ANTLR_USE_NAMESPACE(antlr)RefAST _t) {
#line 693 "csharp_expression.g"
	UnaryCSharpExpression::Type type;
#line 891 "CSharpExpressionCompiler.cc"
	ANTLR_USE_NAMESPACE(antlr)RefAST unary_expression_token_AST_in = _t;
#line 693 "csharp_expression.g"
	
	type = static_cast<UnaryCSharpExpression::Type>(-1);
	
#line 897 "CSharpExpressionCompiler.cc"
	
	#ifdef ANTLR_EXCEPTIONS
	try {      // for error handling
	#else
	{ //_catch3
	#endif
		//DBG genCommonBlk(
		if (_t == ANTLR_USE_NAMESPACE(antlr)nullAST )
			_t = ASTNULL;
		switch ( _t->getType()) {
		//DBG genCases(BitSet)
		case ADD:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp32_AST_in = _t;
			match(_t,ADD);
			if (ActiveException()) goto _catch3;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 696 "csharp_expression.g"
			type = UnaryCSharpExpression::Type::plus;
#line 919 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case SUB:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp33_AST_in = _t;
			match(_t,SUB);
			if (ActiveException()) goto _catch3;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 697 "csharp_expression.g"
			type = UnaryCSharpExpression::Type::minus;
#line 933 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case TILDE:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp34_AST_in = _t;
			match(_t,TILDE);
			if (ActiveException()) goto _catch3;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 698 "csharp_expression.g"
			type = UnaryCSharpExpression::Type::bitwise_complement;
#line 947 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case BANG:
		{
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp35_AST_in = _t;
			match(_t,BANG);
			if (ActiveException()) goto _catch3;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 699 "csharp_expression.g"
			type = UnaryCSharpExpression::Type::logical_complement;
#line 961 "CSharpExpressionCompiler.cc"
			break;
		}
		default:
		{
			SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltException(_t)); goto _catch3;
		}
		}
	}
	#ifdef ANTLR_EXCEPTIONS
		catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
			reportError(ex);
			if ( _t != ANTLR_USE_NAMESPACE(antlr)nullAST )
				_t = _t->getNextSibling();
		}
	#endif
	_catch3:
	if(0){}
	else if ( ANTLR_USE_NAMESPACE(antlr)RecognitionException::DynamicCast(ActiveException()) ){
		ANTLR_USE_NAMESPACE(antlr)RecognitionException* pEx = ANTLR_USE_NAMESPACE(antlr)RecognitionException::DynamicCast(ActiveException());
		reportError(*pEx);
		ClearException();
		if ( _t != ANTLR_USE_NAMESPACE(antlr)nullAST )
			_t = _t->getNextSibling();
	}
	else if ( ActiveException() ){ // uncaught exception handling
		return type;
	}
	_retTree = _t;
	return type;
}

//DBG genRule(selector)
CSharpExpressionSelector*  CSharpExpressionCompiler::selector(ANTLR_USE_NAMESPACE(antlr)RefAST _t) {
#line 702 "csharp_expression.g"
	CSharpExpressionSelector* js;
#line 997 "CSharpExpressionCompiler.cc"
	ANTLR_USE_NAMESPACE(antlr)RefAST selector_AST_in = _t;
#line 702 "csharp_expression.g"
	
	CSharpExpression* a = nullptr;
	CSharpExpressionSelector* ds = nullptr;
	
	js = nullptr;
	
#line 1006 "CSharpExpressionCompiler.cc"
	
	#ifdef ANTLR_EXCEPTIONS
	try {      // for error handling
	#else
	{ //_catch4
	#endif
		//DBG genCommonBlk(
		if (_t == ANTLR_USE_NAMESPACE(antlr)nullAST )
			_t = ASTNULL;
		switch ( _t->getType()) {
		//DBG genCases(BitSet)
		case DOT_SELECTOR:
		{
			ANTLR_USE_NAMESPACE(antlr)RefAST __t175 = _t;
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp36_AST_in = _t;
			match(_t,DOT_SELECTOR);
			if (ActiveException()) goto _catch4;
			_t = _t->getFirstChild();
			//DBG RuleRefElement( dotSelector)
			ds=dotSelector(_t);
			if (ActiveException()) goto _catch4;
			_t = _retTree;
			_t = __t175;
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 708 "csharp_expression.g"
			
			if (ds == nullptr) {
			reportError("Failed to parse dot expression");
			js = nullptr;
			} else {
			js = ds;
			}
			
#line 1041 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case LBRACK:
		{
			ANTLR_USE_NAMESPACE(antlr)RefAST __t176 = _t;
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp37_AST_in = _t;
			match(_t,LBRACK);
			if (ActiveException()) goto _catch4;
			_t = _t->getFirstChild();
			//DBG RuleRefElement( expression)
			a=expression(_t);
			if (ActiveException()) goto _catch4;
			_t = _retTree;
			//DBG genTokenRef(TokenRefElement)
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp38_AST_in = _t;
			match(_t,RBRACK);
			if (ActiveException()) goto _catch4;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			_t = __t176;
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 716 "csharp_expression.g"
			
			if (a == nullptr) {
			reportError("Failed to parse index expression");
			js = nullptr;
			} else {
			js = new CSharpExpressionIndexSelector(a);
			}
			
#line 1073 "CSharpExpressionCompiler.cc"
			break;
		}
		default:
		{
			SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltException(_t)); goto _catch4;
		}
		}
	}
	#ifdef ANTLR_EXCEPTIONS
		catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
			reportError(ex);
			if ( _t != ANTLR_USE_NAMESPACE(antlr)nullAST )
				_t = _t->getNextSibling();
		}
	#endif
	_catch4:
	if(0){}
	else if ( ANTLR_USE_NAMESPACE(antlr)RecognitionException::DynamicCast(ActiveException()) ){
		ANTLR_USE_NAMESPACE(antlr)RecognitionException* pEx = ANTLR_USE_NAMESPACE(antlr)RecognitionException::DynamicCast(ActiveException());
		reportError(*pEx);
		ClearException();
		if ( _t != ANTLR_USE_NAMESPACE(antlr)nullAST )
			_t = _t->getNextSibling();
	}
	else if ( ActiveException() ){ // uncaught exception handling
		return js;
	}
	_retTree = _t;
	return js;
}

//DBG genRule(type_name)
std::string  CSharpExpressionCompiler::type_name(ANTLR_USE_NAMESPACE(antlr)RefAST _t) {
#line 766 "csharp_expression.g"
	std::string t;
#line 1109 "CSharpExpressionCompiler.cc"
	ANTLR_USE_NAMESPACE(antlr)RefAST type_name_AST_in = _t;
	ANTLR_USE_NAMESPACE(antlr)RefAST n1 = ANTLR_USE_NAMESPACE(antlr)nullAST;
#line 766 "csharp_expression.g"
	
	std::string tail;
	
#line 1116 "CSharpExpressionCompiler.cc"
	
	#ifdef ANTLR_EXCEPTIONS
	try {      // for error handling
	#else
	{ //_catch5
	#endif
		//DBG genCommonBlk(
		if (_t == ANTLR_USE_NAMESPACE(antlr)nullAST )
			_t = ASTNULL;
		switch ( _t->getType()) {
		//DBG genCases(BitSet)
		case TYPE_NAME:
		{
			ANTLR_USE_NAMESPACE(antlr)RefAST __t182 = _t;
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp39_AST_in = _t;
			match(_t,TYPE_NAME);
			if (ActiveException()) goto _catch5;
			_t = _t->getFirstChild();
			//DBG genTokenRef(TokenRefElement)
			n1 = _t;
			match(_t,Identifier);
			if (ActiveException()) goto _catch5;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG RuleRefElement( type_name)
			tail=type_name(_t);
			if (ActiveException()) goto _catch5;
			_t = _retTree;
			_t = __t182;
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 769 "csharp_expression.g"
			
			t = n1->getText();
			if (!tail.empty()) {
			t += '.';
			t += tail;
			}
			
#line 1155 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case 3:
		case BINARY_EXPRESSION:
		case UNARY_EXPRESSION:
		case PARENTHESES_EXPRESSION:
		case TYPE_CAST:
		case PRIMARY_SELECTOR:
		case METHOD_CALL:
		case JNULL:
		case JTRUE:
		case JFALSE:
		case HEX_NUMERIC_LITERAL:
		case OCT_NUMERIC_LITERAL:
		case FP_NUMERIC_LITERAL:
		case DEC_NUMERIC_LITERAL:
		case CharacterLiteral:
		case StringLiteral:
		case Identifier:
		case QUESTION:
		{
			//DBG genAction(ActionElement action
#line 776 "csharp_expression.g"
			
#line 1181 "CSharpExpressionCompiler.cc"
			break;
		}
		default:
		{
			SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltException(_t)); goto _catch5;
		}
		}
	}
	#ifdef ANTLR_EXCEPTIONS
		catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
			reportError(ex);
			if ( _t != ANTLR_USE_NAMESPACE(antlr)nullAST )
				_t = _t->getNextSibling();
		}
	#endif
	_catch5:
	if(0){}
	else if ( ANTLR_USE_NAMESPACE(antlr)RecognitionException::DynamicCast(ActiveException()) ){
		ANTLR_USE_NAMESPACE(antlr)RecognitionException* pEx = ANTLR_USE_NAMESPACE(antlr)RecognitionException::DynamicCast(ActiveException());
		reportError(*pEx);
		ClearException();
		if ( _t != ANTLR_USE_NAMESPACE(antlr)nullAST )
			_t = _t->getNextSibling();
	}
	else if ( ActiveException() ){ // uncaught exception handling
		return t;
	}
	_retTree = _t;
	return t;
}

//DBG genRule(arguments)
MethodArguments*  CSharpExpressionCompiler::arguments(ANTLR_USE_NAMESPACE(antlr)RefAST _t) {
#line 745 "csharp_expression.g"
	MethodArguments* args;
#line 1217 "CSharpExpressionCompiler.cc"
	ANTLR_USE_NAMESPACE(antlr)RefAST arguments_AST_in = _t;
#line 745 "csharp_expression.g"
	
	MethodArguments* tail = nullptr;
	CSharpExpression* arg = nullptr;
	
	args = nullptr;
	
#line 1226 "CSharpExpressionCompiler.cc"
	
	#ifdef ANTLR_EXCEPTIONS
	try {      // for error handling
	#else
	{ //_catch6
	#endif
		//DBG genCommonBlk(
		if (_t == ANTLR_USE_NAMESPACE(antlr)nullAST )
			_t = ASTNULL;
		switch ( _t->getType()) {
		//DBG genCases(BitSet)
		case EXPRESSION_LIST:
		{
			ANTLR_USE_NAMESPACE(antlr)RefAST __t180 = _t;
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp40_AST_in = _t;
			match(_t,EXPRESSION_LIST);
			if (ActiveException()) goto _catch6;
			_t = _t->getFirstChild();
			//DBG RuleRefElement( expression)
			arg=expression(_t);
			if (ActiveException()) goto _catch6;
			_t = _retTree;
			//DBG RuleRefElement( arguments)
			tail=arguments(_t);
			if (ActiveException()) goto _catch6;
			_t = _retTree;
			_t = __t180;
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 751 "csharp_expression.g"
			
			if ((arg == nullptr) || (tail == nullptr)) {
			reportError("NULL method argument");
			delete arg;
			delete tail;
			args = nullptr;
			} else {
			args = new MethodArguments(arg, tail);
			}
			
#line 1267 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case 3:
		{
			//DBG genAction(ActionElement action
#line 761 "csharp_expression.g"
			
			args = new MethodArguments();
			
#line 1278 "CSharpExpressionCompiler.cc"
			break;
		}
		default:
		{
			SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltException(_t)); goto _catch6;
		}
		}
	}
	#ifdef ANTLR_EXCEPTIONS
		catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
			reportError(ex);
			if ( _t != ANTLR_USE_NAMESPACE(antlr)nullAST )
				_t = _t->getNextSibling();
		}
	#endif
	_catch6:
	if(0){}
	else if ( ANTLR_USE_NAMESPACE(antlr)RecognitionException::DynamicCast(ActiveException()) ){
		ANTLR_USE_NAMESPACE(antlr)RecognitionException* pEx = ANTLR_USE_NAMESPACE(antlr)RecognitionException::DynamicCast(ActiveException());
		reportError(*pEx);
		ClearException();
		if ( _t != ANTLR_USE_NAMESPACE(antlr)nullAST )
			_t = _t->getNextSibling();
	}
	else if ( ActiveException() ){ // uncaught exception handling
		return args;
	}
	_retTree = _t;
	return args;
}

//DBG genRule(dotSelector)
CSharpExpressionSelector*  CSharpExpressionCompiler::dotSelector(ANTLR_USE_NAMESPACE(antlr)RefAST _t) {
#line 726 "csharp_expression.g"
	CSharpExpressionSelector* js;
#line 1314 "CSharpExpressionCompiler.cc"
	ANTLR_USE_NAMESPACE(antlr)RefAST dotSelector_AST_in = _t;
	ANTLR_USE_NAMESPACE(antlr)RefAST identifier_node = ANTLR_USE_NAMESPACE(antlr)nullAST;
	ANTLR_USE_NAMESPACE(antlr)RefAST method_node = ANTLR_USE_NAMESPACE(antlr)nullAST;
#line 726 "csharp_expression.g"
	
	MethodArguments* r = nullptr;
	
	js = nullptr;
	
#line 1324 "CSharpExpressionCompiler.cc"
	
	#ifdef ANTLR_EXCEPTIONS
	try {      // for error handling
	#else
	{ //_catch7
	#endif
		//DBG genCommonBlk(
		if (_t == ANTLR_USE_NAMESPACE(antlr)nullAST )
			_t = ASTNULL;
		switch ( _t->getType()) {
		//DBG genCases(BitSet)
		case Identifier:
		{
			//DBG genTokenRef(TokenRefElement)
			identifier_node = _t;
			match(_t,Identifier);
			if (ActiveException()) goto _catch7;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 731 "csharp_expression.g"
			
			js = new CSharpExpressionMemberSelector(identifier_node->getText());
			
#line 1348 "CSharpExpressionCompiler.cc"
			break;
		}
		//DBG genCases(BitSet)
		case METHOD_CALL:
		{
			ANTLR_USE_NAMESPACE(antlr)RefAST __t178 = _t;
			ANTLR_USE_NAMESPACE(antlr)RefAST tmp41_AST_in = _t;
			match(_t,METHOD_CALL);
			if (ActiveException()) goto _catch7;
			_t = _t->getFirstChild();
			//DBG genTokenRef(TokenRefElement)
			method_node = _t;
			match(_t,Identifier);
			if (ActiveException()) goto _catch7;//gen(TokenRefElement atom)
			_t = _t->getNextSibling();
			//DBG RuleRefElement( arguments)
			r=arguments(_t);
			if (ActiveException()) goto _catch7;
			_t = _retTree;
			_t = __t178;
			_t = _t->getNextSibling();
			//DBG genAction(ActionElement action
#line 734 "csharp_expression.g"
			
			if (r == nullptr) {
			reportError("NULL method arguments in dotSelector METHOD_CALL");
			js = nullptr;
			} else {
			js = new MethodCallExpression(method_node->getText(), r);
			}
			
#line 1380 "CSharpExpressionCompiler.cc"
			break;
		}
		default:
		{
			SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltException(_t)); goto _catch7;
		}
		}
	}
	#ifdef ANTLR_EXCEPTIONS
		catch (ANTLR_USE_NAMESPACE(antlr)RecognitionException& ex) {
			reportError(ex);
			if ( _t != ANTLR_USE_NAMESPACE(antlr)nullAST )
				_t = _t->getNextSibling();
		}
	#endif
	_catch7:
	if(0){}
	else if ( ANTLR_USE_NAMESPACE(antlr)RecognitionException::DynamicCast(ActiveException()) ){
		ANTLR_USE_NAMESPACE(antlr)RecognitionException* pEx = ANTLR_USE_NAMESPACE(antlr)RecognitionException::DynamicCast(ActiveException());
		reportError(*pEx);
		ClearException();
		if ( _t != ANTLR_USE_NAMESPACE(antlr)nullAST )
			_t = _t->getNextSibling();
	}
	else if ( ActiveException() ){ // uncaught exception handling
		return js;
	}
	_retTree = _t;
	return js;
}

void CSharpExpressionCompiler::initializeASTFactory( ANTLR_USE_NAMESPACE(antlr)ASTFactory& factory )
{
}
const char* CSharpExpressionCompiler::tokenNames[] = {
	"<0>",
	"EOF",
	"<2>",
	"NULL_TREE_LOOKAHEAD",
	"STATEMENT",
	"BINARY_EXPRESSION",
	"UNARY_EXPRESSION",
	"PARENTHESES_EXPRESSION",
	"TYPE_CAST",
	"TYPE_NAME",
	"PRIMARY_SELECTOR",
	"DOT_SELECTOR",
	"METHOD_CALL",
	"EXPRESSION_LIST",
	"\"null\"",
	"\"true\"",
	"\"false\"",
	"DOT",
	"HEX_NUMERIC_LITERAL",
	"OCT_NUMERIC_LITERAL",
	"FP_NUMERIC_LITERAL",
	"DEC_NUMERIC_LITERAL",
	"HexDigit",
	"DecDigit",
	"OctDigit",
	"NumericLiteral",
	"CharacterLiteral",
	"SingleCharacter",
	"StringLiteral",
	"StringCharacters",
	"StringCharacter",
	"EscapeSequence",
	"OctalEscape",
	"UnicodeEscape",
	"ZeroToThree",
	"Identifier",
	"LPAREN",
	"RPAREN",
	"LBRACE",
	"RBRACE",
	"LBRACK",
	"RBRACK",
	"SEMI",
	"COMMA",
	"ASSIGN",
	"CMP_GT",
	"CMP_LT",
	"BANG",
	"TILDE",
	"QUESTION",
	"COLON",
	"EQUAL",
	"CMP_LE",
	"CMP_GE",
	"NOTEQUAL",
	"AND",
	"OR",
	"ADD",
	"SUB",
	"MUL",
	"DIV",
	"BITAND",
	"BITOR",
	"CARET",
	"MOD",
	"SHIFT_LEFT",
	"SHIFT_RIGHT_S",
	"SHIFT_RIGHT_U",
	"WS",
	"COMMENT",
	"LINE_COMMENT",
	0
};



ANTLR_END_NAMESPACE
