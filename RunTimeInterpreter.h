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

class RunTimeInterpreter {
public:
	RunTimeInterpreter(CompileExecTerms & execTerms);
	virtual ~RunTimeInterpreter();
	int execOperation (Token * opr8r, TokenPtrVector & operands, Token & resultTkn);
  void dumpTokenPtrStream (TokenPtrVector tokenStream, std::wstring callersSrcFile, int lineNum);
  void dumpTokenList (std::vector<Token> & tokenList, std::wstring callersSrcFile, int lineNum);
  int consumeUnifiedStream (TokenPtrVector & unifiedStream, int startIdx);
  int resolveExpression(std::vector<Token> & exprTknStream);

protected:

private:
  Token * oneTkn;
  Token * zeroTkn;
  CompileExecTerms execTerms;
	std::wstring thisSrcFile;
	Utilities util;
	int failedOnLineNum;

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
};

#endif /* RUNTIMEINTERPRETER_H_ */
