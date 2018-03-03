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

#include "expression_util.h"

#include <sstream>
#include "antlrgen/CSharpExpressionCompiler.hpp"
#include "antlrgen/CSharpExpressionLexer.hpp"
#include "antlrgen/CSharpExpressionParser.hpp"
#include "csharp_expression.h"
#include "dbg_stack_frame.h"
#include "expression_evaluator.h"

using std::cerr;

namespace google_cloud_debugger {

CompiledExpression CompileExpression(const std::string& string_expression,
                                     DbgStackFrame* stack_frame) {
  if (string_expression.size() > kMaxExpressionLength) {
    std::cerr << "Expression can't be compiled because it is too long: "
              << string_expression.size();
    return {nullptr, string_expression};
  }

  // Parse the expression.
  std::istringstream input_stream(string_expression);
  CSharpExpressionLexer lexer(input_stream);
  CSharpExpressionParser parser(lexer);
  parser.Init();

  parser.statement();
  if (parser.ActiveException()) {
    parser.reportError(parser.ActiveException()->getMessage());
  }

  if (parser.num_errors() > 0) {
    std::cerr << "Expression parsing failed" << std::endl
              << "Input: " << string_expression << std::endl
              << "Parser error: " << parser.errors()[0];
    return {nullptr, string_expression};
  }

  // Transform ANTLR AST into "CSharpExpression" tree.
  CSharpExpressionCompiler compiler;
  compiler.Init();

  std::unique_ptr<CSharpExpression> expression = compiler.Walk(parser.getAST());
  if (expression == nullptr) {
    cerr << "Tree walking on parsed expression failed" << std::endl
         << "Input: " << string_expression << std::endl
         << "AST: " << parser.getAST()->toStringTree();

    return {nullptr, string_expression};
  }

  // Compile the expression.
  CompiledExpression compiled_expression = expression->CreateEvaluator();
  compiled_expression.expression = string_expression;
  if (compiled_expression.evaluator == nullptr) {
    cerr << "Expression not supported by the evaluator" << std::endl
         << "Input: " << string_expression << std::endl
         << "AST: " << parser.getAST()->toStringTree();

    return compiled_expression;
  }

  HRESULT hr = compiled_expression.evaluator->Compile(stack_frame, &cerr);
  if (FAILED(hr)) {
    cerr << "Expression could not be compiled" << std::endl
         << "Input: " << string_expression << std::endl
         << "AST: " << parser.getAST()->toStringTree();
    compiled_expression.evaluator = nullptr;

    return compiled_expression;
  }

  std::cout << "Expression compiled successfully" << std::endl
            << "Input: " << string_expression << std::endl
            << "AST: " << parser.getAST()->toStringTree();

  return compiled_expression;
}

}  // namespace google_cloud_debugger
