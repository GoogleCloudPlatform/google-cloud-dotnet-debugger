#ifndef INC_JavaExpressionLexer_hpp_
#define INC_JavaExpressionLexer_hpp_

#include "third_party/antlr/lib/cpp/antlr/config.hpp"
/* $ANTLR 2.7.2: "csharp_expression.g" -> "JavaExpressionLexer.hpp"$ */
#include "third_party/antlr/lib/cpp/antlr/CommonToken.hpp"
#include "third_party/antlr/lib/cpp/antlr/InputBuffer.hpp"
#include "third_party/antlr/lib/cpp/antlr/BitSet.hpp"
#include "JavaExpressionLexerTokenTypes.hpp"
#include "third_party/antlr/lib/cpp/antlr/CharScanner.hpp"
#line 21 "csharp_expression.g"

  #include <iostream>
  #include <string>

  #include "../../cloud-debug-java/common.h"
  #include "../../cloud-debug-java/java_expression.h"
  #include "../../cloud-debug-java/messages.h"

#line 21 "JavaExpressionLexer.hpp"
ANTLR_BEGIN_NAMESPACE(google_cloud_debugger)
class JavaExpressionLexer : public ANTLR_USE_NAMESPACE(antlr)CharScanner, public JavaExpressionLexerTokenTypes
{
#line 1 "csharp_expression.g"
#line 26 "JavaExpressionLexer.hpp"
private:
	void initLiterals();
public:
	bool getCaseSensitiveLiterals() const
	{
		return true;
	}
public:
	JavaExpressionLexer(ANTLR_USE_NAMESPACE(std)istream& in);
	JavaExpressionLexer(ANTLR_USE_NAMESPACE(antlr)InputBuffer& ib);
	JavaExpressionLexer(const ANTLR_USE_NAMESPACE(antlr)LexerSharedInputState& state);
	ANTLR_USE_NAMESPACE(antlr)RefToken nextToken();
	//DBG genRuleHeader(mHexDigit)
	protected: void mHexDigit(bool _createToken);
	//DBG genRuleHeader(mDecDigit)
	protected: void mDecDigit(bool _createToken);
	//DBG genRuleHeader(mOctDigit)
	protected: void mOctDigit(bool _createToken);
	//DBG genRuleHeader(mNumericLiteral)
	public: void mNumericLiteral(bool _createToken);
	//DBG genRuleHeader(mCharacterLiteral)
	public: void mCharacterLiteral(bool _createToken);
	//DBG genRuleHeader(mSingleCharacter)
	protected: void mSingleCharacter(bool _createToken);
	//DBG genRuleHeader(mEscapeSequence)
	protected: void mEscapeSequence(bool _createToken);
	//DBG genRuleHeader(mStringLiteral)
	public: void mStringLiteral(bool _createToken);
	//DBG genRuleHeader(mStringCharacters)
	protected: void mStringCharacters(bool _createToken);
	//DBG genRuleHeader(mStringCharacter)
	protected: void mStringCharacter(bool _createToken);
	//DBG genRuleHeader(mOctalEscape)
	protected: void mOctalEscape(bool _createToken);
	//DBG genRuleHeader(mUnicodeEscape)
	protected: void mUnicodeEscape(bool _createToken);
	//DBG genRuleHeader(mZeroToThree)
	protected: void mZeroToThree(bool _createToken);
	//DBG genRuleHeader(mIdentifier)
	public: void mIdentifier(bool _createToken);
	//DBG genRuleHeader(mLPAREN)
	public: void mLPAREN(bool _createToken);
	//DBG genRuleHeader(mRPAREN)
	public: void mRPAREN(bool _createToken);
	//DBG genRuleHeader(mLBRACE)
	public: void mLBRACE(bool _createToken);
	//DBG genRuleHeader(mRBRACE)
	public: void mRBRACE(bool _createToken);
	//DBG genRuleHeader(mLBRACK)
	public: void mLBRACK(bool _createToken);
	//DBG genRuleHeader(mRBRACK)
	public: void mRBRACK(bool _createToken);
	//DBG genRuleHeader(mSEMI)
	public: void mSEMI(bool _createToken);
	//DBG genRuleHeader(mCOMMA)
	public: void mCOMMA(bool _createToken);
	//DBG genRuleHeader(mASSIGN)
	public: void mASSIGN(bool _createToken);
	//DBG genRuleHeader(mCMP_GT)
	public: void mCMP_GT(bool _createToken);
	//DBG genRuleHeader(mCMP_LT)
	public: void mCMP_LT(bool _createToken);
	//DBG genRuleHeader(mBANG)
	public: void mBANG(bool _createToken);
	//DBG genRuleHeader(mTILDE)
	public: void mTILDE(bool _createToken);
	//DBG genRuleHeader(mQUESTION)
	public: void mQUESTION(bool _createToken);
	//DBG genRuleHeader(mCOLON)
	public: void mCOLON(bool _createToken);
	//DBG genRuleHeader(mEQUAL)
	public: void mEQUAL(bool _createToken);
	//DBG genRuleHeader(mCMP_LE)
	public: void mCMP_LE(bool _createToken);
	//DBG genRuleHeader(mCMP_GE)
	public: void mCMP_GE(bool _createToken);
	//DBG genRuleHeader(mNOTEQUAL)
	public: void mNOTEQUAL(bool _createToken);
	//DBG genRuleHeader(mAND)
	public: void mAND(bool _createToken);
	//DBG genRuleHeader(mOR)
	public: void mOR(bool _createToken);
	//DBG genRuleHeader(mADD)
	public: void mADD(bool _createToken);
	//DBG genRuleHeader(mSUB)
	public: void mSUB(bool _createToken);
	//DBG genRuleHeader(mMUL)
	public: void mMUL(bool _createToken);
	//DBG genRuleHeader(mDIV)
	public: void mDIV(bool _createToken);
	//DBG genRuleHeader(mBITAND)
	public: void mBITAND(bool _createToken);
	//DBG genRuleHeader(mBITOR)
	public: void mBITOR(bool _createToken);
	//DBG genRuleHeader(mCARET)
	public: void mCARET(bool _createToken);
	//DBG genRuleHeader(mMOD)
	public: void mMOD(bool _createToken);
	//DBG genRuleHeader(mSHIFT_LEFT)
	public: void mSHIFT_LEFT(bool _createToken);
	//DBG genRuleHeader(mSHIFT_RIGHT_S)
	public: void mSHIFT_RIGHT_S(bool _createToken);
	//DBG genRuleHeader(mSHIFT_RIGHT_U)
	public: void mSHIFT_RIGHT_U(bool _createToken);
	//DBG genRuleHeader(mWS)
	public: void mWS(bool _createToken);
	//DBG genRuleHeader(mCOMMENT)
	public: void mCOMMENT(bool _createToken);
	//DBG genRuleHeader(mLINE_COMMENT)
	public: void mLINE_COMMENT(bool _createToken);
private:
	
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
};

ANTLR_END_NAMESPACE
#endif /*INC_JavaExpressionLexer_hpp_*/
