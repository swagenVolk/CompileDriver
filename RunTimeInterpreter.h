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
  void dumpTokenStream (TokenPtrVector tokenStream);
  int consumeUnifiedStream (TokenPtrVector & unifiedStream, int startIdx);
  int resolveExpression(std::vector<Token> & exprTknStream);

protected:

private:
  std::vector<int> randStackLvlOfOpr8r;
  // No longer necessary
  std::vector<int> numRands_Opr8r;
  Token * oneTkn;
  Token * zeroTkn;
  CompileExecTerms execTerms;
	std::wstring thisSrcFile;
	Utilities util;
	int failedOnLineNum;

  // TODO: public, protected or private?
  bool isShortCircuit (Token * opr8rTkn, int operandIdx, Token * operand);
  int consumeShortCircuit (TokenPtrVector & unifiedStream, int startIdx);

	int execBinaryOp (std::vector<Token> & exprTknStream, int opr8rIdx);
	int execTernary1stOp (std::vector<Token> & exprTknStream, int & callersIdx);
	int takeTernaryTrue (std::vector<Token> & exprTknStream, int & callersIdx);
	int takeTernaryFalse (std::vector<Token> & exprTknStream, int & callersIdx);

	int execGr8rThan (TokenCompareResult compareRez, Token & resultTkn);
	int execGr8rThanEqual (TokenCompareResult compareRez, Token & resultTkn);
	int execEquivalenceOp (TokenCompareResult compareRez, Token & resultTkn);
	int execNotEqualsOp (TokenCompareResult compareRez, Token & resultTkn);
	int execLessThan (TokenCompareResult compareRez, Token & resultTkn);
	int execLessThanEqual (TokenCompareResult compareRez, Token & resultTkn);
	int execLogicalAndOp (TokenCompareResult leftRez, TokenCompareResult rightRez, Token & resultTkn);
	int execLogicalOrOp (TokenCompareResult leftRez, TokenCompareResult rightRez, Token & resultTkn);
};

#endif /* RUNTIMEINTERPRETER_H_ */
