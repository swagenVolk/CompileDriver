/*
 * RunTimeInterpreter.h
 *
 *  Created on: Jan 4, 2024
 *      Author: Mike Volk
 */

#ifndef RUNTIMEINTERPRETER_H_
#define RUNTIMEINTERPRETER_H_

#include "InterpretedFileReader.h"
#include "Token.h"
#include <cstdint>
#include <memory>
#include "CompileExecTerms.h"
#include "Utilities.h"
#include "StackOfScopes.h"
#include "UserMessages.h"

class RunTimeInterpreter {
public:
	RunTimeInterpreter();
	RunTimeInterpreter(CompileExecTerms & execTerms, std::shared_ptr<StackOfScopes> inVarNameSpace
		, std::wstring userSrcFileName, std::shared_ptr<UserMessages> userMessages, logLvlEnum logLvl);
	RunTimeInterpreter(std::string interpretedFileName, std::wstring userSrcFileName
		, std::shared_ptr<StackOfScopes> inVarNameSpace,  std::shared_ptr<UserMessages> userMessages
		, logLvlEnum logLvl);

	virtual ~RunTimeInterpreter();
  void dumpTokenPtrStream (TokenPtrVector tokenStream, std::wstring callersSrcFile, int lineNum);
	// TODO: Should I make this static?
	int resolveFlatExpr(std::vector<Token> & flatExprTkns);
	int execRootScope();

protected:

private:
  std::shared_ptr <Token> oneTkn;
  std::shared_ptr <Token> zeroTkn;
  CompileExecTerms execTerms;
	std::wstring thisSrcFile;
	Utilities util;
	Token scratchTkn;
	std::shared_ptr<StackOfScopes> scopedNameSpace;
	std::shared_ptr<UserMessages> userMessages;
	std::wstring userSrcFileName;
	InterpreterModesType usageMode;
	InterpretedFileReader fileReader;
	int failOnSrcLine;
	logLvlEnum logLevel;
	bool isIllustrative;
	std::wstring tknsIllustrativeStr;

	int execCurrScope (uint32_t execStartPos, uint32_t afterBoundaryPos, uint32_t & break_scope_end_pos);
  int execFlatExpr_OLR (std::vector<Token> & exprTknStream, int startIdx);
	int execOperation (Operator opr8r, int opr8rIdx, std::vector<Token> & flatExprTkns);
	int execExpression (uint32_t objStartPos, Token & resultTkn);
	int execVarDeclaration (uint32_t objStartPos, uint32_t objectLen);
	int execPrePostFixOp (std::vector<Token> & exprTknStream, int opr8rIdx);
	int execUnaryOp (std::vector<Token> & exprTknStream, int opr8rIdx);
	int execAssignmentOp(std::vector<Token> & exprTknStream, int opr8rIdx);
	int execBinaryOp (std::vector<Token> & exprTknStream, int opr8rIdx);
	int getEndOfSubExprIdx (std::vector<Token> & exprTknStream, int startIdx, int & lastIdxOfExpr);
	int execTernary1stOp (std::vector<Token> & exprTknStream, int opr8rIdx);
	int exec_logical_and (std::vector<Token> & exprTknStream, int opr8rIdx);
	int exec_logical_or (std::vector<Token> & exprTknStream, int opr8rIdx);
	int execEquivalenceOp (std::vector<Token> & exprTknStream, int opr8rIdx);
	int execShift (std::vector<Token> & exprTknStream, int opr8rIdx);
	int execBitWiseOp (std::vector<Token> & exprTknStream, int opr8rIdx);
	int execStandardMath (std::vector<Token> & exprTknStream, int opr8rIdx);
	int resolveTknOrVar (Token & originalTkn, Token & resolvedTkn, std::wstring & varName, bool isCheckInit);
	int resolveTknOrVar (Token & originalTkn, Token & resolvedTkn, std::wstring & varName);
	int exec_if_block (uint32_t scopeStartPos, uint32_t if_scope_len, uint32_t afterIfParentScopePos, uint32_t & break_scope_end_pos);
  int exec_cached_expr (std::vector<Token> expr_tkn_list, bool & is_result_true);
  int get_expr_from_var_declaration (uint32_t start_pos, std::vector<Token> & expr_tkn_list);
  int exec_for_loop (uint32_t scopeStartPos, uint32_t for_scope_len, uint32_t afterParentScopePos, uint32_t & break_scope_end_pos);
  int exec_while_loop (uint32_t scopeStartPos, uint32_t for_scope_len, uint32_t afterParentScopePos, uint32_t & break_scope_end_pos);
  
  bool isOkToIllustrate ();
  void illustrativeB4op (std::vector<Token> & flatExprTkns, int currIdx);	
	void illustrativeAfterOp (std::vector<Token> & flatExprTkns);

};

#endif /* RUNTIMEINTERPRETER_H_ */
