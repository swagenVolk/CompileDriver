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
#include <cassert>
#include <stdexcept>
#include <stack>
#include <memory>
#include "common.h"
#include "CompileExecTerms.h"
#include "Token.h"
#include "NestedScopeExpr.h"
#include "ExprTreeNode.h"
#include "Operator.h"
#include "Utilities.h"
#include "InterpretedFileWriter.h"
#include "ScopeWindow.h"
#include "ExpressionParser.h"
#include "InfoWarnError.h"
#include "RunTimeInterpreter.h"
#include "StackOfScopes.h"
#include "UserMessages.h"

#define RESUME_COMPILATION  L"Continuing compilation after "

#define FOR_INIT_IDX          0
#define FOR_CONDITIONAL_IDX   1
#define FOR_ITER_IDX          2

enum var_declaration_states_enum {
  GET_VAR_NAME
  ,CHECK_FOR_INIT_EXPR
  ,PARSE_INIT_EXPR
  ,TIDY_AFTER_EXPR
};

typedef var_declaration_states_enum varDeclarationState;

class GeneralParser {
public:
  GeneralParser(TokenPtrVector & inTknStream, std::wstring userSrcFileName, CompileExecTerms & inUsrSrcTerms
      , std::shared_ptr<UserMessages> userMessages, std::string object_file_name, std::shared_ptr<StackOfScopes> inVarNameSpace
      , logLvlEnum logLvl);
  virtual ~GeneralParser();
  int compileRootScope();
  int compileCurrScope ();

protected:

private:
  TokenPtrVector tknStream;
  std::wstring userSrcFileName;
  std::wstring thisSrcFile;
  CompileExecTerms usrSrcTerms;
  Utilities util;
  std::ofstream interpretedFile;
  InterpretedFileWriter interpretedFileWriter;
  RunTimeInterpreter interpreter;
  ExpressionParser exprParser;
  std::shared_ptr<StackOfScopes> scopedNameSpace;
  Token scratchTkn;
  std::shared_ptr<UserMessages> userMessages;
  int userErrorLimit;
  std::vector<std::wstring> ender_list;
  std::vector<std::wstring> ender_comma_list;
  logLvlEnum logLevel;
  int failOnSrcLine;


  int parseVarDeclaration (std::wstring dataTypeStr, std::pair<TokenTypeEnum, uint8_t> tknType_opCode, bool & isDeclarationEnded
    , int & numVarsAdded, int & numInitExpressions);
  int resolveVarInitExpr (Token & varTkn, Token currTkn, Token & exprCloser, bool & isDeclarationEnded);
  bool isProgressBlocked ();
  int chompUntil_infoMsgAfter (std::vector<std::wstring> searchStrings, Token & closerTkn);
  int compile_if_type_block (uint8_t op_code, Token & openingTkn, bool & isClosedByCurly);
  int handleExpression (bool & isStopFail, bool & is_expr_static);
  int compile_break (Token break_tkn);
  int openFloatyScope (Token openScopeTkn);
  int compile_for_loop_control (Token & openingTkn);
  int compile_for_loop_ctrl_expr (int exprIdx, bool & is_expr_full, bool & is_expr_static);
  int validate_closed_for_loop();
  int compile_while_loop_control (Token & openingTkn);
};

#endif /* GENERALPARSER_H_ */
