/*
 * GeneratedCodeTerms.h
 * TODO: Not exactly sure if this class is actually relevantly named anymore
 * There is still good functionality in here though.  Must revisit!
 *  Created on: Jan 4, 2024
 *      Author: Mike Volk
 */

#ifndef RUNTIMEEXECUTOR_H_
#define RUNTIMEEXECUTOR_H_

#include "Token.h"
#include <map>
#include "CompileExecTerms.h"

class RunTimeExecutor: public CompileExecTerms {
public:
	RunTimeExecutor();
	virtual ~RunTimeExecutor();
	int execOperation (Token * opr8r, TokenPtrVector & operands, TokenPtrVector & resultStack);
  void dumpTokenStream (TokenPtrVector tokenStream);
  int consumeUnifiedStream (TokenPtrVector & unifiedStream, int startIdx);

protected:

private:
  std::vector<int> randStackLvlOfOpr8r;
  std::vector<int> numRands_Opr8r;
  Token * oneTkn;
  Token * zeroTkn;

  // TODO: public, protected or private?
  bool isShortCircuit (Token * opr8rTkn, int operandIdx, Token * operand);
  int consumeShortCircuit (TokenPtrVector & unifiedStream, int startIdx);
	int execColonOp ();
	int execTernaryTrue ();
	int execTernaryFalse ();
	int execGr8rThan(TokenCompareResult compareRez, TokenPtrVector & resultStack);
	int execGr8rThanEqual(TokenCompareResult compareRez, TokenPtrVector & resultStack);
	int execEquivalenceOp(TokenCompareResult compareRez, TokenPtrVector & resultStack);
	int execNotEqualsOp(TokenCompareResult compareRez, TokenPtrVector & resultStack);
	int execLessThan(TokenCompareResult compareRez, TokenPtrVector & resultStack);
	int execLessThanEqual(TokenCompareResult compareRez, TokenPtrVector & resultStack);
	int execLogicalAndOp(TokenCompareResult leftRez, TokenCompareResult rightRez, TokenPtrVector & resultStack);
	int execLogicalOrOp(TokenCompareResult leftRez, TokenCompareResult rightRez, TokenPtrVector & resultStack);
};

#endif /* RUNTIMEEXECUTOR_H_ */
