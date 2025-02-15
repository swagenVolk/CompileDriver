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

	int execCurrScope (uint32_t execStartPos, uint32_t afterBoundaryPos);
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
	int execLogicalAndOp (std::vector<Token> & exprTknStream, int opr8rIdx);
	int execLogicalOrOp (std::vector<Token> & exprTknStream, int opr8rIdx);
	int execEquivalenceOp (std::vector<Token> & exprTknStream, int opr8rIdx);
	int execShift (std::vector<Token> & exprTknStream, int opr8rIdx);
	int execBitWiseOp (std::vector<Token> & exprTknStream, int opr8rIdx);
	int execStandardMath (std::vector<Token> & exprTknStream, int opr8rIdx);
	int resolveTknOrVar (Token & originalTkn, Token & resolvedTkn, std::wstring & varName, bool isCheckInit);
	int resolveTknOrVar (Token & originalTkn, Token & resolvedTkn, std::wstring & varName);
	int execIfBlock (uint32_t scopeStartPos, uint32_t if_scope_len, uint32_t afterIfParentScopePos);
	
};

#endif /* RUNTIMEINTERPRETER_H_ */
