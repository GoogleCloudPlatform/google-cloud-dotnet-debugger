//
// Copyright 2015 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// ANTLR2 grammar for CSharp Cloud Debugger expression compiler.
// This grammar was inspired by this ANTLR3 grammar:
//   third_party/antlr3/java_grammar/Java.g

header "post_include_hpp" {
  #include <iostream>
  #include <string>

  #include "../../cloud-debug-java/common.h"
  #include "../../cloud-debug-java/csharp_expression.h"
  #include "../../cloud-debug-java/messages.h"
}

options {
  language="Cpp";
  namespace="google_cloud_debugger";
}

//
// LEXER
//

class CSharpExpressionLexer extends Lexer;

options {
  k = 3;  // Set lookahead.
  charVocabulary = '\3'..'\377';
}

// Tokens:
tokens {
  // We need a few "imaginary" tokens to inject common AST nodes in different
  // places. This technique allows common treatment for all such places in
  // tree walking.
  STATEMENT;
  BINARY_EXPRESSION;
  UNARY_EXPRESSION;
  PARENTHESES_EXPRESSION;
  TYPE_CAST;
  TYPE_NAME;
  PRIMARY_SELECTOR;
  DOT_SELECTOR;
  METHOD_CALL;
  EXPRESSION_LIST;

  JNULL         = "null";
  JTRUE          = "true";
  JFALSE         = "false";

  // No explicit lexer rule exists for DOT. See NumericLiteral lexer rule for
  // details.
  DOT;

  // Numeric literals.
  HEX_NUMERIC_LITERAL;
  OCT_NUMERIC_LITERAL;
  FP_NUMERIC_LITERAL;
  DEC_NUMERIC_LITERAL;
}

protected HexDigit
  : '0'..'9' | 'a'..'f' | 'A'..'F'
  ;

protected DecDigit
  : '0'..'9'
  ;

protected OctDigit
  : '0'..'7'
  ;

NumericLiteral
  : ("0x") => "0x" (HexDigit)+  ('l' | 'L')?
      // hex: 0x12AB, 0x34CDL
      { $setType(HEX_NUMERIC_LITERAL); }
  | ('0' OctDigit) => '0' (OctDigit)+  ('l' | 'L')?
      // oct: 01234, 05670L
      { $setType(OCT_NUMERIC_LITERAL); }
  | ((DecDigit)*  '.' DecDigit) => (DecDigit)* '.' (DecDigit)+ ('d' | 'D' | 'f' | 'F')?
      // fp: 12.3, .4, 5.6f, .6d
      { $setType(FP_NUMERIC_LITERAL); }
  | ((DecDigit)+ ('d' | 'D' | 'f' | 'F')) => (DecDigit)+ ('d' | 'D' | 'f' | 'F')
      // fp: 12f, 34d
      { $setType(FP_NUMERIC_LITERAL); }
  | (DecDigit)+ ('l' | 'L')?
      // dec: 123, 456L
      { $setType(DEC_NUMERIC_LITERAL); }
  | '.'
      { $setType(DOT); }
  ;

CharacterLiteral
  : '\''! SingleCharacter '\''!
  | '\''! EscapeSequence '\''!
  ;

protected SingleCharacter
  : ~('\'' | '\\')
  ;

StringLiteral
  : '"'! (StringCharacters)? '"'!
  ;

protected StringCharacters
  : (StringCharacter)+
  ;

protected StringCharacter
  : ~('"' | '\\')
  | EscapeSequence
  ;

protected EscapeSequence
  : '\\' ('b' | 't' | 'n' | 'f' | 'r' | '"' | '\'' | '\\')
  | OctalEscape
  | UnicodeEscape
  ;

protected OctalEscape
  : ('\\' OctDigit) => '\\' OctDigit
  | ('\\' OctDigit OctDigit) => '\\' OctDigit OctDigit
  | '\\' ZeroToThree OctDigit OctDigit
  ;

protected UnicodeEscape
  : '\\' 'u' HexDigit HexDigit HexDigit HexDigit
  ;

protected ZeroToThree
  : ('0'..'3')
  ;


// Only ASCII characters are supported right now.
// TODO(vlif): add support for Unicode characters in CSharp identifiers.
Identifier
  : ( 'a'..'z' | 'A'..'Z' | '$' | '_' )
    ( 'a'..'z' | 'A'..'Z' | '0'..'9' | '$' | '_' )*
  ;

LPAREN          : "(";
RPAREN          : ")";
LBRACE          : "{";
RBRACE          : "}";
LBRACK          : "[";
RBRACK          : "]";
SEMI            : ";";
COMMA           : ",";
ASSIGN          : "=";
CMP_GT          : ">";
CMP_LT          : "<";
BANG            : "!";
TILDE           : "~";
QUESTION        : "?";
COLON           : ":";
EQUAL           : "==";
CMP_LE          : "<=";
CMP_GE          : ">=";
NOTEQUAL        : "!=";
AND             : "&&";
OR              : "||";
ADD             : "+";
SUB             : "-";
MUL             : "*";
DIV             : "/";
BITAND          : "&";
BITOR           : "|";
CARET           : "^";
MOD             : "%";
SHIFT_LEFT      : "<<";
SHIFT_RIGHT_S   : ">>";
SHIFT_RIGHT_U   : ">>>";


// Skip whitespaces and comments.

WS
  : ('\t' | '\r' | '\n' | ' ')+
    { $setType(antlr::Token::SKIP); }
  ;

COMMENT
  : "/*" ( { LA(2) != '/' }? '*' | ~('*') )* "*/"
    { $setType(antlr::Token::SKIP); }
  ;

LINE_COMMENT
  : ("//" (~('\r' | '\n'))*)
    { $setType(antlr::Token::SKIP); }
  ;


//
// PARSER
//

class CSharpExpressionParser extends Parser;

options {
  k = 2;  // Set lookahead.
  buildAST = true;
  importVocab = CSharpExpressionLexer;
}

{
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
}

statement
  : expression
    { #statement = #([STATEMENT, "statement"], #statement); }
    EOF!
  ;

expression
  : conditionalExpression
  ;

expressionList
  : expression
    (
      COMMA! expressionList
    )?
    { #expressionList = #([EXPRESSION_LIST, "expression_list"], #expressionList); }
  ;

conditionalExpression
  : conditionalOrExpression
    (QUESTION^ expression COLON conditionalExpression)?
  ;

conditionalOrExpression
  : conditionalAndExpression
    (
      { #conditionalOrExpression = #([BINARY_EXPRESSION, "binary_expression"], #conditionalOrExpression); }
      OR conditionalAndExpression
    )*
  ;

conditionalAndExpression
  : inclusiveOrExpression
    (
      { #conditionalAndExpression = #([BINARY_EXPRESSION, "binary_expression"], #conditionalAndExpression); }
      AND inclusiveOrExpression
    )*
  ;

inclusiveOrExpression
  : exclusiveOrExpression
    (
      { #inclusiveOrExpression = #([BINARY_EXPRESSION, "binary_expression"], #inclusiveOrExpression); }
      BITOR exclusiveOrExpression
    )*
  ;

exclusiveOrExpression
  : andExpression
    (
      { #exclusiveOrExpression = #([BINARY_EXPRESSION, "binary_expression"], #exclusiveOrExpression); }
      CARET andExpression
    )*
  ;

andExpression
  : equalityExpression
    (
      { #andExpression = #([BINARY_EXPRESSION, "binary_expression"], #andExpression); }
      BITAND equalityExpression
    )*
  ;

equalityExpression
  : relationalExpression
    (
      { #equalityExpression = #([BINARY_EXPRESSION, "binary_expression"], #equalityExpression); }
      (EQUAL | NOTEQUAL) relationalExpression
    )*
  ;

relationalExpression
  : shiftExpression
    (
      { #relationalExpression = #([BINARY_EXPRESSION, "binary_expression"], #relationalExpression); }
      (CMP_LE | CMP_GE | CMP_LT | CMP_GT) shiftExpression
    )*
  ;

shiftExpression
  : additiveExpression
    (
      { #shiftExpression = #([BINARY_EXPRESSION, "binary_expression"], #shiftExpression); }
      (SHIFT_LEFT | SHIFT_RIGHT_S | SHIFT_RIGHT_U) additiveExpression
    )*
  ;

additiveExpression
  : multiplicativeExpression
    (
      { #additiveExpression = #([BINARY_EXPRESSION, "binary_expression"], #additiveExpression); }
      (ADD | SUB) multiplicativeExpression
    )*
  ;

multiplicativeExpression
  : unaryExpression
    (
      { #multiplicativeExpression = #([BINARY_EXPRESSION, "binary_expression"], #multiplicativeExpression); }
      (MUL | DIV | MOD) unaryExpression
    )*
  ;

unaryExpression
  : (ADD | SUB) unaryExpression
    { #unaryExpression = #([UNARY_EXPRESSION, "unary_expression"], #unaryExpression); }
  | unaryExpressionNotPlusMinus
  ;

unaryExpressionNotPlusMinus
  : (TILDE | BANG) unaryExpression
    { #unaryExpressionNotPlusMinus = #([UNARY_EXPRESSION, "unary_expression"], #unaryExpressionNotPlusMinus); }
  | (castExpression) => castExpression
  | primary
    (
      { #unaryExpressionNotPlusMinus = #([PRIMARY_SELECTOR, "primary_selector"], #unaryExpressionNotPlusMinus); }
      selector
    )*
  ;

castExpression
  : LPAREN! classOrInterfaceType RPAREN! unaryExpressionNotPlusMinus
    { #castExpression = #([TYPE_CAST, "type_cast"], #castExpression); }
  ;

primary
  : LPAREN! expression RPAREN!
    { #primary = #([PARENTHESES_EXPRESSION, "parentheses_expression"], #primary); }
  | Identifier
    (
      arguments
      { #primary = #([METHOD_CALL, "method_call"], #primary); }
    )?
  | literal
  ;

arguments
  : LPAREN! (expressionList)? RPAREN!
  ;

selector
  : DOT!
    Identifier
    (
      arguments
      { #selector = #([METHOD_CALL, "method_call"], #selector); }
    )?
    { #selector = #([DOT_SELECTOR, "dot_selector"], #selector); }
  | LBRACK^ expression RBRACK
  ;

literal
  : HEX_NUMERIC_LITERAL
  | OCT_NUMERIC_LITERAL
  | FP_NUMERIC_LITERAL
  | DEC_NUMERIC_LITERAL
  | CharacterLiteral
  | StringLiteral
  | JTRUE
  | JFALSE
  | JNULL
  ;

// TODO(vlif): add support for generic types.
classOrInterfaceType
  : Identifier
    (
      DOT!
      classOrInterfaceType
    )?
    { #classOrInterfaceType = #([TYPE_NAME, "type_name"], #classOrInterfaceType); }
  ;


//
// TREE WALKING (COMPILATION)
//

class CSharpExpressionCompiler extends TreeParser;

options {
  importVocab = CSharpExpressionLexer;
}

{
 public:
  // Since the process of compilation and evaluation is recursive and uses OS
  // stack, very deep expression trees can cause stack overflow. "MaxTreeDepth"
  // limits the maximum recursion depth to a safe threshold.
  static constexpr int kMaxTreeDepth = 25;

  void Init() {
  }

  std::unique_ptr<CSharpExpression> Walk(antlr::RefAST ast) {
    ResetErrorMessage();

    if (!VerifyMaxDepth(ast, kMaxTreeDepth)) {
      std::cerr << "The parsed expression tree is too deep";
      SetErrorMessage(ExpressionTreeTooDeep);
      return nullptr;
    }

    std::unique_ptr<CSharpExpression> expression(statement(ast));
    if (expression == nullptr) {
      // Set generic error message if specific error wasn't set.
      SetErrorMessage(ExpressionParserError);
    }

    return expression;
  }

  // Getter for the formatted error message. The error message will only be
  // set when "Walk" fails.
  const std::string &error_message() const { return error_message_; }

 private:
  void ResetErrorMessage() {
    error_message_.clear();
  }

  void SetErrorMessage(const std::string &new_error_message) {
    error_message_ = new_error_message;
  }

  void reportError(const antlr::RecognitionException& ex) {
    reportError(ex.getMessage());
  }

  void reportError(const std::string& msg) {
    std::cerr << "Internal parser error: " << msg;
  }

  // Verifies that the maximum depth of the subtree "node" does
  // not exceed "max_depth".
  static bool VerifyMaxDepth(antlr::RefAST node, int max_depth) {
    if (max_depth == 0) {
      return false;
    }

    antlr::RefAST child_node = node->getFirstChild();
    while (child_node != nullptr) {
      if (!VerifyMaxDepth(child_node, max_depth - 1)) {
        return false;
      }

      child_node = child_node->getNextSibling();
    }

    return true;
  }

 private:
  // Formatted localizable error message reported back to the user. The
  // message template are defined in "messages.h" file. Empty
  // "error_message_.format" indicates that the error message is not available.
  // Only first error message encountered is captured. Subsequent error 
  // messages are ignored.
  std::string error_message_;
}


statement returns [CSharpExpression* root] {
    root = nullptr;
  }
  : #(STATEMENT root=expression)
  ;

expression returns [CSharpExpression* je] {
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
  }
  : #(QUESTION a=expression b=expression COLON c=expression) {
    if ((a == nullptr) || (b == nullptr) || (c == nullptr)) {
      reportError("NULL argument in conditional expression");
      delete a;
      delete b;
      delete c;
      je = nullptr;
    } else {
      je = new ConditionalCSharpExpression(a, b, c);
    }
  }
  | #(PARENTHESES_EXPRESSION je=expression)
  | #(BINARY_EXPRESSION a=expression binary_expression_type=binary_expression_token b=expression) {
    if ((a == nullptr) || (b == nullptr)) {
      reportError("NULL argument in binary expression");
      delete a;
      delete b;
      je = nullptr;
    } else {
      je = new BinaryCSharpExpression(binary_expression_type, a, b);
    }
  }
  | #(UNARY_EXPRESSION unary_expression_type=unary_expression_token a=expression) {
    if (a == nullptr) {
      reportError("NULL argument in unary expression");
      je = nullptr;
    } else {
      je = new UnaryCSharpExpression(unary_expression_type, a);
    }
  }
  | identifier_node:Identifier {
    je = new CSharpIdentifier(identifier_node->getText());
  }
  | hex_numeric_literal_node:HEX_NUMERIC_LITERAL {
    CSharpIntLiteral* nl = new CSharpIntLiteral;
    if (!nl->ParseString(hex_numeric_literal_node->getText(), 16)) {
      reportError("Hex integer literal could not be parsed");
      SetErrorMessage(BadNumericLiteral);

      delete nl;
      je = nullptr;
    } else {
      je = nl;
    }
  }
  | oct_numeric_literal_node:OCT_NUMERIC_LITERAL {
    CSharpIntLiteral* nl = new CSharpIntLiteral;
    if (!nl->ParseString(oct_numeric_literal_node->getText(), 8)) {
      reportError("Octal integer literal could not be parsed");
      SetErrorMessage(BadNumericLiteral);

      delete nl;
      je = nullptr;
    } else {
      je = nl;
    }
  }
  | fp_numeric_literal_node:FP_NUMERIC_LITERAL {
    CSharpFloatLiteral* nl = new CSharpFloatLiteral;
    if (!nl->ParseString(fp_numeric_literal_node->getText())) {
      reportError("Floating point literal could not be parsed");
      SetErrorMessage(BadNumericLiteral);

      delete nl;
      je = nullptr;
    } else {
      je = nl;
    }
  }
  | dec_numeric_literal_node:DEC_NUMERIC_LITERAL {
    CSharpIntLiteral* nl = new CSharpIntLiteral;
    if (!nl->ParseString(dec_numeric_literal_node->getText(), 10)) {
      reportError("Decimal integer literal could not be parsed");
      SetErrorMessage(BadNumericLiteral);

      delete nl;
      je = nullptr;
    } else {
      je = nl;
    }
  }
  | character_literal_node:CharacterLiteral {
    CSharpCharLiteral* cl = new CSharpCharLiteral;
    if (!cl->ParseString(character_literal_node->getText())) {
      reportError("Invalid character");
      delete cl;
      je = nullptr;
    } else {
       je = cl;
    }
  }
  | string_literal_node:StringLiteral {
    CSharpStringLiteral* sl = new CSharpStringLiteral;
    if (!sl->ParseString(string_literal_node->getText())) {
      reportError("Invalid string");
      delete sl;
      je = nullptr;
    } else {
       je = sl;
    }
  }
  | JTRUE {
    je = new CSharpBooleanLiteral(true);
  }
  | JFALSE {
    je = new CSharpBooleanLiteral(false);
  }
  | JNULL {
    je = new CSharpNullLiteral;
  }
  | #(PRIMARY_SELECTOR a=expression s=selector) {
    if ((a == nullptr) || (s == nullptr)) {
      reportError("PRIMARY_SELECTED inner expressions failed to compile");
      delete a;
      delete s;
      je = nullptr;
    } else {
      s->set_source(a);
      je = s;
    }
  }
  | #(TYPE_CAST type=type_name a=expression) {
    if (a == nullptr) {
      reportError("NULL source in type cast expression");
      je = nullptr;
    } else {
      je = new TypeCastCSharpExpression(type, a);
    }
  }
  | #(METHOD_CALL method_node:Identifier r=arguments) {
    if (r == nullptr) {
      reportError("NULL method arguments in expression METHOD_CALL");
      je = nullptr;
    } else {
      je = new MethodCallExpression(method_node->getText(), r);
    }
  }
  ;

binary_expression_token returns [BinaryCSharpExpression::Type type] {
    type = static_cast<BinaryCSharpExpression::Type>(-1);
  }
  : ADD { type = BinaryCSharpExpression::Type::add; }
  | SUB { type = BinaryCSharpExpression::Type::sub; }
  | MUL { type = BinaryCSharpExpression::Type::mul; }
  | DIV { type = BinaryCSharpExpression::Type::div; }
  | MOD { type = BinaryCSharpExpression::Type::mod; }
  | AND { type = BinaryCSharpExpression::Type::conditional_and; }
  | OR { type = BinaryCSharpExpression::Type::conditional_or; }
  | EQUAL { type = BinaryCSharpExpression::Type::eq; }
  | NOTEQUAL { type = BinaryCSharpExpression::Type::ne; }
  | CMP_LE { type = BinaryCSharpExpression::Type::le; }
  | CMP_GE { type = BinaryCSharpExpression::Type::ge; }
  | CMP_LT { type = BinaryCSharpExpression::Type::lt; }
  | CMP_GT { type = BinaryCSharpExpression::Type::gt; }
  | BITAND { type = BinaryCSharpExpression::Type::bitwise_and; }
  | BITOR { type = BinaryCSharpExpression::Type::bitwise_or; }
  | CARET { type = BinaryCSharpExpression::Type::bitwise_xor; }
  | SHIFT_LEFT { type = BinaryCSharpExpression::Type::shl; }
  | SHIFT_RIGHT_S { type = BinaryCSharpExpression::Type::shr_s; }
  | SHIFT_RIGHT_U { type = BinaryCSharpExpression::Type::shr_u; }
  ;

unary_expression_token returns [UnaryCSharpExpression::Type type] {
    type = static_cast<UnaryCSharpExpression::Type>(-1);
  }
  : ADD { type = UnaryCSharpExpression::Type::plus; }
  | SUB { type = UnaryCSharpExpression::Type::minus; }
  | TILDE { type = UnaryCSharpExpression::Type::bitwise_complement; }
  | BANG { type = UnaryCSharpExpression::Type::logical_complement; }
  ;

selector returns [CSharpExpressionSelector* js] {
    CSharpExpression* a = nullptr;
    CSharpExpressionSelector* ds = nullptr;

    js = nullptr;
  }
  : #(DOT_SELECTOR ds=dotSelector) {
    if (ds == nullptr) {
      reportError("Failed to parse dot expression");
      js = nullptr;
    } else {
      js = ds;
    }
  }
  | #(LBRACK a=expression RBRACK) {
    if (a == nullptr) {
      reportError("Failed to parse index expression");
      js = nullptr;
    } else {
      js = new CSharpExpressionIndexSelector(a);
    }
  }
  ;

dotSelector returns [CSharpExpressionSelector* js] {
    MethodArguments* r = nullptr;

    js = nullptr;
  }
  : identifier_node:Identifier {
    js = new CSharpExpressionMemberSelector(identifier_node->getText());
  }
  | #(METHOD_CALL method_node:Identifier r=arguments) {
    if (r == nullptr) {
      reportError("NULL method arguments in dotSelector METHOD_CALL");
      js = nullptr;
    } else {
      js = new MethodCallExpression(method_node->getText(), r);
    }
  }
  ;


arguments returns [MethodArguments* args] {
    MethodArguments* tail = nullptr;
    CSharpExpression* arg = nullptr;

    args = nullptr;
  }
  : #(EXPRESSION_LIST arg=expression tail=arguments) {
    if ((arg == nullptr) || (tail == nullptr)) {
      reportError("NULL method argument");
      delete arg;
      delete tail;
      args = nullptr;
    } else {
      args = new MethodArguments(arg, tail);
    }
  }
  | {
    args = new MethodArguments();
  }
  ;

type_name returns [std::string t] {
    std::string tail;
  }
  : #(TYPE_NAME n1:Identifier tail=type_name) {
    t = n1->getText();
    if (!tail.empty()) {
      t += '.';
      t += tail;
    }
  }
  | { }
  ;
