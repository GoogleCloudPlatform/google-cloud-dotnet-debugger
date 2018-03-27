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

#include "csharp_expression.h"

#include <iomanip>
#include "array_expression_evaluator.h"
#include "binary_expression_evaluator.h"
#include "conditional_operator_evaluator.h"
#include "expression_evaluator.h"
#include "field_evaluator.h"
#include "identifier_evaluator.h"
#include "literal_evaluator.h"
#include "method_call_evaluator.h"
#include "string_evaluator.h"
#include "type_cast_operator_evaluator.h"
#include "unary_expression_evaluator.h"
#include "dbg_primitive.h"
#include "dbg_null_object.h"

using std::string;

namespace google_cloud_debugger {

// Single character de-escaping for "UnescapeCSharpString". Returns the de-escaped
// character and iterator to the next byte in the input sequence.
static void UnescapeCharacter(
    string::const_iterator it,
    string::const_iterator end,
    char* unicode_character,
    string::const_iterator* next) {
  assert(it < end);

  if ((*it == '\\') && (it + 1 != end)) {
    switch (*(it + 1)) {
      case 't':
        *unicode_character = '\t';
        *next = it + 2;
        return;

      case 'b':
        *unicode_character = '\b';
        *next = it + 2;
        return;

      case 'n':
        *unicode_character = '\n';
        *next = it + 2;
        return;

      case 'r':
        *unicode_character = '\r';
        *next = it + 2;
        return;

      case 'f':
        *unicode_character = '\f';
        *next = it + 2;
        return;

      case '\'':
        *unicode_character = '\'';
        *next = it + 2;
        return;

      case '"':
        *unicode_character = '"';
        *next = it + 2;
        return;

      case '\\':
        *unicode_character = '\\';
        *next = it + 2;
        return;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7': {
        int num_length = 1;
        char num[4] = { *(it + 1) };
        if ((it + 2 < end) && (*(it + 2) >= '0') && (*(it + 2) <= '7')) {
          num[num_length++] = *(it + 2);
          if ((it + 3 < end) && (*(it + 3) >= '0') && (*(it + 3) <= '7')) {
            num[num_length++] = *(it + 3);
          }
        }

        num[num_length] = '\0';

        *unicode_character = strtol(num, nullptr, 8);  // NOLINT

        // Octal escape code can only represent ASCII characters.
        if (*unicode_character <= 0xFF) {
          *next = it + (1 + num_length);
          return;
        }

        break;
      }

      case 'u':
        if ((it + 6 <= end) &&
            isxdigit(*(it + 2)) &&
            isxdigit(*(it + 3)) &&
            isxdigit(*(it + 4)) &&
            isxdigit(*(it + 5))) {
          char num[] = {
            *(it + 2),
            *(it + 3),
            *(it + 4),
            *(it + 5),
            '\0'
          };

          *unicode_character = strtol(num, nullptr, 16);  // NOLINT
          *next = it + 6;
          return;
        }

        break;
    }
  }

  *unicode_character = *it;
  *next = it + 1;
}

// Converts escaped ASCII string to CSharp Unicode string. Escaping includes:
//    1. C-style escape codes: \r, \n, \\.
//    2. Octal escape codes: \3, \71, \152.
//    3. Uncode escape codes: \u883C.
static std::vector<char> UnescapeCSharpString(const string& escaped_string) {
  std::vector<char> unicode_string;
  unicode_string.reserve(escaped_string.size());  // Pessimistic estimation.

  auto it = escaped_string.begin();
  while (it != escaped_string.end()) {
    char unicode_character = 0;
    string::const_iterator next;
    UnescapeCharacter(it, escaped_string.end(), &unicode_character, &next);

    unicode_string.push_back(unicode_character);
    it = next;
  }

  return unicode_string;
}


// Prints single expression to a stream.
static void SafePrintChild(
    std::ostream* os,
    CSharpExpression* expression,
    bool concise) {
  assert(expression != nullptr);
  if (expression == nullptr) {
    (*os) << "<NULL>";
  } else {
    expression->Print(os, concise);
  }
}


// Escapes and prints a single CSharp Unicode character. This function is only
// used for debugging purposes.
static void PrintCharacter(std::ostream* os, char ch) {
  if ((ch >= ' ') &&
      (ch < 127) &&
      (ch != '\\') &&
      (ch != '"') &&
      (ch != '\'') &&
      (ch != '\n') &&
      (ch != '\r') &&
      (ch != '\t') &&
      (ch != '\b')) {
    (*os) << static_cast<char>(ch);
  } else {  // Print in Unicode encoding
    (*os) << "\\u"
          << std::setfill('0') << std::setw(4)
          << std::hex << std::noshowbase << ch;
  }
}


ConditionalCSharpExpression::ConditionalCSharpExpression(
    CSharpExpression* condition,
    CSharpExpression* if_true,
    CSharpExpression* if_false)
    : condition_(condition),
      if_true_(if_true),
      if_false_(if_false) {
}


void ConditionalCSharpExpression::Print(std::ostream* os, bool concise) {
  if (!concise) {
    (*os) << '(';
  }

  SafePrintChild(os, condition_.get(), concise);
  (*os) << " ? ";
  SafePrintChild(os, if_true_.get(), concise);
  (*os) << " : ";
  SafePrintChild(os, if_false_.get(), concise);

  if (!concise) {
    (*os) << ')';
  }
}


CompiledExpression ConditionalCSharpExpression::CreateEvaluator() {
  CompiledExpression comp_condition = condition_->CreateEvaluator();
  if (comp_condition.evaluator == nullptr) {
    return comp_condition;
  }

  CompiledExpression comp_if_true = if_true_->CreateEvaluator();
  if (comp_if_true.evaluator == nullptr) {
    return comp_if_true;
  }

  CompiledExpression comp_if_false = if_false_->CreateEvaluator();
  if (comp_if_false.evaluator == nullptr) {
    return comp_if_false;
  }

  return {
    std::unique_ptr<ExpressionEvaluator>(
        new ConditionalOperatorEvaluator(
            std::move(comp_condition.evaluator),
            std::move(comp_if_true.evaluator),
            std::move(comp_if_false.evaluator)))
  };
}


BinaryCSharpExpression::BinaryCSharpExpression(
    Type type,
    CSharpExpression* a,
    CSharpExpression* b)
    : type_(type),
      a_(a),
      b_(b) {
}


void BinaryCSharpExpression::Print(std::ostream* os, bool concise) {
  if (!concise) {
    (*os) << '(';
  }

  SafePrintChild(os, a_.get(), concise);
  (*os) << ' ';

  switch (type_) {
    case Type::add:
      (*os) << '+';
      break;

    case Type::sub:
      (*os) << '-';
      break;

    case Type::mul:
      (*os) << '*';
      break;

    case Type::div:
      (*os) << '/';
      break;

    case Type::mod:
      (*os) << '%';
      break;

    case Type::conditional_and:
      (*os) << "&&";
      break;

    case Type::conditional_or:
      (*os) << "||";
      break;

    case Type::eq:
      (*os) << "==";
      break;

    case Type::ne:
      (*os) << "!=";
      break;

    case Type::le:
      (*os) << "<=";
      break;

    case Type::ge:
      (*os) << ">=";
      break;

    case Type::lt:
      (*os) << '<';
      break;

    case Type::gt:
      (*os) << '>';
      break;

    case Type::bitwise_and:
      (*os) << '&';
      break;

    case Type::bitwise_or:
      (*os) << '|';
      break;

    case Type::bitwise_xor:
      (*os) << '^';
      break;

    case Type::shl:
      (*os) << "<<";
      break;

    case Type::shr_s:
      (*os) << ">>";
      break;

    case Type::shr_u:
      (*os) << ">>>";
      break;
  }

  (*os) << ' ';
  SafePrintChild(os, b_.get(), concise);

  if (!concise) {
    (*os) << ')';
  }
}


CompiledExpression BinaryCSharpExpression::CreateEvaluator() {
  CompiledExpression arg1 = a_->CreateEvaluator();
  if (arg1.evaluator == nullptr) {
    return arg1;
  }

  CompiledExpression arg2 = b_->CreateEvaluator();
  if (arg2.evaluator == nullptr) {
    return arg2;
  }

  return {
    std::unique_ptr<ExpressionEvaluator>(
        new BinaryExpressionEvaluator(
            type_,
            std::move(arg1.evaluator),
            std::move(arg2.evaluator)))
  };
}


UnaryCSharpExpression::UnaryCSharpExpression(Type type, CSharpExpression* a)
    : type_(type),
      a_(a) {
}


void UnaryCSharpExpression::Print(std::ostream* os, bool concise) {
  switch (type_) {
    case Type::plus:
      (*os) << '+';
      break;

    case Type::minus:
      (*os) << '-';
      break;

    case Type::bitwise_complement:
      (*os) << '~';
      break;

    case Type::logical_complement:
      (*os) << '!';
      break;
  }

  SafePrintChild(os, a_.get(), concise);
}


CompiledExpression UnaryCSharpExpression::CreateEvaluator() {
  CompiledExpression arg = a_->CreateEvaluator();
  if (arg.evaluator == nullptr) {
    return arg;
  }

  return {
    std::unique_ptr<ExpressionEvaluator>(
        new UnaryExpressionEvaluator(type_, std::move(arg.evaluator)))
  };
}


bool CSharpIntLiteral::ParseString(const string& str, int base) {
  const char* node_cstr = str.c_str();
  char* literal_end = nullptr;

  errno = 0;
  n_ = strtoll(node_cstr, &literal_end, base);  // NOLINT
  if (errno == ERANGE) {
    std::cerr << "Number " << str << " in base " << base
              << " could not be parsed";
    return false;
  }

  if (*literal_end == 'l' || *literal_end == 'L') {
    ++literal_end;
    is_long_ = true;
  }

  if (*literal_end != '\0') {
    std::cerr << "Unexpected trailing characters after number parser: "
              << str;
    return false;
  }

  if (!is_long_) {
    if (static_cast<std::int32_t>(n_) != n_) {
      std::cerr << "Number can't be represented as int32: " << str;
      return false;
    }
  }

  return true;
}


void CSharpIntLiteral::Print(std::ostream* os, bool concise) {
  if (!concise) {
    if (is_long_) {
      (*os) << "<long>";
    } else {
      (*os) << "<int>";
    }
  }

  (*os) << n_;

  if (is_long_) {
    (*os) << "L";
  }
}


CompiledExpression CSharpIntLiteral::CreateEvaluator() {
  if (is_long_) {
    std::shared_ptr<DbgObject> literal_obj(new DbgPrimitive<int64_t>(n_));
    return {
      std::unique_ptr<ExpressionEvaluator>(new LiteralEvaluator(literal_obj))
    };
  }
  else {
    std::shared_ptr<DbgObject> literal_obj(new DbgPrimitive<int32_t>(n_));
    return {
      std::unique_ptr<ExpressionEvaluator>(new LiteralEvaluator(literal_obj))
    };
  }
}

bool CSharpFloatLiteral::ParseString(const string& str) {
  const char* node_cstr = str.c_str();
  char* literal_end = nullptr;

  d_ = strtod(node_cstr, &literal_end);

  if (*literal_end == 'f' || *literal_end == 'F') {
    ++literal_end;
    is_double_ = false;
  } else if (*literal_end == 'd' || *literal_end == 'D') {
    ++literal_end;
  }

  if (*literal_end != '\0') {
    return false;
  }

  return true;
}


void CSharpFloatLiteral::Print(std::ostream* os, bool concise) {
  if (!concise) {
    if (!is_double_) {
      (*os) << "<float>";
    } else {
      (*os) << "<double>";
    }
  }

  (*os) << d_;

  if (!is_double_) {
    (*os) << "F";
  }
}


CompiledExpression CSharpFloatLiteral::CreateEvaluator() {
  if (is_double_) {
    std::shared_ptr<DbgObject> literal_obj(new DbgPrimitive<double_t>(d_));
    return {
      std::unique_ptr<ExpressionEvaluator>(new LiteralEvaluator(literal_obj))
    };
  } else {
    std::shared_ptr<DbgObject> literal_obj(new DbgPrimitive<float_t>(d_));
    return {
      std::unique_ptr<ExpressionEvaluator>(new LiteralEvaluator(literal_obj))
    };
  }
}


bool CSharpCharLiteral::ParseString(const string& str) {
  std::vector<char> CSharp_str = UnescapeCSharpString(str);
  if (CSharp_str.size() != 1) {
    return false;
  }

  ch_ = CSharp_str[0];

  return true;
}


void CSharpCharLiteral::Print(std::ostream* os, bool concise) {
  if (!concise) {
    (*os) << "<char>'";
  } else {
    (*os) << '\'';
  }

  PrintCharacter(os, ch_);

  if (!concise) {
    (*os) << "'";
  } else {
    (*os) << '\'';
  }
}


CompiledExpression CSharpCharLiteral::CreateEvaluator() {
  std::shared_ptr<DbgObject> literal_obj(new DbgPrimitive<char>(ch_));
  return {
    std::unique_ptr<ExpressionEvaluator>(new LiteralEvaluator(literal_obj))
  };
}


bool CSharpStringLiteral::ParseString(const string& str) {
  std::vector<char> unescaped_char_vec = UnescapeCSharpString(str);
  str_ = std::string(unescaped_char_vec.begin(), unescaped_char_vec.end());
  return true;
}


void CSharpStringLiteral::Print(std::ostream* os, bool concise) {
  (*os) << '"';

  for (char ch : str_) {
    PrintCharacter(os, ch);
  }

  (*os) << '"';
}


CompiledExpression CSharpStringLiteral::CreateEvaluator() {
  return {
    std::unique_ptr<ExpressionEvaluator>(new StringEvaluator(str_))
  };
}


void CSharpBooleanLiteral::Print(std::ostream* os, bool concise) {
  switch (n_) {
    case false:
      (*os) << "false";
      break;

    case true:
      (*os) << "true";
      break;

    default:
      (*os) << "bad_boolean";
      break;
  }
}


CompiledExpression CSharpBooleanLiteral::CreateEvaluator() {
  std::shared_ptr<DbgObject> literal_obj(new DbgPrimitive<bool>(n_));
  return {
    std::unique_ptr<ExpressionEvaluator>(new LiteralEvaluator(literal_obj))
  };
}


void CSharpNullLiteral::Print(std::ostream* os, bool concise) {
  (*os) << "null";
}


CompiledExpression CSharpNullLiteral::CreateEvaluator() {
  std::shared_ptr<DbgObject> null_obj(new DbgNullObject(nullptr));
  return {
    std::unique_ptr<ExpressionEvaluator>(new LiteralEvaluator(null_obj))
  };
}


void CSharpIdentifier::Print(std::ostream* os, bool concise) {
  if (concise) {
    (*os) << identifier_;
  } else {
    (*os) << '\'' << identifier_ << '\'';
  }
}


CompiledExpression CSharpIdentifier::CreateEvaluator() {
  return {
    std::unique_ptr<ExpressionEvaluator>(new IdentifierEvaluator(identifier_))
  };
}


bool CSharpIdentifier::TryGetTypeName(string* name) const {
  *name = identifier_;
  return true;
}


void TypeCastCSharpExpression::Print(std::ostream* os, bool concise) {
  if (concise) {
    (*os) << '(' << type_ << ") ";
  } else {
    (*os) << "cast<" << type_ << ">(";
  }

  SafePrintChild(os, source_.get(), concise);

  if (!concise) {
    (*os) << ')';
  }
}


CompiledExpression TypeCastCSharpExpression::CreateEvaluator() {
  CompiledExpression arg = source_->CreateEvaluator();
  if (arg.evaluator == nullptr) {
    return arg;
  }

  return {
    std::unique_ptr<ExpressionEvaluator>(
        new TypeCastOperatorEvaluator(std::move(arg.evaluator), type_))
  };
}


void CSharpExpressionIndexSelector::Print(std::ostream* os, bool concise) {
  SafePrintChild(os, source_.get(), concise);

  (*os) << '[';
  SafePrintChild(os, index_.get(), concise);
  (*os) << ']';
}


CompiledExpression CSharpExpressionIndexSelector::CreateEvaluator() {
  CompiledExpression source_evaluator = source_->CreateEvaluator();
  if (source_evaluator.evaluator == nullptr) {
    return source_evaluator;
  }

  CompiledExpression index_evaluator = index_->CreateEvaluator();
  if (index_evaluator.evaluator == nullptr) {
    return index_evaluator;
  }

  return {
    std::unique_ptr<IndexerAccessExpressionEvaluator>(
        new IndexerAccessExpressionEvaluator(
            std::move(source_evaluator.evaluator),
            std::move(index_evaluator.evaluator)))
  };
}


void CSharpExpressionMemberSelector::Print(std::ostream* os, bool concise) {
  SafePrintChild(os, source_.get(), concise);

  (*os) << '.' << member_;
}


CompiledExpression CSharpExpressionMemberSelector::CreateEvaluator() {
  CompiledExpression source_evaluator = source_->CreateEvaluator();
  if (source_evaluator.evaluator == nullptr) {
    return source_evaluator;
  }

  string possible_class_name;
  if (!source_->TryGetTypeName(&possible_class_name)) {
    possible_class_name.clear();
  }

  string identifier_name;
  if (!TryGetTypeName(&identifier_name)) {
    identifier_name = member_;
  }

  return {
    std::unique_ptr<ExpressionEvaluator>(
        new FieldEvaluator(
            std::move(source_evaluator.evaluator),
            std::move(identifier_name),
            std::move(possible_class_name),
            std::move(member_))),
  };
}


bool CSharpExpressionMemberSelector::TryGetTypeName(
    string* name) const {
  if (!source_->TryGetTypeName(name)) {
    return false;
  }

  (*name) += '.';
  (*name) += member_;

  return true;
}


void MethodCallExpression::Print(std::ostream* os, bool concise) {
  if (!concise) {
    (*os) << "<call>( ";
  }

  if (source_ != nullptr) {
    SafePrintChild(os, source_.get(), concise);
    (*os) << '.';
  }

  (*os) << method_;

  (*os) << '(';

  if (arguments_ != nullptr) {
    bool first_argument = true;
    for (auto& it : *arguments_) {
      if (!first_argument) {
        (*os) << ", ";
      }

      SafePrintChild(os, it.get(), concise);

      first_argument = false;
    }
  } else {
    (*os) << "NULL";
  }

  (*os) << ')';

  if (!concise) {
    (*os) << " )";
  }
}


CompiledExpression MethodCallExpression::CreateEvaluator() {
  CompiledExpression source_evaluator;
  string possible_class_name;

  if (source_ != nullptr) {
    source_evaluator = source_->CreateEvaluator();
    if (source_evaluator.evaluator == nullptr) {
      return source_evaluator;
    }

    if (!source_->TryGetTypeName(&possible_class_name)) {
      std::cerr << "Couldn't retrieve type name, method: " << method_;
      possible_class_name.clear();
    }
  }

  std::vector<std::unique_ptr<ExpressionEvaluator>> argument_evaluators;
  for (std::unique_ptr<CSharpExpression>& argument : *arguments_) {
    CompiledExpression argument_evaluator = argument->CreateEvaluator();
    if (argument_evaluator.evaluator == nullptr) {
      return argument_evaluator;
    }

    argument_evaluators.push_back(std::move(argument_evaluator.evaluator));
  }

  return {
    std::unique_ptr<ExpressionEvaluator>(
        new MethodCallEvaluator(
            method_,
            std::move(source_evaluator.evaluator),
            possible_class_name,
            std::move(argument_evaluators)))
  };
}


}  // namespace google_cloud_debugger
