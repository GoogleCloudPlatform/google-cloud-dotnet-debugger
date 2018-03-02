/* $ANTLR 2.7.2: "csharp_expression.g" -> "JavaExpressionLexer.cc"$ */
#include "JavaExpressionLexer.hpp"
#include "third_party/antlr/lib/cpp/antlr/CharBuffer.hpp"
#include "third_party/antlr/lib/cpp/antlr/TokenStreamException.hpp"
#include "third_party/antlr/lib/cpp/antlr/TokenStreamIOException.hpp"
#include "third_party/antlr/lib/cpp/antlr/TokenStreamRecognitionException.hpp"
#include "third_party/antlr/lib/cpp/antlr/CharStreamException.hpp"
#include "third_party/antlr/lib/cpp/antlr/CharStreamIOException.hpp"
#include "third_party/antlr/lib/cpp/antlr/NoViableAltForCharException.hpp"
#include "third_party/antlr/lib/cpp/antlr/SemanticException.hpp"

ANTLR_BEGIN_NAMESPACE(google_cloud_debugger)
#line 1 "csharp_expression.g"
#line 15 "JavaExpressionLexer.cc"
JavaExpressionLexer::JavaExpressionLexer(ANTLR_USE_NAMESPACE(std)istream& in)
	: ANTLR_USE_NAMESPACE(antlr)CharScanner(new ANTLR_USE_NAMESPACE(antlr)CharBuffer(in),true)
{
	initLiterals();
}

JavaExpressionLexer::JavaExpressionLexer(ANTLR_USE_NAMESPACE(antlr)InputBuffer& ib)
	: ANTLR_USE_NAMESPACE(antlr)CharScanner(ib,true)
{
	initLiterals();
}

JavaExpressionLexer::JavaExpressionLexer(const ANTLR_USE_NAMESPACE(antlr)LexerSharedInputState& state)
	: ANTLR_USE_NAMESPACE(antlr)CharScanner(state,true)
{
	initLiterals();
}

void JavaExpressionLexer::initLiterals()
{
	literals["null"] = 14;
	literals["true"] = 15;
	literals["false"] = 16;
}

ANTLR_USE_NAMESPACE(antlr)RefToken JavaExpressionLexer::nextToken()
{
	ANTLR_USE_NAMESPACE(antlr)RefToken theRetToken;
	for (;;) {
		ANTLR_USE_NAMESPACE(antlr)RefToken theRetToken;
		int _ttype = ANTLR_USE_NAMESPACE(antlr)Token::INVALID_TYPE;
		resetText();
		{   // TRY for lexical and char stream error handling
			//DBG genCommonBlk(
			switch ( LA(1)) {
			//DBG genCases(BitSet)
			case static_cast<unsigned char>('.'):
			case static_cast<unsigned char>('0'):
			case static_cast<unsigned char>('1'):
			case static_cast<unsigned char>('2'):
			case static_cast<unsigned char>('3'):
			case static_cast<unsigned char>('4'):
			case static_cast<unsigned char>('5'):
			case static_cast<unsigned char>('6'):
			case static_cast<unsigned char>('7'):
			case static_cast<unsigned char>('8'):
			case static_cast<unsigned char>('9'):
			{
				//DBG RuleRefElement( mNumericLiteral)
				mNumericLiteral(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>('\''):
			{
				//DBG RuleRefElement( mCharacterLiteral)
				mCharacterLiteral(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>('"'):
			{
				//DBG RuleRefElement( mStringLiteral)
				mStringLiteral(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>('$'):
			case static_cast<unsigned char>('A'):
			case static_cast<unsigned char>('B'):
			case static_cast<unsigned char>('C'):
			case static_cast<unsigned char>('D'):
			case static_cast<unsigned char>('E'):
			case static_cast<unsigned char>('F'):
			case static_cast<unsigned char>('G'):
			case static_cast<unsigned char>('H'):
			case static_cast<unsigned char>('I'):
			case static_cast<unsigned char>('J'):
			case static_cast<unsigned char>('K'):
			case static_cast<unsigned char>('L'):
			case static_cast<unsigned char>('M'):
			case static_cast<unsigned char>('N'):
			case static_cast<unsigned char>('O'):
			case static_cast<unsigned char>('P'):
			case static_cast<unsigned char>('Q'):
			case static_cast<unsigned char>('R'):
			case static_cast<unsigned char>('S'):
			case static_cast<unsigned char>('T'):
			case static_cast<unsigned char>('U'):
			case static_cast<unsigned char>('V'):
			case static_cast<unsigned char>('W'):
			case static_cast<unsigned char>('X'):
			case static_cast<unsigned char>('Y'):
			case static_cast<unsigned char>('Z'):
			case static_cast<unsigned char>('_'):
			case static_cast<unsigned char>('a'):
			case static_cast<unsigned char>('b'):
			case static_cast<unsigned char>('c'):
			case static_cast<unsigned char>('d'):
			case static_cast<unsigned char>('e'):
			case static_cast<unsigned char>('f'):
			case static_cast<unsigned char>('g'):
			case static_cast<unsigned char>('h'):
			case static_cast<unsigned char>('i'):
			case static_cast<unsigned char>('j'):
			case static_cast<unsigned char>('k'):
			case static_cast<unsigned char>('l'):
			case static_cast<unsigned char>('m'):
			case static_cast<unsigned char>('n'):
			case static_cast<unsigned char>('o'):
			case static_cast<unsigned char>('p'):
			case static_cast<unsigned char>('q'):
			case static_cast<unsigned char>('r'):
			case static_cast<unsigned char>('s'):
			case static_cast<unsigned char>('t'):
			case static_cast<unsigned char>('u'):
			case static_cast<unsigned char>('v'):
			case static_cast<unsigned char>('w'):
			case static_cast<unsigned char>('x'):
			case static_cast<unsigned char>('y'):
			case static_cast<unsigned char>('z'):
			{
				//DBG RuleRefElement( mIdentifier)
				mIdentifier(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>('('):
			{
				//DBG RuleRefElement( mLPAREN)
				mLPAREN(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>(')'):
			{
				//DBG RuleRefElement( mRPAREN)
				mRPAREN(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>('{'):
			{
				//DBG RuleRefElement( mLBRACE)
				mLBRACE(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>('}'):
			{
				//DBG RuleRefElement( mRBRACE)
				mRBRACE(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>('['):
			{
				//DBG RuleRefElement( mLBRACK)
				mLBRACK(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>(']'):
			{
				//DBG RuleRefElement( mRBRACK)
				mRBRACK(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>(';'):
			{
				//DBG RuleRefElement( mSEMI)
				mSEMI(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>(','):
			{
				//DBG RuleRefElement( mCOMMA)
				mCOMMA(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>('~'):
			{
				//DBG RuleRefElement( mTILDE)
				mTILDE(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>('?'):
			{
				//DBG RuleRefElement( mQUESTION)
				mQUESTION(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>(':'):
			{
				//DBG RuleRefElement( mCOLON)
				mCOLON(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>('+'):
			{
				//DBG RuleRefElement( mADD)
				mADD(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>('-'):
			{
				//DBG RuleRefElement( mSUB)
				mSUB(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>('*'):
			{
				//DBG RuleRefElement( mMUL)
				mMUL(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>('^'):
			{
				//DBG RuleRefElement( mCARET)
				mCARET(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>('%'):
			{
				//DBG RuleRefElement( mMOD)
				mMOD(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>('\t'):
			case static_cast<unsigned char>('\n'):
			case static_cast<unsigned char>('\r'):
			case static_cast<unsigned char>(' '):
			{
				//DBG RuleRefElement( mWS)
				mWS(true);
				if (ActiveException()) goto _catch31;
				theRetToken=_returnToken;
				break;
			}
			default:
				if ((LA(1) == static_cast<unsigned char>('>')) && (LA(2) == static_cast<unsigned char>('>')) && (LA(3) == static_cast<unsigned char>('>'))) {
					//DBG RuleRefElement( mSHIFT_RIGHT_U)
					mSHIFT_RIGHT_U(true);
					if (ActiveException()) goto _catch31;
					theRetToken=_returnToken;
				}
				else if ((LA(1) == static_cast<unsigned char>('=')) && (LA(2) == static_cast<unsigned char>('='))) {
					//DBG RuleRefElement( mEQUAL)
					mEQUAL(true);
					if (ActiveException()) goto _catch31;
					theRetToken=_returnToken;
				}
				else if ((LA(1) == static_cast<unsigned char>('<')) && (LA(2) == static_cast<unsigned char>('='))) {
					//DBG RuleRefElement( mCMP_LE)
					mCMP_LE(true);
					if (ActiveException()) goto _catch31;
					theRetToken=_returnToken;
				}
				else if ((LA(1) == static_cast<unsigned char>('>')) && (LA(2) == static_cast<unsigned char>('='))) {
					//DBG RuleRefElement( mCMP_GE)
					mCMP_GE(true);
					if (ActiveException()) goto _catch31;
					theRetToken=_returnToken;
				}
				else if ((LA(1) == static_cast<unsigned char>('!')) && (LA(2) == static_cast<unsigned char>('='))) {
					//DBG RuleRefElement( mNOTEQUAL)
					mNOTEQUAL(true);
					if (ActiveException()) goto _catch31;
					theRetToken=_returnToken;
				}
				else if ((LA(1) == static_cast<unsigned char>('&')) && (LA(2) == static_cast<unsigned char>('&'))) {
					//DBG RuleRefElement( mAND)
					mAND(true);
					if (ActiveException()) goto _catch31;
					theRetToken=_returnToken;
				}
				else if ((LA(1) == static_cast<unsigned char>('|')) && (LA(2) == static_cast<unsigned char>('|'))) {
					//DBG RuleRefElement( mOR)
					mOR(true);
					if (ActiveException()) goto _catch31;
					theRetToken=_returnToken;
				}
				else if ((LA(1) == static_cast<unsigned char>('<')) && (LA(2) == static_cast<unsigned char>('<'))) {
					//DBG RuleRefElement( mSHIFT_LEFT)
					mSHIFT_LEFT(true);
					if (ActiveException()) goto _catch31;
					theRetToken=_returnToken;
				}
				else if ((LA(1) == static_cast<unsigned char>('>')) && (LA(2) == static_cast<unsigned char>('>')) && (true)) {
					//DBG RuleRefElement( mSHIFT_RIGHT_S)
					mSHIFT_RIGHT_S(true);
					if (ActiveException()) goto _catch31;
					theRetToken=_returnToken;
				}
				else if ((LA(1) == static_cast<unsigned char>('/')) && (LA(2) == static_cast<unsigned char>('*'))) {
					//DBG RuleRefElement( mCOMMENT)
					mCOMMENT(true);
					if (ActiveException()) goto _catch31;
					theRetToken=_returnToken;
				}
				else if ((LA(1) == static_cast<unsigned char>('/')) && (LA(2) == static_cast<unsigned char>('/'))) {
					//DBG RuleRefElement( mLINE_COMMENT)
					mLINE_COMMENT(true);
					if (ActiveException()) goto _catch31;
					theRetToken=_returnToken;
				}
				else if ((LA(1) == static_cast<unsigned char>('=')) && (true)) {
					//DBG RuleRefElement( mASSIGN)
					mASSIGN(true);
					if (ActiveException()) goto _catch31;
					theRetToken=_returnToken;
				}
				else if ((LA(1) == static_cast<unsigned char>('>')) && (true)) {
					//DBG RuleRefElement( mCMP_GT)
					mCMP_GT(true);
					if (ActiveException()) goto _catch31;
					theRetToken=_returnToken;
				}
				else if ((LA(1) == static_cast<unsigned char>('<')) && (true)) {
					//DBG RuleRefElement( mCMP_LT)
					mCMP_LT(true);
					if (ActiveException()) goto _catch31;
					theRetToken=_returnToken;
				}
				else if ((LA(1) == static_cast<unsigned char>('!')) && (true)) {
					//DBG RuleRefElement( mBANG)
					mBANG(true);
					if (ActiveException()) goto _catch31;
					theRetToken=_returnToken;
				}
				else if ((LA(1) == static_cast<unsigned char>('/')) && (true)) {
					//DBG RuleRefElement( mDIV)
					mDIV(true);
					if (ActiveException()) goto _catch31;
					theRetToken=_returnToken;
				}
				else if ((LA(1) == static_cast<unsigned char>('&')) && (true)) {
					//DBG RuleRefElement( mBITAND)
					mBITAND(true);
					if (ActiveException()) goto _catch31;
					theRetToken=_returnToken;
				}
				else if ((LA(1) == static_cast<unsigned char>('|')) && (true)) {
					//DBG RuleRefElement( mBITOR)
					mBITOR(true);
					if (ActiveException()) goto _catch31;
					theRetToken=_returnToken;
				}
			// dbg: genBlockFinish - a
			else {
				if (LA(1)==EOF_CHAR)
				{
					uponEOF();
					_returnToken = makeToken(ANTLR_USE_NAMESPACE(antlr)Token::EOF_TYPE);
				}
				else {SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); goto _catch31;}
			}
			}
			if ( !_returnToken )
				goto tryAgain; // found SKIP token

			_ttype = _returnToken->getType();
			_ttype = testLiteralsTable(_ttype);
			_returnToken->setType(_ttype);
			return _returnToken;
		} // closing TRY block
		_catch31: ;
		if (0) {}
		else if ( ANTLR_USE_NAMESPACE(antlr)RecognitionException::DynamicCast(ActiveException()) ){
			ANTLR_USE_NAMESPACE(antlr)RecognitionException* pEx = ANTLR_USE_NAMESPACE(antlr)RecognitionException::DynamicCast(ActiveException());
			{
				SetException(new ANTLR_USE_NAMESPACE(antlr)TokenStreamRecognitionException(*pEx));
				return theRetToken;
			}
		}  // close of RecognitionException catch
		else if ( ANTLR_USE_NAMESPACE(antlr)CharStreamIOException::DynamicCast(ActiveException()) ){
			ANTLR_USE_NAMESPACE(antlr)CharStreamIOException* pEx = ANTLR_USE_NAMESPACE(antlr)CharStreamIOException::DynamicCast(ActiveException());
			SetException(new ANTLR_USE_NAMESPACE(antlr)TokenStreamIOException(pEx->getMessage()));
			return theRetToken;
		}
		else if ( ANTLR_USE_NAMESPACE(antlr)CharStreamException::DynamicCast(ActiveException()) ){
			ANTLR_USE_NAMESPACE(antlr)CharStreamException* pEx = ANTLR_USE_NAMESPACE(antlr)CharStreamException::DynamicCast(ActiveException());
			SetException(new ANTLR_USE_NAMESPACE(antlr)TokenStreamException(pEx->getMessage()));
			return theRetToken;
		}
		else if ( ActiveException() ){ // uncaught exception handling
			return theRetToken;
		}
tryAgain:;
	}
}

//DBG genRule(mHexDigit)
void JavaExpressionLexer::mHexDigit(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = HexDigit;
	int _saveIndex;
	
	//DBG genCommonBlk(
	switch ( LA(1)) {
	//DBG genCases(BitSet)
	case static_cast<unsigned char>('0'):
	case static_cast<unsigned char>('1'):
	case static_cast<unsigned char>('2'):
	case static_cast<unsigned char>('3'):
	case static_cast<unsigned char>('4'):
	case static_cast<unsigned char>('5'):
	case static_cast<unsigned char>('6'):
	case static_cast<unsigned char>('7'):
	case static_cast<unsigned char>('8'):
	case static_cast<unsigned char>('9'):
	{
		matchRange(static_cast<unsigned char>('0'),static_cast<unsigned char>('9'));
		if (ActiveException()) return ;//gen(CharRangeElement r)
		break;
	}
	//DBG genCases(BitSet)
	case static_cast<unsigned char>('a'):
	case static_cast<unsigned char>('b'):
	case static_cast<unsigned char>('c'):
	case static_cast<unsigned char>('d'):
	case static_cast<unsigned char>('e'):
	case static_cast<unsigned char>('f'):
	{
		matchRange(static_cast<unsigned char>('a'),static_cast<unsigned char>('f'));
		if (ActiveException()) return ;//gen(CharRangeElement r)
		break;
	}
	//DBG genCases(BitSet)
	case static_cast<unsigned char>('A'):
	case static_cast<unsigned char>('B'):
	case static_cast<unsigned char>('C'):
	case static_cast<unsigned char>('D'):
	case static_cast<unsigned char>('E'):
	case static_cast<unsigned char>('F'):
	{
		matchRange(static_cast<unsigned char>('A'),static_cast<unsigned char>('F'));
		if (ActiveException()) return ;//gen(CharRangeElement r)
		break;
	}
	default:
	{
		SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); return ;
	}
	}
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mDecDigit)
void JavaExpressionLexer::mDecDigit(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = DecDigit;
	int _saveIndex;
	
	matchRange(static_cast<unsigned char>('0'),static_cast<unsigned char>('9'));
	if (ActiveException()) return ;//gen(CharRangeElement r)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mOctDigit)
void JavaExpressionLexer::mOctDigit(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = OctDigit;
	int _saveIndex;
	
	matchRange(static_cast<unsigned char>('0'),static_cast<unsigned char>('7'));
	if (ActiveException()) return ;//gen(CharRangeElement r)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mNumericLiteral)
void JavaExpressionLexer::mNumericLiteral(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = NumericLiteral;
	int _saveIndex;
	
	//DBG genCommonBlk(
	//DBG gen=>(SynPredBlock)
	bool synPredMatched6 = false;
	if (((LA(1) == static_cast<unsigned char>('0')) && (LA(2) == static_cast<unsigned char>('x')))) {
		int _m6 = mark();
		synPredMatched6 = true;
		inputState->guessing++;
		{ //xxx_catch32
			//DBG gen(AlternativeBlock blk)
			{
			//DBG genCommonBlk(
			//DBG genString(StringLiteralElement)
			match("0x");
			if (ActiveException()) goto _catch32;//gen(StringLiteralElement atom)
			}
		}
		_catch32:
		{
			if (ActiveException()) {
			synPredMatched6 = false;
			ClearException();
			}
		}
		rewind(_m6);
		inputState->guessing--;
	}
	if ( synPredMatched6 ) {
		//DBG genString(StringLiteralElement)
		match("0x");
		if (ActiveException()) return ;//gen(StringLiteralElement atom)
		//DBG gen+(OneOrMoreBlock)
		{ // ( ... )+
		int _cnt8=0;
		for (;;) {
			//DBG genCommonBlk(
			if ((_tokenSet_0.member(LA(1)))) {
				//DBG RuleRefElement( mHexDigit)
				mHexDigit(false);
				if (ActiveException()) return ;
			}
			else {
				if ( _cnt8>=1 ) { goto _loop8; } else {SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); return ;}
			}
			
			_cnt8++;
		}
		_loop8:;
		}  // ( ... )+
		//DBG gen(AlternativeBlock blk)
		{
		//DBG genCommonBlk(
		switch ( LA(1)) {
		//DBG genCases(BitSet)
		case static_cast<unsigned char>('l'):
		{
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('l'));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			break;
		}
		//DBG genCases(BitSet)
		case static_cast<unsigned char>('L'):
		{
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('L'));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			break;
		}
		default:
			{
			}
		}
		}
		//DBG genAction(ActionElement action
		if ( inputState->guessing==0 ) {
#line 92 "csharp_expression.g"
			_ttype = HEX_NUMERIC_LITERAL;
#line 635 "JavaExpressionLexer.cc"
		}
	}
	else {
		//DBG gen=>(SynPredBlock)
		bool synPredMatched11 = false;
		if (((LA(1) == static_cast<unsigned char>('0')) && ((LA(2) >= static_cast<unsigned char>('0') && LA(2) <= static_cast<unsigned char>('7'))) && (true))) {
			int _m11 = mark();
			synPredMatched11 = true;
			inputState->guessing++;
			{ //xxx_catch33
				//DBG gen(AlternativeBlock blk)
				{
				//DBG genCommonBlk(
				//DBG genChar(CharLiteralElement)
				match(static_cast<unsigned char>('0'));
				if (ActiveException()) goto _catch33;//gen(CharLiteralElement atom)
				//DBG RuleRefElement( mOctDigit)
				mOctDigit(false);
				if (ActiveException()) goto _catch33;
				}
			}
			_catch33:
			{
				if (ActiveException()) {
				synPredMatched11 = false;
				ClearException();
				}
			}
			rewind(_m11);
			inputState->guessing--;
		}
		if ( synPredMatched11 ) {
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('0'));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			//DBG gen+(OneOrMoreBlock)
			{ // ( ... )+
			int _cnt13=0;
			for (;;) {
				//DBG genCommonBlk(
				if (((LA(1) >= static_cast<unsigned char>('0') && LA(1) <= static_cast<unsigned char>('7')))) {
					//DBG RuleRefElement( mOctDigit)
					mOctDigit(false);
					if (ActiveException()) return ;
				}
				else {
					if ( _cnt13>=1 ) { goto _loop13; } else {SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); return ;}
				}
				
				_cnt13++;
			}
			_loop13:;
			}  // ( ... )+
			//DBG gen(AlternativeBlock blk)
			{
			//DBG genCommonBlk(
			switch ( LA(1)) {
			//DBG genCases(BitSet)
			case static_cast<unsigned char>('l'):
			{
				//DBG genChar(CharLiteralElement)
				match(static_cast<unsigned char>('l'));
				if (ActiveException()) return ;//gen(CharLiteralElement atom)
				break;
			}
			//DBG genCases(BitSet)
			case static_cast<unsigned char>('L'):
			{
				//DBG genChar(CharLiteralElement)
				match(static_cast<unsigned char>('L'));
				if (ActiveException()) return ;//gen(CharLiteralElement atom)
				break;
			}
			default:
				{
				}
			}
			}
			//DBG genAction(ActionElement action
			if ( inputState->guessing==0 ) {
#line 95 "csharp_expression.g"
				_ttype = OCT_NUMERIC_LITERAL;
#line 718 "JavaExpressionLexer.cc"
			}
		}
		else {
			//DBG gen=>(SynPredBlock)
			bool synPredMatched18 = false;
			if (((_tokenSet_1.member(LA(1))) && (_tokenSet_1.member(LA(2))) && (true))) {
				int _m18 = mark();
				synPredMatched18 = true;
				inputState->guessing++;
				{ //xxx_catch34
					//DBG gen(AlternativeBlock blk)
					{
					//DBG genCommonBlk(
					//DBG gen*(ZeroOrMoreBlock)
					{ // ( ... )*
					for (;;) {
						//DBG genCommonBlk(
						if (((LA(1) >= static_cast<unsigned char>('0') && LA(1) <= static_cast<unsigned char>('9')))) {
							//DBG RuleRefElement( mDecDigit)
							mDecDigit(false);
							if (ActiveException()) goto _catch34;
						}
						else {
							goto _loop17;
						}
						
					}
					_loop17:;
					} // ( ... )*
					//DBG genChar(CharLiteralElement)
					match(static_cast<unsigned char>('.'));
					if (ActiveException()) goto _catch34;//gen(CharLiteralElement atom)
					//DBG RuleRefElement( mDecDigit)
					mDecDigit(false);
					if (ActiveException()) goto _catch34;
					}
				}
				_catch34:
				{
					if (ActiveException()) {
					synPredMatched18 = false;
					ClearException();
					}
				}
				rewind(_m18);
				inputState->guessing--;
			}
			if ( synPredMatched18 ) {
				//DBG gen*(ZeroOrMoreBlock)
				{ // ( ... )*
				for (;;) {
					//DBG genCommonBlk(
					if (((LA(1) >= static_cast<unsigned char>('0') && LA(1) <= static_cast<unsigned char>('9')))) {
						//DBG RuleRefElement( mDecDigit)
						mDecDigit(false);
						if (ActiveException()) return ;
					}
					else {
						goto _loop20;
					}
					
				}
				_loop20:;
				} // ( ... )*
				//DBG genChar(CharLiteralElement)
				match(static_cast<unsigned char>('.'));
				if (ActiveException()) return ;//gen(CharLiteralElement atom)
				//DBG gen+(OneOrMoreBlock)
				{ // ( ... )+
				int _cnt22=0;
				for (;;) {
					//DBG genCommonBlk(
					if (((LA(1) >= static_cast<unsigned char>('0') && LA(1) <= static_cast<unsigned char>('9')))) {
						//DBG RuleRefElement( mDecDigit)
						mDecDigit(false);
						if (ActiveException()) return ;
					}
					else {
						if ( _cnt22>=1 ) { goto _loop22; } else {SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); return ;}
					}
					
					_cnt22++;
				}
				_loop22:;
				}  // ( ... )+
				//DBG gen(AlternativeBlock blk)
				{
				//DBG genCommonBlk(
				switch ( LA(1)) {
				//DBG genCases(BitSet)
				case static_cast<unsigned char>('d'):
				{
					//DBG genChar(CharLiteralElement)
					match(static_cast<unsigned char>('d'));
					if (ActiveException()) return ;//gen(CharLiteralElement atom)
					break;
				}
				//DBG genCases(BitSet)
				case static_cast<unsigned char>('D'):
				{
					//DBG genChar(CharLiteralElement)
					match(static_cast<unsigned char>('D'));
					if (ActiveException()) return ;//gen(CharLiteralElement atom)
					break;
				}
				//DBG genCases(BitSet)
				case static_cast<unsigned char>('f'):
				{
					//DBG genChar(CharLiteralElement)
					match(static_cast<unsigned char>('f'));
					if (ActiveException()) return ;//gen(CharLiteralElement atom)
					break;
				}
				//DBG genCases(BitSet)
				case static_cast<unsigned char>('F'):
				{
					//DBG genChar(CharLiteralElement)
					match(static_cast<unsigned char>('F'));
					if (ActiveException()) return ;//gen(CharLiteralElement atom)
					break;
				}
				default:
					{
					}
				}
				}
				//DBG genAction(ActionElement action
				if ( inputState->guessing==0 ) {
#line 98 "csharp_expression.g"
					_ttype = FP_NUMERIC_LITERAL;
#line 849 "JavaExpressionLexer.cc"
				}
			}
			else {
				//DBG gen=>(SynPredBlock)
				bool synPredMatched28 = false;
				if ((((LA(1) >= static_cast<unsigned char>('0') && LA(1) <= static_cast<unsigned char>('9'))) && (_tokenSet_2.member(LA(2))) && (true))) {
					int _m28 = mark();
					synPredMatched28 = true;
					inputState->guessing++;
					{ //xxx_catch35
						//DBG gen(AlternativeBlock blk)
						{
						//DBG genCommonBlk(
						//DBG gen+(OneOrMoreBlock)
						{ // ( ... )+
						int _cnt26=0;
						for (;;) {
							//DBG genCommonBlk(
							if (((LA(1) >= static_cast<unsigned char>('0') && LA(1) <= static_cast<unsigned char>('9')))) {
								//DBG RuleRefElement( mDecDigit)
								mDecDigit(false);
								if (ActiveException()) goto _catch35;
							}
							else {
								if ( _cnt26>=1 ) { goto _loop26; } else {SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); goto _catch35;}
							}
							
							_cnt26++;
						}
						_loop26:;
						}  // ( ... )+
						//DBG gen(AlternativeBlock blk)
						{
						//DBG genCommonBlk(
						switch ( LA(1)) {
						//DBG genCases(BitSet)
						case static_cast<unsigned char>('d'):
						{
							//DBG genChar(CharLiteralElement)
							match(static_cast<unsigned char>('d'));
							if (ActiveException()) goto _catch35;//gen(CharLiteralElement atom)
							break;
						}
						//DBG genCases(BitSet)
						case static_cast<unsigned char>('D'):
						{
							//DBG genChar(CharLiteralElement)
							match(static_cast<unsigned char>('D'));
							if (ActiveException()) goto _catch35;//gen(CharLiteralElement atom)
							break;
						}
						//DBG genCases(BitSet)
						case static_cast<unsigned char>('f'):
						{
							//DBG genChar(CharLiteralElement)
							match(static_cast<unsigned char>('f'));
							if (ActiveException()) goto _catch35;//gen(CharLiteralElement atom)
							break;
						}
						//DBG genCases(BitSet)
						case static_cast<unsigned char>('F'):
						{
							//DBG genChar(CharLiteralElement)
							match(static_cast<unsigned char>('F'));
							if (ActiveException()) goto _catch35;//gen(CharLiteralElement atom)
							break;
						}
						default:
						{
							SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); goto _catch35;
						}
						}
						}
						}
					}
					_catch35:
					{
						if (ActiveException()) {
						synPredMatched28 = false;
						ClearException();
						}
					}
					rewind(_m28);
					inputState->guessing--;
				}
				if ( synPredMatched28 ) {
					//DBG gen+(OneOrMoreBlock)
					{ // ( ... )+
					int _cnt30=0;
					for (;;) {
						//DBG genCommonBlk(
						if (((LA(1) >= static_cast<unsigned char>('0') && LA(1) <= static_cast<unsigned char>('9')))) {
							//DBG RuleRefElement( mDecDigit)
							mDecDigit(false);
							if (ActiveException()) return ;
						}
						else {
							if ( _cnt30>=1 ) { goto _loop30; } else {SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); return ;}
						}
						
						_cnt30++;
					}
					_loop30:;
					}  // ( ... )+
					//DBG gen(AlternativeBlock blk)
					{
					//DBG genCommonBlk(
					switch ( LA(1)) {
					//DBG genCases(BitSet)
					case static_cast<unsigned char>('d'):
					{
						//DBG genChar(CharLiteralElement)
						match(static_cast<unsigned char>('d'));
						if (ActiveException()) return ;//gen(CharLiteralElement atom)
						break;
					}
					//DBG genCases(BitSet)
					case static_cast<unsigned char>('D'):
					{
						//DBG genChar(CharLiteralElement)
						match(static_cast<unsigned char>('D'));
						if (ActiveException()) return ;//gen(CharLiteralElement atom)
						break;
					}
					//DBG genCases(BitSet)
					case static_cast<unsigned char>('f'):
					{
						//DBG genChar(CharLiteralElement)
						match(static_cast<unsigned char>('f'));
						if (ActiveException()) return ;//gen(CharLiteralElement atom)
						break;
					}
					//DBG genCases(BitSet)
					case static_cast<unsigned char>('F'):
					{
						//DBG genChar(CharLiteralElement)
						match(static_cast<unsigned char>('F'));
						if (ActiveException()) return ;//gen(CharLiteralElement atom)
						break;
					}
					default:
					{
						SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); return ;
					}
					}
					}
					//DBG genAction(ActionElement action
					if ( inputState->guessing==0 ) {
#line 101 "csharp_expression.g"
						_ttype = FP_NUMERIC_LITERAL;
#line 1000 "JavaExpressionLexer.cc"
					}
				}
				else if (((LA(1) >= static_cast<unsigned char>('0') && LA(1) <= static_cast<unsigned char>('9'))) && (true) && (true)) {
					//DBG gen+(OneOrMoreBlock)
					{ // ( ... )+
					int _cnt33=0;
					for (;;) {
						//DBG genCommonBlk(
						if (((LA(1) >= static_cast<unsigned char>('0') && LA(1) <= static_cast<unsigned char>('9')))) {
							//DBG RuleRefElement( mDecDigit)
							mDecDigit(false);
							if (ActiveException()) return ;
						}
						else {
							if ( _cnt33>=1 ) { goto _loop33; } else {SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); return ;}
						}
						
						_cnt33++;
					}
					_loop33:;
					}  // ( ... )+
					//DBG gen(AlternativeBlock blk)
					{
					//DBG genCommonBlk(
					switch ( LA(1)) {
					//DBG genCases(BitSet)
					case static_cast<unsigned char>('l'):
					{
						//DBG genChar(CharLiteralElement)
						match(static_cast<unsigned char>('l'));
						if (ActiveException()) return ;//gen(CharLiteralElement atom)
						break;
					}
					//DBG genCases(BitSet)
					case static_cast<unsigned char>('L'):
					{
						//DBG genChar(CharLiteralElement)
						match(static_cast<unsigned char>('L'));
						if (ActiveException()) return ;//gen(CharLiteralElement atom)
						break;
					}
					default:
						{
						}
					}
					}
					//DBG genAction(ActionElement action
					if ( inputState->guessing==0 ) {
#line 104 "csharp_expression.g"
						_ttype = DEC_NUMERIC_LITERAL;
#line 1051 "JavaExpressionLexer.cc"
					}
				}
				else if ((LA(1) == static_cast<unsigned char>('.')) && (true)) {
					//DBG genChar(CharLiteralElement)
					match(static_cast<unsigned char>('.'));
					if (ActiveException()) return ;//gen(CharLiteralElement atom)
					//DBG genAction(ActionElement action
					if ( inputState->guessing==0 ) {
#line 106 "csharp_expression.g"
						_ttype = DOT;
#line 1062 "JavaExpressionLexer.cc"
					}
				}
	else {
		SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); return ;
	}
	}}}
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mCharacterLiteral)
void JavaExpressionLexer::mCharacterLiteral(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = CharacterLiteral;
	int _saveIndex;
	
	//DBG genCommonBlk(
	if ((LA(1) == static_cast<unsigned char>('\'')) && (_tokenSet_3.member(LA(2)))) {
		//DBG genChar(CharLiteralElement)
		_saveIndex=text.length();
		match(static_cast<unsigned char>('\''));
		text.erase(_saveIndex);
		if (ActiveException()) return ;//gen(CharLiteralElement atom)
		//DBG RuleRefElement( mSingleCharacter)
		mSingleCharacter(false);
		if (ActiveException()) return ;
		//DBG genChar(CharLiteralElement)
		_saveIndex=text.length();
		match(static_cast<unsigned char>('\''));
		text.erase(_saveIndex);
		if (ActiveException()) return ;//gen(CharLiteralElement atom)
	}
	else if ((LA(1) == static_cast<unsigned char>('\'')) && (LA(2) == static_cast<unsigned char>('\\'))) {
		//DBG genChar(CharLiteralElement)
		_saveIndex=text.length();
		match(static_cast<unsigned char>('\''));
		text.erase(_saveIndex);
		if (ActiveException()) return ;//gen(CharLiteralElement atom)
		//DBG RuleRefElement( mEscapeSequence)
		mEscapeSequence(false);
		if (ActiveException()) return ;
		//DBG genChar(CharLiteralElement)
		_saveIndex=text.length();
		match(static_cast<unsigned char>('\''));
		text.erase(_saveIndex);
		if (ActiveException()) return ;//gen(CharLiteralElement atom)
	}
	else {
		SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); return ;
	}
	
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mSingleCharacter)
void JavaExpressionLexer::mSingleCharacter(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = SingleCharacter;
	int _saveIndex;
	
	//DBG gen(AlternativeBlock blk)
	{
	//DBG genCommonBlk(
	match(_tokenSet_3);
	if (ActiveException()) return ;
	}
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mEscapeSequence)
void JavaExpressionLexer::mEscapeSequence(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = EscapeSequence;
	int _saveIndex;
	
	//DBG genCommonBlk(
	if ((LA(1) == static_cast<unsigned char>('\\')) && (_tokenSet_4.member(LA(2)))) {
		//DBG genChar(CharLiteralElement)
		match(static_cast<unsigned char>('\\'));
		if (ActiveException()) return ;//gen(CharLiteralElement atom)
		//DBG gen(AlternativeBlock blk)
		{
		//DBG genCommonBlk(
		switch ( LA(1)) {
		//DBG genCases(BitSet)
		case static_cast<unsigned char>('b'):
		{
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('b'));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			break;
		}
		//DBG genCases(BitSet)
		case static_cast<unsigned char>('t'):
		{
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('t'));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			break;
		}
		//DBG genCases(BitSet)
		case static_cast<unsigned char>('n'):
		{
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('n'));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			break;
		}
		//DBG genCases(BitSet)
		case static_cast<unsigned char>('f'):
		{
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('f'));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			break;
		}
		//DBG genCases(BitSet)
		case static_cast<unsigned char>('r'):
		{
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('r'));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			break;
		}
		//DBG genCases(BitSet)
		case static_cast<unsigned char>('"'):
		{
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('"'));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			break;
		}
		//DBG genCases(BitSet)
		case static_cast<unsigned char>('\''):
		{
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('\''));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			break;
		}
		//DBG genCases(BitSet)
		case static_cast<unsigned char>('\\'):
		{
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('\\'));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			break;
		}
		default:
		{
			SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); return ;
		}
		}
		}
	}
	else if ((LA(1) == static_cast<unsigned char>('\\')) && ((LA(2) >= static_cast<unsigned char>('0') && LA(2) <= static_cast<unsigned char>('7')))) {
		//DBG RuleRefElement( mOctalEscape)
		mOctalEscape(false);
		if (ActiveException()) return ;
	}
	else if ((LA(1) == static_cast<unsigned char>('\\')) && (LA(2) == static_cast<unsigned char>('u'))) {
		//DBG RuleRefElement( mUnicodeEscape)
		mUnicodeEscape(false);
		if (ActiveException()) return ;
	}
	else {
		SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); return ;
	}
	
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mStringLiteral)
void JavaExpressionLexer::mStringLiteral(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = StringLiteral;
	int _saveIndex;
	
	//DBG genChar(CharLiteralElement)
	_saveIndex=text.length();
	match(static_cast<unsigned char>('"'));
	text.erase(_saveIndex);
	if (ActiveException()) return ;//gen(CharLiteralElement atom)
	//DBG gen(AlternativeBlock blk)
	{
	//DBG genCommonBlk(
	if ((_tokenSet_5.member(LA(1)))) {
		//DBG RuleRefElement( mStringCharacters)
		mStringCharacters(false);
		if (ActiveException()) return ;
	}
	else if ((LA(1) == static_cast<unsigned char>('"'))) {
	}
	else {
		SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); return ;
	}
	
	}
	//DBG genChar(CharLiteralElement)
	_saveIndex=text.length();
	match(static_cast<unsigned char>('"'));
	text.erase(_saveIndex);
	if (ActiveException()) return ;//gen(CharLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mStringCharacters)
void JavaExpressionLexer::mStringCharacters(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = StringCharacters;
	int _saveIndex;
	
	//DBG gen+(OneOrMoreBlock)
	{ // ( ... )+
	int _cnt42=0;
	for (;;) {
		//DBG genCommonBlk(
		if ((_tokenSet_5.member(LA(1)))) {
			//DBG RuleRefElement( mStringCharacter)
			mStringCharacter(false);
			if (ActiveException()) return ;
		}
		else {
			if ( _cnt42>=1 ) { goto _loop42; } else {SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); return ;}
		}
		
		_cnt42++;
	}
	_loop42:;
	}  // ( ... )+
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mStringCharacter)
void JavaExpressionLexer::mStringCharacter(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = StringCharacter;
	int _saveIndex;
	
	//DBG genCommonBlk(
	if ((_tokenSet_6.member(LA(1)))) {
		//DBG gen(AlternativeBlock blk)
		{
		//DBG genCommonBlk(
		match(_tokenSet_6);
		if (ActiveException()) return ;
		}
	}
	else if ((LA(1) == static_cast<unsigned char>('\\'))) {
		//DBG RuleRefElement( mEscapeSequence)
		mEscapeSequence(false);
		if (ActiveException()) return ;
	}
	else {
		SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); return ;
	}
	
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mOctalEscape)
void JavaExpressionLexer::mOctalEscape(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = OctalEscape;
	int _saveIndex;
	
	//DBG genCommonBlk(
	//DBG gen=>(SynPredBlock)
	bool synPredMatched49 = false;
	if (((LA(1) == static_cast<unsigned char>('\\')) && ((LA(2) >= static_cast<unsigned char>('0') && LA(2) <= static_cast<unsigned char>('7'))) && ((LA(3) >= static_cast<unsigned char>('\3') && LA(3) <= static_cast<unsigned char>('\377'))))) {
		int _m49 = mark();
		synPredMatched49 = true;
		inputState->guessing++;
		{ //xxx_catch36
			//DBG gen(AlternativeBlock blk)
			{
			//DBG genCommonBlk(
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('\\'));
			if (ActiveException()) goto _catch36;//gen(CharLiteralElement atom)
			//DBG RuleRefElement( mOctDigit)
			mOctDigit(false);
			if (ActiveException()) goto _catch36;
			}
		}
		_catch36:
		{
			if (ActiveException()) {
			synPredMatched49 = false;
			ClearException();
			}
		}
		rewind(_m49);
		inputState->guessing--;
	}
	if ( synPredMatched49 ) {
		//DBG genChar(CharLiteralElement)
		match(static_cast<unsigned char>('\\'));
		if (ActiveException()) return ;//gen(CharLiteralElement atom)
		//DBG RuleRefElement( mOctDigit)
		mOctDigit(false);
		if (ActiveException()) return ;
	}
	else {
		//DBG gen=>(SynPredBlock)
		bool synPredMatched51 = false;
		if (((LA(1) == static_cast<unsigned char>('\\')) && ((LA(2) >= static_cast<unsigned char>('0') && LA(2) <= static_cast<unsigned char>('7'))) && ((LA(3) >= static_cast<unsigned char>('0') && LA(3) <= static_cast<unsigned char>('7'))))) {
			int _m51 = mark();
			synPredMatched51 = true;
			inputState->guessing++;
			{ //xxx_catch37
				//DBG gen(AlternativeBlock blk)
				{
				//DBG genCommonBlk(
				//DBG genChar(CharLiteralElement)
				match(static_cast<unsigned char>('\\'));
				if (ActiveException()) goto _catch37;//gen(CharLiteralElement atom)
				//DBG RuleRefElement( mOctDigit)
				mOctDigit(false);
				if (ActiveException()) goto _catch37;
				//DBG RuleRefElement( mOctDigit)
				mOctDigit(false);
				if (ActiveException()) goto _catch37;
				}
			}
			_catch37:
			{
				if (ActiveException()) {
				synPredMatched51 = false;
				ClearException();
				}
			}
			rewind(_m51);
			inputState->guessing--;
		}
		if ( synPredMatched51 ) {
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('\\'));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			//DBG RuleRefElement( mOctDigit)
			mOctDigit(false);
			if (ActiveException()) return ;
			//DBG RuleRefElement( mOctDigit)
			mOctDigit(false);
			if (ActiveException()) return ;
		}
		else if ((LA(1) == static_cast<unsigned char>('\\')) && ((LA(2) >= static_cast<unsigned char>('0') && LA(2) <= static_cast<unsigned char>('3'))) && ((LA(3) >= static_cast<unsigned char>('0') && LA(3) <= static_cast<unsigned char>('7')))) {
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('\\'));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			//DBG RuleRefElement( mZeroToThree)
			mZeroToThree(false);
			if (ActiveException()) return ;
			//DBG RuleRefElement( mOctDigit)
			mOctDigit(false);
			if (ActiveException()) return ;
			//DBG RuleRefElement( mOctDigit)
			mOctDigit(false);
			if (ActiveException()) return ;
		}
	else {
		SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); return ;
	}
	}
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mUnicodeEscape)
void JavaExpressionLexer::mUnicodeEscape(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = UnicodeEscape;
	int _saveIndex;
	
	//DBG genChar(CharLiteralElement)
	match(static_cast<unsigned char>('\\'));
	if (ActiveException()) return ;//gen(CharLiteralElement atom)
	//DBG genChar(CharLiteralElement)
	match(static_cast<unsigned char>('u'));
	if (ActiveException()) return ;//gen(CharLiteralElement atom)
	//DBG RuleRefElement( mHexDigit)
	mHexDigit(false);
	if (ActiveException()) return ;
	//DBG RuleRefElement( mHexDigit)
	mHexDigit(false);
	if (ActiveException()) return ;
	//DBG RuleRefElement( mHexDigit)
	mHexDigit(false);
	if (ActiveException()) return ;
	//DBG RuleRefElement( mHexDigit)
	mHexDigit(false);
	if (ActiveException()) return ;
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mZeroToThree)
void JavaExpressionLexer::mZeroToThree(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = ZeroToThree;
	int _saveIndex;
	
	//DBG gen(AlternativeBlock blk)
	{
	//DBG genCommonBlk(
	matchRange(static_cast<unsigned char>('0'),static_cast<unsigned char>('3'));
	if (ActiveException()) return ;//gen(CharRangeElement r)
	}
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mIdentifier)
void JavaExpressionLexer::mIdentifier(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = Identifier;
	int _saveIndex;
	
	//DBG gen(AlternativeBlock blk)
	{
	//DBG genCommonBlk(
	switch ( LA(1)) {
	//DBG genCases(BitSet)
	case static_cast<unsigned char>('a'):
	case static_cast<unsigned char>('b'):
	case static_cast<unsigned char>('c'):
	case static_cast<unsigned char>('d'):
	case static_cast<unsigned char>('e'):
	case static_cast<unsigned char>('f'):
	case static_cast<unsigned char>('g'):
	case static_cast<unsigned char>('h'):
	case static_cast<unsigned char>('i'):
	case static_cast<unsigned char>('j'):
	case static_cast<unsigned char>('k'):
	case static_cast<unsigned char>('l'):
	case static_cast<unsigned char>('m'):
	case static_cast<unsigned char>('n'):
	case static_cast<unsigned char>('o'):
	case static_cast<unsigned char>('p'):
	case static_cast<unsigned char>('q'):
	case static_cast<unsigned char>('r'):
	case static_cast<unsigned char>('s'):
	case static_cast<unsigned char>('t'):
	case static_cast<unsigned char>('u'):
	case static_cast<unsigned char>('v'):
	case static_cast<unsigned char>('w'):
	case static_cast<unsigned char>('x'):
	case static_cast<unsigned char>('y'):
	case static_cast<unsigned char>('z'):
	{
		matchRange(static_cast<unsigned char>('a'),static_cast<unsigned char>('z'));
		if (ActiveException()) return ;//gen(CharRangeElement r)
		break;
	}
	//DBG genCases(BitSet)
	case static_cast<unsigned char>('A'):
	case static_cast<unsigned char>('B'):
	case static_cast<unsigned char>('C'):
	case static_cast<unsigned char>('D'):
	case static_cast<unsigned char>('E'):
	case static_cast<unsigned char>('F'):
	case static_cast<unsigned char>('G'):
	case static_cast<unsigned char>('H'):
	case static_cast<unsigned char>('I'):
	case static_cast<unsigned char>('J'):
	case static_cast<unsigned char>('K'):
	case static_cast<unsigned char>('L'):
	case static_cast<unsigned char>('M'):
	case static_cast<unsigned char>('N'):
	case static_cast<unsigned char>('O'):
	case static_cast<unsigned char>('P'):
	case static_cast<unsigned char>('Q'):
	case static_cast<unsigned char>('R'):
	case static_cast<unsigned char>('S'):
	case static_cast<unsigned char>('T'):
	case static_cast<unsigned char>('U'):
	case static_cast<unsigned char>('V'):
	case static_cast<unsigned char>('W'):
	case static_cast<unsigned char>('X'):
	case static_cast<unsigned char>('Y'):
	case static_cast<unsigned char>('Z'):
	{
		matchRange(static_cast<unsigned char>('A'),static_cast<unsigned char>('Z'));
		if (ActiveException()) return ;//gen(CharRangeElement r)
		break;
	}
	//DBG genCases(BitSet)
	case static_cast<unsigned char>('$'):
	{
		//DBG genChar(CharLiteralElement)
		match(static_cast<unsigned char>('$'));
		if (ActiveException()) return ;//gen(CharLiteralElement atom)
		break;
	}
	//DBG genCases(BitSet)
	case static_cast<unsigned char>('_'):
	{
		//DBG genChar(CharLiteralElement)
		match(static_cast<unsigned char>('_'));
		if (ActiveException()) return ;//gen(CharLiteralElement atom)
		break;
	}
	default:
	{
		SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); return ;
	}
	}
	}
	//DBG gen*(ZeroOrMoreBlock)
	{ // ( ... )*
	for (;;) {
		//DBG genCommonBlk(
		switch ( LA(1)) {
		//DBG genCases(BitSet)
		case static_cast<unsigned char>('a'):
		case static_cast<unsigned char>('b'):
		case static_cast<unsigned char>('c'):
		case static_cast<unsigned char>('d'):
		case static_cast<unsigned char>('e'):
		case static_cast<unsigned char>('f'):
		case static_cast<unsigned char>('g'):
		case static_cast<unsigned char>('h'):
		case static_cast<unsigned char>('i'):
		case static_cast<unsigned char>('j'):
		case static_cast<unsigned char>('k'):
		case static_cast<unsigned char>('l'):
		case static_cast<unsigned char>('m'):
		case static_cast<unsigned char>('n'):
		case static_cast<unsigned char>('o'):
		case static_cast<unsigned char>('p'):
		case static_cast<unsigned char>('q'):
		case static_cast<unsigned char>('r'):
		case static_cast<unsigned char>('s'):
		case static_cast<unsigned char>('t'):
		case static_cast<unsigned char>('u'):
		case static_cast<unsigned char>('v'):
		case static_cast<unsigned char>('w'):
		case static_cast<unsigned char>('x'):
		case static_cast<unsigned char>('y'):
		case static_cast<unsigned char>('z'):
		{
			matchRange(static_cast<unsigned char>('a'),static_cast<unsigned char>('z'));
			if (ActiveException()) return ;//gen(CharRangeElement r)
			break;
		}
		//DBG genCases(BitSet)
		case static_cast<unsigned char>('A'):
		case static_cast<unsigned char>('B'):
		case static_cast<unsigned char>('C'):
		case static_cast<unsigned char>('D'):
		case static_cast<unsigned char>('E'):
		case static_cast<unsigned char>('F'):
		case static_cast<unsigned char>('G'):
		case static_cast<unsigned char>('H'):
		case static_cast<unsigned char>('I'):
		case static_cast<unsigned char>('J'):
		case static_cast<unsigned char>('K'):
		case static_cast<unsigned char>('L'):
		case static_cast<unsigned char>('M'):
		case static_cast<unsigned char>('N'):
		case static_cast<unsigned char>('O'):
		case static_cast<unsigned char>('P'):
		case static_cast<unsigned char>('Q'):
		case static_cast<unsigned char>('R'):
		case static_cast<unsigned char>('S'):
		case static_cast<unsigned char>('T'):
		case static_cast<unsigned char>('U'):
		case static_cast<unsigned char>('V'):
		case static_cast<unsigned char>('W'):
		case static_cast<unsigned char>('X'):
		case static_cast<unsigned char>('Y'):
		case static_cast<unsigned char>('Z'):
		{
			matchRange(static_cast<unsigned char>('A'),static_cast<unsigned char>('Z'));
			if (ActiveException()) return ;//gen(CharRangeElement r)
			break;
		}
		//DBG genCases(BitSet)
		case static_cast<unsigned char>('0'):
		case static_cast<unsigned char>('1'):
		case static_cast<unsigned char>('2'):
		case static_cast<unsigned char>('3'):
		case static_cast<unsigned char>('4'):
		case static_cast<unsigned char>('5'):
		case static_cast<unsigned char>('6'):
		case static_cast<unsigned char>('7'):
		case static_cast<unsigned char>('8'):
		case static_cast<unsigned char>('9'):
		{
			matchRange(static_cast<unsigned char>('0'),static_cast<unsigned char>('9'));
			if (ActiveException()) return ;//gen(CharRangeElement r)
			break;
		}
		//DBG genCases(BitSet)
		case static_cast<unsigned char>('$'):
		{
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('$'));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			break;
		}
		//DBG genCases(BitSet)
		case static_cast<unsigned char>('_'):
		{
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('_'));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			break;
		}
		default:
		{
			goto _loop58;
		}
		}
	}
	_loop58:;
	} // ( ... )*
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mLPAREN)
void JavaExpressionLexer::mLPAREN(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = LPAREN;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("(");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mRPAREN)
void JavaExpressionLexer::mRPAREN(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = RPAREN;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match(")");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mLBRACE)
void JavaExpressionLexer::mLBRACE(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = LBRACE;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("{");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mRBRACE)
void JavaExpressionLexer::mRBRACE(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = RBRACE;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("}");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mLBRACK)
void JavaExpressionLexer::mLBRACK(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = LBRACK;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("[");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mRBRACK)
void JavaExpressionLexer::mRBRACK(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = RBRACK;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("]");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mSEMI)
void JavaExpressionLexer::mSEMI(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = SEMI;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match(";");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mCOMMA)
void JavaExpressionLexer::mCOMMA(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = COMMA;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match(",");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mASSIGN)
void JavaExpressionLexer::mASSIGN(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = ASSIGN;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("=");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mCMP_GT)
void JavaExpressionLexer::mCMP_GT(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = CMP_GT;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match(">");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mCMP_LT)
void JavaExpressionLexer::mCMP_LT(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = CMP_LT;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("<");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mBANG)
void JavaExpressionLexer::mBANG(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = BANG;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("!");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mTILDE)
void JavaExpressionLexer::mTILDE(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = TILDE;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("~");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mQUESTION)
void JavaExpressionLexer::mQUESTION(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = QUESTION;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("?");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mCOLON)
void JavaExpressionLexer::mCOLON(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = COLON;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match(":");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mEQUAL)
void JavaExpressionLexer::mEQUAL(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = EQUAL;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("==");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mCMP_LE)
void JavaExpressionLexer::mCMP_LE(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = CMP_LE;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("<=");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mCMP_GE)
void JavaExpressionLexer::mCMP_GE(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = CMP_GE;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match(">=");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mNOTEQUAL)
void JavaExpressionLexer::mNOTEQUAL(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = NOTEQUAL;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("!=");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mAND)
void JavaExpressionLexer::mAND(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = AND;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("&&");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mOR)
void JavaExpressionLexer::mOR(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = OR;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("||");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mADD)
void JavaExpressionLexer::mADD(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = ADD;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("+");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mSUB)
void JavaExpressionLexer::mSUB(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = SUB;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("-");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mMUL)
void JavaExpressionLexer::mMUL(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = MUL;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("*");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mDIV)
void JavaExpressionLexer::mDIV(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = DIV;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("/");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mBITAND)
void JavaExpressionLexer::mBITAND(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = BITAND;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("&");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mBITOR)
void JavaExpressionLexer::mBITOR(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = BITOR;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("|");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mCARET)
void JavaExpressionLexer::mCARET(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = CARET;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("^");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mMOD)
void JavaExpressionLexer::mMOD(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = MOD;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("%");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mSHIFT_LEFT)
void JavaExpressionLexer::mSHIFT_LEFT(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = SHIFT_LEFT;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("<<");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mSHIFT_RIGHT_S)
void JavaExpressionLexer::mSHIFT_RIGHT_S(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = SHIFT_RIGHT_S;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match(">>");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mSHIFT_RIGHT_U)
void JavaExpressionLexer::mSHIFT_RIGHT_U(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = SHIFT_RIGHT_U;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match(">>>");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mWS)
void JavaExpressionLexer::mWS(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = WS;
	int _saveIndex;
	
	//DBG gen+(OneOrMoreBlock)
	{ // ( ... )+
	int _cnt93=0;
	for (;;) {
		//DBG genCommonBlk(
		switch ( LA(1)) {
		//DBG genCases(BitSet)
		case static_cast<unsigned char>('\t'):
		{
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('\t'));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			break;
		}
		//DBG genCases(BitSet)
		case static_cast<unsigned char>('\r'):
		{
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('\r'));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			break;
		}
		//DBG genCases(BitSet)
		case static_cast<unsigned char>('\n'):
		{
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('\n'));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			break;
		}
		//DBG genCases(BitSet)
		case static_cast<unsigned char>(' '):
		{
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>(' '));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
			break;
		}
		default:
		{
			if ( _cnt93>=1 ) { goto _loop93; } else {SetException(new ANTLR_USE_NAMESPACE(antlr)NoViableAltForCharException(LA(1), getFilename(), getLine(), getColumn())); return ;}
		}
		}
		_cnt93++;
	}
	_loop93:;
	}  // ( ... )+
	//DBG genAction(ActionElement action
	if ( inputState->guessing==0 ) {
#line 197 "csharp_expression.g"
		_ttype = antlr::Token::SKIP;
#line 2335 "JavaExpressionLexer.cc"
	}
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mCOMMENT)
void JavaExpressionLexer::mCOMMENT(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = COMMENT;
	int _saveIndex;
	
	//DBG genString(StringLiteralElement)
	match("/*");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	//DBG gen*(ZeroOrMoreBlock)
	{ // ( ... )*
	for (;;) {
		//DBG genCommonBlk(
		if (((LA(1) == static_cast<unsigned char>('*')) && ((LA(2) >= static_cast<unsigned char>('\3') && LA(2) <= static_cast<unsigned char>('\377'))) && ((LA(3) >= static_cast<unsigned char>('\3') && LA(3) <= static_cast<unsigned char>('\377'))))&&( LA(2) != '/' )) {
			//DBG genChar(CharLiteralElement)
			match(static_cast<unsigned char>('*'));
			if (ActiveException()) return ;//gen(CharLiteralElement atom)
		}
		else if ((_tokenSet_7.member(LA(1)))) {
			//DBG gen(AlternativeBlock blk)
			{
			//DBG genCommonBlk(
			match(_tokenSet_7);
			if (ActiveException()) return ;
			}
		}
		else {
			goto _loop97;
		}
		
	}
	_loop97:;
	} // ( ... )*
	//DBG genString(StringLiteralElement)
	match("*/");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	//DBG genAction(ActionElement action
	if ( inputState->guessing==0 ) {
#line 202 "csharp_expression.g"
		_ttype = antlr::Token::SKIP;
#line 2385 "JavaExpressionLexer.cc"
	}
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}

//DBG genRule(mLINE_COMMENT)
void JavaExpressionLexer::mLINE_COMMENT(bool _createToken) {
	int _ttype; ANTLR_USE_NAMESPACE(antlr)RefToken _token; int _begin=text.length();
	_ttype = LINE_COMMENT;
	int _saveIndex;
	
	//DBG gen(AlternativeBlock blk)
	{
	//DBG genCommonBlk(
	//DBG genString(StringLiteralElement)
	match("//");
	if (ActiveException()) return ;//gen(StringLiteralElement atom)
	//DBG gen*(ZeroOrMoreBlock)
	{ // ( ... )*
	for (;;) {
		//DBG genCommonBlk(
		if ((_tokenSet_8.member(LA(1)))) {
			//DBG gen(AlternativeBlock blk)
			{
			//DBG genCommonBlk(
			match(_tokenSet_8);
			if (ActiveException()) return ;
			}
		}
		else {
			goto _loop102;
		}
		
	}
	_loop102:;
	} // ( ... )*
	}
	//DBG genAction(ActionElement action
	if ( inputState->guessing==0 ) {
#line 207 "csharp_expression.g"
		_ttype = antlr::Token::SKIP;
#line 2431 "JavaExpressionLexer.cc"
	}
	if ( _createToken && _token==ANTLR_USE_NAMESPACE(antlr)nullToken && _ttype!=ANTLR_USE_NAMESPACE(antlr)Token::SKIP ) {
	   _token = makeToken(_ttype);
	   _token->setText(text.substr(_begin, text.length()-_begin));
	}
	_returnToken = _token;
	_saveIndex=0;
}


const unsigned long JavaExpressionLexer::_tokenSet_0_data_[] = { 0UL, 67043328UL, 126UL, 126UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL };
// TILDE QUESTION COLON EQUAL CMP_LE CMP_GE NOTEQUAL AND OR ADD SHIFT_LEFT 
// SHIFT_RIGHT_S SHIFT_RIGHT_U WS COMMENT LINE_COMMENT 
const ANTLR_USE_NAMESPACE(antlr)BitSet JavaExpressionLexer::_tokenSet_0(_tokenSet_0_data_,10);
const unsigned long JavaExpressionLexer::_tokenSet_1_data_[] = { 0UL, 67059712UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL };
// CMP_LT TILDE QUESTION COLON EQUAL CMP_LE CMP_GE NOTEQUAL AND OR ADD 
const ANTLR_USE_NAMESPACE(antlr)BitSet JavaExpressionLexer::_tokenSet_1(_tokenSet_1_data_,10);
const unsigned long JavaExpressionLexer::_tokenSet_2_data_[] = { 0UL, 67043328UL, 80UL, 80UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL };
// TILDE QUESTION COLON EQUAL CMP_LE CMP_GE NOTEQUAL AND OR ADD WS LINE_COMMENT 
const ANTLR_USE_NAMESPACE(antlr)BitSet JavaExpressionLexer::_tokenSet_2(_tokenSet_2_data_,10);
const unsigned long JavaExpressionLexer::_tokenSet_3_data_[] = { 4294967288UL, 4294967167UL, 4026531839UL, 4294967295UL, 4294967295UL, 4294967295UL, 4294967295UL, 4294967295UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL };
// NULL_TREE_LOOKAHEAD STATEMENT BINARY_EXPRESSION UNARY_EXPRESSION PARENTHESES_EXPRESSION 
// TYPE_CAST TYPE_NAME PRIMARY_SELECTOR DOT_SELECTOR METHOD_CALL EXPRESSION_LIST 
// "null" "true" "false" DOT HEX_NUMERIC_LITERAL OCT_NUMERIC_LITERAL FP_NUMERIC_LITERAL 
// DEC_NUMERIC_LITERAL HexDigit DecDigit OctDigit NumericLiteral CharacterLiteral 
// SingleCharacter StringLiteral StringCharacters StringCharacter EscapeSequence 
// OctalEscape UnicodeEscape ZeroToThree Identifier LPAREN RPAREN LBRACE 
// LBRACK RBRACK SEMI COMMA ASSIGN CMP_GT CMP_LT BANG TILDE QUESTION COLON 
// EQUAL CMP_LE CMP_GE NOTEQUAL AND OR ADD SUB MUL DIV BITAND BITOR CARET 
// MOD SHIFT_LEFT SHIFT_RIGHT_S SHIFT_RIGHT_U WS COMMENT LINE_COMMENT 
const ANTLR_USE_NAMESPACE(antlr)BitSet JavaExpressionLexer::_tokenSet_3(_tokenSet_3_data_,16);
const unsigned long JavaExpressionLexer::_tokenSet_4_data_[] = { 0UL, 132UL, 268435456UL, 1327172UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL };
// ZeroToThree RBRACE 
const ANTLR_USE_NAMESPACE(antlr)BitSet JavaExpressionLexer::_tokenSet_4(_tokenSet_4_data_,10);
const unsigned long JavaExpressionLexer::_tokenSet_5_data_[] = { 4294967288UL, 4294967291UL, 4294967295UL, 4294967295UL, 4294967295UL, 4294967295UL, 4294967295UL, 4294967295UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL };
// NULL_TREE_LOOKAHEAD STATEMENT BINARY_EXPRESSION UNARY_EXPRESSION PARENTHESES_EXPRESSION 
// TYPE_CAST TYPE_NAME PRIMARY_SELECTOR DOT_SELECTOR METHOD_CALL EXPRESSION_LIST 
// "null" "true" "false" DOT HEX_NUMERIC_LITERAL OCT_NUMERIC_LITERAL FP_NUMERIC_LITERAL 
// DEC_NUMERIC_LITERAL HexDigit DecDigit OctDigit NumericLiteral CharacterLiteral 
// SingleCharacter StringLiteral StringCharacters StringCharacter EscapeSequence 
// OctalEscape UnicodeEscape Identifier LPAREN RPAREN LBRACE RBRACE LBRACK 
// RBRACK SEMI COMMA ASSIGN CMP_GT CMP_LT BANG TILDE QUESTION COLON EQUAL 
// CMP_LE CMP_GE NOTEQUAL AND OR ADD SUB MUL DIV BITAND BITOR CARET MOD 
// SHIFT_LEFT SHIFT_RIGHT_S SHIFT_RIGHT_U WS COMMENT LINE_COMMENT 
const ANTLR_USE_NAMESPACE(antlr)BitSet JavaExpressionLexer::_tokenSet_5(_tokenSet_5_data_,16);
const unsigned long JavaExpressionLexer::_tokenSet_6_data_[] = { 4294967288UL, 4294967291UL, 4026531839UL, 4294967295UL, 4294967295UL, 4294967295UL, 4294967295UL, 4294967295UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL };
// NULL_TREE_LOOKAHEAD STATEMENT BINARY_EXPRESSION UNARY_EXPRESSION PARENTHESES_EXPRESSION 
// TYPE_CAST TYPE_NAME PRIMARY_SELECTOR DOT_SELECTOR METHOD_CALL EXPRESSION_LIST 
// "null" "true" "false" DOT HEX_NUMERIC_LITERAL OCT_NUMERIC_LITERAL FP_NUMERIC_LITERAL 
// DEC_NUMERIC_LITERAL HexDigit DecDigit OctDigit NumericLiteral CharacterLiteral 
// SingleCharacter StringLiteral StringCharacters StringCharacter EscapeSequence 
// OctalEscape UnicodeEscape Identifier LPAREN RPAREN LBRACE RBRACE LBRACK 
// RBRACK SEMI COMMA ASSIGN CMP_GT CMP_LT BANG TILDE QUESTION COLON EQUAL 
// CMP_LE CMP_GE NOTEQUAL AND OR ADD SUB MUL DIV BITAND BITOR CARET MOD 
// SHIFT_LEFT SHIFT_RIGHT_S SHIFT_RIGHT_U WS COMMENT LINE_COMMENT 
const ANTLR_USE_NAMESPACE(antlr)BitSet JavaExpressionLexer::_tokenSet_6(_tokenSet_6_data_,16);
const unsigned long JavaExpressionLexer::_tokenSet_7_data_[] = { 4294967288UL, 4294966271UL, 4294967295UL, 4294967295UL, 4294967295UL, 4294967295UL, 4294967295UL, 4294967295UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL };
// NULL_TREE_LOOKAHEAD STATEMENT BINARY_EXPRESSION UNARY_EXPRESSION PARENTHESES_EXPRESSION 
// TYPE_CAST TYPE_NAME PRIMARY_SELECTOR DOT_SELECTOR METHOD_CALL EXPRESSION_LIST 
// "null" "true" "false" DOT HEX_NUMERIC_LITERAL OCT_NUMERIC_LITERAL FP_NUMERIC_LITERAL 
// DEC_NUMERIC_LITERAL HexDigit DecDigit OctDigit NumericLiteral CharacterLiteral 
// SingleCharacter StringLiteral StringCharacters StringCharacter EscapeSequence 
// OctalEscape UnicodeEscape ZeroToThree Identifier LPAREN RPAREN LBRACE 
// RBRACE LBRACK RBRACK COMMA ASSIGN CMP_GT CMP_LT BANG TILDE QUESTION 
// COLON EQUAL CMP_LE CMP_GE NOTEQUAL AND OR ADD SUB MUL DIV BITAND BITOR 
// CARET MOD SHIFT_LEFT SHIFT_RIGHT_S SHIFT_RIGHT_U WS COMMENT LINE_COMMENT 
const ANTLR_USE_NAMESPACE(antlr)BitSet JavaExpressionLexer::_tokenSet_7(_tokenSet_7_data_,16);
const unsigned long JavaExpressionLexer::_tokenSet_8_data_[] = { 4294958072UL, 4294967295UL, 4294967295UL, 4294967295UL, 4294967295UL, 4294967295UL, 4294967295UL, 4294967295UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL };
// NULL_TREE_LOOKAHEAD STATEMENT BINARY_EXPRESSION UNARY_EXPRESSION PARENTHESES_EXPRESSION 
// TYPE_CAST TYPE_NAME DOT_SELECTOR METHOD_CALL "null" "true" "false" DOT 
// HEX_NUMERIC_LITERAL OCT_NUMERIC_LITERAL FP_NUMERIC_LITERAL DEC_NUMERIC_LITERAL 
// HexDigit DecDigit OctDigit NumericLiteral CharacterLiteral SingleCharacter 
// StringLiteral StringCharacters StringCharacter EscapeSequence OctalEscape 
// UnicodeEscape ZeroToThree Identifier LPAREN RPAREN LBRACE RBRACE LBRACK 
// RBRACK SEMI COMMA ASSIGN CMP_GT CMP_LT BANG TILDE QUESTION COLON EQUAL 
// CMP_LE CMP_GE NOTEQUAL AND OR ADD SUB MUL DIV BITAND BITOR CARET MOD 
// SHIFT_LEFT SHIFT_RIGHT_S SHIFT_RIGHT_U WS COMMENT LINE_COMMENT 
const ANTLR_USE_NAMESPACE(antlr)BitSet JavaExpressionLexer::_tokenSet_8(_tokenSet_8_data_,16);

ANTLR_END_NAMESPACE
