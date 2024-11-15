/*
 * GeneralParser.h
 *
 *  Created on: Nov 13, 2024
 *      Author: Mike Volk
 */

#ifndef GENERALPARSER_H_
#define GENERALPARSER_H_

#include <string>
#include <stdint.h>
#include <assert.h>
#include <stdexcept>
#include <stack>
#include "common.h"
#include "CompileExecTerms.h"
#include "Token.h"
#include "NestedScopeExpr.h"
#include "ExprTreeNode.h"
#include "Operator.h"
#include "Utilities.h"
#include "InterpretedFileWriter.h"
#include "ScopeFrame.h"
#include "ExpressionParser.h"
#include "RunTimeInterpreter.h"
#include "ErrorInfo.h"

enum var_declaration_states_enum {
  GET_VAR_NAME
	,CHECK_FOR_EXPRESSION
	,PARSE_EXPRESSION
	,TIDY_AFTER_EXPRESSION
};

typedef var_declaration_states_enum varDeclarationState;


class GeneralParser {
public:
	GeneralParser(TokenPtrVector & inTknStream, CompileExecTerms & inUsrSrcTerms, std::string object_file_name);
	virtual ~GeneralParser();
	int findKeyWordObjects();

private:
  TokenPtrVector tknStream;
  std::vector<NestedScopeExpr *> exprScopeStack;
  std::wstring thisSrcFile;
  ErrorInfo errorInfo;
  CompileExecTerms usrSrcTerms;
  Utilities util;
  std::vector<ScopeFrame> scopeStack;
  std::ofstream interpretedFile;
  std::shared_ptr<InterpretedFileWriter> interpretedFileWriter;
  std::shared_ptr<RunTimeInterpreter> interpreter;
  std::unique_ptr<ExpressionParser> exprParser;

  int parseVarDeclaration (uint8_t dataTypeOpCode);


};

#endif /* GENERALPARSER_H_ */
