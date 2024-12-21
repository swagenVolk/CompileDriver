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
#include <cassert>
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
#include "VariablesScope.h"
#include "UserMessages.h"

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

#define END_COMMA_IS_EXPECTED				true
#define END_COMMA_NOT_EXPECTED			false

enum opr8r_ready_state_enum  {
  OPR8R_NOT_READY
  ,ATTACH_1ST
  ,ATTACH_2ND
  ,ATTACH_BOTH
};

typedef opr8r_ready_state_enum opr8rReadyState;

class ExpressionParser {
public:
	ExpressionParser(CompileExecTerms & inUsrSrcTerms, std::shared_ptr<VariablesScope> inVarScopeStack, std::wstring userSrcFileName
    , std::shared_ptr<UserMessages> userMessages);
	virtual ~ExpressionParser();
	int makeExprTree (TokenPtrVector & tknStream, std::shared_ptr<ExprTreeNode> & expressionTree, Token & enderTkn
			, bool isEndedByComma, bool & isExprClosed);

private:
  TokenPtrVector tknStream;
  std::wstring userSrcFileName;
  std::vector<std::shared_ptr<NestedScopeExpr>> exprScopeStack;
  std::wstring thisSrcFile;
  CompileExecTerms usrSrcTerms;
  Utilities util;
  std::shared_ptr<VariablesScope> varScopeStack;
  Token scratchTkn;
	std::shared_ptr<UserMessages> userMessages;

  void cleanScopeStack ();
  std::wstring makeExpectedTknTypesStr (uint32_t expected_tkn_types);
  bool isExpectedTknType (uint32_t allowed_tkn_types, uint32_t & next_legal_tkn_types, std::shared_ptr<Token> curr_tkn);
  int openSubExprScope (TokenPtrVector & tknStream);
  int makeTreeAndLinkParent (bool & isOpenParenFndYet);
  bool isTernaryOpen ();
  int get2ndTernaryCnt ();
  std::wstring getMyParentSymbol ();
  int closeNestedScopes(Token currTkn, Token expectedEndTkn, bool & isExprClosed, Token & enderTkn);
  int moveNeighborsIntoTree (Operator & opr8r, ExprTreeNodePtrVector & currScope
	, int opr8rIdx, opr8rReadyState opr8rState, bool isMoveLeftNbr, bool isMoveRightNbr);
  int turnClosedScopeIntoTree (ExprTreeNodePtrVector & currScope);
  int getExpectedEndToken (std::shared_ptr<Token> startTkn, uint32_t & _1stTknTypMsk, Token & expectedEndTkn, bool isEndedByComma);
  void printScopeStack (std::wstring fileName, int lineNumber);
  void showDebugInfo (std::wstring srcFileName, int lineNum);

};

#endif /* EXPRESSIONPARSER_H_ */
