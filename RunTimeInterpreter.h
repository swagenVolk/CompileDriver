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
#include <memory>
#include "CompileExecTerms.h"
#include "Utilities.h"
#include "VariablesScope.h"
#include "UserMessages.h"

enum interpreter_modes_enum {
	COMPILE_TIME_CHECKING
	,RUN_TIME_EXECUTION

};

typedef interpreter_modes_enum InterpreterModesType;

class RunTimeInterpreter {
public:
	RunTimeInterpreter();
	RunTimeInterpreter(CompileExecTerms & execTerms, std::shared_ptr<VariablesScope> inVarScope
		, std::wstring userSrcFileName, std::shared_ptr<UserMessages> userMessages);
	RunTimeInterpreter(std::string interpretedFileName, std::wstring userSrcFileName
		, std::shared_ptr<VariablesScope> inVarScope,  std::shared_ptr<UserMessages> userMessages);

	virtual ~RunTimeInterpreter();
	int execOperation (std::shared_ptr <Token> opr8r, TokenPtrVector & operands, Token & resultTkn);
  void dumpTokenPtrStream (TokenPtrVector tokenStream, std::wstring callersSrcFile, int lineNum);
	// TODO: Should I make this static?
  int resolveFlattenedExpr(std::vector<Token> & exprTknStream);
	int rootScopeExec();

protected:

private:
  std::shared_ptr <Token> oneTkn;
  std::shared_ptr <Token> zeroTkn;
  CompileExecTerms execTerms;
	std::wstring thisSrcFile;
	Utilities util;
	Token scratchTkn;
	std::shared_ptr<VariablesScope> varScopeStack;
	std::shared_ptr<UserMessages> userMessages;
	std::wstring userSrcFileName;
	int usageMode;
	InterpretedFileReader fileReader;

	int execVarDeclaration (uint32_t objStartPos, uint32_t objectLen);
	int execPrefixGatherPostfix (std::vector<Token> & flatExprTkns, int & numPrefixOpsFnd, std::vector<Token> & postFixOps);
	int execGatheredPostfix (int & numDone, std::vector<Token> & postFixOps);
	int execPrePostFixNoOp (std::vector<Token> & exprTknStream, int & callersIdx);
	int execPrePostFixOp (Token & opCodeTkn);
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
