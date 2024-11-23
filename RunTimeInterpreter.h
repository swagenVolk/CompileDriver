/*
 * RunTimeExecutor.h
 *
 *  Created on: Jan 4, 2024
 *      Author: Mike Volk
 */

#ifndef RUNTIMEINTERPRETER_H_
#define RUNTIMEINTERPRETER_H_

#include "Token.h"
#include <map>
#include "CompileExecTerms.h"
#include "Utilities.h"
#include "VariablesScope.h"
#include "UserMessages.h"

class RunTimeInterpreter {
public:
	RunTimeInterpreter();
	RunTimeInterpreter(CompileExecTerms & execTerms, std::shared_ptr<VariablesScope>, std::wstring userSrcFileName, UserMessages & userMessages);
	virtual ~RunTimeInterpreter();
	int execOperation (std::shared_ptr <Token> opr8r, TokenPtrVector & operands, Token & resultTkn);
  void dumpTokenPtrStream (TokenPtrVector tokenStream, std::wstring callersSrcFile, int lineNum);
  int resolveFlattenedExpr(std::vector<Token> & exprTknStream, UserMessages & userMessages);

protected:

private:
  std::shared_ptr <Token> oneTkn;
  std::shared_ptr <Token> zeroTkn;
  CompileExecTerms execTerms;
	std::wstring thisSrcFile;
	Utilities util;
	Token scratchTkn;
	std::shared_ptr<VariablesScope> varScopeStack;
	UserMessages userMessages;
	std::wstring userSrcFileName;

	int execUnaryOp (std::vector<Token> & exprTknStream, int & callersIdx);
	int execAssignmentOp(std::vector<Token> & exprTknStream, int & callersIdx);
	int execBinaryOp (std::vector<Token> & exprTknStream, int & callersIdx);
	int findNextTernary2ndIdx (std::vector<Token> & exprTknStream, int ternary1stOpIdx);
	int getEndOfSubExprIdx (std::vector<Token> & exprTknStream, int startIdx, int & lastIdxOfExpr);
	int execTernary1stOp (std::vector<Token> & exprTknStream, int & callersIdx);
	int takeTernaryTrue (std::vector<Token> & exprTknStream, int & callersIdx);
	int takeTernaryFalse (std::vector<Token> & exprTknStream, int & callersIdx);

	int execEquivalenceOp (std::vector<Token> & exprTknStream, int & callersIdx);
	int execShift (std::vector<Token> & exprTknStream, int & callersIdx);
	int execBitWiseOp (std::vector<Token> & exprTknStream, int & callersIdx);
	int execStandardMath (std::vector<Token> & exprTknStream, int & callersIdx);
	int resolveIfVariable (Token & originalTkn, Token & resolvedTkn, std::wstring & varName);
};

#endif /* RUNTIMEINTERPRETER_H_ */
