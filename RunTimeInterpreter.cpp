/*
 * RunTimeInterpreter.cpp
 *
 *  Created on: Jan 4, 2024
 *      Author: Mike Volk
 *
 * TODO:
 * POST_DECR_OPR8R_OPCODE
 * POST_INCR_OPR8R_OPCODE
 * PRE_DECR_OPR8R_OPCODE
 * PRE_INCR_OPR8R_OPCODE
 * STATEMENT_ENDER_OPR8R_OPCODE - Is there really anything to be done here? Not currently using it.
 * POST_INCR_NO_OP_OPCODE
 * POST_DECR_NO_OP_OPCODE
 * PRE_INCR_NO_OP_OPCODE
 * PRE_DECR_NO_OP_OPCODE
 *
 * Error handling on mismatched data types
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
#include "OpCodes.h"
#include "Token.h"
#include "TokenCompareResult.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "InfoWarnError.h"
#include "UserMessages.h"
#include "VariablesScope.h"
#include "common.h"

/* ****************************************************************************
 *
 * ***************************************************************************/
RunTimeInterpreter::RunTimeInterpreter() {
	// TODO Auto-generated constructor stub
  oneTkn = std::make_shared<Token> (INT64_TKN, L"1");
  // TODO: Token value not automatically filled in currently
  oneTkn->_signed = 1;
  zeroTkn = std::make_shared<Token> (INT64_TKN, L"0");
  zeroTkn->_signed = 0;
	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");
	// The no parameter constructor should never get called
	assert (0);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
RunTimeInterpreter::RunTimeInterpreter(CompileExecTerms & execTerms, std::shared_ptr<VariablesScope> inVarScopeStack
		, std::wstring userSrcFileName, std::shared_ptr<UserMessages> userMessages) {
	// TODO Auto-generated constructor stub
  oneTkn = std::make_shared<Token> (INT64_TKN, L"1");
  // TODO: Token value not automatically filled in currently
  oneTkn->_signed = 1;
  zeroTkn = std::make_shared<Token> (INT64_TKN, L"0");
  zeroTkn->_signed = 0;
  this->execTerms = execTerms;
	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");
	varScopeStack = inVarScopeStack;
	this->userMessages = userMessages;
	this->userSrcFileName = userSrcFileName;
	usageMode = COMPILE_TIME_CHECKING;
}

/* ****************************************************************************
 *
 * ***************************************************************************/
RunTimeInterpreter::RunTimeInterpreter(std::string interpretedFileName, std::wstring userSrcFileName
	, std::shared_ptr<VariablesScope> inVarScope,  std::shared_ptr<UserMessages> userMessages)
		: execTerms ()
		, fileReader (interpretedFileName, execTerms)	{
	// TODO Auto-generated constructor stub
  oneTkn = std::make_shared<Token> (INT64_TKN, L"1");
  // TODO: Token value not automatically filled in currently
  oneTkn->_signed = 1;
  zeroTkn = std::make_shared<Token> (INT64_TKN, L"0");
  zeroTkn->_signed = 0;
	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");
	varScopeStack = inVarScope;
	this->userMessages = userMessages;
	this->userSrcFileName = userSrcFileName;
	usageMode = RUN_TIME_EXECUTION;

}

/* ****************************************************************************
 *
 * ***************************************************************************/
RunTimeInterpreter::~RunTimeInterpreter() {
	// TODO Auto-generated destructor stub
	oneTkn.reset();
	zeroTkn.reset();
}

/* ****************************************************************************
 * Analogous to GeneralParser::rootScopeCompile.  
 * Parse through the user's interpreted file, looking for objects such as:
 * variable declarations
 * Expressions
 * [if] 
 * [else if]
 * [else] 
 * [while] 
 * [for] 
 * [fxn declaration]
 * [fxn call]
 * ***************************************************************************/
int RunTimeInterpreter::rootScopeExec()	{
	int ret_code = GENERAL_FAILURE;
	uint8_t	op_code;
	uint32_t objStartPos;
	uint32_t objectLen;
	bool isFailed = false;
	bool isDone = false;
	std::wstringstream hexStream;

	if (usageMode == RUN_TIME_EXECUTION)	{

		while (!isDone && !isFailed)	{
			objStartPos = fileReader.getReadFilePos();

			if (fileReader.isEOF())
				isDone = true;

			else if (OK != fileReader.peekNextByte(op_code))
				// TODO: isEOF doesn't seem to work. What's the issue?
				isDone = true;

			else if (OK != fileReader.readNextByte (op_code))	{
				isFailed = true;
			
			} else if (op_code >= FIRST_VALID_FLEX_LEN_OPCODE && op_code <= LAST_VALID_FLEX_LEN_OPCODE)		{
				if (OK != fileReader.readNextDword (objectLen))	{
					isFailed = true;
					hexStream.str(L"");
					hexStream << L"0x" << std::hex << op_code;
					std::wstring msg = L"Failed to get length of object (opcode = ";
					msg.append(hexStream.str());
					msg.append(L") starting at ");
					hexStream.str(L"");
					hexStream << L"0x" << std::hex << objStartPos;
					msg.append(hexStream.str());
					userMessages->logMsg(INTERNAL_ERROR, msg, thisSrcFile, __LINE__, 0);

				} else {
					if (op_code == VARIABLES_DECLARATION_OPCODE)	{
						if (OK != execVarDeclaration (objStartPos, objectLen))	{
							isFailed = true;
						}

					} else if (op_code == EXPRESSION_OPCODE)	{			
						if (OK != fileReader.setFilePos(objStartPos))	{
							// Follow on fxn expects to start at beginning of expression
							// TODO: Standardize on this?
							isFailed = true;
							hexStream.str(L"");
							hexStream << L"0x" << std::hex << objStartPos;
							userMessages->logMsg(INTERNAL_ERROR
								, L"Failed to reset interpreter file position to " + hexStream.str(), thisSrcFile, __LINE__, 0);

						} else {
							std::vector<Token> exprTkns;
							if (OK != fileReader.readExprIntoList(exprTkns))	{
								isFailed = true;
								hexStream.str(L"");
								hexStream << L"0x" << std::hex << op_code;
								userMessages->logMsg(INTERNAL_ERROR
									, L"Failed to retrieve expression starting at " + hexStream.str(), thisSrcFile, __LINE__, 0);
							
							}	else 	{
								if (OK != resolveFlatExpr(exprTkns)) {
									isFailed = true;
								}
							}
						}
					} else if (op_code == IF_BLOCK_OPCODE)	{									
							isFailed = true;
							userMessages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", thisSrcFile, __LINE__, 0);

					} else if (op_code == ELSE_IF_BLOCK_OPCODE)	{						
							isFailed = true;
							userMessages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", thisSrcFile, __LINE__, 0);

					} else if (op_code == ELSE_BLOCK_OPCODE)	{								
							isFailed = true;
							userMessages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", thisSrcFile, __LINE__, 0);

					} else if (op_code == WHILE_LOOP_OPCODE)	{								
							isFailed = true;
							userMessages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", thisSrcFile, __LINE__, 0);

					} else if (op_code == FOR_LOOP_OPCODE)	{									
							isFailed = true;
							userMessages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", thisSrcFile, __LINE__, 0);

					} else if (op_code == SCOPE_OPEN_OPCODE)	{								
							isFailed = true;
							userMessages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", thisSrcFile, __LINE__, 0);

					} else if (op_code == FXN_DECLARATION_OPCODE)	{					
							isFailed = true;
							userMessages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", thisSrcFile, __LINE__, 0);

					} else if (op_code == FXN_CALL_OPCODE)	{									
							isFailed = true;
							userMessages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", thisSrcFile, __LINE__, 0);

					} else if (op_code == SYSTEM_CALL_OPCODE)	{							
							isFailed = true;
							userMessages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", thisSrcFile, __LINE__, 0);

					} else {
						isFailed = true;
						hexStream.str(L"");
						hexStream << L"0x" << std::hex << op_code;
						std::wstring msg = L"Unknown opcode [";
						msg.append(hexStream.str());
						msg.append(L"] found at ");
						hexStream.str(L"");
						hexStream << L"0x" << std::hex << objStartPos;
						msg.append(hexStream.str());
						userMessages->logMsg(INTERNAL_ERROR, msg, thisSrcFile, __LINE__, 0);
					}
				}
			}
		}
	}

	if (isDone && !isFailed)
		ret_code = OK;

	return (ret_code);
}

/* ****************************************************************************
 * VARIABLES_DECLARATION_OPCODE		0x6F	
 * [op_code][total_length][datatype op_code][[string var_name][init_expression]]+
 * ***************************************************************************/
int RunTimeInterpreter::execVarDeclaration (uint32_t objStartPos, uint32_t objectLen)	{
	int ret_code = GENERAL_FAILURE;
	uint8_t	op_code;
	bool isDone = false;
	bool isFailed = false;
	std::wstringstream hexStrOpCode;
	std::wstringstream hexStrFilePos;
	std::wstring devMsg;
	uint32_t currObjStartPos;

	if (OK == fileReader.readNextByte(op_code))	{
		// Got the DATA_TYPE_[]_OPCODE
		TokenTypeEnum tknType = execTerms.getTokenTypeForOpCode (op_code);
		if (tknType == USER_WORD_TKN || !Token::isDirectOperand (tknType))	{
			hexStrOpCode.str(L"");
			hexStrOpCode << L"0x" << std::hex << op_code;
			std::wstring devMsg = L"Expected op_code that would resolve to a datatype but got ";
			devMsg.append(hexStrOpCode.str());
			devMsg.append (L" at file position ");
			hexStrFilePos.str(L"");
			hexStrFilePos << L"0x" << std::hex << fileReader.getReadFilePos();
			devMsg.append(hexStrFilePos.str());
			userMessages->logMsg(INTERNAL_ERROR, devMsg, thisSrcFile, __LINE__, 0);
			isFailed = true;

		} else {
			uint32_t pastLimitFilePos = objStartPos + objectLen;
			Token varNameTkn;
			std::vector<Token> tokenList;

			while (!isDone && !isFailed)	{
				varNameTkn.resetToString(L"");
				Token varTkn;
				varTkn.resetToken();
				varTkn.tkn_type = tknType;

				if (fileReader.isEOF())	{
					isDone = true;
				
				} else if (pastLimitFilePos <= (currObjStartPos = fileReader.getReadFilePos()))	{
					isDone = true;
				
				} else {
						hexStrFilePos.str(L"");
						hexStrFilePos << L"0x" << std::hex << currObjStartPos;

					if (OK != fileReader.readNextByte(op_code) || VAR_NAME_OPCODE != op_code)	{
						devMsg = L"Did not get expected VAR_NAME_OPCODE at file position " + hexStrFilePos.str();
						userMessages->logMsg(INTERNAL_ERROR, devMsg, thisSrcFile, __LINE__, 0);
						isFailed = true;
			
					} else if (OK != fileReader.readString (op_code, varNameTkn))	{
						devMsg = L"Failed reading variable name in declaration after file position " + hexStrFilePos.str();
						userMessages->logMsg(INTERNAL_ERROR, devMsg, thisSrcFile, __LINE__, 0);
						isFailed = true;

					} else if (!execTerms.isViableVarName(varNameTkn._string))	{
						devMsg = L"Variable name in declaration is invalid [" + varNameTkn._string + L"] after file position " + hexStrFilePos.str();
						userMessages->logMsg(INTERNAL_ERROR, devMsg, thisSrcFile, __LINE__, 0);
						isFailed = true;
					
					} else if (OK != varScopeStack->insertNewVarAtCurrScope(varNameTkn._string, varTkn))	{
							devMsg = L"Failed to insert variable into NameSpace [" + varNameTkn._string + L"] after file position " + hexStrFilePos.str();
							userMessages->logMsg(INTERNAL_ERROR, devMsg, thisSrcFile, __LINE__, 0);
							isFailed = true;
					
					} else if (OK != fileReader.peekNextByte(op_code))	{
							if (fileReader.isEOF())	{
								isDone = true;
							
							} else {
								userMessages->logMsg(INTERNAL_ERROR, L"Peek failed", thisSrcFile, __LINE__, 0);
								isFailed = true;
							}
					} else if (op_code == EXPRESSION_OPCODE)	{
						// If there's an initialization expression for this variable in the declaration, then resolve it
						// e.g. uint32 numFruits = 3 + 4, numVeggies = (3 * (1 + 2)), numPizzas = (4 + (2 * 3));
						//                         ^                    ^                         ^
						// Otherwise, go back to top of loop and look for the next variable name
						hexStrFilePos.str(L"");
						hexStrFilePos << L"0x" << std::hex << fileReader.getReadFilePos();

						std::vector<Token> exprTkns;
						if (OK != fileReader.readExprIntoList(exprTkns))	{
							isFailed = true;
							userMessages->logMsg(INTERNAL_ERROR, L"Failed to retrieve expression starting at " + hexStrFilePos.str(), thisSrcFile, __LINE__, 0);
						
						}	else if (OK != resolveFlatExpr(exprTkns)) {
							isFailed = true;
						
						} else if (exprTkns.size() != 1)	{
							// flattenedExpr should have 1 Token left - the result of the expression
							devMsg = L"Failed to resolve variable initialization expression for [" + varNameTkn._string + L"] after file position " + hexStrFilePos.str();
							userMessages->logMsg (INTERNAL_ERROR, devMsg, thisSrcFile, __LINE__, 0);
							isFailed = true;

						} else if (OK != varScopeStack->findVar(varNameTkn._string, 0, exprTkns[0], COMMIT_WRITE, userMessages))	{
							// Don't limit search to current scope
							userMessages->logMsg (INTERNAL_ERROR
									, L"After resolving initialization expression, could not find " + varNameTkn._string, thisSrcFile, __LINE__, 0);
							isFailed = true;
						}
					}
				}
			}
		}
	}

	if (isDone && !isFailed)
		ret_code = OK;

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execPrePostFixOp (std::vector<Token> & exprTknStream, int opr8rIdx)	{
	int ret_code = GENERAL_FAILURE;
	bool isSuccess = false;

	if (opr8rIdx >= 0 && exprTknStream.size() > (opr8rIdx + 1) && exprTknStream[opr8rIdx].tkn_type == EXEC_OPR8R_TKN)	{
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t op_code = exprTknStream[opr8rIdx]._unsigned;
		Operator opr8r;
		execTerms.getExecOpr8rDetails(op_code, opr8r);

		// Our operand Token could be a USER_WORD variable name, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		std::wstring varName1;
		resolveIfVariable (exprTknStream[opr8rIdx+1], operand1, varName1);

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
			ret_code = varScopeStack->findVar(varName1, 0, operand1, COMMIT_WRITE, userMessages);
		}

	} else	{
		userMessages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", thisSrcFile, __LINE__, 0);
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execEquivalenceOp(std::vector<Token> & exprTknStream, int opr8rIdx)     {
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;

	if (opr8rIdx >= 0 && exprTknStream.size() > (opr8rIdx + 2) && exprTknStream[opr8rIdx].tkn_type == EXEC_OPR8R_TKN)	{
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t op_code = exprTknStream[opr8rIdx]._unsigned;

		// Either|both of our operand Tokens could be USER_WORD variable names, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		Token operand2;
		std::wstring varName1;
		std::wstring varName2;
		resolveIfVariable (exprTknStream[opr8rIdx + 1], operand1, varName1);
		resolveIfVariable (exprTknStream[opr8rIdx + 2], operand2, varName2);

		TokenCompareResult compareRez = operand1.compare (operand2);

		switch (op_code)	{
			case LESS_THAN_OPR8R_OPCODE :
				if (compareRez.lessThan == isTrue)
					exprTknStream[opr8rIdx] = *oneTkn;
				else if (compareRez.lessThan == isFalse)
					exprTknStream[opr8rIdx] = *zeroTkn;
				else
					isFailed = true;
				break;
			case LESS_EQUALS_OPR8R8_OPCODE :
				if (compareRez.lessEquals == isTrue)
					exprTknStream[opr8rIdx] = *oneTkn;
				else if (compareRez.lessEquals == isFalse)
					exprTknStream[opr8rIdx] = *zeroTkn;
				else
					isFailed = true;
				break;
			case GREATER_THAN_OPR8R_OPCODE :
				if (compareRez.gr8rThan == isTrue)
					exprTknStream[opr8rIdx] = *oneTkn;
				else if (compareRez.gr8rThan == isFalse)
					exprTknStream[opr8rIdx] = *zeroTkn;
				else
					isFailed = true;
				break;
			case GREATER_EQUALS_OPR8R8_OPCODE :
				if (compareRez.gr8rEquals == isTrue)
					exprTknStream[opr8rIdx] = *oneTkn;
				else if (compareRez.gr8rEquals == isFalse)
					exprTknStream[opr8rIdx] = *zeroTkn;
				else
					isFailed = true;
				break;
			case EQUALITY_OPR8R_OPCODE :
				if (compareRez.equals == isTrue)
					exprTknStream[opr8rIdx] = *oneTkn;
				else if (compareRez.equals == isFalse)
					exprTknStream[opr8rIdx] = *zeroTkn;
				else
					isFailed = true;
				break;
			case NOT_EQUALS_OPR8R_OPCODE:
				if (compareRez.equals == isFalse)
					exprTknStream[opr8rIdx] = *oneTkn;
				else if (compareRez.equals == isTrue)
					exprTknStream[opr8rIdx] = *zeroTkn;
				else
					isFailed = true;
				break;
			default:
				isFailed = true;
				break;
		}

		if (!isFailed)
			ret_code = OK;
		else	{
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
 *
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
		resolveIfVariable (exprTknStream[opr8rIdx+1], operand1, varName1);
		resolveIfVariable (exprTknStream[opr8rIdx+2], operand2, varName2);

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

		if (isParamsValid && !isMissedCase && !isDivByZero)
			ret_code = OK;
		else if (isDivByZero)	{
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
 *
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
		resolveIfVariable (exprTknStream[opr8rIdx+1], operand1, varName1);
		resolveIfVariable (exprTknStream[opr8rIdx+2], operand2, varName2);

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
 *
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
		resolveIfVariable (exprTknStream[opr8rIdx+1], operand1, varName1);
		resolveIfVariable (exprTknStream[opr8rIdx+2], operand2, varName2);

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
 *
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
		resolveIfVariable (exprTknStream[opr8rIdx+1], operand1, varName1);
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
			if (operand1.isUnsigned())	{
				if (operand1._unsigned == 0)
					exprTknStream[opr8rIdx] = *oneTkn;
				else
					exprTknStream[opr8rIdx] = *zeroTkn;

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
			operand1._unsigned = ~(operand1._unsigned);
			exprTknStream[opr8rIdx] = operand1;
			ret_code = OK;
		}

		if (OK != ret_code)	{
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
 *
 * ***************************************************************************/
int RunTimeInterpreter::execAssignmentOp(std::vector<Token> & exprTknStream, int opr8rIdx)     {
	int ret_code = GENERAL_FAILURE;
	bool isSuccess = false;
	
	if (opr8rIdx >= 0 && exprTknStream.size() > (opr8rIdx + 2) && exprTknStream[opr8rIdx].tkn_type == EXEC_OPR8R_TKN)	{
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t original_op_code = exprTknStream[opr8rIdx]._unsigned;

		// Either|both of our operand Tokens could be USER_WORD variable names, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		Token operand2;
		std::wstring varName1;
		std::wstring varName2;
		resolveIfVariable (exprTknStream[opr8rIdx + 1], operand1, varName1);
		resolveIfVariable (exprTknStream[opr8rIdx + 2], operand2, varName2);

		bool isOpSuccess = false;

		if (varName1.empty())	{
			// TODO: operand1 *MUST* resolve to a variable that we can store a result to!

		} else	{
			switch (original_op_code)	{
				case ASSIGNMENT_OPR8R_OPCODE :
					if (OK == varScopeStack->findVar(varName1, 0, operand2, COMMIT_WRITE, userMessages))	{
						// We've updated the NS Variable Token; now overwrite the OPR8R with the result also
						exprTknStream[opr8rIdx] = operand2;
						isOpSuccess = true;
						ret_code = OK;
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
			if (original_op_code != ASSIGNMENT_OPR8R_OPCODE
					&& OK == varScopeStack->findVar(varName1, 0, exprTknStream[opr8rIdx], COMMIT_WRITE, userMessages))	{
				// Commit the result to the stored NS variable. OPR8R Token (previously @ opr8rIdx) has already been overwritten with result 
				ret_code = OK;
			}
		} else	{
			userMessages->logMsg (INTERNAL_ERROR, L"Failed executing [" + execTerms.getSrcOpr8rStrFor(original_op_code) + L"] operator!", thisSrcFile, __LINE__, 0);
		}
	}  else	{
		userMessages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", thisSrcFile, __LINE__, 0);
	}

	return (ret_code);
}

/* ****************************************************************************
 *
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
		resolveIfVariable (exprTknStream[opr8rIdx+1], operand1, varName1);
		resolveIfVariable (exprTknStream[opr8rIdx+2], operand2, varName2);

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

		if (ret_code != OK)
			userMessages->logMsg (INTERNAL_ERROR, L"Failed to execute OPR8R " + opr8r.symbol, thisSrcFile, __LINE__, 0);

	} else	{
		userMessages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", thisSrcFile, __LINE__, 0);
	}

	return (ret_code);
}

/* ****************************************************************************
 * Example expression with variable count -> 1
 * (1 + 2 * (3 + 4 * (5 + 6 * 7 * (count == 1 ? 10 : count == 2 ? 11 : count == 3 ? 12 : count == 4 ? 13 : 33))))
 *
 * [1] [2] [3] [4] [5] [6] [7] [*] [count] [1] [==] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                      ^^ 1st ^^^ ^^^ 2nd ^^^^^^^^
 * [1] [2] [3] [4] [5] [42] [1] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                      1st 2nd
 * [1] [2] [3] [4] [5] [42] [1] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                           ^^^1st - TRUE path; leave expression before [:] (e.g. [10]) in the stream and consume the FALSE path (1 complete sub-expression within)
 * [1] [2] [3] [4] [5] [42] [1] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                              numOperands: 1       2   1    1(?) - but need to consume this nested TERNARY - if this was a BINARY OPR8R (UNARY? POSTFIX? PREFIX? STATEMENT_ENDER?)
 *                                                                   then we'd be done with it. Consume everything up to and including the [:] OPR8R, then consume the next expression
 *
 * [1] [2] [3] [4] [5] [42] [1] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                                                             numOprands: 1       2   1    1(?) ^^^ ^^^  1       2   1   1(?) ^^^ ^^^ 1    ^ Not enough operands for this OPR8R, so
 *                                                                                                                                            we've closed off the nested TERNARYs
 *                                                                                                                                            and need to preserve these OPR8Rs
 *
 *
 *
 *
 * Example expression with variable count -> 1
 * (1 + 2 * (3 + 4 * (5 + 6 * 7 * (count == 1 ? 10 : count == 2 ? 11 : count == 3 ? 12 : count == 4 ? 13 : 33))))
 *
 * [1] [2] [3] [4] [5] [6] [7] [*] [count] [1] [==] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                      ^^ 1st ^^^ ^^^ 2nd ^^^^^^^^
 * [1] [2] [3] [4] [5] [42] [1] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                      1st 2nd
 *                           ^^^1st - TRUE path; leave expression before [:] (e.g. [10]) in stream and mark idx of [:] as 1st element to delete
 *
 * Find end idx of this expression
 * [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 * #rand 1   2    1 ^ Consume expression up to [:]
 *
 * Find end idx of this expression
 * [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 * #rand 1   2    1 ^ Consume expression up to [:]
 *
 * Find end idx of this expression
 * [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 * #rand 1   2    1 ^ Consume expression up to [:]
 *
 * Find end idx of this expression
 * [33] [*] [+] [*] [+] [*] [+]
 * #1   ^ Binary OPR8R - precedes a single element which satisfies a complete sub-expression. Mark the final idx and return
 * Consume complete sub-expression #2
 * ***************************************************************************/
int RunTimeInterpreter::takeTernaryTrue (std::vector<Token> & exprTknStream, int opr8rIdx)     {
	int ret_code = GENERAL_FAILURE;
	int ternary1stIdx = opr8rIdx;
	int lastIdxOfExpr = 0;

	if (ternary1stIdx >= 1 && ternary1stIdx < exprTknStream.size())	{
		// Guard against invalid parameters
		int ternary2ndIdx = findNextTernary2ndIdx (exprTknStream, ternary1stIdx);

		if (ternary2ndIdx > 0 && OK == getEndOfSubExprIdx (exprTknStream, ternary2ndIdx + 1, lastIdxOfExpr))	{
			if (lastIdxOfExpr > ternary2ndIdx && lastIdxOfExpr < exprTknStream.size())	{
				// Delete all Tokens that make up the TERNARY FALSE path
				exprTknStream.erase(exprTknStream.begin() + ternary2ndIdx, exprTknStream.begin() + lastIdxOfExpr + 1);

				// Also need to delete the original ternary conditional and the original ternary1st OPR8R,
				// but leave the expression in place for the TERNARY TRUE path
				exprTknStream.erase(exprTknStream.begin() + (ternary1stIdx - 1), exprTknStream.begin() + ternary1stIdx + 1);

				// After deletions, we need to adjust the caller's current idx, so point to
				// expression that starts directly AFTER ternary1stIdx, but account for the deletions
				// of the resolved conditional and TERNARY_1ST [1][?]
				// TODO: opr8rIdx = (ternary1stIdx + 1) - 2;
				ret_code = OK;
			}
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * Example expression with variable count -> 2
 * (1 + 2 * (3 + 4 * (5 + 6 * 7 * (count == 1 ? 10 : count == 2 ? 11 : count == 3 ? 12 : count == 4 ? 13 : 33))))
 *               This ternary conditional ^ will evaluate FALSE
 *
 * [1] [2] [3] [4] [5] [6] [7] [*] [count] [1] [==] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                     ^ this op 1 ^ and this op 2
 * [1] [2] [3] [4] [5] [42] [0] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                     ^ 1  ^2
 * [1] [2] [3] [4] [5] [42] [0] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                           ^ Ternary conditional is FALSE!
 *                           ^ Toss contents up to and including [:]
 *
 * [1] [2] [3] [4] [5] [42] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [5] [42] [1] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                           ^ Take true path, but we need to figure out where the false path ends
 *
 * ***************************************************************************/
int RunTimeInterpreter::takeTernaryFalse (std::vector<Token> & exprTknStream, int opr8rIdx)     {
	int ret_code = GENERAL_FAILURE;

	int ternary1stIdx = opr8rIdx;
	int idx;
	int ternary2ndIdx = findNextTernary2ndIdx (exprTknStream, ternary1stIdx);

	if (ternary2ndIdx > ternary1stIdx && ternary2ndIdx < exprTknStream.size())	{
		// Remove ternary conditional, TERNARY_1ST and up to and including TERNARY_2ND
		exprTknStream.erase(exprTknStream.begin() + (ternary1stIdx - 1), exprTknStream.begin() + ternary2ndIdx + 1);

		// Adjust opr8rIdx by accounting for deletion of [Conditional Operand][?]
		// TODO: opr8rIdx = ternary1stIdx - 2;
		ret_code = OK;
	}

	return (ret_code);
}

/* ****************************************************************************
 * (ternaryConditional) ? (truePath) : (falsePath)
 *          1st ternary ^ 2nd ternary^
 * ***************************************************************************/
int RunTimeInterpreter::execTernary1stOp(std::vector<Token> & exprTknStream, int opr8rIdx)     {
	int ret_code = GENERAL_FAILURE;

	if (opr8rIdx >= 1 && exprTknStream.size() >= 2)	{
		bool isTruePath = exprTknStream[opr8rIdx + 1].evalResolvedTokenAsIf();

		if (isTruePath)
			ret_code = takeTernaryTrue (exprTknStream, opr8rIdx);
		else
			ret_code = takeTernaryFalse (exprTknStream, opr8rIdx);
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::findNextTernary2ndIdx (std::vector<Token> & exprTknStream, int ternary1stOpIdx)     {
	int nextTernary2ndIdx = 0;
	int idx;

	for (idx = ternary1stOpIdx + 2; idx < exprTknStream.size() && nextTernary2ndIdx < ternary1stOpIdx; idx++)	{
		// Start at ternary1stOpIdx + 2: Must be at least 1 Token after ternary1stOpIdx for the TRUE path
		if (exprTknStream[idx].tkn_type == EXEC_OPR8R_TKN && exprTknStream[idx]._unsigned == TERNARY_2ND_OPR8R_OPCODE)	{
			nextTernary2ndIdx = idx;
			break;
		}
	}

	if (nextTernary2ndIdx == 0 && idx == exprTknStream.size())
		// Signal that we made it all the way to the end without finding TERNARY_2ND
		nextTernary2ndIdx = exprTknStream.size();

	return (nextTernary2ndIdx);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::getEndOfSubExprIdx (std::vector<Token> & exprTknStream, int startIdx, int & lastIdxOfSubExpr)	{
	int ret_code = GENERAL_FAILURE;

	int foundRandCnt = 0;
	int requiredRandCnt = 0;
	int idx;
	bool isFailed = false;
	lastIdxOfSubExpr = -1;

	for (idx = startIdx; idx < exprTknStream.size() && !isFailed && lastIdxOfSubExpr == -1; idx++)	{
		Token currTkn = exprTknStream[idx];

		if (currTkn.tkn_type == EXEC_OPR8R_TKN)	{
			// Get details on this OPR8R to determine how it affects resolvedRandCnt
			Operator opr8rDeets;
			if (OK != execTerms.getExecOpr8rDetails(currTkn._unsigned, opr8rDeets))	{
				isFailed = true;

			} else  if (opr8rDeets.type_mask & TERNARY_2ND)	{
				// TERNARY_2ND was *NOT* expected!
				isFailed = true;

			} else	{
				requiredRandCnt += opr8rDeets.numReqExecOperands;
			}
		} else {
			// TODO: Will have to deal with fxn calls at some point
			foundRandCnt++;
		}

		if (foundRandCnt == requiredRandCnt + 1)
			lastIdxOfSubExpr = idx;
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
			userMessages->logMsg (INTERNAL_ERROR, L"Failed executing UNARY OPR8R!", thisSrcFile, __LINE__, 0);

	} else if ((opr8r.type_mask & TERNARY_1ST) && opr8r.numReqExecOperands == 1)	{
		ret_code = execTernary1stOp (flatExprTkns, opr8rIdx);

	} else if (opr8r.type_mask & TERNARY_2ND)	{
		// TODO: Is this an error condition? Or should there be a state machine or something?
		userMessages->logMsg (INTERNAL_ERROR, L"", thisSrcFile, __LINE__, 0);

	} else if ((opr8r.type_mask & BINARY) && opr8r.numReqExecOperands == 2)	{
		ret_code = execBinaryOp (flatExprTkns, opr8rIdx);
		if (OK != ret_code)	
			userMessages->logMsg (INTERNAL_ERROR, L"Failure executing [" + opr8r.symbol + L"] operator.", thisSrcFile, __LINE__, 0);
	} else	{
		userMessages->logMsg (INTERNAL_ERROR, L"", thisSrcFile, __LINE__, 0);
	}

	return (ret_code);

}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execFlatExpr_OLR(std::vector<Token> & flatExprTkns, int startIdx)     {
	int ret_code = GENERAL_FAILURE;

	bool isFailed = false;
	bool is1RandLeft = false;
	int prevTknCnt = flatExprTkns.size();
	int currTknCnt;
	int numSeqRandsFnd = 0;

	if (startIdx >= prevTknCnt)	{
		// TODO: Great opportunity to use InfoWarnError!
		isFailed = true;
		userMessages->logMsg (INTERNAL_ERROR, L"Parameter startIdx goes beyond Token stream", thisSrcFile, __LINE__, 0);

	} else if (flatExprTkns[startIdx].tkn_type != EXEC_OPR8R_TKN)	{
		// 1st thing we hit is an Operand, so we're done! 
		ret_code = OK;

	} else 	{
		int opr8rCnt = 0;
		bool isOutOfTkns = false;
		bool isSubExprComplete = false;

		while (!isOutOfTkns && !isFailed && !isSubExprComplete)	{
			int prevTknCnt = flatExprTkns.size();
			bool isOneOpr8rDone = false;
			int currIdx = startIdx;

			while (!isOutOfTkns && !isOneOpr8rDone && !isFailed)	{
				if (currIdx >= flatExprTkns.size() - 1)
					isOutOfTkns = true;
				
				if (!isOutOfTkns && flatExprTkns[currIdx].tkn_type == EXEC_OPR8R_TKN) {
					Operator opr8r;
					if (OK != execTerms.getExecOpr8rDetails (flatExprTkns[currIdx]._unsigned, opr8r))	{
						isFailed = true;
					
					} else {
						// TODO: Add short-circuiting in for [&&] and [||]
						int numRandsReq = opr8r.numReqExecOperands;
						if (currIdx + numRandsReq >= flatExprTkns.size())	{
							isFailed = true;
						
						} else if (opr8r.op_code == TERNARY_1ST_OPR8R_OPCODE)	{
							// Resolve the ? conditional; determine if [TRUE|FALSE] path will be taken
							isOneOpr8rDone = true;
							if (OK != execFlatExpr_OLR(flatExprTkns, currIdx + 1))	{
								isFailed = true;
							
							} else {
								bool isTernaryConditionTrue = flatExprTkns[currIdx + 1].evalResolvedTokenAsIf();
								// Remove TERNARY_1ST and conditional result from list
								flatExprTkns.erase(flatExprTkns.begin() + currIdx, flatExprTkns.begin() + currIdx + 2);
								int lastIdxOfSubExpr;

								if (isTernaryConditionTrue)	{
									if (flatExprTkns[currIdx].tkn_type == EXEC_OPR8R_TKN && OK != execFlatExpr_OLR(flatExprTkns, currIdx))	{
										// Must resolve the TRUE path 
										isFailed = true;
									}
								
									if (!isFailed && OK != getEndOfSubExprIdx (flatExprTkns, currIdx + 1, lastIdxOfSubExpr))	{
										// Determine the end of the FALSE path
										isFailed = true;

									} else if (!isFailed)	{
										// Short-circuit the FALSE path
										flatExprTkns.erase(flatExprTkns.begin() + currIdx + 1, flatExprTkns.begin() + lastIdxOfSubExpr + 1);
										if (currIdx == startIdx)
											isSubExprComplete = true;
									}

								} else {
									// Determine the end of the TRUE path sub-expression
									if (OK != getEndOfSubExprIdx (flatExprTkns, currIdx, lastIdxOfSubExpr))
										isFailed = true;
									else	{
										// Short-circuit the TRUE path
										flatExprTkns.erase(flatExprTkns.begin() + currIdx, flatExprTkns.begin() + lastIdxOfSubExpr + 1);
										if (flatExprTkns[currIdx].tkn_type == EXEC_OPR8R_TKN && OK != execFlatExpr_OLR(flatExprTkns, currIdx))	{
											// Resolving the FALSE path failed
											isFailed = true;
										
										} else if (flatExprTkns[currIdx].tkn_type != EXEC_OPR8R_TKN && currIdx == startIdx)	{
											isSubExprComplete = true;
										}
									}
								}
							}
						} else {
							// If next sequential [numReqRands] in stream are Operands, OPR8R can be executed
							int numRandsFnd;
							for (numRandsFnd = 0; numRandsFnd < numRandsReq;) {
								if (flatExprTkns[(currIdx + 1) + numRandsFnd].tkn_type == EXEC_OPR8R_TKN)
									break;
								else
									numRandsFnd++;
							}

							if (numRandsFnd == numRandsReq)	{
								// EXECUTE THE OPR8R!!!
								isOneOpr8rDone = true;
								if (OK == execOperation (opr8r, currIdx, flatExprTkns))	{
									// Operation result stored in Token that previously held the OPR8R. We need to delete any associatd operands
									flatExprTkns.erase(flatExprTkns.begin() + currIdx + 1, flatExprTkns.begin() + currIdx + numRandsReq + 1);

									if (currIdx == startIdx)	{
										isSubExprComplete = true;
									}

								} else {
									isFailed = true;
								}
							}
						}
					}
				} 


				if (!isOutOfTkns && !isFailed && !isSubExprComplete)	{
					currIdx++;
					if (currIdx > flatExprTkns.size())
						isFailed = true;
				}
			}

			if (prevTknCnt <= flatExprTkns.size())	{
				// Inner while made ZERO forward progress.....time to quit and think about what we havn't done
				isFailed = true;
				// TODO: See the final result
				std::wcout << L"startIdx = " << startIdx << L"; currIdx = " << currIdx << L";" << std::endl;
				util.dumpTokenList (flatExprTkns, execTerms, thisSrcFile, __LINE__);
			}
		}
		
		if (isSubExprComplete && !isFailed && flatExprTkns[startIdx].tkn_type != EXEC_OPR8R_TKN)
			ret_code = OK;
	}

	if (ret_code != OK && !isFailed)	{
		userMessages->logMsg (INTERNAL_ERROR, L"Unhandled error!", thisSrcFile, __LINE__, 0);
		// TODO: See the final result
		util.dumpTokenList (flatExprTkns, execTerms, thisSrcFile, __LINE__);
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execFlatExpr_LRO(std::vector<Token> & flatExprTkns)     {
	int ret_code = GENERAL_FAILURE;

	bool isFailed = false;
	bool is1RandLeft = false;
	int prevTknCnt = flatExprTkns.size();
	int currTknCnt;
	int idx;
	int numOpr8rsFnd = 0;

	if (0 == prevTknCnt)	{
		// TODO: Great opportunity to use InfoWarnError!
		isFailed = true;
		userMessages->logMsg (INTERNAL_ERROR, L"Token stream unexpectedly EMPTY!", thisSrcFile, __LINE__, 0);

	} else 	{

		while (!isFailed && !is1RandLeft)	{
			prevTknCnt = flatExprTkns.size();

			int currIdx = 0;
			bool isEOLreached = false;
			int numSeqOperands = 0;

			while (!isFailed && !isEOLreached)	{
				Token nxtTkn = flatExprTkns[currIdx];

				if (prevTknCnt == 1)	{
					// TODO: If it's an Operand, then the Token list has been resolved
					// If it's an OPR8R, then something's BUSTICATED
					if (nxtTkn.isDirectOperand() || execTerms.isViableVarName(nxtTkn._string))
						is1RandLeft = true;
					else	{
						userMessages->logMsg (INTERNAL_ERROR, L"Last remaining Token must be an Operand but is " + nxtTkn.get_type_str() + L" [" + nxtTkn._string + L"]"
								, thisSrcFile, __LINE__, 0);
						isFailed = true;
					}
				}

				else if (nxtTkn.tkn_type != EXEC_OPR8R_TKN)	{
					numSeqOperands++;

				} else	{
					// How many operands does this OPR8R require?
					numOpr8rsFnd++;
					Operator opr8r_obj;

					if (OK != execTerms.getExecOpr8rDetails(nxtTkn._unsigned, opr8r_obj))	{
						userMessages->logMsg (INTERNAL_ERROR, L"", thisSrcFile, __LINE__, 0);
						isFailed = true;

					} else if (numSeqOperands >= opr8r_obj.numReqExecOperands)	{
						// Current OPR8R has what it needs, so do the operation
						// TODO: What about PREFIX and POSTFIX?

						if ((opr8r_obj.type_mask & STATEMENT_ENDER) && opr8r_obj.numReqExecOperands == 0)	{
							// TODO: msg?
							userMessages->logMsg (INTERNAL_ERROR, L"", thisSrcFile, __LINE__, 0);
							isFailed = true;

						} else if (nxtTkn._unsigned == POST_INCR_OPR8R_OPCODE || nxtTkn._unsigned == POST_DECR_OPR8R_OPCODE 
							|| nxtTkn._unsigned == PRE_INCR_OPR8R_OPCODE || nxtTkn._unsigned == PRE_DECR_OPR8R_OPCODE)	{
								// For "actionable" [PRE|POST]FIX OPR8Rs, a CSV list of variable names is packed inside the OP_CODE's
								// structure, as opposed to being a separate Token.
								if (OK != execPrePostFixOp (flatExprTkns, currIdx))
									isFailed = true;
								else
								// Deletions made; Break out of loop so currIdx -> 0
								break;

						} else if ((opr8r_obj.type_mask & UNARY) && opr8r_obj.numReqExecOperands == 1)	{
							if (OK != execUnaryOp (flatExprTkns, currIdx))	{
								userMessages->logMsg (INTERNAL_ERROR, L"Failed executing UNARY OPR8R!", thisSrcFile, __LINE__, 0);
								isFailed = true;
							} else	{
								// Deletions made; Break out of loop so currIdx -> 0
								// TODO: Is there a way to be SMRT about this?  Maybe currIdx points to NEXT idx after handled
								// OPR8R, and if it's an OPERAND we can continue w/o breaking out of the loop?
								break;
							}

						} else if ((opr8r_obj.type_mask & TERNARY_1ST) && opr8r_obj.numReqExecOperands == 1)	{
							if (OK != execTernary1stOp (flatExprTkns, currIdx))
								isFailed = true;
							else	{
								// Deletions made; Break out of loop so currIdx -> 0
								// TODO: Is there a way to be SMRT about this?  Maybe currIdx points to NEXT idx after handled
								// OPR8R, and if it's an OPERAND we can continue w/o breaking out of the loop?
								break;
							}

						} else if (opr8r_obj.type_mask & TERNARY_2ND)	{
							// TODO: Is this an error condition? Or should there be a state machine or something?
							userMessages->logMsg (INTERNAL_ERROR, L"", thisSrcFile, __LINE__, 0);
							isFailed = true;

						} else if ((opr8r_obj.type_mask & BINARY) && opr8r_obj.numReqExecOperands == 2)	{
							if (OK != execBinaryOp (flatExprTkns, currIdx))	{
								userMessages->logMsg (INTERNAL_ERROR, L"Failure executing [" + opr8r_obj.symbol + L"] operator.", thisSrcFile, __LINE__, 0);
								isFailed = true;
							} else	{
								// Deletions made; Break out of loop so currIdx -> 0
								// TODO: Is there a way to be SMRT about this?  Maybe currIdx points to NEXT idx after handled
								// OPR8R, and if it's an OPERAND we can continue w/o breaking out of the loop?
								break;
							}
						} else	{
							userMessages->logMsg (INTERNAL_ERROR, L"", thisSrcFile, __LINE__, 0);
							isFailed = true;
						}

						if (!isFailed)	{
							// Operation result stored in Token that previously held the OPR8R. We need to delete any associatd operands
							int StartDelIdx = currIdx - opr8r_obj.numReqExecOperands;
							flatExprTkns.erase(flatExprTkns.begin() + StartDelIdx, flatExprTkns.begin() + StartDelIdx + 1);
							currIdx -= opr8r_obj.numReqExecOperands;
						}
					}
				}

				if (currIdx + 1 == flatExprTkns.size())
					isEOLreached = true;
				else
					currIdx++;
			}

			currTknCnt = flatExprTkns.size();

			if (!is1RandLeft && currTknCnt >= prevTknCnt)	{
				userMessages->logMsg (INTERNAL_ERROR, L"Token stream not reduced; no meaningful work done in this loop", thisSrcFile, __LINE__, 0);
				isFailed = true;

			} else if (is1RandLeft)	{
				ret_code = OK;
			}
		}
	}

	if (ret_code != OK && !isFailed)	{
		userMessages->logMsg (INTERNAL_ERROR, L"Unhandled error!", thisSrcFile, __LINE__, 0);
		// TODO: See the final result
		util.dumpTokenList (flatExprTkns, execTerms, thisSrcFile, __LINE__);
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::resolveFlatExpr(std::vector<Token> & flatExprTkns)     {
	int ret_code = GENERAL_FAILURE;

	if (0 == flatExprTkns.size())
		userMessages->logMsg (INTERNAL_ERROR, L"Token stream unexpectedly EMPTY!", thisSrcFile, __LINE__, 0);
	else
		ret_code = execFlatExpr_OLR(flatExprTkns, 0);

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
void RunTimeInterpreter::dumpTokenPtrStream (TokenPtrVector tokenStream, std::wstring callersSrcFile, int lineNum)	{
	std::wstring tknStrmStr = L"";

	std::wcout << L"********** dumpTokenPtrStream called from " << callersSrcFile << L":" << lineNum << L" **********" << std::endl;
	int idx;
	for (idx = 0; idx < tokenStream.size(); idx++)	{
		std::shared_ptr<Token> listTkn = tokenStream[idx];
		std::wcout << L"[" ;
		if (listTkn->tkn_type == EXEC_OPR8R_TKN)
			std::wcout << execTerms.getSrcOpr8rStrFor(listTkn->_unsigned);
		else if (listTkn->_string.length() > 0)
			std::wcout << listTkn->_string;
		else	{
			std::wcout << listTkn->_signed;
		}
		std::wcout << L"] ";
	}

	std::wcout << std::endl;

}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::resolveIfVariable (Token & originalTkn, Token & resolvedTkn, std::wstring & varName)	{
	int ret_code = GENERAL_FAILURE;
	std::shared_ptr<UserMessages> tmpUserMsg = std::make_shared<UserMessages>();

	if (originalTkn.tkn_type == USER_WORD_TKN)	{
		varName = originalTkn._string;
		if (OK != varScopeStack->findVar(varName, 0, resolvedTkn, READ_ONLY, tmpUserMsg))	{
			// TODO: Log an error message
		} else	{
			ret_code = OK;
		}
	} else	{
		resolvedTkn = originalTkn;
		ret_code = OK;
	}

	return (ret_code);
}

