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
  void dumpTokenPtrStream (TokenPtrVector tokenStream, std::wstring callersSrcFile, int lineNum);
	// TODO: Should I make this static?
	int resolveFlatExpr(std::vector<Token> & flatExprTkns);
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

  int execFlatExpr_LRO(std::vector<Token> & exprTknStream);
  int execFlatExpr_OLR(std::vector<Token> & exprTknStream, int startIdx);
	int execOperation (Operator opr8r, int opr8rIdx, std::vector<Token> & flatExprTkns);
	int execVarDeclaration (uint32_t objStartPos, uint32_t objectLen);
	int execPrePostFixOp (std::vector<Token> & exprTknStream, int opr8rIdx);
	int execUnaryOp (std::vector<Token> & exprTknStream, int opr8rIdx);
	int execAssignmentOp(std::vector<Token> & exprTknStream, int opr8rIdx);
	int execBinaryOp (std::vector<Token> & exprTknStream, int opr8rIdx);
	int findNextTernary2ndIdx (std::vector<Token> & exprTknStream, int ternary1stOpIdx);
	int getEndOfSubExprIdx (std::vector<Token> & exprTknStream, int startIdx, int & lastIdxOfExpr);
	int execTernary1stOp (std::vector<Token> & exprTknStream, int opr8rIdx);
	int takeTernaryTrue (std::vector<Token> & exprTknStream, int opr8rIdx);
	int takeTernaryFalse (std::vector<Token> & exprTknStream, int opr8rIdx);

	int execEquivalenceOp (std::vector<Token> & exprTknStream, int opr8rIdx);
	int execShift (std::vector<Token> & exprTknStream, int opr8rIdx);
	int execBitWiseOp (std::vector<Token> & exprTknStream, int opr8rIdx);
	int execStandardMath (std::vector<Token> & exprTknStream, int opr8rIdx);
	int resolveIfVariable (Token & originalTkn, Token & resolvedTkn, std::wstring & varName);

};

#endif /* RUNTIMEINTERPRETER_H_ */
