/*
 * ExpressionParser.h
 *
 *  Created on: Jun 11, 2024
 *      Author: Mike Volk
 */

#ifndef EXPRESSIONPARSER_H_
#define EXPRESSIONPARSER_H_

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

// Values below used in a bit mask variable that indicates
// allowable next states.
#define	VAR_NAME_NXT_OK							0x1
#define LITERAL_NXT_OK							0x2
#define PREFIX_OPR8R_NXT_OK					0x4
#define UNARY_OPR8R_NXT_OK					0x8
#define POSTFIX_OPR8R_NXT_OK				0x10	// TODO: Should this be a peek-ahead situation?
#define BINARY_OPR8R_NXT_OK					0x20
#define TERNARY_OPR8R_1ST_NXT_OK		0x40	// TODO: Separate out ? and : opr8rs? Ternary stack?
#define TERNARY_OPR8R_2ND_NXT_OK		0x80	// TODO: Separate out ? and : opr8rs? Ternary stack?
#define	OPEN_PAREN_NXT_OK						0x100
#define CLOSE_PAREN_NXT_OK					0x200
#define DCLR_VAR_OR_FXN_NXT_OK			0x400
#define FXN_CALL_NXT_OK							0x800
#define STATEMENT_ENDER_NXT_OK			0x1000

class ExpressionParser {
public:
	ExpressionParser(TokenPtrVector & inTknStream, CompileExecTerms & inUsrSrcTerms);
	virtual ~ExpressionParser();
	int parseExpression (InterpretedFileWriter & intrprtrWriter);

private:
  TokenPtrVector tknStream;
  std::vector<NestedScopeExpr *> exprScopeStack;
  std::wstring errorMsg;
  std::wstring thisSrcFile;
  int errOnOurSrcLineNum;
  CompileExecTerms usrSrcTerms;
  Utilities util;
  void cleanScopeStack();
  std::wstring makeExpectedTknTypesStr (uint32_t expected_tkn_types);
  bool isExpectedTknType (uint32_t allowed_tkn_types, uint32_t & next_legal_tkn_types, Token * curr_tkn);
  int openSubExprScope ();
  int closeParenClosesScope (bool & isOpenParenFndYet);
  bool isTernaryOpen();
  int get2ndTernaryCnt ();
  int turnClosedScopeIntoTree (ExprTreeNodePtrVector & currScope);
  int getExpectedEndToken (Token * startTkn, uint32_t & _1stTknTypMsk, Token & expectedEndTkn);
  void printScopeStack(std::wstring fileName, int lineNumber);

};

#endif /* EXPRESSIONPARSER_H_ */
