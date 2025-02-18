/*
 * RunTimeInterpreter.cpp
 *
 *  Created on: Jan 4, 2024
 *      Author: Mike Volk
 *
 * A parse tree was created from the expression in the user's source file, but we need
 * to flatten this tree before writing out to the interpreted file since the file is 
 * essentially 2D while a tree is 3D.  The flattened xpression has been written out in 
 * recursive [OPR8R][LEFT][RIGHT] order, with higher precedence operations located deeper 
 * in the tree.  The OPR8R precedes its operands to enable short-circuiting of the 
 * [&&], [||] and [?] OPR8Rs.  Some example source expressions and their corresponding 
 * flattened Token lists that get written out to the interpreted file are shown below. 
 *
 * seven = three + four;				<-- user's C source expression
 * [=][seven][B+][three][four]	<-- Flattened expression for Interpreter
 * ^ Exec [=] OPR8R when we've got the required 2 operands. Can't do that right away because of the 
 * [B+] (binary +) OPR8R after the [seven] operand, but the [B+] OPR8R can be executed right away 
 * because its requirement for 2 operands is met immediately.
 *
 * [=][seven][B+][three][four]
 *           ^ binary +
 * Gets reduced to 
 * [=][seven][7]
 *           
 * one = 1;
 * [=][one][1]
 *
 * seven * seven + init1++; 
 * init1 == 1 at time of this expression
 * [B+][*][seven][seven][1+][init1]
 *                      ^ postfix increment
 * [B+][49]             [1] <-- postfix increment takes place AFTER grabbing the value of [init1]
 * [50]
 * 
 * seven * seven + ++init2;
 * init2 == 2 at time of this expression
 * [B+][*][seven][seven][+1][init2]
 *                      ^ prefix increment
 * [B+][49]             [3] <-- prefix increment takes place BEFORE grabbing the value of [init2]
 * [52]
 * 
 * one >= two ? 1 : three <= two ? 2 : three == four ? 3 : six > seven ? 4 : six > (two << two) ? 5 : 12345;
 * [?][>=][one][two][1][?][<=][three][two][2][?][==][three][four][3][?][>][six][seven][4][?][>][six][<<][two][two][5][12345]
 * 
 * count == 1 ? "one" : count == 2 ? "two" : "MANY";
 * [?][==][count][1]["one"][?][==][count][2]["two"]["MANY"]
 * Example: count == 1
 * [?][==][count][1]["one"][?][==][count][2]["two"]["MANY"]
 * [?][1]           ["one"][?][==][count][2]["two"]["MANY"]
 *    ^ Conditional resolves TRUE; so take TRUE path & discard FALSE path
 * [?][1]           ["one"][?][==][count][2]["two"]["MANY"]
 *                  ^ TRUE ^ FALSE path that gets discarded.
 * ["one"] <-- expression resolved
 * 
 * Example: count == 2
 * [?][==][count][1]["one"][?][==][count][2]["two"]["MANY"]
 * [?][0]           ["one"][?][==][count][2]["two"]["MANY"]
 *    ^ Conditional resolves FALSE; discard TRUE path
 * [?][==][count][2]["two"]["MANY"]
 * [?][1]           ["two"]["MANY"]
 *    ^ Conditional resolves TRUE; take TRUE path & discard FALSE path
 * ["two"] <-- expression resolved
 *
 * Example: count == 3
 * [?][==][count][1]["one"][?][==][count][2]["two"]["MANY"]
 * [?][0]           ["one"][?][==][count][2]["two"]["MANY"]
 *    ^ Conditional resolves FALSE; discard TRUE path
 * [?][==][count][2]["two"]["MANY"]
 * [?][0]           ["two"]["MANY"]
 *    ^ Conditional resolves FALSE; discard TRUE path
 * ["MANY"] <-- expression resolved
 *
 *
 * 3 * 4 / 3 + 4 << 4;
 * [<<][B+][/][*][3][4][3][4][4]
 *            ^ 1st OPR8R followed by required # of operands
 * [<<][B+][/][12]     [3][4][4]
 * [<<][B+][4]            [4][4]
 * [<<][8]                   [4]
 * [128]
 * 
 * 3 * 4 / 3 + 4 << 4 + 1;
 * [<<][B+][/][*][3][4][3][4][B+][4][1]
 *            ^              ^ These 2 OPR8Rs have their operand requirements met
 * [<<][B+][/][12]     [3][4][5]
 * [<<][B+][4]            [4][5]
 * [<<][8]                   [5]
 * [256]
 * 
 * (one * two >= three || two * three > six || three * four < seven || four / two < one) && (three % two > 1 || (shortCircuitAnd987 = 654));
 * [&&][||][||][||][>=][*][one][two][three][>][*][two][three][six][<][*][three][four][seven][<][/][four][two][one][||][>][%][three][two][1][=][shortCircuitAnd987][654]
 *
 * TODO:
 * Error handling on mismatched data types
 * 
 * DONE:
 * Complete assignment operations
 * Will need to be able to recognize complete, yet contained sub-expressions for the untaken TERNARY
 * branch, expressions as fxn|system calls, and potentially for short-circuiting (early break) on the [||] or [&&] OPR8Rs
 * Align comments below with reality
 *
 * Where will fxn calls go: on the operand or opr8r stack? If fxn calls go on the opr8r stack, then
 * the number of arguments the fxn call expects has to be looked up @ run time, OR the # of expected
 * operands could be put on the operand stack.
 *  */

#include "RunTimeInterpreter.h"
#include "BaseLanguageTerms.h"
#include "OpCodes.h"
#include "Operator.h"
#include "Token.h"
#include "TokenCompareResult.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "InfoWarnError.h"
#include "UserMessages.h"
#include "StackOfScopes.h"
#include "common.h"

/* ****************************************************************************
 *
 * ***************************************************************************/
RunTimeInterpreter::RunTimeInterpreter() {
	// TODO Auto-generated constructor stub
  oneTkn = std::make_shared<Token> (UINT64_TKN, L"1");
  // TODO: Token value not automatically filled in currently
  oneTkn->_unsigned = 1;
	oneTkn->isInitialized = true;
  zeroTkn = std::make_shared<Token> (UINT64_TKN, L"0");
  zeroTkn->_unsigned = 0;
	zeroTkn->isInitialized = true;
	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");
	failOnSrcLine = 0;
	logLevel = SILENT;
	isIllustrative = false;
	// The no parameter constructor should never get called
	assert (0);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
RunTimeInterpreter::RunTimeInterpreter(CompileExecTerms & execTerms, std::shared_ptr<StackOfScopes> inVarScopeStack
		, std::wstring userSrcFileName, std::shared_ptr<UserMessages> userMessages, logLvlEnum logLvl) {
	// TODO Auto-generated constructor stub
  oneTkn = std::make_shared<Token> (UINT64_TKN, L"1");
  // TODO: Token value not automatically filled in currently
  oneTkn->_unsigned = 1;
	oneTkn->isInitialized = true;
  zeroTkn = std::make_shared<Token> (UINT64_TKN, L"0");
  zeroTkn->_unsigned = 0;
	zeroTkn->isInitialized = true;

  this->execTerms = execTerms;
	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");
	scopedNameSpace = inVarScopeStack;
	this->userMessages = userMessages;
	this->userSrcFileName = userSrcFileName;
	failOnSrcLine = 0;
	logLevel = logLvl;
	isIllustrative = false;
	usageMode = COMPILE_TIME;
}

/* ****************************************************************************
 *
 * ***************************************************************************/
RunTimeInterpreter::RunTimeInterpreter(std::string interpretedFileName, std::wstring userSrcFileName
	, std::shared_ptr<StackOfScopes> inVarScope,  std::shared_ptr<UserMessages> userMessages, logLvlEnum logLvl)
		: execTerms ()
		, fileReader (interpretedFileName, execTerms)	{
	// TODO Auto-generated constructor stub
  oneTkn = std::make_shared<Token> (UINT64_TKN, L"1");
  // TODO: Token value not automatically filled in currently
  oneTkn->_unsigned = 1;
	oneTkn->isInitialized = true;
  zeroTkn = std::make_shared<Token> (UINT64_TKN, L"0");
  zeroTkn->_unsigned = 0;
	zeroTkn->isInitialized = true;

	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");
	scopedNameSpace = inVarScope;
	this->userMessages = userMessages;
	this->userSrcFileName = userSrcFileName;
	failOnSrcLine = 0;
	logLevel = logLvl;
	isIllustrative = false;
	usageMode = INTERPRETER;

}

/* ****************************************************************************
 *
 * ***************************************************************************/
RunTimeInterpreter::~RunTimeInterpreter() {
	// TODO Auto-generated destructor stub
	oneTkn.reset();
	zeroTkn.reset();

	if (failOnSrcLine > 0 && !userMessages->isExistsInternalError(thisSrcFile, failOnSrcLine))	{
		// Dump out a debugging hint
		std::wcout << L"FAILURE on " << thisSrcFile << L":" << failOnSrcLine << std::endl;
	}
}

/* ****************************************************************************
 * Analogous to GeneralParser::rootScopeCompile.  
 * This proc gives us an opportunity to implementation specific objects that
 * only appear at the global scope.  Otherwise, call execCurrScope.
 * ***************************************************************************/
int RunTimeInterpreter::execRootScope()	{
	int ret_code = GENERAL_FAILURE;
	uint8_t	root_scope_op_code;
	uint32_t root_scope_len;

	if (usageMode == INTERPRETER)	{
		fileReader.setFilePos(0);
		if (OK != fileReader.readNextByte(root_scope_op_code) || root_scope_op_code != ANON_SCOPE_OPCODE)
			userMessages->logMsg(INTERNAL_ERROR, L"Failure reading ROOT scope op_code", thisSrcFile, failOnSrcLine, 0);
		
		else if (OK != fileReader.readNextDword(root_scope_len))
			userMessages->logMsg(INTERNAL_ERROR, L"Failure reading ROOT scope length", thisSrcFile, failOnSrcLine, 0);

		else {
			ret_code = execCurrScope (0, root_scope_len);
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * Analogous to GeneralParser::compileCurrScope.  
 * Parse through the user's interpreted file, looking for objects such as:
 * Variable declarations
 * Expressions
 * [if] 
 * [else if]
 * [else] 
 * [while] 
 * [for] 
 * [fxn declaration]
 * [fxn call]
 * ***************************************************************************/
int RunTimeInterpreter::execCurrScope (uint32_t execStartPos, uint32_t afterScopeBndry)	{
	int ret_code = GENERAL_FAILURE;
	uint8_t	op_code;
	uint32_t objStartPos;
	uint32_t objectLen;
	bool isDone = false;
	std::wstringstream hexStream;
	std::wstringstream objStartPosStr;

	if (usageMode == INTERPRETER)	{
		while (!isDone && !failOnSrcLine)	{
			objStartPos = fileReader.getReadFilePos();

			objStartPosStr.str(L"");
			objStartPosStr << L"0x" << std::hex << objStartPos;

			if (fileReader.isEOF())
				isDone = true;

			else if (/* execStartPos > 0 && */objStartPos == afterScopeBndry)
				// scopeEndPos > 0 means non-global scope and whole scope processed
				isDone = true;

			else if (OK != fileReader.peekNextByte(op_code))	{
				// TODO: isEOF doesn't seem to work. What's the issue?
				isDone = true;
				if (execStartPos > 0)	{
					failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
					userMessages->logMsg(INTERNAL_ERROR, L"Failed while peeking next op_code in non-global scope"
						, thisSrcFile, failOnSrcLine, 0);
				}

			} else if (OK != fileReader.readNextByte (op_code))	{
				failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
			
			} else if (op_code >= FIRST_VALID_FLEX_LEN_OPCODE && op_code <= LAST_VALID_FLEX_LEN_OPCODE)		{
				if (OK != fileReader.readNextDword (objectLen))	{
					failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
					hexStream.str(L"");
					hexStream << L"0x" << std::hex << op_code;
					std::wstring msg = L"Failed to get length of object (opcode = ";
					msg.append(hexStream.str());
					msg.append(L") starting at ");
					msg.append(objStartPosStr.str());
					userMessages->logMsg(INTERNAL_ERROR, msg, thisSrcFile, failOnSrcLine, 0);

				} else {
					if (op_code == VARIABLES_DECLARATION_OPCODE)	{
						if (OK != execVarDeclaration (objStartPos, objectLen))	{
							failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
						}

					} else if (op_code == EXPRESSION_OPCODE)	{		
						Token resultTkn;	
						isIllustrative = (usageMode == INTERPRETER ? true : false);
						
						if (OK != execExpression (objStartPos, resultTkn))	{
							failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
						}
						isIllustrative = false;
					
					} else if (op_code == IF_SCOPE_OPCODE)	{	
						if (OK != execIfBlock (objStartPos, objectLen, afterScopeBndry))	{
							failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
						}

					} else if (op_code == ELSE_IF_SCOPE_OPCODE)	{						
							failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
							userMessages->logMsg(INTERNAL_ERROR, L"Floating [else if] block encountered at " + objStartPosStr.str(), thisSrcFile, failOnSrcLine, 0);

					} else if (op_code == ELSE_SCOPE_OPCODE)	{								
							failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
							userMessages->logMsg(INTERNAL_ERROR, L"Floating [else] block encountered at " + objStartPosStr.str(), thisSrcFile, failOnSrcLine, 0);

					} else if (op_code == WHILE_SCOPE_OPCODE)	{								
							failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
							userMessages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", thisSrcFile, failOnSrcLine, 0);

					} else if (op_code == FOR_SCOPE_OPCODE)	{									
							failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
							userMessages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", thisSrcFile, failOnSrcLine, 0);

					} else if (op_code == ANON_SCOPE_OPCODE)	{								
							failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
							userMessages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", thisSrcFile, failOnSrcLine, 0);

					} else if (op_code == FXN_DECLARATION_OPCODE)	{					
							failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
							userMessages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", thisSrcFile, failOnSrcLine, 0);

					} else if (op_code == FXN_CALL_OPCODE)	{									
							failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
							userMessages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", thisSrcFile, failOnSrcLine, 0);

					} else if (op_code == SYSTEM_CALL_OPCODE)	{							
							failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
							userMessages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", thisSrcFile, failOnSrcLine, 0);

					} else {
						failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
						hexStream.str(L"");
						hexStream << L"0x" << std::hex << op_code;
						std::wstring msg = L"Unknown opcode [";
						msg.append(hexStream.str());
						msg.append(L"] found at ");
						msg.append(objStartPosStr.str());
						userMessages->logMsg(INTERNAL_ERROR, msg, thisSrcFile, failOnSrcLine, 0);
					}
				}
			}
		}
	}

	if (isDone && !failOnSrcLine)
		ret_code = OK;

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execExpression (uint32_t objStartPos, Token & resultTkn)	{
	int ret_code = GENERAL_FAILURE;
	std::wstringstream objStartPosStr;
	objStartPosStr.str(L"");
	objStartPosStr << L"0x" << std::hex << objStartPos;

	if (OK != fileReader.setFilePos(objStartPos))	{
		// Follow on fxn expects to start at beginning of expression
		// TODO: Standardize on this?
		failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
		userMessages->logMsg(INTERNAL_ERROR
			, L"Failed to reset interpreter file position to " + objStartPosStr.str(), thisSrcFile, failOnSrcLine, 0);

	} else {
		std::vector<Token> exprTkns;
		if (OK != fileReader.readExprIntoList(exprTkns))	{
			failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
			userMessages->logMsg(INTERNAL_ERROR
				, L"Failed to retrieve expression starting at " + objStartPosStr.str(), thisSrcFile, failOnSrcLine, 0);
		
		}	else if (OK != resolveFlatExpr(exprTkns)) {
				failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
				userMessages->logMsg(INTERNAL_ERROR
					, L"Failed to resolve flat expression starting at " + objStartPosStr.str(), thisSrcFile, failOnSrcLine, 0);

		} else if (exprTkns.size() != 1)	{
			// TODO: Should not have returned OK!
			// flattenedExpr should have 1 Token left - the result of the expression
			failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
			std::wstring devMsg = L"Failed to resolve at file position " + objStartPosStr.str();
			userMessages->logMsg (INTERNAL_ERROR, devMsg, thisSrcFile, failOnSrcLine, 0);

		} else {
			resultTkn = exprTkns[0];
			ret_code = OK;
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * VARIABLES_DECLARATION_OPCODE		0x6F	
 * [op_code][total_length][datatype op_code][[string var_name][init_expression]]+
 * Examples of user source that would result in a run time call to this fxn:
 *  
 * int8 count = 3;
 * uint8 one, two, three, four, five, six, seven;
 * uint8 init1 = 1, init2 = 2;
 * uint16 twenty = four * five, twentySix = twenty + six, fortyTwo = six * seven, fiftySix = seven * seven + seven;
 * uint16 fifty = seven * seven + init1++;
 * uint16 fiftyTwo = seven * seven + ++init2;
 * int8 result = one >= two ? 1 : three <= two ? 2 : three == four ? 3 : six > seven ? 4 : six > (two << two) ? 5 : 12345; 
 * string countDesc = count == 1 ? "one" : count == 2 ? "two" : count == 3 ? "three" : count == 4 ? "four" : "MANY";
 * bool isShouldBeTrue = one <= two ? true : false;
 * bool isShouldBeFalse = fiftySix <= fiftyTwo ? true : false;
 * string MikeWasHere = "Mike was HERE!!!!";
 * ***************************************************************************/
int RunTimeInterpreter::execVarDeclaration (uint32_t objStartPos, uint32_t objectLen)	{
	int ret_code = GENERAL_FAILURE;
	uint8_t	op_code;
	bool isDone = false;
	std::wstringstream hexStrOpCode;
	std::wstringstream hexStrFilePos;
	std::wstring devMsg;
	uint32_t currObjStartPos;

	if (OK == fileReader.readNextByte(op_code))	{
		// Got the DATA_TYPE_[]_OPCODE
		TokenTypeEnum tknType = execTerms.getTokenTypeForOpCode (op_code);
		if (tknType == USER_WORD_TKN || !Token::isDirectOperand (tknType))	{
			failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
			hexStrOpCode.str(L"");
			hexStrOpCode << L"0x" << std::hex << op_code;
			std::wstring devMsg = L"Expected op_code that would resolve to a datatype but got ";
			devMsg.append(hexStrOpCode.str());
			devMsg.append (L" at file position ");
			hexStrFilePos.str(L"");
			hexStrFilePos << L"0x" << std::hex << fileReader.getReadFilePos();
			devMsg.append(hexStrFilePos.str());
			userMessages->logMsg(INTERNAL_ERROR, devMsg, thisSrcFile, failOnSrcLine, 0);

		} else {
			uint32_t pastLimitFilePos = objStartPos + objectLen;
			Token varNameTkn;
			std::vector<Token> tokenList;
			std::wstring lookUpMsg;

			while (!isDone && !failOnSrcLine)	{
				varNameTkn.resetToString(L"");
				Token varTkn;
				varTkn.resetTokenExceptSrc();
				varTkn.tkn_type = tknType;

				if (fileReader.isEOF())	{
					isDone = true;
				
				} else if (pastLimitFilePos <= (currObjStartPos = fileReader.getReadFilePos()))	{
					isDone = true;
				
				} else {
						hexStrFilePos.str(L"");
						hexStrFilePos << L"0x" << std::hex << currObjStartPos;

					if (OK != fileReader.readNextByte(op_code) || VAR_NAME_OPCODE != op_code)	{
						failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
						devMsg = L"Did not get expected VAR_NAME_OPCODE at file position " + hexStrFilePos.str();
						userMessages->logMsg(INTERNAL_ERROR, devMsg, thisSrcFile, failOnSrcLine, 0);
			
					} else if (OK != fileReader.readString (op_code, varNameTkn))	{
						failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
						devMsg = L"Failed reading variable name in declaration after file position " + hexStrFilePos.str();
						userMessages->logMsg(INTERNAL_ERROR, devMsg, thisSrcFile, failOnSrcLine, 0);

					} else if (!execTerms.isViableVarName(varNameTkn._string))	{
						failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
						devMsg = L"Variable name in declaration is invalid [" + varNameTkn._string + L"] after file position " + hexStrFilePos.str();
						userMessages->logMsg(INTERNAL_ERROR, devMsg, thisSrcFile, failOnSrcLine, 0);
					
					} else if (OK != scopedNameSpace->insertNewVarAtCurrScope(varNameTkn._string, varTkn))	{
							failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
							devMsg = L"Failed to insert variable into NameSpace [" + varNameTkn._string + L"] after file position " + hexStrFilePos.str();
							userMessages->logMsg(INTERNAL_ERROR, devMsg, thisSrcFile, failOnSrcLine, 0);
					
					} else if (pastLimitFilePos <= (fileReader.getReadFilePos()))	{
						// NOTE: Need to check where we are; variable declaration might not have initialization expression(s)
						isDone = true;

					} else if (OK != fileReader.peekNextByte(op_code))	{
							if (fileReader.isEOF())	{
								isDone = true;
							
							} else {
								failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
								userMessages->logMsg(INTERNAL_ERROR, L"Peek failed", thisSrcFile, failOnSrcLine, 0);
							}
					} else if (op_code == EXPRESSION_OPCODE)	{
						// If there's an initialization expression for this variable in the declaration, then resolve it
						// e.g. uint32 numFruits = 3 + 4, numVeggies = (3 * (1 + 2)), numPizzas = (4 + (2 * 3));
						//                         ^                    ^                         ^
						// Otherwise, go back to top of loop and look for the next variable name
						uint32_t exprStartPos = fileReader.getReadFilePos();
						hexStrFilePos.str(L"");
						hexStrFilePos << L"0x" << std::hex << exprStartPos;
						Token resolvedTkn;

						if (OK != execExpression(exprStartPos, resolvedTkn))	{
								failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;

						} else if (OK != scopedNameSpace->findVar(varNameTkn._string, 0, resolvedTkn, COMMIT_WRITE, lookUpMsg))	{
							// Don't limit search to current scope
							failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
							userMessages->logMsg (INTERNAL_ERROR
									, L"After resolving initialization expression starting on|near " + hexStrFilePos.str() + L": " + lookUpMsg
									, thisSrcFile, failOnSrcLine, 0);
						}
					}
				}
			}
		}
	} else {
			uint32_t currFilePos = fileReader.getReadFilePos();
	}

	if (isDone && !failOnSrcLine)
		ret_code = OK;

	return (ret_code);
}

/* ****************************************************************************
 * For the PREFIX OPR8Rs [++()] and [--()], the increment|decrement will happen
 * BEFORE the value is placed on our "stack", so the change will be visible in 
 * the current expression
 * 
 * For the POSTFIX OPR8Rs [()++] and [()--], the increment|decrement will happen
 * AFTER the value is placed on our "stack", so the change will NOT be visible in 
 * the current expression
 * ***************************************************************************/
int RunTimeInterpreter::execPrePostFixOp (std::vector<Token> & exprTknStream, int opr8rIdx)	{
	int ret_code = GENERAL_FAILURE;
	bool isSuccess = false;
	std::wstring lookUpMsg;

	if (opr8rIdx >= 0 && exprTknStream.size() > (opr8rIdx + 1) && exprTknStream[opr8rIdx].tkn_type == EXEC_OPR8R_TKN)	{
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t op_code = exprTknStream[opr8rIdx]._unsigned;
		Operator opr8r;
		execTerms.getExecOpr8rDetails(op_code, opr8r);

		// Our operand Token could be a USER_WORD variable name, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		std::wstring varName1;
		resolveTknOrVar (exprTknStream[opr8rIdx+1], operand1, varName1);

		if (varName1.empty())	{
			std::wstring userMsg = L"Failed to execute OPR8R ";
			userMsg.append (opr8r.symbol);
			userMsg.append (L"; ");
			userMsg.append (operand1.descr_line_num_col());
			userMsg.append (L" is an r-value");
			userMessages->logMsg (USER_ERROR, userMsg, thisSrcFile, __LINE__, 0);

		} else if (!operand1.isSigned() &&  !operand1.isUnsigned())	{

		} else if (op_code == PRE_INCR_OPR8R_OPCODE || op_code == PRE_DECR_OPR8R_OPCODE)	{
			int addValue = (op_code == PRE_INCR_OPR8R_OPCODE ? 1 : -1);
			operand1.isUnsigned() ? operand1._unsigned += addValue : operand1._signed += addValue;

			// Return altered value to our "stack" for use in the expression
			exprTknStream[opr8rIdx] = operand1;
			isSuccess = true;

		} else if (op_code == POST_INCR_OPR8R_OPCODE || op_code == POST_DECR_OPR8R_OPCODE)	{
			int addValue = (op_code == POST_INCR_OPR8R_OPCODE ? 1 : -1);

			// Return current value to our "stack" for use in the expression, THEN alter NameSpace value
			exprTknStream[opr8rIdx] = operand1;
			operand1.isUnsigned() ? operand1._unsigned += addValue : operand1._signed += addValue;
			isSuccess = true;
		}

		if (isSuccess)	{
			exprTknStream[opr8rIdx].isInitialized = true;
			operand1.isInitialized = true;
			ret_code = scopedNameSpace->findVar(varName1, 0, operand1, COMMIT_WRITE, lookUpMsg);
			if (OK!= ret_code)
				userMessages->logMsg(INTERNAL_ERROR, lookUpMsg, thisSrcFile, __LINE__, 0);
		}

	} else	{
		userMessages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", thisSrcFile, __LINE__, 0);
	}

	return (ret_code);
}

/* ****************************************************************************
 * Handle these OPR8Rs:
 * [<] [<=] [>] [>=] [==] [!=]
 * ***************************************************************************/
int RunTimeInterpreter::execEquivalenceOp(std::vector<Token> & exprTknStream, int opr8rIdx)     {
	int ret_code = GENERAL_FAILURE;
	
	if (opr8rIdx >= 0 && exprTknStream.size() > (opr8rIdx + 2) && exprTknStream[opr8rIdx].tkn_type == EXEC_OPR8R_TKN)	{
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t op_code = exprTknStream[opr8rIdx]._unsigned;

		// Either|both of our operand Tokens could be USER_WORD variable names, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		Token operand2;
		std::wstring varName1;
		std::wstring varName2;
		resolveTknOrVar (exprTknStream[opr8rIdx + 1], operand1, varName1);
		resolveTknOrVar (exprTknStream[opr8rIdx + 2], operand2, varName2);

		TokenCompareResult compareRez = operand1.compare (operand2);

		switch (op_code)	{
			case LESS_THAN_OPR8R_OPCODE :
				if (compareRez.lessThan == isTrue)
					exprTknStream[opr8rIdx] = *oneTkn;
				else if (compareRez.lessThan == isFalse)
					exprTknStream[opr8rIdx] = *zeroTkn;
				else
					failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
				break;
			case LESS_EQUALS_OPR8R8_OPCODE :
				if (compareRez.lessEquals == isTrue)
					exprTknStream[opr8rIdx] = *oneTkn;
				else if (compareRez.lessEquals == isFalse)
					exprTknStream[opr8rIdx] = *zeroTkn;
				else
					failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
				break;
			case GREATER_THAN_OPR8R_OPCODE :
				if (compareRez.gr8rThan == isTrue)
					exprTknStream[opr8rIdx] = *oneTkn;
				else if (compareRez.gr8rThan == isFalse)
					exprTknStream[opr8rIdx] = *zeroTkn;
				else
					failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
				break;
			case GREATER_EQUALS_OPR8R8_OPCODE :
				if (compareRez.gr8rEquals == isTrue)
					exprTknStream[opr8rIdx] = *oneTkn;
				else if (compareRez.gr8rEquals == isFalse)
					exprTknStream[opr8rIdx] = *zeroTkn;
				else
					failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
				break;
			case EQUALITY_OPR8R_OPCODE :
				if (compareRez.equals == isTrue)
					exprTknStream[opr8rIdx] = *oneTkn;
				else if (compareRez.equals == isFalse)
					exprTknStream[opr8rIdx] = *zeroTkn;
				else
					failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
				break;
			case NOT_EQUALS_OPR8R_OPCODE:
				if (compareRez.equals == isFalse)	
					exprTknStream[opr8rIdx] = *oneTkn;

				else if (compareRez.equals == isTrue)
					exprTknStream[opr8rIdx] = *zeroTkn;
			 	else	
					failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
				break;
			default:
				failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
				break;
		}

		if (!failOnSrcLine)	{
			exprTknStream[opr8rIdx].isInitialized = true;
			ret_code = OK;
		
		} else	{ 
			std::wstring tmpStr = execTerms.getSrcOpr8rStrFor(op_code);
			if (tmpStr.empty())
				tmpStr = L"UNKNOWN OP_CODE [" + std::to_wstring(op_code) + L"]";
			userMessages->logMsg (INTERNAL_ERROR, L"Failed to execute " + tmpStr, thisSrcFile, __LINE__, 0);
		}

	} else	{
		userMessages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", thisSrcFile, __LINE__, 0);
	}

  return (ret_code);
}

/* ****************************************************************************
 * Handle these OPR8Rs:
 * [+] [-] [*] [/] [%]
 * ***************************************************************************/
int RunTimeInterpreter::execStandardMath (std::vector<Token> & exprTknStream, int opr8rIdx)	{
	int ret_code = GENERAL_FAILURE;
	bool isParamsValid = false;
	bool isMissedCase = false;
	bool isDivByZero = false;
	bool isNowDouble = false;
	uint64_t tmpUnsigned;
	int64_t tmpSigned;
	double tmpDouble;
	std::wstring tmpStr;

	if (opr8rIdx >= 0 && exprTknStream.size() > (opr8rIdx + 2) && exprTknStream[opr8rIdx].tkn_type == EXEC_OPR8R_TKN)	{
		
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t op_code = exprTknStream[opr8rIdx]._unsigned;

		// Either|both of our operand Tokens could be USER_WORD variable names, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		Token operand2;
		std::wstring varName1;
		std::wstring varName2;
		resolveTknOrVar (exprTknStream[opr8rIdx+1], operand1, varName1);
		resolveTknOrVar (exprTknStream[opr8rIdx+2], operand2, varName2);

		// 1st check for valid passed parameters
		if (op_code == MULTIPLY_OPR8R_OPCODE || op_code == DIV_OPR8R_OPCODE || op_code == BINARY_MINUS_OPR8R_OPCODE)	{
			if ((operand1.isSigned() || operand1.isUnsigned() || operand1.tkn_type == DOUBLE_TKN)
					&& (operand2.isSigned() || operand2.isUnsigned() || operand2.tkn_type == DOUBLE_TKN))
				isParamsValid = true;

		} else if (op_code == MOD_OPR8R_OPCODE)	{
			if ((operand1.isSigned() || operand1.isUnsigned()) && (operand2.isSigned() || operand2.isUnsigned()))
				isParamsValid = true;

		} else if (op_code == BINARY_PLUS_OPR8R_OPCODE)	{
			if ((operand1.isSigned() || operand1.isUnsigned() || operand1.tkn_type == DOUBLE_TKN)
					&& (operand2.isSigned() || operand2.isUnsigned() || operand2.tkn_type == DOUBLE_TKN))
				isParamsValid = true;

			if (operand1.tkn_type == STRING_TKN && operand2.tkn_type == STRING_TKN)
				isParamsValid = true;
		}

		if (isParamsValid)	{
			bool isAddingStrings;
			(op_code == BINARY_PLUS_OPR8R_OPCODE && operand1.tkn_type == STRING_TKN) ? isAddingStrings = true : isAddingStrings = false;


			if (!isAddingStrings && operand1.isSigned() && operand2.isSigned())	{
				// signed vs. signed
				switch (op_code)	{
					case MULTIPLY_OPR8R_OPCODE :
						tmpSigned = operand1._signed * operand2._signed;
						break;
					case DIV_OPR8R_OPCODE :
						if (operand2._signed == 0)
							isDivByZero = true;
						else if (0 == operand1._signed % operand2._signed)
							tmpSigned = operand1._signed / operand2._signed;
						else	{
							tmpDouble = (double) operand1._signed / (double) operand2._signed;
							isNowDouble = true;
						}
						break;
					case BINARY_MINUS_OPR8R_OPCODE :
						tmpSigned = operand1._signed - operand2._signed;
						break;
					case BINARY_PLUS_OPR8R_OPCODE :
						tmpSigned = operand1._signed + operand2._signed;
						break;
					case MOD_OPR8R_OPCODE:
						if (operand2._signed == 0)
							isDivByZero = true;
						else
							tmpSigned = operand1._signed % operand2._signed;
						break;
					default:
						isMissedCase = true;
						break;
				}

				if (!isMissedCase &&!isDivByZero)	{
					if (isNowDouble)
						exprTknStream[opr8rIdx].resetToDouble(tmpDouble);
					else
						exprTknStream[opr8rIdx].resetToSigned(tmpSigned);
				}

			} else if (!isAddingStrings && operand1.isSigned() && operand2.isUnsigned())	{
				// signed vs. unsigned
				switch (op_code)	{
					case MULTIPLY_OPR8R_OPCODE :
						tmpSigned = operand1._signed * operand2._unsigned;
						break;
					case DIV_OPR8R_OPCODE :
						if (operand2._unsigned == 0)
							isDivByZero = true;
						else if (0 == operand1._signed % operand2._unsigned)
							tmpSigned = operand1._signed / operand2._unsigned;
						else	{
							tmpDouble = (double) operand1._signed / (double) operand2._unsigned;
							isNowDouble = true;
						}
						break;
					case BINARY_MINUS_OPR8R_OPCODE :
						tmpSigned = operand1._signed - operand2._unsigned;
						break;
					case BINARY_PLUS_OPR8R_OPCODE :
						tmpSigned = operand1._signed + operand2._unsigned;
						break;
					case MOD_OPR8R_OPCODE:
						if (operand2._unsigned == 0)
							isDivByZero = true;
						else
							tmpSigned = operand1._signed % operand2._unsigned;
						break;
					default:
						isMissedCase = true;
						break;
				}

				if (!isMissedCase &&!isDivByZero)	{
					if (isNowDouble)
						exprTknStream[opr8rIdx].resetToDouble(tmpDouble);
					else
						exprTknStream[opr8rIdx].resetToSigned(tmpSigned);
				}

			} else if (!isAddingStrings && operand1.isSigned() && operand2.tkn_type == DOUBLE_TKN)	{
				// signed vs. double
				switch (op_code)	{
					case MULTIPLY_OPR8R_OPCODE :
						tmpDouble = operand1._signed * operand2._double;
						break;
					case DIV_OPR8R_OPCODE :
						if (operand2._double == 0)
							isDivByZero = true;
						else
							tmpDouble = operand1._signed / operand2._double;
						break;
					case BINARY_MINUS_OPR8R_OPCODE :
						tmpDouble = operand1._signed - operand2._double;
						break;
					case BINARY_PLUS_OPR8R_OPCODE :
						tmpDouble = operand1._signed + operand2._double;
						break;
					default:
						isMissedCase = true;
						break;
				}

				if (!isMissedCase &&!isDivByZero)
					exprTknStream[opr8rIdx].resetToDouble (tmpDouble);

			} else if (!isAddingStrings && operand1.isUnsigned() && operand2.isSigned())	{
				// unsigned vs. signed
				switch (op_code)	{
					case MULTIPLY_OPR8R_OPCODE :
						tmpSigned = operand1._unsigned * operand2._signed;
						break;
					case DIV_OPR8R_OPCODE :
						if (operand2._signed == 0)
							isDivByZero = true;
						else	{
							// TODO: Result of integer division used in a floating point context; possible loss of precision
							tmpDouble = operand1._unsigned / operand2._signed;
							isNowDouble = true;
						}
						break;
					case BINARY_MINUS_OPR8R_OPCODE :
						tmpSigned = operand1._unsigned - operand2._signed;
						break;
					case BINARY_PLUS_OPR8R_OPCODE :
						tmpSigned = operand1._unsigned + operand2._signed;
						break;
					case MOD_OPR8R_OPCODE:
						if (operand2._signed == 0)
							isDivByZero = true;
						else
							tmpSigned = operand1._unsigned % operand2._signed;
						break;
					default:
						isMissedCase = true;
						break;
				}

				if (!isMissedCase &&!isDivByZero)	{
					if (isNowDouble)
						exprTknStream[opr8rIdx].resetToDouble(tmpDouble);
					else
						exprTknStream[opr8rIdx].resetToSigned (tmpSigned);
				}

			} else if (!isAddingStrings && operand1.isUnsigned() && operand2.tkn_type == DOUBLE_TKN)	{
				// unsigned vs. double
				switch (op_code)	{
					case MULTIPLY_OPR8R_OPCODE :
						tmpDouble = operand1._unsigned * operand2._double;
						break;
					case DIV_OPR8R_OPCODE :
						if (operand2._double == 0)
							isDivByZero = true;
						else
							tmpDouble = operand1._unsigned / operand2._double;
						break;
					case BINARY_MINUS_OPR8R_OPCODE :
						tmpDouble = operand1._unsigned - operand2._double;
						break;
					case BINARY_PLUS_OPR8R_OPCODE :
						tmpDouble = operand1._unsigned + operand2._double;
						break;
					case MOD_OPR8R_OPCODE:
						if (operand2._signed == 0)
							isDivByZero = true;
						else
							tmpSigned = operand1._unsigned % operand2._signed;
						break;
					default:
						isMissedCase = true;
						break;
				}

				if (!isMissedCase &&!isDivByZero)
					exprTknStream[opr8rIdx].resetToDouble(tmpDouble);

			} else if (!isAddingStrings && operand1.isUnsigned() && operand2.isUnsigned())	{
				// unsigned vs. unsigned
				switch (op_code)	{
					case MULTIPLY_OPR8R_OPCODE :
						tmpUnsigned = operand1._unsigned * operand2._unsigned;
						break;
					case DIV_OPR8R_OPCODE :
						if (operand2._unsigned == 0)
							isDivByZero = true;
						else
							tmpUnsigned = operand1._unsigned / operand2._unsigned;
						break;
						if (operand2._unsigned == 0)
							isDivByZero = true;
						else if (0 == operand1._unsigned % operand2._unsigned)
							tmpUnsigned = operand1._unsigned / operand2._unsigned;
						else	{
							tmpDouble = (double) operand1._unsigned / (double) operand2._unsigned;
							isNowDouble = true;
						}
						break;
					case BINARY_MINUS_OPR8R_OPCODE :
						tmpUnsigned = operand1._unsigned - operand2._unsigned;
						break;
					case BINARY_PLUS_OPR8R_OPCODE :
						tmpUnsigned = operand1._unsigned + operand2._unsigned;
						break;
					case MOD_OPR8R_OPCODE:
						if (operand2._unsigned == 0)
							isDivByZero = true;
						else
							tmpUnsigned = operand1._unsigned % operand2._unsigned;
						break;
					default:
						isMissedCase = true;
						break;
				}

				if (!isMissedCase &&!isDivByZero)	{
					if (isNowDouble)
						exprTknStream[opr8rIdx].resetToDouble(tmpDouble);
					else
						exprTknStream[opr8rIdx].resetToUnsigned(tmpUnsigned);
				}

			} else if (!isAddingStrings && operand1.isUnsigned() && operand2.tkn_type == DOUBLE_TKN)	{
				// unsigned vs. double
				switch (op_code)	{
					case MULTIPLY_OPR8R_OPCODE :
						tmpDouble = operand1._unsigned * operand2._double;
						break;
					case DIV_OPR8R_OPCODE :
						if (operand2._double == 0)
							isDivByZero = true;
						else
							tmpDouble = operand1._unsigned / operand2._double;
						break;
					case BINARY_MINUS_OPR8R_OPCODE :
						tmpDouble = operand1._unsigned - operand2._double;
						break;
					case BINARY_PLUS_OPR8R_OPCODE :
						tmpDouble = operand1._unsigned + operand2._double;
						break;
					default:
						isMissedCase = true;
						break;
				}

				if (!isMissedCase &&!isDivByZero)
					exprTknStream[opr8rIdx].resetToDouble (tmpDouble);

			} else if (!isAddingStrings && operand1.tkn_type == DOUBLE_TKN && operand2.isSigned())	{
				// double vs. signed
				switch (op_code)	{
					case MULTIPLY_OPR8R_OPCODE :
						tmpDouble = operand1._double * operand2._signed;
						break;
					case DIV_OPR8R_OPCODE :
						if (operand2._signed == 0)
							isDivByZero = true;
						else
							tmpDouble = operand1._double / operand2._signed;
						break;
					case BINARY_MINUS_OPR8R_OPCODE :
						tmpDouble = operand1._double - operand2._signed;
						break;
					case BINARY_PLUS_OPR8R_OPCODE :
						tmpDouble = operand1._double + operand2._signed;
						break;
					default:
						isMissedCase = true;
						break;
				}
				if (!isMissedCase &&!isDivByZero)
					exprTknStream[opr8rIdx].resetToDouble (tmpDouble);

			} else if (!isAddingStrings && operand1.tkn_type == DOUBLE_TKN && operand2.isUnsigned())	{
				// double vs. unsigned
				switch (op_code)	{
					case MULTIPLY_OPR8R_OPCODE :
						tmpDouble = operand1._double * operand2._unsigned;
						break;
					case DIV_OPR8R_OPCODE :
						if (operand2._unsigned == 0)
							isDivByZero = true;
						else
							tmpDouble = operand1._double / operand2._unsigned;
						break;
					case BINARY_MINUS_OPR8R_OPCODE :
						tmpDouble = operand1._double - operand2._unsigned;
						break;
					case BINARY_PLUS_OPR8R_OPCODE :
						tmpDouble = operand1._double + operand2._unsigned;
						break;
					default:
						isMissedCase = true;
						break;
				}

				if (!isMissedCase &&!isDivByZero)
					exprTknStream[opr8rIdx].resetToDouble (tmpDouble);

			} else if (!isAddingStrings && operand1.tkn_type == DOUBLE_TKN && operand2.tkn_type == DOUBLE_TKN)	{
				// double vs. double
				switch (op_code)	{
				case MULTIPLY_OPR8R_OPCODE :
					tmpDouble = operand1._double * operand2._double;
					break;
				case DIV_OPR8R_OPCODE :
					if (operand2._double == 0)
						isDivByZero = true;
					else
						tmpDouble = operand1._double / operand2._double;
					break;
				case BINARY_MINUS_OPR8R_OPCODE :
					tmpDouble = operand1._double - operand2._double;
					break;
				case BINARY_PLUS_OPR8R_OPCODE :
					tmpDouble = operand1._double + operand2._double;
					break;
				default:
					isMissedCase = true;
					break;
				}

				if (!isMissedCase &&!isDivByZero)
					exprTknStream[opr8rIdx].resetToDouble (tmpDouble);

			} else if (isAddingStrings && operand1.tkn_type == STRING_TKN && operand2.tkn_type == STRING_TKN)	{
				if (!operand1._string.empty())
					tmpStr = operand1._string;
				if (!operand2._string.empty())
					tmpStr.append(operand2._string);

				exprTknStream[opr8rIdx].resetToString (tmpStr);
			}
		}	else	{
			userMessages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", thisSrcFile, __LINE__, 0);
		}

		if (isParamsValid && !isMissedCase && !isDivByZero)	{
			exprTknStream[opr8rIdx].isInitialized = true;
			ret_code = OK;
		
		} else if (isDivByZero)	{
			// TODO: How can the error msg indicate where this div by zero occurred in the code?
			// Should enclosing scope contain opening line # and possibly source file name
			// void InfoWarnError::set(info_warn_error_type type, std::wstring userSrcFileName, int userSrcLineNum, int userSrcColPos, std::wstring srcFileName, int srcLineNum, std::wstring msgForUser) {
			std::wstring badUserMsg = L"User code attempted to divide by ZERO! [";
			badUserMsg.append (varName1.empty() ? operand1.getValueStr() : varName1);
			badUserMsg.append (L" / ");
			badUserMsg.append (varName2.empty() ? operand2.getValueStr() : varName2);
			badUserMsg.append (L"]");
			userMessages->logMsg (USER_ERROR, badUserMsg, !userSrcFileName.empty() ? userSrcFileName : L"???", 0, 0);

		} else if (isMissedCase)
			userMessages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", thisSrcFile, __LINE__, 0);
	}

	return (ret_code);
}

/* ****************************************************************************
 * Handle these OPR8Rs:
 * [<<] [>>]
 * ***************************************************************************/
int RunTimeInterpreter::execShift (std::vector<Token> & exprTknStream, int opr8rIdx)	{
	int ret_code = GENERAL_FAILURE;

	if (opr8rIdx >= 0 && exprTknStream.size() > (opr8rIdx + 2) && exprTknStream[opr8rIdx].tkn_type == EXEC_OPR8R_TKN)	{
		// Passed in resultTkn has OPR8R op_code BEFORE it is overwritten by the result
		
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t op_code = exprTknStream[opr8rIdx]._unsigned;

		// Either|both of our operand Tokens could be USER_WORD variable names, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		Token operand2;
		std::wstring varName1;
		std::wstring varName2;
		resolveTknOrVar (exprTknStream[opr8rIdx+1], operand1, varName1);
		resolveTknOrVar (exprTknStream[opr8rIdx+2], operand2, varName2);

		// Operand #1 must be of type UINT[N] or INT[N]; Operand #2 can be either UINT[N] or INT[N] > 0
		if ((op_code == LEFT_SHIFT_OPR8R_OPCODE || op_code == RIGHT_SHIFT_OPR8R_OPCODE) && (operand1.isUnsigned() || operand1.isSigned())
				&& ((operand2.isUnsigned() && operand2._unsigned >= 0) || operand2.isSigned() && operand2._signed >= 0))	{

			uint64_t tmpUnsigned;
			int64_t tmpSigned;

			if (operand1.isUnsigned())	{
				if (operand2.isUnsigned())	{
					if (op_code == LEFT_SHIFT_OPR8R_OPCODE)	{
						tmpUnsigned = operand1._unsigned << operand2._unsigned;
					} else {
						tmpUnsigned = operand1._unsigned >> operand2._unsigned;
					}
				} else 	{
					if (op_code == LEFT_SHIFT_OPR8R_OPCODE)	{
						tmpUnsigned = operand1._unsigned << operand2._signed;
					} else {
						tmpUnsigned = operand1._unsigned >> operand2._signed;
					}
				}

				exprTknStream[opr8rIdx].resetToUnsigned (tmpUnsigned);

			} else if (operand1._signed > 0)	{
				if (operand2.isUnsigned())	{
					if (op_code == LEFT_SHIFT_OPR8R_OPCODE)	{
						tmpUnsigned = operand1._signed << operand2._unsigned;
					} else {
						tmpUnsigned = operand1._signed >> operand2._unsigned;
					}
				} else	{
					// operand2 is SIGNED
					if (op_code == LEFT_SHIFT_OPR8R_OPCODE)	{
						tmpUnsigned = operand1._signed << operand2._signed;
					} else {
						tmpUnsigned = operand1._signed >> operand2._signed;
					}
				}

				exprTknStream[opr8rIdx].resetToUnsigned (tmpUnsigned);

			} else	{
				// operand1 is negative, so we have to keep the sign bit around
				if (op_code == LEFT_SHIFT_OPR8R_OPCODE)	{
					tmpSigned = operand1._signed << operand2._signed;
				} else {
					tmpSigned = operand1._signed >> operand2._signed;
				}
				exprTknStream[opr8rIdx].resetToSigned (tmpSigned);
			}

			ret_code = OK;
		}
	}	else	{
		userMessages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", thisSrcFile, __LINE__, 0);
	}

	if (OK != ret_code)
		userMessages->logMsg (INTERNAL_ERROR, L"Function failed!", thisSrcFile, __LINE__, 0);

	return (ret_code);
}

/* ****************************************************************************
 * Handle these OPR8Rs:
 * [&] [|] [^]
 * ***************************************************************************/
int RunTimeInterpreter::execBitWiseOp (std::vector<Token> & exprTknStream, int opr8rIdx)	{
	int ret_code = GENERAL_FAILURE;
	bool isParamsValid = true;
	bool isMissedCase = false;

	if (opr8rIdx >= 0 && exprTknStream.size() > (opr8rIdx + 2) && exprTknStream[opr8rIdx].tkn_type == EXEC_OPR8R_TKN)	{

		// Passed in resultTkn has OPR8R op_code BEFORE it is overwritten by the result
		// TODO: If these are USER_WORD_TKNs, then do a variable name lookup in our NameSpace
		
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t op_code = exprTknStream[opr8rIdx]._unsigned;

		// Either|both of our operand Tokens could be USER_WORD variable names, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		Token operand2;
		std::wstring varName1;
		std::wstring varName2;
		resolveTknOrVar (exprTknStream[opr8rIdx+1], operand1, varName1);
		resolveTknOrVar (exprTknStream[opr8rIdx+2], operand2, varName2);

		uint64_t bitWiseResult;

		if (op_code == BITWISE_AND_OPR8R_OPCODE || op_code == BITWISE_XOR_OPR8R_OPCODE || op_code == BITWISE_OR_OPR8R_OPCODE)	{
			if (operand1.isUnsigned() && operand2.isUnsigned())	{
				switch (op_code)	{
					case BITWISE_AND_OPR8R_OPCODE :
						bitWiseResult = operand1._unsigned & operand2._unsigned;
						break;
					case BITWISE_XOR_OPR8R_OPCODE :
						bitWiseResult = operand1._unsigned ^ operand2._unsigned;
						break;
					case BITWISE_OR_OPR8R_OPCODE :
						bitWiseResult = operand1._unsigned | operand2._unsigned;
						break;
					default :
						isMissedCase = true;
						break;
				}

			} else if (operand1.isUnsigned() && operand2.isSigned() && operand2._signed >= 0)	{
				switch (op_code)	{
					case BITWISE_AND_OPR8R_OPCODE :
						bitWiseResult = operand1._unsigned & operand2._signed;
						break;
					case BITWISE_XOR_OPR8R_OPCODE :
						bitWiseResult = operand1._unsigned ^ operand2._signed;
						break;
					case BITWISE_OR_OPR8R_OPCODE :
						bitWiseResult = operand1._unsigned | operand2._signed;
						break;
					default :
						isMissedCase = true;
						break;
				}

			} else if (operand1.isSigned() && operand1._signed >= 0 && operand2.isUnsigned())	{
				switch (op_code)	{
					case BITWISE_AND_OPR8R_OPCODE :
						bitWiseResult = operand1._signed & operand2._unsigned;
						break;
					case BITWISE_XOR_OPR8R_OPCODE :
						bitWiseResult = operand1._signed ^ operand2._unsigned;
						break;
					case BITWISE_OR_OPR8R_OPCODE :
						bitWiseResult = operand1._signed | operand2._unsigned;
						break;
					default :
						isMissedCase = true;
						break;
				}

			} else if (operand1.isSigned() && operand1._signed >= 0 && operand2.isSigned() && operand2._signed >= 0)	{
				switch (op_code)	{
					case BITWISE_AND_OPR8R_OPCODE :
						bitWiseResult = operand1._signed & operand2._signed;
						break;
					case BITWISE_XOR_OPR8R_OPCODE :
						bitWiseResult = operand1._signed ^ operand2._signed;
						break;
					case BITWISE_OR_OPR8R_OPCODE :
						bitWiseResult = operand1._signed | operand2._signed;
						break;
					default :
						isMissedCase = true;
						break;
				}

			} else	{
				isParamsValid = false;
			}
		}

		if (isParamsValid && !isMissedCase)	{
			exprTknStream[opr8rIdx].resetToUnsigned(bitWiseResult);
			exprTknStream[opr8rIdx].isInitialized = true;
			ret_code = OK;
		
		} else	{
			Operator opr8r;
			execTerms.getExecOpr8rDetails(op_code, opr8r);
			userMessages->logMsg (INTERNAL_ERROR, L"Failed to execute OPR8R " + opr8r.symbol, thisSrcFile, __LINE__, 0);
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * Handle these OPR8Rs:
 * [~] [-] [!]
 * ***************************************************************************/
int RunTimeInterpreter::execUnaryOp (std::vector<Token> & exprTknStream, int opr8rIdx)	{
	int ret_code = GENERAL_FAILURE;
	bool isSuccess = false;

	if (opr8rIdx >= 0 && exprTknStream.size() > (opr8rIdx + 1) && exprTknStream[opr8rIdx].tkn_type == EXEC_OPR8R_TKN)	{

		// TODO: If these are USER_WORD_TKNs, then do a variable name lookup in our NameSpace
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t op_code = exprTknStream[opr8rIdx]._unsigned;

		// Our operand Token could be a USER_WORD variable name, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		std::wstring varName1;
		resolveTknOrVar (exprTknStream[opr8rIdx+1], operand1, varName1);
		uint64_t unaryResult;

		if (op_code == UNARY_PLUS_OPR8R_OPCODE)	{
			// UNARY_PLUS is a NO-OP with the right data types
			if (operand1.isSigned() || operand1.isUnsigned())	{
				exprTknStream[opr8rIdx] = operand1;
				ret_code = OK;
			}

		} else if (op_code == UNARY_MINUS_OPR8R_OPCODE)	{
			if (operand1.isSigned())	{
				exprTknStream[opr8rIdx] = operand1;
				exprTknStream[opr8rIdx]._signed = (0 - operand1._signed);
				ret_code = OK;

			} else if (operand1.isUnsigned())	{
				int64_t tmpInt64 = exprTknStream[opr8rIdx]._unsigned;
				exprTknStream[opr8rIdx].resetToSigned (-tmpInt64);
				ret_code = OK;

			} else if (operand1.tkn_type == DOUBLE_TKN)	{
				double tmpDouble = exprTknStream[opr8rIdx]._double;
				exprTknStream[opr8rIdx].resetToDouble (-tmpDouble);
				ret_code = OK;
			}

		} else if (op_code == LOGICAL_NOT_OPR8R_OPCODE)	{
			if (operand1.isUnsigned() || operand1.tkn_type == BOOL_TKN)	{
				if (operand1._unsigned == 0)
					exprTknStream[opr8rIdx] = *oneTkn;
				else
					exprTknStream[opr8rIdx] = *zeroTkn;

				if (operand1.tkn_type == BOOL_TKN)
					// Preserve the data type
					exprTknStream[opr8rIdx].tkn_type = BOOL_TKN;
				ret_code = OK;

			} else if (operand1.isSigned())	{
				if (operand1._signed == 0)
					exprTknStream[opr8rIdx] = *oneTkn;
				else
					exprTknStream[opr8rIdx] = *zeroTkn;
				ret_code = OK;

			} else if (operand1.tkn_type == DOUBLE_TKN)	{
				if (operand1._double == 0.0)
					exprTknStream[opr8rIdx] = *oneTkn;
				else
					exprTknStream[opr8rIdx] = *zeroTkn;
				ret_code = OK;
			}
		} else if (op_code == BITWISE_NOT_OPR8R_OPCODE && operand1.isUnsigned())	{
			// Need to guard against uncalled for overflow
			// TODO: Chicken|egg problem with warning user about overflow
			uint64_t mask = ~(0x0);
			if (operand1.tkn_type == UINT8_TKN)
				mask = 0xFF;
			else if (operand1.tkn_type == UINT16_TKN)
				mask = 0xFFFF;
			else if (operand1.tkn_type == UINT32_TKN)
				mask = 0xFFFFFFFF;
			
			operand1._unsigned = ~(operand1._unsigned) & mask;
			exprTknStream[opr8rIdx] = operand1;
			ret_code = OK;
		}

		if (OK == ret_code) {
			exprTknStream[opr8rIdx].isInitialized = true;
		
		} else	{
			Operator opr8r;
			execTerms.getExecOpr8rDetails(op_code, opr8r);
			userMessages->logMsg (INTERNAL_ERROR, L"Failed to execute OPR8R " + opr8r.symbol, thisSrcFile, __LINE__, 0);
		}

	} else	{
		userMessages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", thisSrcFile, __LINE__, 0);
	}

	return (ret_code);
}

/* ****************************************************************************
 * Handle these assignment OPR8Rs:
 * [=] [+=] [-=] [*=] [/=] [%=] [<<=] [>>=] [&==] [|==] [^==]
 * ***************************************************************************/
int RunTimeInterpreter::execAssignmentOp(std::vector<Token> & exprTknStream, int opr8rIdx)     {
	int ret_code = GENERAL_FAILURE;
	bool isSuccess = false;
	std::wstring lookUpMsg;
	
	if (opr8rIdx >= 0 && exprTknStream.size() > (opr8rIdx + 2) && exprTknStream[opr8rIdx].tkn_type == EXEC_OPR8R_TKN)	{
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t original_op_code = exprTknStream[opr8rIdx]._unsigned;

		// Either|both of our operand Tokens could be USER_WORD variable names, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		Token operand2;
		std::wstring varName1;
		std::wstring varName2;
		resolveTknOrVar (exprTknStream[opr8rIdx + 1], operand1, varName1, false);
		resolveTknOrVar (exprTknStream[opr8rIdx + 2], operand2, varName2);

		bool isOpSuccess = false;

		std::wstring bgnNottaVarMsg = L"Left operand of an assignment operator must be a named variable: ";
		Token opr8rTkn = exprTknStream[opr8rIdx];
		
		if (varName1.empty() && usageMode == COMPILE_TIME)	{
			userMessages->logMsg(USER_ERROR
				, bgnNottaVarMsg + opr8rTkn.descr_sans_line_num_col() + L" Assignment operation may need to be enclosed in parentheses."
				, userSrcFileName, opr8rTkn.get_line_number(), opr8rTkn.get_column_pos());

		} else if (varName1.empty() && usageMode == INTERPRETER)	{
			userMessages->logMsg(INTERNAL_ERROR, bgnNottaVarMsg + opr8rTkn.descr_sans_line_num_col()
				, thisSrcFile, __LINE__, 0);

		} else	{
			switch (original_op_code)	{
				case ASSIGNMENT_OPR8R_OPCODE :
					if (OK == scopedNameSpace->findVar(varName1, 0, operand2, COMMIT_WRITE, lookUpMsg))	{
						// We've updated the NS Variable Token; now overwrite the OPR8R with the result also
						exprTknStream[opr8rIdx] = operand2;
						isOpSuccess = true;
						ret_code = OK;
					
					} else if (!lookUpMsg.empty())	{
						userMessages->logMsg(INTERNAL_ERROR, lookUpMsg, thisSrcFile, __LINE__, 0);
					}
					break;
				case PLUS_ASSIGN_OPR8R_OPCODE :
					exprTknStream[opr8rIdx]._unsigned = BINARY_PLUS_OPR8R_OPCODE;
					if (OK == execBinaryOp (exprTknStream, opr8rIdx))
						isOpSuccess = true;
					break;
				case MINUS_ASSIGN_OPR8R_OPCODE :
					exprTknStream[opr8rIdx]._unsigned = BINARY_MINUS_OPR8R_OPCODE;
					if (OK == execBinaryOp (exprTknStream, opr8rIdx))
						isOpSuccess = true;
					break;
				case MULTIPLY_ASSIGN_OPR8R_OPCODE :
					exprTknStream[opr8rIdx]._unsigned = MULTIPLY_OPR8R_OPCODE;
					if (OK == execBinaryOp (exprTknStream, opr8rIdx))
						isOpSuccess = true;
					break;
				case DIV_ASSIGN_OPR8R_OPCODE :
					exprTknStream[opr8rIdx]._unsigned = DIV_OPR8R_OPCODE;
					if (OK == execBinaryOp (exprTknStream, opr8rIdx))
						isOpSuccess = true;
					break;
				case MOD_ASSIGN_OPR8R_OPCODE :
					exprTknStream[opr8rIdx]._unsigned = MOD_OPR8R_OPCODE;
					if (OK == execBinaryOp (exprTknStream, opr8rIdx))
						isOpSuccess = true;
					break;
				case LEFT_SHIFT_ASSIGN_OPR8R_OPCODE :
					exprTknStream[opr8rIdx]._unsigned = LEFT_SHIFT_OPR8R_OPCODE;
					if (OK == execBinaryOp (exprTknStream, opr8rIdx))
						isOpSuccess = true;
					break;
				case RIGHT_SHIFT_ASSIGN_OPR8R_OPCODE :
					exprTknStream[opr8rIdx]._unsigned = RIGHT_SHIFT_OPR8R_OPCODE;
					if (OK == execBinaryOp (exprTknStream, opr8rIdx))
						isOpSuccess = true;
					break;
				case BITWISE_AND_ASSIGN_OPR8R_OPCODE :
					exprTknStream[opr8rIdx]._unsigned = BITWISE_AND_OPR8R_OPCODE;
					if (OK == execBinaryOp (exprTknStream, opr8rIdx))
						isOpSuccess = true;
					break;
				case BITWISE_XOR_ASSIGN_OPR8R_OPCODE :
					exprTknStream[opr8rIdx]._unsigned = BITWISE_XOR_OPR8R_OPCODE;
					if (OK == execBinaryOp (exprTknStream, opr8rIdx))
						isOpSuccess = true;
					break;
				case BITWISE_OR_ASSIGN_OPR8R_OPCODE :
					exprTknStream[opr8rIdx]._unsigned = BITWISE_OR_OPR8R_OPCODE;
					if (OK == execBinaryOp (exprTknStream, opr8rIdx))
						isOpSuccess = true;
					break;
				default:
					break;
			}
		}

		if (isOpSuccess)	{
			exprTknStream[opr8rIdx].isInitialized = true;
			if (original_op_code != ASSIGNMENT_OPR8R_OPCODE
					&& OK == scopedNameSpace->findVar(varName1, 0, exprTknStream[opr8rIdx], COMMIT_WRITE, lookUpMsg))	{
				// Commit the result to the stored NS variable. OPR8R Token (previously @ opr8rIdx) has already been overwritten with result 
				ret_code = OK;
			} else if (!lookUpMsg.empty())	{
				userMessages->logMsg(INTERNAL_ERROR, lookUpMsg, thisSrcFile, __LINE__, 0);
			}
		}
	}  else	{
		userMessages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", thisSrcFile, __LINE__, 0);
	}

	return (ret_code);
}

/* ****************************************************************************
 * Jump gate for handling BINARY OPR8Rs
 * ***************************************************************************/
int RunTimeInterpreter::execBinaryOp(std::vector<Token> & exprTknStream, int opr8rIdx)     {
	int ret_code = GENERAL_FAILURE;

	if (opr8rIdx >= 0 && exprTknStream.size() > (opr8rIdx + 2) && exprTknStream[opr8rIdx].tkn_type == EXEC_OPR8R_TKN)	{
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t op_code = exprTknStream[opr8rIdx]._unsigned;
		Operator opr8r;
		execTerms.getExecOpr8rDetails(op_code, opr8r);

		// Either|both of our operand Tokens could be USER_WORD variable names, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		Token operand2;
		std::wstring varName1;
		std::wstring varName2;
		resolveTknOrVar (exprTknStream[opr8rIdx+1], operand1, varName1, op_code == ASSIGNMENT_OPR8R_OPCODE ?  false : true);
		resolveTknOrVar (exprTknStream[opr8rIdx+2], operand2, varName2);

		switch (op_code)	{
			case MULTIPLY_OPR8R_OPCODE :
			case DIV_OPR8R_OPCODE :
			case MOD_OPR8R_OPCODE :
			case BINARY_PLUS_OPR8R_OPCODE :
			case BINARY_MINUS_OPR8R_OPCODE :
				ret_code = execStandardMath (exprTknStream, opr8rIdx);
				break;

			case LEFT_SHIFT_OPR8R_OPCODE :
			case RIGHT_SHIFT_OPR8R_OPCODE :
				ret_code = execShift (exprTknStream, opr8rIdx);
				break;

			case LESS_THAN_OPR8R_OPCODE :
			case LESS_EQUALS_OPR8R8_OPCODE :
			case GREATER_THAN_OPR8R_OPCODE :
			case GREATER_EQUALS_OPR8R8_OPCODE :
			case EQUALITY_OPR8R_OPCODE :
			case NOT_EQUALS_OPR8R_OPCODE :
				ret_code = execEquivalenceOp (exprTknStream, opr8rIdx);
				break;

			case BITWISE_AND_OPR8R_OPCODE :
			case BITWISE_XOR_OPR8R_OPCODE :
			case BITWISE_OR_OPR8R_OPCODE :
				ret_code = execBitWiseOp (exprTknStream, opr8rIdx);
				break;

			case LOGICAL_AND_OPR8R_OPCODE :
				if (operand1.evalResolvedTokenAsIf() && operand2.evalResolvedTokenAsIf())
					exprTknStream[opr8rIdx] = *oneTkn;
				else
					exprTknStream[opr8rIdx] = *zeroTkn;
				ret_code = OK;

				break;

			case LOGICAL_OR_OPR8R_OPCODE :
				if (operand1.evalResolvedTokenAsIf() || operand2.evalResolvedTokenAsIf())
					exprTknStream[opr8rIdx] = *oneTkn;
				else
					exprTknStream[opr8rIdx] = *zeroTkn;
				ret_code = OK;
				break;

			case ASSIGNMENT_OPR8R_OPCODE :
			case PLUS_ASSIGN_OPR8R_OPCODE :
			case MINUS_ASSIGN_OPR8R_OPCODE :
			case MULTIPLY_ASSIGN_OPR8R_OPCODE :
			case DIV_ASSIGN_OPR8R_OPCODE :
			case MOD_ASSIGN_OPR8R_OPCODE :
			case LEFT_SHIFT_ASSIGN_OPR8R_OPCODE :
			case RIGHT_SHIFT_ASSIGN_OPR8R_OPCODE :
			case BITWISE_AND_ASSIGN_OPR8R_OPCODE :
			case BITWISE_XOR_ASSIGN_OPR8R_OPCODE :
			case BITWISE_OR_ASSIGN_OPR8R_OPCODE :
				ret_code = execAssignmentOp (exprTknStream, opr8rIdx);
				break;
			default:
				break;
		}


		if (ret_code == OK)
			exprTknStream[opr8rIdx].isInitialized = true;

	} else	{
		userMessages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", thisSrcFile, __LINE__, 0);
	}

	return (ret_code);
}

/* ****************************************************************************
 * The [&&] OPR8R can be short-circuited if the 1st [operand|expression] can be 
 * resolved to FALSE - evaluating the 2nd [operand|expression] is redundant
 * ***************************************************************************/
int RunTimeInterpreter::execLogicalAndOp (std::vector<Token> & exprTknStream, int opr8rIdx)	{
	int ret_code = GENERAL_FAILURE;
	
	if (OK != execFlatExpr_OLR(exprTknStream, opr8rIdx + 1))	{
		// Resolve the 1st expression
		failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
	
	} else if (exprTknStream[opr8rIdx + 1].evalResolvedTokenAsIf())	{
		// 1st expression evaluated as TRUE
		bool is2ndTrue = false;

		if (OK != execFlatExpr_OLR(exprTknStream, opr8rIdx + 2))	{
			// Resolve the 2nd expression
			failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
		
		} else if (exprTknStream[opr8rIdx + 2].evalResolvedTokenAsIf())	{
			// 2nd expression evaluated as TRUE
			is2ndTrue = true;
		} 

		if (!failOnSrcLine)	{
		 	exprTknStream.erase(exprTknStream.begin() + opr8rIdx + 1, exprTknStream.begin() + opr8rIdx + 3);
			if (is2ndTrue)
				exprTknStream[opr8rIdx] = *oneTkn;
			else
				exprTknStream[opr8rIdx] = *zeroTkn;
		}

	} else {
		// 1st|Left [operand|expression] evaluated FALSE; short-circuit 2nd|Right side
		int lastIdxOfSubExpr;
		if (OK != getEndOfSubExprIdx (exprTknStream, opr8rIdx + 2, lastIdxOfSubExpr))	{
			failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
		} else	{
			if (lastIdxOfSubExpr == opr8rIdx + 1)
				exprTknStream.erase (exprTknStream.begin() + lastIdxOfSubExpr);
			else
				exprTknStream.erase(exprTknStream.begin() + opr8rIdx + 1, exprTknStream.begin() + lastIdxOfSubExpr + 1);
			exprTknStream[opr8rIdx] = *zeroTkn;

		}
	}

	if (!failOnSrcLine)	{
		exprTknStream[opr8rIdx].isInitialized = true;
		ret_code = OK;
	}

	return (ret_code);
}

/* ****************************************************************************
 * The [||] OPR8R can be short-circuited if the 1st [operand|expression] can be 
 * resolved to TRUE - evaluating the 2nd [operand|expression] is redundant
 * ***************************************************************************/
int RunTimeInterpreter::execLogicalOrOp (std::vector<Token> & exprTknStream, int opr8rIdx)	{
	int ret_code = GENERAL_FAILURE;
		bool is1RandTrue = false;

	if (OK != execFlatExpr_OLR(exprTknStream, opr8rIdx + 1))	{
		// Resolve the 1st expression
		failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
	
	} else {
		bool is1RandTrue = exprTknStream[opr8rIdx + 1].evalResolvedTokenAsIf();
		// Erase the 1st|Left operand; we've already resolved it and have the result
		exprTknStream.erase (exprTknStream.begin() + opr8rIdx + 1);

		if (is1RandTrue)	{
			// 1st|Left [operand|expression] evaluated TRUE; short-circuit 2nd|Right side

			int lastIdxOfSubExpr;
			if (OK != getEndOfSubExprIdx (exprTknStream, opr8rIdx + 1, lastIdxOfSubExpr))	{
				failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
			} else	{
				// Erase the 2nd|Right expression|operand
				if (lastIdxOfSubExpr == opr8rIdx + 1)
					exprTknStream.erase(exprTknStream.begin() + lastIdxOfSubExpr);
				else
					exprTknStream.erase(exprTknStream.begin() + opr8rIdx + 1, exprTknStream.begin() + lastIdxOfSubExpr + 1);
				exprTknStream[opr8rIdx] = *oneTkn;
			}
		
		} else {
			// Evaluate 2nd|Right [operand|expression] for final [TRUE|FALSE]
			bool is2ndTrue = false;

			if (OK != execFlatExpr_OLR(exprTknStream, opr8rIdx + 1))	{
				// Resolve the 2nd expression
				failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
			
			} else if (exprTknStream[opr8rIdx + 1].evalResolvedTokenAsIf())	{
				// 2nd expression evaluated as TRUE
				is2ndTrue = true;
			} 

			if (!failOnSrcLine)	{
				exprTknStream.erase(exprTknStream.begin() + opr8rIdx + 1);
				if (is2ndTrue)
					exprTknStream[opr8rIdx] = *oneTkn;
				else
					exprTknStream[opr8rIdx] = *zeroTkn;
			}
		}
	}

	if (!failOnSrcLine)	{
		exprTknStream[opr8rIdx].isInitialized = true;
		ret_code = OK;
	}

	return (ret_code);

}


/* ****************************************************************************
 * ? (ternaryConditional) (truePath) (falsePath)
 * Resolve the ? conditional; determine which of [TRUE|FALSE] path will be taken
 * and short-circuit the other path
 * ***************************************************************************/
int RunTimeInterpreter::execTernary1stOp(std::vector<Token> & exprTknStream, int opr8rIdx)     {
	int ret_code = GENERAL_FAILURE;

	if (OK != execFlatExpr_OLR(exprTknStream, opr8rIdx + 1))	{
		// Resolve the conditional
		failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
	
	} else {
		illustrativeB4op (exprTknStream, opr8rIdx);

		bool isTernaryConditionTrue = exprTknStream[opr8rIdx + 1].evalResolvedTokenAsIf();
		// Remove TERNARY_1ST and conditional result from list
		exprTknStream.erase(exprTknStream.begin() + opr8rIdx, exprTknStream.begin() + opr8rIdx + 2);
		int lastIdxOfSubExpr;

		if (isTernaryConditionTrue)	{
			if (exprTknStream[opr8rIdx].tkn_type == EXEC_OPR8R_TKN && OK != execFlatExpr_OLR(exprTknStream, opr8rIdx))	
				// Must resolve the TRUE path 
				failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
		
			if (!failOnSrcLine && OK != getEndOfSubExprIdx (exprTknStream, opr8rIdx + 1, lastIdxOfSubExpr))
				// Determine the end of the FALSE path
				failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;

			else if (!failOnSrcLine)	{
				// Short-circuit the FALSE path
				if (lastIdxOfSubExpr == opr8rIdx + 1)
					exprTknStream.erase(exprTknStream.begin() + lastIdxOfSubExpr);
				else
					exprTknStream.erase(exprTknStream.begin() + opr8rIdx + 1, exprTknStream.begin() + lastIdxOfSubExpr + 1);
			}

		} else {
			// Determine the end of the TRUE path sub-expression. NOTE: [?] and [conditional] have already been removed
			if (OK != getEndOfSubExprIdx (exprTknStream, opr8rIdx, lastIdxOfSubExpr))
				failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
			else	{
				// Short-circuit the TRUE path
				exprTknStream.erase(exprTknStream.begin() + opr8rIdx, exprTknStream.begin() + lastIdxOfSubExpr + 1);
				if (exprTknStream[opr8rIdx].tkn_type == EXEC_OPR8R_TKN)	{
					if (OK != execFlatExpr_OLR(exprTknStream, opr8rIdx))
						// Resolving the FALSE path failed
						failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
				}
			}
		}
	}

	if (!failOnSrcLine)	{
		illustrativeAfterOp(exprTknStream);
		ret_code = OK;
	}

	return (ret_code);
}

/* ****************************************************************************
 * Figure out where this sub-expression ends by tallying up OPR8Rs and operands.
 * The stream of Tokens that represent the expression follow [OPR8R][1][2] where
 * [1] and [2] are expressions. If either starts with something that can be
 * evaluated right away (e.g. NOT an EXEC_OPR8R_TKN), then there's no nested
 * expressions that need to be resolved 1st. Since we know how many operands each
 * OPR8R requires, we can determine when all the nested expressions have bottomed
 * out.  This fxn allows us to short circuit sub-expressions that don't need to
 * be evaluated.
 * ***************************************************************************/
int RunTimeInterpreter::getEndOfSubExprIdx (std::vector<Token> & exprTknStream, int startIdx, int & lastIdxOfSubExpr)	{
	int ret_code = GENERAL_FAILURE;

	int foundRandCnt = 0;
	int requiredRandCnt = 0;
	int idx;
	lastIdxOfSubExpr = -1;
	std::vector<int> opr8rReqStack;
	std::vector<int> operandStack;

/* 	if (startIdx >= 0 && startIdx < exprTknStream.size() && exprTknStream[startIdx].tkn_type != EXEC_OPR8R_TKN)
		// Starting off with a variable, so this sub-expression is ALREADY complete
		lastIdxOfSubExpr = startIdx;
 */
	for (idx = startIdx; idx < exprTknStream.size() && !failOnSrcLine && lastIdxOfSubExpr == -1; idx++)	{
		Token currTkn = exprTknStream[idx];

		if (currTkn.tkn_type == EXEC_OPR8R_TKN)	{
			// Get details on this OPR8R to determine how it affects resolvedRandCnt
			Operator opr8rDeets;
			if (OK != execTerms.getExecOpr8rDetails(currTkn._unsigned, opr8rDeets))	{
				failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;

			} else  if (opr8rDeets.type_mask & TERNARY_2ND)	{
				// TERNARY_2ND was *NOT* expected!
				failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;

			} else	{
				opr8rReqStack.push_back(opr8rDeets.numReqExecOperands);
			}
		} else {
			// TODO: Will have to deal with fxn calls at some point
			operandStack.push_back(1);

			bool isAllOpsDone = false;

			while (!isAllOpsDone)	{
				if (opr8rReqStack.empty())	{
					isAllOpsDone = true;
				
				} else	{
					int topIdx = opr8rReqStack.size() - 1;
					int numReqRands = opr8rReqStack[topIdx];

					if (operandStack.size() >= numReqRands)	{
						// Remove the OPR8R
						opr8rReqStack.pop_back();
						for (int edx = 0; edx < numReqRands; edx++)	{
							// Remove Operands associated with this OPR8R
							operandStack.pop_back();
						}
						// Need a RESULT Operand marker put back
						operandStack.push_back(1);
					
					} else {
						isAllOpsDone = true;
					}
				}
			}

			if (opr8rReqStack.empty())	{
				if (operandStack.size() == 1)
					lastIdxOfSubExpr = idx;
				else
				 	failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
			} 
		}
	}

	if (lastIdxOfSubExpr >= 0 && lastIdxOfSubExpr < exprTknStream.size())	{
		ret_code = OK;
	}

	return (ret_code);
}

/* ****************************************************************************
 * TODO: Might need to differentiate between compile and interpret mode.  Depends
 * on where|when I'm doing final type and other bounds checking.
 * ***************************************************************************/
int RunTimeInterpreter::execOperation (Operator opr8r, int opr8rIdx, std::vector<Token> & flatExprTkns)	{
	int ret_code = GENERAL_FAILURE;

	if (opr8r.op_code == POST_INCR_OPR8R_OPCODE || opr8r.op_code == POST_DECR_OPR8R_OPCODE 
		|| opr8r.op_code == PRE_INCR_OPR8R_OPCODE || opr8r.op_code == PRE_DECR_OPR8R_OPCODE)	{
		ret_code = execPrePostFixOp (flatExprTkns, opr8rIdx);

	} else if ((opr8r.type_mask & UNARY) && opr8r.numReqExecOperands == 1)	{
		ret_code = execUnaryOp (flatExprTkns, opr8rIdx); 
		if (OK != ret_code)
			userMessages->logMsg (INTERNAL_ERROR, L"Failed executing UNARY OPR8R [" + opr8r.symbol + L"]"
				, thisSrcFile, __LINE__, 0);

	} else if (opr8r.type_mask & TERNARY_2ND)	{
		// TODO: Is this an error condition? Or should there be a state machine or something?
		userMessages->logMsg (INTERNAL_ERROR, L"", thisSrcFile, __LINE__, 0);

	} else if ((opr8r.type_mask & BINARY) && opr8r.numReqExecOperands == 2)	{
		ret_code = execBinaryOp (flatExprTkns, opr8rIdx);
	} else	{
		userMessages->logMsg (INTERNAL_ERROR, L"", thisSrcFile, __LINE__, 0);
	}

	return (ret_code);

}

/* ****************************************************************************
 * For ILLUSTRATIVE display purposes. Before performing an operation, show the 
 * current Token list with a caret [^] below that line showing which OPR8R will
 * be executed next.
 * ***************************************************************************/
 void RunTimeInterpreter::illustrativeB4op (std::vector<Token> & flatExprTkns, int opr8rIdx)	{
	int caretPos = -1;
	std::wstring tmpStr;

	if (isIllustrative && usageMode == INTERPRETER && logLevel >= ILLUSTRATIVE)	{
		if (tknsIllustrativeStr.empty())	{
			tknsIllustrativeStr = util.getTokenListStr(flatExprTkns, opr8rIdx, caretPos);
			std::wcout << tknsIllustrativeStr << std::endl;
			caretPos >= 0 ? caretPos++ : 1;

		} else {
			// Calc caret pos on Token list displayed after previous operation
			int nxtBrktPos = -1;
			int brktCnt = 0;
			int currPos = 0;
			
			while (caretPos < 0)	{
				nxtBrktPos = tknsIllustrativeStr.find(L"[", currPos);
				if (nxtBrktPos != std::string::npos)	{
					brktCnt++;
					currPos = nxtBrktPos + 1;
				}
				else	{
				 	break;
				}
				
				if (brktCnt == opr8rIdx + 1)
					caretPos = nxtBrktPos + 1;
			}
		}

		if (caretPos >= 0)	{
			// Put the ^ underneath the target OPR8R from the previous line
			tmpStr.insert (0, caretPos, ' ');
			tmpStr.append(L"^");
			if (opr8rIdx >= 0 && opr8rIdx < flatExprTkns.size())	{
				Operator opr8r;
				if (OK == execTerms.getExecOpr8rDetails (flatExprTkns[opr8rIdx]._unsigned, opr8r))	{
					int randCnt = opr8r.numReqExecOperands;
					tmpStr.append (L" takes next ");
					tmpStr.append (std::to_wstring(randCnt));
					tmpStr.append (L" operand");
					if (randCnt != 1)
						tmpStr.append(L"s");

					if (opr8r.op_code == TERNARY_1ST_OPR8R_OPCODE)	{
						tmpStr.append (L"; [Conditional][TRUE path][FALSE path]");
					}

					if (!opr8r.description.empty())	{
						tmpStr.append (L" (" + opr8r.description + L")");
					}
				}
			}
			std::wcout << tmpStr << std::endl;
		}
	}
}

/* ****************************************************************************
 * 
 * ***************************************************************************/
 void RunTimeInterpreter::illustrativeAfterOp (std::vector<Token> & flatExprTkns)	{

	if (isIllustrative && usageMode == INTERPRETER && logLevel >= ILLUSTRATIVE)	{
		int caretPos = 0;
		std::wstring prevStr = tknsIllustrativeStr;
		tknsIllustrativeStr = util.getTokenListStr(flatExprTkns, 0, caretPos);

		if (tknsIllustrativeStr != prevStr)	{
			if (flatExprTkns.size() == 1)
				tknsIllustrativeStr.append(L" expression resolved");
			std::wcout << tknsIllustrativeStr << std::endl;
		}
	}
}
/* ****************************************************************************
 * Fxn to evaluate expressions.  
 * NOTE the startIdx parameter. Fxn can also be called recursively to resolve
 * sub-expressions further down the expression stream. Useful for resolving the
 * TERNARY_1ST conditional, and short-circuiting either the TERNARY [TRUE|FALSE]
 * paths after the conditional is resolved.  Also useful for short-circuiting
 * the 2nd expression in the [&&] and [||] OPR8Rs.
 * ***************************************************************************/
int RunTimeInterpreter::execFlatExpr_OLR(std::vector<Token> & flatExprTkns, int startIdx)     {
	int ret_code = GENERAL_FAILURE;
	bool is1RandLeft = false;
	int prevTknCnt = flatExprTkns.size();
	int currTknCnt;
	int numSeqRandsFnd = 0;
	bool isSubExprComplete = false;

	if (startIdx >= prevTknCnt)	{
		// TODO: Great opportunity to use InfoWarnError!
		failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
		userMessages->logMsg (INTERNAL_ERROR, L"Parameter startIdx goes beyond Token stream", thisSrcFile, failOnSrcLine, 0);

	} else if (flatExprTkns[startIdx].tkn_type != EXEC_OPR8R_TKN)	{
		// 1st thing we hit is an Operand, so we're done! 
		isSubExprComplete = true;

	} else 	{
		int opr8rCnt = 0;
		bool isOutOfTkns = false;
		int caretPos;

		while (!isOutOfTkns && !failOnSrcLine && !isSubExprComplete)	{
			prevTknCnt = flatExprTkns.size();
			bool isOneOpr8rDone = false;
			int currIdx = startIdx;

			while (!isOutOfTkns && !isOneOpr8rDone && !failOnSrcLine)	{
				if (currIdx >= flatExprTkns.size() - 1)
					isOutOfTkns = true;
				
				if (!isOutOfTkns && flatExprTkns[currIdx].tkn_type == EXEC_OPR8R_TKN) {
					Operator opr8r;
					if (OK != execTerms.getExecOpr8rDetails (flatExprTkns[currIdx]._unsigned, opr8r))	{
						failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
					
					} else {
						// TODO: Add short-circuiting in for [&&] and [||]
						
						if (opr8r.op_code == TERNARY_1ST_OPR8R_OPCODE)	{
							// [?] is a special case, AND there is the potential for short-circuiting
							isOneOpr8rDone = true;
						
							if (OK != execTernary1stOp(flatExprTkns, currIdx))
								failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;

						} else if (opr8r.op_code == LOGICAL_AND_OPR8R_OPCODE)	{
							// Special case this OPR8R in case there is short circuiting
							isOneOpr8rDone = true;
							if (OK != execLogicalAndOp(flatExprTkns, currIdx))
								failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;

						} else if (opr8r.op_code == LOGICAL_OR_OPR8R_OPCODE)	{
							// Special case this OPR8R in case there is short circuiting
							isOneOpr8rDone = true;
							if (OK != execLogicalOrOp(flatExprTkns, currIdx))
								failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;

						} else {
							// If next sequential [numReqRands] in stream are Operands, OPR8R can be executed
							int numRandsReq = opr8r.numReqExecOperands;
							if (currIdx + numRandsReq >= flatExprTkns.size())
								failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
							
							int numRandsFnd;
							for (numRandsFnd = 0; !failOnSrcLine && numRandsFnd < numRandsReq;) {
								if (flatExprTkns[(currIdx + 1) + numRandsFnd].tkn_type == EXEC_OPR8R_TKN)
									break;
								else
									numRandsFnd++;
							}

							if (!failOnSrcLine && numRandsFnd == numRandsReq)	{
								// EXECUTE THE OPR8R!!!
								isOneOpr8rDone = true;
								illustrativeB4op (flatExprTkns, currIdx);
								// TODO: Build a string that displays the Tokens and send it out if different from prev

								// Make a string with a caret to point to the OPR8R that's going to get executed....search for N "[" for placement
								if (OK == execOperation (opr8r, currIdx, flatExprTkns))	{
									// Operation result stored in Token that previously held the OPR8R. We need to delete any associatd operands
									flatExprTkns.erase(flatExprTkns.begin() + currIdx + 1, flatExprTkns.begin() + currIdx + numRandsReq + 1);
									// TODO: Build a string to display remaining Tokens
									illustrativeAfterOp (flatExprTkns);
								} else	{
									failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
								}
							}
						}
					}
				} 

				if (!isOneOpr8rDone && !isOutOfTkns && !failOnSrcLine)	{
					currIdx++;
					if (currIdx > flatExprTkns.size())
						failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
				}
			}

			if (!failOnSrcLine && currIdx == startIdx)	{
				// We completed the sub-expression we were tasked with
				isSubExprComplete = true;
			
			} else if (!failOnSrcLine && prevTknCnt <= flatExprTkns.size())	{
				// Inner while made ZERO forward progress.....time to quit and think about what we havn't done
				failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
				std::wstring devMsg = L"Inner while did NOT make forward progress; startIdx = ";
				devMsg.append(std::to_wstring(startIdx));
				devMsg.append (L"; currIdx = ");
				devMsg.append (std::to_wstring(currIdx));
				devMsg.append (L";");
				userMessages->logMsg (INTERNAL_ERROR, devMsg, thisSrcFile, failOnSrcLine, 0);
				util.dumpTokenList (flatExprTkns, execTerms, thisSrcFile, __LINE__);
			}
		}
	}

	if (isSubExprComplete && !failOnSrcLine && flatExprTkns[startIdx].tkn_type == USER_WORD_TKN)	{
		// SUCCESS IFF we can resolve this variable from our NameSpace to its final value
		Token resolvedTkn;
		std::wstring lookUpMsg;
		if (OK == scopedNameSpace->findVar(flatExprTkns[startIdx]._string, 0, resolvedTkn, READ_ONLY, lookUpMsg))	{
			flatExprTkns[startIdx] = resolvedTkn;
			flatExprTkns[startIdx].isInitialized = true;
			ret_code = OK;
		
		} else if (!lookUpMsg.empty())	{
			userMessages->logMsg(INTERNAL_ERROR, lookUpMsg, thisSrcFile, __LINE__, 0);
		}
	} else if (isSubExprComplete && !failOnSrcLine && flatExprTkns[startIdx].tkn_type != EXEC_OPR8R_TKN)	{
		if (flatExprTkns[startIdx].isDirectOperand())
			flatExprTkns[startIdx].isInitialized = true;
		ret_code = OK;
	}

	if (ret_code != OK && !failOnSrcLine)	{
		userMessages->logMsg (INTERNAL_ERROR, L"Unhandled error!", thisSrcFile, __LINE__, 0);
		// TODO: See the final result
		util.dumpTokenList (flatExprTkns, execTerms, thisSrcFile, __LINE__);
	}

	return (ret_code);
}

/* ****************************************************************************
 * Publicly facing fxn
 * ***************************************************************************/
int RunTimeInterpreter::resolveFlatExpr(std::vector<Token> & flatExprTkns)     {
	int ret_code = GENERAL_FAILURE;

	if (0 == flatExprTkns.size())	{
		userMessages->logMsg (INTERNAL_ERROR, L"Token stream unexpectedly EMPTY!", thisSrcFile, __LINE__, 0);
	
	} else	{
		// util.dumpTokenList(flatExprTkns, execTerms, thisSrcFile, __LINE__);
		tknsIllustrativeStr.clear();
		ret_code = execFlatExpr_OLR(flatExprTkns, 0);
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::resolveTknOrVar (Token & originalTkn, Token & resolvedTkn, std::wstring & varName, bool isCheckInit)	{
	int ret_code = GENERAL_FAILURE;

	if (originalTkn.tkn_type == USER_WORD_TKN)	{
		varName = originalTkn._string;
		std::wstring lookUpMsg;
		if (OK != scopedNameSpace->findVar(varName, 0, resolvedTkn, READ_ONLY, lookUpMsg))	{
			userMessages->logMsg(INTERNAL_ERROR, lookUpMsg, thisSrcFile, __LINE__, 0);

		} else	{
			if (isCheckInit && usageMode == COMPILE_TIME && !resolvedTkn.isInitialized)	{
				if (varName == L"countHordes")
					std::wcout << L"STOP" << std::endl;
				userMessages->logMsg(WARNING, L"Uninitialized variable used - " + originalTkn.descr_sans_line_num_col()
					, userSrcFileName, originalTkn.get_line_number(), originalTkn.get_column_pos());
			}
			ret_code = OK;
		}
	} else	{
		resolvedTkn = originalTkn;
		ret_code = OK;
	}


	return (ret_code);

}


/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::resolveTknOrVar (Token & originalTkn, Token & resolvedTkn, std::wstring & varName)	{
	return resolveTknOrVar(originalTkn, resolvedTkn, varName, true);
}

/* ****************************************************************************
 * Encountered the IF_SCOPE_OPCODE. Evaluate the conditional to determine if 
 * the enclosed block will be executed.  Check for follow on [else if] and|or
 * [else] blocks at the same scope. We'll need to know where the scope that
 * encloses this [if] block ends.
 * [op_code][total_length][conditional EXPRESSION][code block]
 * TODO: Probably need to get the enclosing scope's end pos
 * ***************************************************************************/
int RunTimeInterpreter::execIfBlock (uint32_t scopeStartPos, uint32_t if_scope_len
	, uint32_t afterParentScopePos)	{
	
	int ret_code = GENERAL_FAILURE;

	bool chainedElseBlocksDone = false;
	bool isCondPrevTrue = false;
	uint32_t posAfterElseScope = 0;
	uint8_t curr_obj_op_code = INVALID_OPCODE;
	uint32_t currObjStartPos = 0;
	uint32_t currObjLen = 0;
	uint32_t nxtObjStartPos = 0;
	Token ifConditional;

	if (OK != execExpression(scopeStartPos + OPCODE_NUM_BYTES + NUM_BYTES_IN_DWORD, ifConditional))	{
		failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
	
	} else if (ifConditional.evalResolvedTokenAsIf())	{
		// [if] condition is TRUE, so execute the code block
		isCondPrevTrue = true;
		if (OK != execCurrScope(fileReader.getReadFilePos(), scopeStartPos + if_scope_len))
			failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;


	} else {
		// TODO: check enclosing scope's end
		// [if] condition is FALSE, so jump around the [if] block
		fileReader.setFilePos(scopeStartPos + if_scope_len);
	}

	if (!failOnSrcLine)	{
		bool isSkipBlock;

		while (!chainedElseBlocksDone && !failOnSrcLine)	{
			// Consume any [else if][else] blocks after our initial [if] block
			currObjStartPos = fileReader.getReadFilePos();
			isSkipBlock = false;

			if (currObjStartPos == afterParentScopePos)	{
				// We've just gone past the parent scope that contains the [if] block
				// and any chained [else if]+, [else] blocks
				chainedElseBlocksDone = true;

			} else if (OK != fileReader.peekNextByte(curr_obj_op_code))	{
				failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;

			} else if (curr_obj_op_code != ELSE_IF_SCOPE_OPCODE && curr_obj_op_code != ELSE_SCOPE_OPCODE)	{
				chainedElseBlocksDone = true;

			} else 	{
				if (OK != fileReader.readNextByte(curr_obj_op_code))	
					failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
				else if (OK != fileReader.readNextDword(currObjLen))
					failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;

				if (!failOnSrcLine && curr_obj_op_code == ELSE_IF_SCOPE_OPCODE)	{
					isSkipBlock = isCondPrevTrue;

					if (!isSkipBlock)	{
						// Test the conditional
						Token elseIfConditional;
						if (OK != execExpression(currObjStartPos + OPCODE_NUM_BYTES + NUM_BYTES_IN_DWORD, elseIfConditional))	{
							failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
						
						} else if (elseIfConditional.evalResolvedTokenAsIf())	{
							// [else if] condition is TRUE, so execute the code block
							isCondPrevTrue = true;
							if (OK != execCurrScope(fileReader.getReadFilePos(), currObjStartPos + currObjLen))
								failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
						
						} else	{
							isSkipBlock = true;
						}
					}
				
				} else if (!failOnSrcLine && curr_obj_op_code == ELSE_SCOPE_OPCODE)	{
					chainedElseBlocksDone = true;
					if (isCondPrevTrue)	{
						isSkipBlock = true;
					
					} else {
						if (OK != execCurrScope(fileReader.getReadFilePos(), currObjStartPos + currObjLen))
							failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;
					}
				}
			}

			if (!failOnSrcLine && isSkipBlock)	{
				nxtObjStartPos = currObjStartPos + currObjLen;
				if (nxtObjStartPos >= afterParentScopePos)	
					chainedElseBlocksDone = true;
				if (OK != fileReader.setFilePos(nxtObjStartPos))
					// TODO: What about EOF? Or past scope?
					failOnSrcLine = failOnSrcLine == 0 ? __LINE__ : failOnSrcLine;

			}
		}
	}

	if (!failOnSrcLine && chainedElseBlocksDone)
		ret_code = OK;

	uint32_t finalFilePos = fileReader.getReadFilePos();

	return (ret_code);
}
