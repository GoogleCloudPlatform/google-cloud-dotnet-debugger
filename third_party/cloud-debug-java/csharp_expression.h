/**
 * Copyright 2015 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DEVTOOLS_CDBG_DEBUGLETS_CSharp_CSharp_EXPRESSION_H_
#define DEVTOOLS_CDBG_DEBUGLETS_CSharp_CSharp_EXPRESSION_H_

#include <list>
#include <memory>
#include "expression_util.h"

using std::string;

namespace google_cloud_debugger {

class ExpressionEvaluator;

// Interface representing a node in parsed expression tree.
class CSharpExpression {
 public:
  virtual ~CSharpExpression() { }

  // Prints the expression subtree to the stream. When "concise" is true, the
  // function prints expression in CSharp format. When "concise" is false, a
  // much more verbose format is used (this mode is used by unit test to
  // disambiguate different types of expressions that might look the same in
  // concise format).
  virtual void Print(std::ostream* os, bool concise) = 0;

  // Tries to convert the expression subtree into a type name. For
  // example: Member("String", Member("lang", Identifier("CSharp")) can be
  // converted to "CSharp.lang.String". At the same time (a+b) cannot.
  virtual bool TryGetTypeName(string* name) const = 0;

  // Compiles the expression into executable format. The caller owns the
  // returned instance. If a particular language feature is not yet supported,
  // the function returns null and prints description in "error_message".
  virtual CompiledExpression CreateEvaluator() = 0;
};


// Represents (a ? b : c) conditional expression.
class ConditionalCSharpExpression : public CSharpExpression {
 public:
  // Takes ownership over the passed instances of "CSharpExpression";
  ConditionalCSharpExpression(
      CSharpExpression* condition,
      CSharpExpression* if_true,
      CSharpExpression* if_false);

  void Print(std::ostream* os, bool concise) override;

  bool TryGetTypeName(string* name) const override { return false; }

  CompiledExpression CreateEvaluator() override;

 private:
  std::unique_ptr<CSharpExpression> condition_;
  std::unique_ptr<CSharpExpression> if_true_;
  std::unique_ptr<CSharpExpression> if_false_;

  DISALLOW_COPY_AND_ASSIGN(ConditionalCSharpExpression);
};


// Represents any kind of binary expression (like a + b).
class BinaryCSharpExpression : public CSharpExpression {
 public:
  enum class Type {
    add,
    sub,
    mul,
    div,
    mod,
    conditional_and,
    conditional_or,
    eq,
    ne,
    le,
    ge,
    lt,
    gt,
    bitwise_and,
    bitwise_or,
    bitwise_xor,
    shl,
    shr_s,
    shr_u
  };

  // Takes ownership over "a" and "b" expression subtrees.
  BinaryCSharpExpression(Type type, CSharpExpression* a, CSharpExpression* b);

  void Print(std::ostream* os, bool concise) override;

  bool TryGetTypeName(string* name) const override { return false; }

  CompiledExpression CreateEvaluator() override;

 private:
  const Type type_;
  std::unique_ptr<CSharpExpression> a_;
  std::unique_ptr<CSharpExpression> b_;

  DISALLOW_COPY_AND_ASSIGN(BinaryCSharpExpression);
};


// Represents unary expression (like ~a).
class UnaryCSharpExpression : public CSharpExpression {
 public:
  enum class Type {
    plus,
    minus,
    bitwise_complement,
    logical_complement
  };

  // Takes ownership over "a" expression subtree.
  UnaryCSharpExpression(Type type, CSharpExpression* a);

  void Print(std::ostream* os, bool concise) override;

  bool TryGetTypeName(string* name) const override { return false; }

  CompiledExpression CreateEvaluator() override;

 private:
  const Type type_;
  std::unique_ptr<CSharpExpression> a_;

  DISALLOW_COPY_AND_ASSIGN(UnaryCSharpExpression);
};


class CSharpIntLiteral : public CSharpExpression {
 public:
  CSharpIntLiteral() : is_long_(false) { }

  // Parses an integer in the given base from the given string.
  bool ParseString(const string& str, int base);

  void Print(std::ostream* os, bool concise) override;

  bool TryGetTypeName(string* name) const override { return false; }

  CompiledExpression CreateEvaluator() override;

 private:
  // Returns true if this is a long integer.
  bool IsLong() const { return is_long_; }

 private:
  // Indicates whether this is an int or a long.
  bool is_long_;

  std::int64_t n_;

  DISALLOW_COPY_AND_ASSIGN(CSharpIntLiteral);
};


class CSharpFloatLiteral : public CSharpExpression {
 public:
  CSharpFloatLiteral() : is_double_(true) { }

  // Parses a floating point number from the given string.
  bool ParseString(const string& str);

  void Print(std::ostream* os, bool concise) override;

  bool TryGetTypeName(string* name) const override { return false; }

  CompiledExpression CreateEvaluator() override;

  // Returns true if this is a double.
  bool IsDouble() const { return is_double_; }

 private:
  // Indicates whether this is an float or a double.
  bool is_double_;

  std::double_t d_;

  DISALLOW_COPY_AND_ASSIGN(CSharpFloatLiteral);
};

// Represents character constant. All characters in CSharp are Unicode, so
// this is a 16 bit integer.
class CSharpCharLiteral : public CSharpExpression {
 public:
  CSharpCharLiteral() : ch_('\0') { }

  // Decodes the potentially escaped character into a Unicode character.
  // Examples for encoding are: '\n', '\\', '\293', '\u5C7f'.
  bool ParseString(const string& str);

  void Print(std::ostream* os, bool concise) override;

  bool TryGetTypeName(string* name) const override { return false; }

  CompiledExpression CreateEvaluator() override;

 private:
  char ch_;

  DISALLOW_COPY_AND_ASSIGN(CSharpCharLiteral);
};


// Represents a CSharp string constant.
class CSharpStringLiteral : public CSharpExpression {
 public:
  CSharpStringLiteral() { }

  // Decodes the potentially escaped characters sequence into CSharp string.
  // Example of encoded string: "This\n is\t an\" \u1E3B encoded \88 string".
  bool ParseString(const string& str);

  void Print(std::ostream* os, bool concise) override;

  bool TryGetTypeName(string* name) const override { return false; }

  CompiledExpression CreateEvaluator() override;

 private:
  std::string str_;

  DISALLOW_COPY_AND_ASSIGN(CSharpStringLiteral);
};


// Represents a boolean constant.
class CSharpBooleanLiteral : public CSharpExpression {
 public:
  explicit CSharpBooleanLiteral(bool n) : n_(n) { }

  void Print(std::ostream* os, bool concise) override;

  bool TryGetTypeName(string* name) const override { return false; }

  CompiledExpression CreateEvaluator() override;

 private:
  const bool n_;

  DISALLOW_COPY_AND_ASSIGN(CSharpBooleanLiteral);
};


// Represents CSharp "null".
class CSharpNullLiteral : public CSharpExpression {
 public:
  CSharpNullLiteral() { }

  void Print(std::ostream* os, bool concise) override;

  bool TryGetTypeName(string* name) const override { return false; }

  CompiledExpression CreateEvaluator() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(CSharpNullLiteral);
};


// Represents a local or a static variable.
class CSharpIdentifier : public CSharpExpression {
 public:
  explicit CSharpIdentifier(string identifier)
      : identifier_(identifier) {
  }

  void Print(std::ostream* os, bool concise) override;

  bool TryGetTypeName(string* name) const override;

  CompiledExpression CreateEvaluator() override;

 private:
  const string identifier_;

  DISALLOW_COPY_AND_ASSIGN(CSharpIdentifier);
};


// Represents a type cast for classes or interfaces.
class TypeCastCSharpExpression : public CSharpExpression {
 public:
  explicit TypeCastCSharpExpression(string type, CSharpExpression* source)
      : type_(type),
        source_(std::unique_ptr<CSharpExpression>(source)) {
  }

  void Print(std::ostream* os, bool concise) override;

  bool TryGetTypeName(string* name) const override { return false; }

  CompiledExpression CreateEvaluator() override;

 private:
  const string type_;
  std::unique_ptr<CSharpExpression> source_;

  DISALLOW_COPY_AND_ASSIGN(TypeCastCSharpExpression);
};


// Base class representing additional transformation applied to an object. Two
// types of selectors are currently supported:
//    1. Array indexer: a[8].
//    2. Dereferencing a member: a.member.
class CSharpExpressionSelector : public CSharpExpression {
 public:
  // Setter for "source_". See "source_" comments for explanation. Takes
  // ownership over the "source" expression subtree.
  void set_source(CSharpExpression* source) {
    source_.reset(source);
  }

 protected:
  // Represents the base expression on which the selector is applied. In the
  // two examples above, "source_" will be "a". A more complicated example:
  //    (flag ? a1 : a2)[8]
  // In this case "source_" will be the expression corresponding to
  // "flag ? a1 : a2".
  std::unique_ptr<CSharpExpression> source_;
};


// Selector for array item. The index is also expression to support
// constructions like a[x + y].
class CSharpExpressionIndexSelector : public CSharpExpressionSelector {
 public:
  // Takes ownership over the "index" expression subtree.
  explicit CSharpExpressionIndexSelector(CSharpExpression* index) : index_(index) {
  }

  void Print(std::ostream* os, bool concise) override;

  bool TryGetTypeName(string* name) const override { return false; }

  CompiledExpression CreateEvaluator() override;

 private:
  std::unique_ptr<CSharpExpression> index_;

  DISALLOW_COPY_AND_ASSIGN(CSharpExpressionIndexSelector);
};


// Selector for a class member.
class CSharpExpressionMemberSelector : public CSharpExpressionSelector {
 public:
  explicit CSharpExpressionMemberSelector(const string& member)
      : member_(member) {
  }

  void Print(std::ostream* os, bool concise) override;

  bool TryGetTypeName(string* name) const override;

  CompiledExpression CreateEvaluator() override;

 private:
  const string member_;

  DISALLOW_COPY_AND_ASSIGN(CSharpExpressionMemberSelector);
};


// List of arguments for method invocation.
class MethodArguments {
 public:
  // Empty arguments list.
  MethodArguments() {}

  // Single argument. Takes ownership over "argument".
  MethodArguments(CSharpExpression* argument, MethodArguments* tail) {
    if (tail) {
      arguments_.swap(tail->arguments_);
      delete tail;
    }

    arguments_.push_front(std::unique_ptr<CSharpExpression>(argument));
  }

  std::list<std::unique_ptr<CSharpExpression>>::iterator begin() {
    return arguments_.begin();
  }

  std::list<std::unique_ptr<CSharpExpression>>::iterator end() {
    return arguments_.end();
  }

 private:
  std::list<std::unique_ptr<CSharpExpression>> arguments_;

  DISALLOW_COPY_AND_ASSIGN(MethodArguments);
};


// Represents method call (with arguments). The method call can be either
// direct like f(1) or through selectors like my.util.f(1) or a.getY().f(1).
// In case of direct method call, "source_" will be null.
class MethodCallExpression : public CSharpExpressionSelector {
 public:
  // Takes ownership over "method" and "arguments".
  MethodCallExpression(const string& method, MethodArguments* arguments)
      : method_(method),
        arguments_(std::unique_ptr<MethodArguments>(arguments)) {
  }

  void Print(std::ostream* os, bool concise) override;

  bool TryGetTypeName(string* name) const override { return false; }

  CompiledExpression CreateEvaluator() override;

 private:
  const string method_;
  std::unique_ptr<MethodArguments> arguments_;

  DISALLOW_COPY_AND_ASSIGN(MethodCallExpression);
};


}  // namespace google_cloud_debugger

#endif  // DEVTOOLS_CDBG_DEBUGLETS_CSharp_CSharp_EXPRESSION_H_


