/* ANTLR Translator Generator
 * Project led by Terence Parr at http://www.jGuru.com
 * Software rights: http://www.antlr.org/RIGHTS.html
 *
 * $Id: //depot/code/org.antlr/release/antlr-2.7.2/lib/cpp/src/NoViableAltException.cpp#1 $
 */

#include "antlr/NoViableAltException.hpp"
#include "antlr/String.hpp"

#ifdef ANTLR_CXX_SUPPORTS_NAMESPACE
namespace antlr {
#endif

ANTLR_IMPLEMENT_DYNAMIC( NoViableAltException, RecognitionException, ANTLRException );

ANTLR_USING_NAMESPACE(std)

NoViableAltException::NoViableAltException(RefAST t)
  : RecognitionException("NoViableAlt","<AST>",-1,-1),
    token(0), node(t)
{
}

NoViableAltException::NoViableAltException(
	RefToken t,
	const string& fileName_
) : RecognitionException("NoViableAlt",fileName_,t->getLine(),t->getColumn()),
    token(t), node(nullASTptr)
{
}

string NoViableAltException::getMessage() const
{
	if (token)
	{
		if( token->getType() == Token::EOF_TYPE )
			return string("unexpected end of file");
		else if( token->getType() == Token::NULL_TREE_LOOKAHEAD )
			return string("unexpected end of tree");
		else
			return string("unexpected token: ")+token->getText();
	}

	// must a tree parser error if token==null
	if (!node)
		return "unexpected end of subtree";

	return string("unexpected AST node: ")+node->toString();
}

#ifdef ANTLR_CXX_SUPPORTS_NAMESPACE
}
#endif
