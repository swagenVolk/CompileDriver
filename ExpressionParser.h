/*
 * ExpressionParser.h
 *
 *  Created on: Jun 11, 2024
 *      Author: Mike Volk
 */

#ifndef EXPRESSIONPARSER_H_
#define EXPRESSIONPARSER_H_

#include <memory>
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
#include "StackOfScopes.h"
#include "UserMessages.h"
#include "RunTimeInterpreter.h"

// Values below used in a bit mask variable that indicates
// allowable next states.
#define VAR_NAME_NXT_OK             0x1
#define LITERAL_NXT_OK              0x2
#define PREFIX_OPR8R_NXT_OK         0x4
#define UNARY_OPR8R_NXT_OK          0x8
#define POSTFIX_OPR8R_NXT_OK        0x10  
#define BINARY_OPR8R_NXT_OK         0x20
#define TERNARY_OPR8R_1ST_NXT_OK    0x40  
#define TERNARY_OPR8R_2ND_NXT_OK    0x80  
#define OPEN_PAREN_NXT_OK           0x100
#define CLOSE_PAREN_NXT_OK          0x200
#define DCLR_VAR_OR_FXN_NXT_OK      0x400
#define SYSTEM_CALL_NXT_OK          0x800
#define USER_FXN_CALL_NXT_OK        0x1000
#define STATEMENT_ENDER_NXT_OK      0x2000

#define END_COMMA_IS_EXPECTED       true
#define END_COMMA_NOT_EXPECTED      false

#define DISPLAY_GAP_SPACES          3

enum opr8r_ready_state_enum  {
  OPR8R_NOT_READY
  ,ATTACH_1ST
  ,ATTACH_2ND
  ,ATTACH_BOTH
};

typedef opr8r_ready_state_enum opr8rReadyState;

enum expr_ender_enum  {
  ENDS_IN_PARENTHESES
  ,ENDS_IN_COMMA
  ,ENDS_IN_STATEMENT_ENDER
};

typedef expr_ender_enum expr_ender_type;

class ExpressionParser {
public:
  ExpressionParser(CompileExecTerms & inUsrSrcTerms, std::shared_ptr<StackOfScopes> inVarScopeStack, std::wstring userSrcFileName
    , std::shared_ptr<UserMessages> userMessages, logLvlEnum logLvl);
  virtual ~ExpressionParser();
  int makeExprTree (TokenPtrVector & tknStream, std::shared_ptr<ExprTreeNode> & expressionTree, Token & enderTkn
      , expr_ender_type ended_by, bool & isCallerExprClosed, bool isInVarDec, bool & is_expr_static);

  int displayParseTree (std::shared_ptr<ExprTreeNode> startBranch, int adjustToRight);
  int displayParseTree (std::shared_ptr<ExprTreeNode> startBranch, std::wstring callersSrcFile, int srcLineNum);
  int check_for_expected_token (TokenPtrVector & tknStream, Token & curr_tkn, std::wstring pattern_str, bool is_consume_tkn);
  int compile_system_call (TokenPtrVector & tknStream, std::shared_ptr<ExprTreeNode> sys_call_node);

private:
  std::wstring userSrcFileName;
  std::wstring thisSrcFile;
  CompileExecTerms usrSrcTerms;
  Utilities util;
  std::shared_ptr<StackOfScopes> scopedNameSpace;
  Token scratchTkn;
  std::shared_ptr<UserMessages> userMessages;
  logLvlEnum logLevel;
  bool isExprVarDeclaration;
  int failed_on_src_line;
  int num_var_leaf_nodes;

  std::vector<int> leftTreeMaxCol;
  std::vector<int> rightTreeMaxCol;

  int makeExprTree (TokenPtrVector & tknStream, std::shared_ptr<ExprTreeNode> & expressionTree, Token & enderTkn
    , expr_ender_type ended_by, bool & isCallerExprClosed, bool isInVarDec, bool & is_expr_static, bool is_nested_call);
  void cleanScopeStack (std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack);
  std::wstring makeExpectedTknTypesStr (uint32_t expected_tkn_types);
  bool isExpectedTknType (uint32_t allowed_tkn_types, uint32_t & next_legal_tkn_types, std::shared_ptr<Token> curr_tkn
    , std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack);
  int openSubExprScope (TokenPtrVector & tknStream, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack);
  int makeTreeAndLinkParent (bool & isOpenParenFndYet, bool isExprClosed, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack);
  bool isTernaryOpen (std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack);
  int get2ndTernaryCnt (std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack);
  int closeNestedScopes(bool isExprClosed, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack);
  int moveNeighborsIntoTree (ExprTreeNodePtrVector & currScope, int opr8rIdx, opr8rReadyState opr8rState, bool isMoveLeftNbr
    , bool isMoveRightNbr, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack);
  int exec_delayed_ternary_2nd (ExprTreeNodePtrVector & currScope, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack);
  bool is_delay_tern2nd (ExprTreeNodePtrVector & currScope);
  int exec_prec_lvl_opr8rs (ExprTreeNodePtrVector & currScope,  Opr8rPrecedenceLvl & precedenceLvl, bool & is_skip_tern2nd, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack);
    
  int turnClosedScopeIntoTree (ExprTreeNodePtrVector & currScope, bool isExprClosed, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack);
  int turnClosedScopeIntoTree (ExprTreeNodePtrVector & currScope, bool isOpenedByTernary, bool isExprClosed, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack);
  int getExpectedEndToken (std::shared_ptr<Token> startTkn, uint32_t & _1stTknTypMsk, Token & expectedEndTkn, expr_ender_type ended_by);
  
  // Debug helper procs
  void printSingleScope (std::wstring headerMsg, int scopeLvl, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack);
  void printSingleScope (std::wstring headerMsg, int scopeLvl, int tgtIdx, int & tgtStartPos, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack);
  void printScopeStack (std::wstring fileName, int lineNumber, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack);
  void printScopeStack (std::wstring bannerMsg, bool isUseDefault, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack);
  void showDebugInfo (std::wstring srcFileName, int lineNum, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack);

  // Parse tree display procedures below
  int setIsCenterNode (bool isLeftTree, int halfTreeLevel, std::shared_ptr<ExprTreeNode> currBranch);
    std::wstring makeTreeNodeStr (std::shared_ptr<ExprTreeNode> treeNode);
  int setDownstreamCenters (bool isLeftTree, std::shared_ptr<ExprTreeNode> topBranch);
  int setCtrStartByPrevBndry (bool isLeftTree, std::shared_ptr<ExprTreeNode> currBranch);  
  int findMaxOuterNodeEndPos (bool isLeftTree, std::shared_ptr<ExprTreeNode> searchBranch, int & maxEndPos);
  int setOuterLeafNodeDisplayPos (bool isLeftTree, int halfTreeLevel, std::shared_ptr<ExprTreeNode> currBranch);
  int setBranchNodeDisplayPos (bool isLeftTree, int halfTreeLevel, std::shared_ptr<ExprTreeNode> currBranch);
  int setHalfTreeDisplayPos (bool isLeftTree, int halfTreeLevel, std::shared_ptr<ExprTreeNode> currBranch
    , std::vector<std::vector<std::shared_ptr<ExprTreeNode>>> & halfDisplayLines);
    
  int setFullTreeDisplayPos (std::shared_ptr<ExprTreeNode> startBranch, std::vector<std::wstring> & displayLines, int & maxLeftLineLen);

  void getMaxLineLen (std::vector<std::vector<std::shared_ptr<ExprTreeNode>>> & arrayOfNodeLists, bool isLefty, int & maxLineLen);
  int fillDisplayLeft (std::vector<std::wstring> & displayLines, std::vector<std::vector<std::shared_ptr<ExprTreeNode>>> & arrayOfNodeLists
    , int maxLineLen);
  int fillDisplayRight (std::vector<std::wstring> & displayLines, std::vector<std::vector<std::shared_ptr<ExprTreeNode>>> & arrayOfNodeLists
    , int centerGapSpaces);

};
#endif /* EXPRESSIONPARSER_H_ */
