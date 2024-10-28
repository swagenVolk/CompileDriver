/*
 * RunTimeExecutor.cpp
 *
 *  Created on: Jan 4, 2024
 *      Author: Mike Volk
 *
 * TODO:
 * Will need to be able to recognize complete, yet contained sub-expressions for the untaken TERNARY
 * branch, expressions as fxn|system calls, and potentially for short-circuiting (early break) on the [||] or [&&] OPR8Rs
 * Align comments below with reality
 *
 * Where will fxn calls go: on the operand or opr8r stack? If fxn calls go on the opr8r stack, then
 * the number of arguments the fxn call expects has to be looked up @ run time, OR the # of expected
 * operands could be put on the operand stack.
 * Why did I think putting the fxn call on the operand stack might be a good idea?
 * IIRC it had something to do with making short circuiting easier.
 * Might not be able to do short circuiting in the Interpreter, given that it is expected to be stack
 * based and higher precedence (deeper tree) operations below and || operator will be resolved before
 * we get to the ||, so we'll have calculated BEFORE we even knew there was an opportunity to short circuit.
 *
 * Is there a way to encapsulate an expression in the stacks?  Maybe something like a stack frame?
 * Should operators or operands come 1st to enable short circuiting?
 * Short circuiting OPR8Rs: [&&,||,:]
 *
 * Stack approaches:
 * LOR - Left OPR8R Right - Higher precedence goes to the left branch and compile tree is resolved deep 1st
 * Can hopefully use the unified token stream and forego separate OPR8R and Operand stacks
 * Want to create an absolutely precedence ordered stream to enable short circuiting.
 * TODO: Elaborate on any shortcomings with this approach
 *
 * LOR+depth
 * 1st thing encountered is a LEFT operand. Perform the >= comparison @ depth 3 and it is evaluated FALSE.
 * Short circuit any follow on expressions with a depth > 3; stop the short circuit when a depth of 2 or
 * less is encountered.  Retain the 0 result on the stack with the current depth
 * TODO: How does this work with the ternary operator?
 *
 * Bikes		4
 * >=				3
 * 5				4
 * &&				2
 * Cars			4
 * >				3
 * 14				4
 * &&				1
 * Shovels	3
 * ==				2
 * 2				3
 * &&				0
 * CClamps	2
 * >				1
 * 3				2
 *
 *
 * O12[3]+depth
 * TODO: What about including the # of Tokens encapsulated by current Token, instead of depth?  Leaves behind instruction on how many to flush.
 * TODO: Is it possible to leave out this metadata, since we'll know how many operands each OPR8R consumes, and the oprnds and r8rs will be
 * "stacked" such that we can recursively clean up?
 *
 * ||,1,&&,>=,numBikes,5,&&,>,numCars,14,&&,==,numShovels,2,>,numCClamps,3
 * isShortCircuit on ||; operandIdx = 0;
 * If next token in stream is an Operand or KeyWord, toss it! If it's an OPR8R, then consume it & all recursive operations
 * Make all result Tokens 1 as a precaution against division by zero
 * &&,>=,numBikes,5,&&,>,numCars,14,&&,==,numShovels,2,>,numCClamps,3
 * &&,>=,numBikes,5,&&,>,numCars,14,&&,==,numShovels,2,>,numCClamps,3
 * &&,1,&&,>,numCars,14,&&,==,numShovels,2,>,numCClamps,3
 * &&,1,&&,1,&&,==,numShovels,2,>,numCClamps,3
 * &&,1,&&,1,&&,1,>,numCClamps,3
 * &&,1,&&,1,&&,1,1
 * &&,1,&&,1,1
 * &&,1,1
 * 1
 *
 * TODO: Test with middle short circuit
 * TODO: Test with unary operator
 * TODO: Test with ternary operator
 *
 *  */

#include "RunTimeInterpreter.h"

#include "Token.h"
#include "TokenCompareResult.h"
#include <iterator>

RunTimeInterpreter::RunTimeInterpreter(CompileExecTerms & execTerms) {
	// TODO Auto-generated constructor stub
  // TODO: Add special case operators to handle the result of the ":" opr8r?
  // e.g. TRUE - Keep 1st expression|operand on the stack, but consume the 2nd
  // FALSE - Consume the 1st expression|operand and leave the result from the 2nd expression|operand on the stack

  oneTkn = new Token(INT64_TKN, L"1", 0, 0);
  zeroTkn = new Token(INT64_TKN, L"0", 0, 0);
  this->execTerms = execTerms;
	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");
	failedOnLineNum = 0;
}

RunTimeInterpreter::~RunTimeInterpreter() {
	// TODO Auto-generated destructor stub

	if (oneTkn != NULL)	{
		delete oneTkn;
		oneTkn = NULL;
	}

	if (zeroTkn != NULL)	{
		delete zeroTkn;
		zeroTkn = NULL;
	}
}


/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execOperation (Token * opr8r, TokenPtrVector & operands, Token & resultTkn) {
  int ret_code = GENERAL_FAILURE;

  if (opr8r != NULL)	{
  	std::wstring opr8rStr = opr8r->_string;

  	int numReqRands = execTerms.get_operand_cnt(opr8r->_string);

  	if (numReqRands == 2 && operands.size() >= 2)	{
  		// TODO: == 2 OR >= 2?
  		// TODO: Do I need to delete the Tokens? Probably....
  		// TODO: How many of these operations really belong in CCompilerOpr8rs.cpp?
    	Token *leftTkn = operands[0];
    	Token *rightTkn = operands[1];

    	TokenCompareResult compareRez = leftTkn->compare(*rightTkn);
			TokenCompareResult leftRez = leftTkn->compare (*oneTkn);
			TokenCompareResult rightRez = rightTkn->compare (*oneTkn);

  		if (0 == opr8r->_string.compare (L">"))	{
  			ret_code = execGr8rThan (compareRez, resultTkn);

  		} else if (0 == opr8r->_string.compare (L">="))	{
  			ret_code = execGr8rThanEqual(compareRez, resultTkn);

  		} else if (0 == opr8r->_string.compare (L"=="))	{
  			ret_code = execEquivalenceOp(compareRez, resultTkn);

  		} else if (0 == opr8r->_string.compare (L"!="))	{
  			ret_code = execNotEqualsOp(compareRez, resultTkn);

  		} else if (0 == opr8r->_string.compare (L"<"))	{
    			ret_code = execLessThan (compareRez, resultTkn);

    	} else if (0 == opr8r->_string.compare (L"<="))	{
    			ret_code = execLessThanEqual(compareRez, resultTkn);

  		} else if (0 == opr8r->_string.compare (L"&&"))	{
  			ret_code = execLogicalAndOp(leftRez, rightRez, resultTkn);

  		} else if (0 == opr8r->_string.compare (L"||"))	{
  			ret_code = execLogicalOrOp(leftRez, rightRez, resultTkn);
  		}

  	}
  }

  	if (0 == opr8r->_string.compare (L"!"))	{

  	} else if (0 == opr8r->_string.compare (L"~"))	{

  }

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
void RunTimeInterpreter::dumpTokenStream (TokenPtrVector tokenStream)	{
	std::wstring tknStrmStr = L"";

	for (TokenPtrVector::iterator itr8r = tokenStream.begin(); itr8r != tokenStream.end(); itr8r++)	{
		Token * currTkn = *itr8r;
		if (!tknStrmStr.empty())
			tknStrmStr.append(L",");
		tknStrmStr.append(currTkn->_string);
	}

	std::wcout << tknStrmStr << std::endl;
}


/* ****************************************************************************
 *
 * ***************************************************************************/
bool RunTimeInterpreter::isShortCircuit (Token * opr8rTkn, int operandIdx, Token * operand)	{
	bool isJumpOutEarly = false;
	if (0 == opr8rTkn->_string.compare(L"||") && operandIdx == 0 && operand->_signed > 0)	{
		isJumpOutEarly = true;
	}

	else if (0 == opr8rTkn->_string.compare(L"&&") && operandIdx == 0 && operand->_signed <= 0)	{
		isJumpOutEarly = true;
	}

	if (isJumpOutEarly)	{
		// TODO
		std::wcout << "TODO: isShortCircuit on " << opr8rTkn->_string << "; operandIdx = " << operandIdx << "; " << std::endl;
	}
	return (isJumpOutEarly);

}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::consumeShortCircuit (TokenPtrVector & unifiedStream, int startIdx)	{
	int ret_code = GENERAL_FAILURE;

	bool isFailed = false;
	bool isDone = false;
	int currIdx = startIdx;

	while (!isFailed && !isDone)	{
		if (currIdx >= unifiedStream.size())	{
			isFailed = true;

		} else	{
			Token * currTkn = unifiedStream[currIdx];
			TokenPtrVector operands;
			int oprndIdx;

			if (currTkn->tkn_type == OPR8R_TKN)	{
				int numReqRands = execTerms.get_operand_cnt(currTkn->_string);

				for (oprndIdx = 1; oprndIdx <= numReqRands && !isFailed; oprndIdx++)	{
					if (currIdx + oprndIdx >= unifiedStream.size())	{
						isFailed = true;
						std::wcout << "isFailed on line " << __LINE__ << std::endl;
					} else	{
						int nxtRandAbsIdx = currIdx + oprndIdx;
						Token * oprndTkn = unifiedStream[nxtRandAbsIdx];
						if (oprndTkn->tkn_type == OPR8R_TKN)	{
							if (OK != consumeShortCircuit (unifiedStream, nxtRandAbsIdx))	{
								isFailed = true;
								std::wcout << "isFailed on line " << __LINE__ << std::endl;
							} else {
								operands.push_back (unifiedStream[nxtRandAbsIdx]);
							}

						} else if (nxtRandAbsIdx >= unifiedStream.size())	{
								isFailed = true;
								std::wcout << "isFailed on line " << __LINE__ << std::endl;

						} else {
							// Operand ready to go at this index; no need to resolve an OPR8R
							operands.push_back (unifiedStream[nxtRandAbsIdx]);
						}
					}
				}

				if (operands.size() != numReqRands)	{
					isFailed = true;
					std::wcout << "isFailed on line " << __LINE__ << "; currIdx = " << currIdx << "; " << std::endl;

				} else	{
					// After performing the operation, clear out all associated tokens (1 OPR8R + [0-3] operands
					// and put the operation result back on the stack @ the right location
					// TODO: Do I need to explicitly call delete on the Token pointers, or will erase handle it for me?
					unifiedStream.erase(unifiedStream.begin() + currIdx, unifiedStream.begin() + currIdx + operands.size() + 1);
					Token * trueToken = new Token (INT64_TKN, L"1", 0, 0);
					trueToken->_unsigned = 1;
					unifiedStream.insert(unifiedStream.begin() + currIdx, trueToken);
					isDone = true;
					ret_code = OK;
				}
			} else	{
				std::wcout << "Not an OPR8R on line " << __LINE__ << "; currIdx = " << currIdx << "; " << std::endl;
			}
		}
	}

	return (ret_code);
}

#if 0
/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::consumeUnifiedStream (TokenPtrVector & unifiedStream, int startIdx)	{
	int ret_code = GENERAL_FAILURE;

	//	||					0
	//	>						1
	//	numSprings	2
	//	5						2
	//	&&					1
	//	>=					2
	//	numBikes		3
	//	5						3
	//	&&					2
	//	>						3
	//	numCars			4
	//	14					4
	//	&&					3
	//	==					4
	//	numShovels	5
	//	2						5
	//	>						4
	//	numCClamps	5
	//	3						5

	bool isFailed = false;
	bool isDone = false;
	int currIdx = startIdx;

	while (!isFailed && !isDone)	{
		if (currIdx >= unifiedStream.size())	{
			isFailed = true;
			std::wcout << "isFailed on line " << __LINE__ << std::endl;

		} else	{
			Token * currTkn = unifiedStream[currIdx];
			TokenPtrVector operands;
			int oprndIdx;

			if (currTkn->tkn_type == OPR8R_TKN)	{
				int numReqRands = get_operand_cnt(currTkn->_string);

				for (oprndIdx = 1; oprndIdx <= numReqRands && !isFailed; oprndIdx++)	{
					if (currIdx + oprndIdx >= unifiedStream.size())	{
						isFailed = true;
						std::wcout << "isFailed on line " << __LINE__ << std::endl;
					} else	{
						int nxtOprndAbsIdx = currIdx + oprndIdx;
						Token * oprndTkn = unifiedStream[nxtOprndAbsIdx];
						if (oprndTkn->tkn_type == OPR8R_TKN)	{
							if (OK != consumeUnifiedStream (unifiedStream, nxtOprndAbsIdx))	{
								isFailed = true;
								std::wcout << "isFailed on line " << __LINE__ << std::endl;
							} else {
								// Recursive call should have resolved OPR8R @ this index down to an Operand
								// TODO: Here is an opportunity to short-circuit for the
								// [&&;||;:] OPR8Rs on the 1st operand.
								operands.push_back (unifiedStream[nxtOprndAbsIdx]);

								if (isShortCircuit (currTkn, oprndIdx - 1, unifiedStream[nxtOprndAbsIdx]))	{
									if (nxtOprndAbsIdx + 1 < unifiedStream.size())	{
										if (OK != consumeShortCircuit (unifiedStream, nxtOprndAbsIdx + 1))	{
											// Our 1st operand was already resolved and
											isFailed = true;
											std::wcout << "isFailed on line " << __LINE__ << std::endl;
										} else	{
											operands.push_back (unifiedStream[nxtOprndAbsIdx + 1]);
											break;
										}
									} else	{
										// Expected more Tokens In The Stream
										isFailed = true;
										std::wcout << "isFailed on line " << __LINE__ << std::endl;
									}
								}
							}

						} else if (nxtOprndAbsIdx >= unifiedStream.size())	{
								isFailed = true;
								std::wcout << "isFailed on line " << __LINE__ << std::endl;

						} else {
							// Operand ready to go at this index; no need to resolve an OPR8R
							// TODO: Here is an opportunity to short-circuit for the
							// [&&;||;:] OPR8Rs on the 1st operand.
							if (isShortCircuit (currTkn, oprndIdx - 1, unifiedStream[nxtOprndAbsIdx]))	{

							}
							operands.push_back (unifiedStream[nxtOprndAbsIdx]);
						}
					}
				}

				Token opResultTkn (START_UNDEF_TKN, L"", 0, 0);

				if (operands.size() != numReqRands)	{
					isFailed = true;
					std::wcout << "isFailed on line " << __LINE__ << "; currIdx = " << currIdx << "; " << std::endl;
					// isFailed on line 432; currIdx = 6;
					// ||,1,&&,0,&&,0,&&,1,1
					// TODO:  Nothing was put on the operands stack because there were recursive calls to resolve OPR8Rs
					// Need to check back when we get here, because && @ currIdx == 6 *has* 2 operands to work with: 1,1
				}

 				if (!isFailed && OK != execOperation(currTkn, operands, opResultTkn))	{
					isFailed = true;
					std::wcout << "isFailed on line " << __LINE__ << std::endl;
				}

 				if (!isFailed && opResultTkn.tkn_type == START_UNDEF_TKN)	{
 					// TODO: Re-visit this
					isFailed = true;
					std::wcout << "isFailed on line " << __LINE__ << std::endl;
					std::wcout << currTkn->_string << "TODO: line # " << currTkn->line_number << "; oprndIdx = " << oprndIdx << "; " << std::endl;
				}

 				if (!isFailed) {
					// After performing the operation, clear out all associated tokens (1 OPR8R + [0-3] operands
					// and put the operation result back on the stack @ the right location
					// TODO: Do I need to explicitly call delete on the Token pointers, or will erase handle it for me?
					unifiedStream.erase(unifiedStream.begin() + currIdx, unifiedStream.begin() + currIdx + operands.size() + 1);
					unifiedStream.insert(unifiedStream.begin() + currIdx, opResultTkn);
					isDone = true;
					ret_code = OK;
				}
			} else	{
				std::wcout << "Not an OPR8R on line " << __LINE__ << "; currIdx = " << currIdx << "; " << std::endl;
			}
		}
	}

	std::wcout << "Exiting consumeUnifiedStream startIdx = " << startIdx << "; ret_code = " << ret_code << "; " << std::endl;
	dumpTokenStream(unifiedStream);

	return (ret_code);
}

#endif


/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execGr8rThan(TokenCompareResult compareRez, Token & resultTkn)     {
	int ret_code = GENERAL_FAILURE;

	if (compareRez.gr8rThan == isTrue)	{
		resultTkn.tkn_type = INT64_TKN;
		resultTkn._signed = 1;
		ret_code = OK;

	} else if (compareRez.gr8rThan == isFalse) {
		resultTkn.tkn_type = INT64_TKN;
		resultTkn._signed = 0;
		ret_code = OK;
	}

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execGr8rThanEqual(TokenCompareResult compareRez, Token & resultTkn)     {
	int ret_code = GENERAL_FAILURE;

	if (compareRez.gr8rEquals == isTrue)	{
		resultTkn.tkn_type = INT64_TKN;
		resultTkn._signed = 1;
		ret_code = OK;

	} else if (compareRez.gr8rEquals == isFalse) {
		resultTkn.tkn_type = INT64_TKN;
		resultTkn._signed = 0;
		ret_code = OK;
	}

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execEquivalenceOp(TokenCompareResult compareRez, Token & resultTkn)     {
	int ret_code = GENERAL_FAILURE;

	if (compareRez.equals == isTrue)	{
		resultTkn.tkn_type = INT64_TKN;
		resultTkn._signed = 1;
		ret_code = OK;

	} else if (compareRez.equals == isFalse) {
		resultTkn.tkn_type = INT64_TKN;
		resultTkn._signed = 0;
		ret_code = OK;
	}

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execNotEqualsOp(TokenCompareResult compareRez, Token & resultTkn)     {
	int ret_code = GENERAL_FAILURE;

	if (compareRez.equals == isFalse)	{
		resultTkn.tkn_type = INT64_TKN;
		resultTkn._signed = 1;
		ret_code = OK;

	} else if (compareRez.equals == isTrue) {
		resultTkn.tkn_type = INT64_TKN;
		resultTkn._signed = 0;
		ret_code = OK;
	}

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execLessThan(TokenCompareResult compareRez, Token & resultTkn)     {
	int ret_code = GENERAL_FAILURE;

	if (compareRez.lessThan == isTrue)	{
		resultTkn.tkn_type = INT64_TKN;
		resultTkn._signed = 1;
		ret_code = OK;

	} else if (compareRez.lessThan == isFalse){
		resultTkn.tkn_type = INT64_TKN;
		resultTkn._signed = 0;
		ret_code = OK;
	}

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execLessThanEqual(TokenCompareResult compareRez, Token & resultTkn)     {
	int ret_code = GENERAL_FAILURE;

	if (compareRez.lessEquals == isTrue)	{
		resultTkn.tkn_type = INT64_TKN;
		resultTkn._signed = 1;
		ret_code = OK;

	} else if (compareRez.lessEquals == isFalse)	{
		resultTkn.tkn_type = INT64_TKN;
		resultTkn._signed = 0;
		ret_code = OK;
	}

  return (ret_code);
}


/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execLogicalAndOp(TokenCompareResult leftRez, TokenCompareResult rightRez, Token & resultTkn)     {
	int ret_code = GENERAL_FAILURE;

	if (leftRez.gr8rEquals != compareFailed && rightRez.gr8rEquals != compareFailed) {
		if (leftRez.gr8rEquals == isTrue && rightRez.gr8rEquals == isTrue)	{
			resultTkn.tkn_type = INT64_TKN;
			resultTkn._signed = 1;
			ret_code = OK;

		} else if (leftRez.gr8rEquals != isTrue || rightRez.gr8rEquals != isTrue)	{
			resultTkn.tkn_type = INT64_TKN;
			resultTkn._signed = 0;
			ret_code = OK;
		}
	}

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execLogicalOrOp(TokenCompareResult leftRez, TokenCompareResult rightRez, Token & resultTkn)     {
	int ret_code = GENERAL_FAILURE;

	if (leftRez.gr8rEquals == isTrue || rightRez.gr8rEquals == isTrue)	{
		resultTkn.tkn_type = INT64_TKN;
		resultTkn._signed = 1;
		ret_code = OK;

	} else if (leftRez.gr8rEquals == isFalse && rightRez.gr8rEquals == isFalse) {
		resultTkn.tkn_type = INT64_TKN;
		resultTkn._signed = 0;
		ret_code = OK;
	}

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execBinaryOp(std::vector<Token> & exprTknStream, int opr8rIdx)     {
	int ret_code = GENERAL_FAILURE;

	if (exprTknStream.size() > opr8rIdx && exprTknStream.size() >= 3 && opr8rIdx >= 2 && exprTknStream[opr8rIdx].tkn_type == EXEC_OPR8R_TKN)	{
		uint8_t op_code = exprTknStream[opr8rIdx]._unsigned;

		switch (op_code)	{
			case MULTIPLY_OPR8R_OPCODE :
				break;
			case DIV_OPR8R_OPCODE :
				break;
			case MOD_OPR8R_OPCODE :
				break;
			case BINARY_PLUS_OPR8R_OPCODE :
				break;
			case BINARY_MINUS_OPR8R_OPCODE :
				break;
			case LEFT_SHIFT_OPR8R_OPCODE :
				break;
			case RIGHT_SHIFT_OPR8R_OPCODE :
				break;
			case LESS_THAN_OPR8R_OPCODE :
				break;
			case LESS_EQUALS_OPR8R8_OPCODE :
				break;
			case GREATER_THAN_OPR8R_OPCODE :
				break;
			case GREATER_EQUALS_OPR8R8_OPCODE :
				break;
			case EQUALITY_OPR8R_OPCODE :
				break;
			case NOT_EQUALS_OPR8R_OPCODE :
				break;
			case BITWISE_AND_OPR8R_OPCODE :
				break;
			case BITWISE_XOR_OPR8R_OPCODE :
				break;
			case BITWISE_OR_OPR8R_OPCODE :
				break;
			case LOGICAL_AND_OPR8R_OPCODE :
				break;
			case LOGICAL_OR_OPR8R_OPCODE :
				break;
			case TERNARY_1ST_OPR8R_OPCODE :
				break;
			case TERNARY_2ND_OPR8R_OPCODE :
				break;
			case ASSIGNMENT_OPR8R_OPCODE :
				break;
			case PLUS_ASSIGN_OPR8R_OPCODE :
				break;
			case MINUS_ASSIGN_OPR8R_OPCODE :
				break;
			case MULTIPLY_ASSIGN_OPR8R_OPCODE :
				break;
			case DIV_ASSIGN_OPR8R_OPCODE :
				break;
			case MOD_ASSIGN_OPR8R_OPCODE :
				break;
			case LEFT_SHIFT_ASSIGN_OPR8R_OPCODE :
				break;
			case RIGHT_SHIFT_ASSIGN_OPR8R_OPCODE :
				break;
			case BITWISE_AND_ASSIGN_OPR8R_OPCODE :
				break;
			case BITWISE_XOR_ASSIGN_OPR8R_OPCODE :
				break;
			case BITWISE_OR_ASSIGN_OPR8R_OPCODE :
				break;
			default:
				break;
		}
		// TODO:
		ret_code = OK;
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execTernary1stOp(std::vector<Token> & exprTknStream, int & callersIdx)     {
	int ret_code = GENERAL_FAILURE;

	if (callersIdx >= 1 && exprTknStream.size() >= 2)	{
		bool isTruePath = exprTknStream[callersIdx - 1].evalResolvedTokenAsIf();

		if (isTruePath)
			ret_code = takeTernaryTrue (exprTknStream, callersIdx);
		else
			ret_code = takeTernaryFalse (exprTknStream, callersIdx);
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::takeTernaryTrue (std::vector<Token> & exprTknStream, int & callersIdx)     {
	int ret_code = GENERAL_FAILURE;
	int ternary1stOpIdx = callersIdx;

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
int RunTimeInterpreter::takeTernaryFalse (std::vector<Token> & exprTknStream, int & callersIdx)     {
	int ret_code = GENERAL_FAILURE;

	int ternary1stOpIdx = callersIdx;
	int idx;
	int ternary2ndOpIdx = -1;
	for (idx = ternary1stOpIdx + 2; idx < exprTknStream.size() && ternary2ndOpIdx < ternary1stOpIdx; idx++)	{
		// Start at ternary1stOpIdx + 2: Must be at least 1 Token after ternary1stOpIdx for the TRUE path
		if (exprTknStream[idx].tkn_type == EXEC_OPR8R_TKN && exprTknStream[idx]._unsigned == TERNARY_2ND_OPR8R_OPCODE)	{
			ternary2ndOpIdx = idx;
			break;
		}
	}

	if (ternary2ndOpIdx > ternary1stOpIdx && ternary2ndOpIdx < exprTknStream.size())	{
		// Remove ternary conditional, TERNARY_1ST and up to and including TERNARY_2ND
		int delCnt = (ternary2ndOpIdx + 1) - (ternary1stOpIdx - 1);
		int delIdx = ternary1stOpIdx - 1;
		for (idx = 0; idx < delCnt; idx++)	{
			exprTknStream.erase(exprTknStream.begin() + delIdx);
		}

		if (exprTknStream.size() > (ternary1stOpIdx - 2))	{
			// After deletions, we need to adjust the caller's current idx
			callersIdx = ternary1stOpIdx - 2;
			ret_code = OK;
		}
	}

	return (ret_code);
}





/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::resolveExpression(std::vector<Token> & exprTknStream)     {
	int ret_code = GENERAL_FAILURE;

	bool isFailed = false;
	bool is1RandLeft = false;
	int prevTknCnt = exprTknStream.size();
	int currTknCnt;
	int idx;

	while (!isFailed && !is1RandLeft)	{
		prevTknCnt = exprTknStream.size();

		int currIdx = 0;
		bool isEOLreached = false;
		int numSeqOperands = 0;

		while (!isFailed && !isEOLreached)	{
			Token nxtTkn = exprTknStream[currIdx];

			if (prevTknCnt == 1)	{
				// TODO: If it's an Operand, then the Token list has been resolved
				// If it's an OPR8R, then something's BUSTICATED
				is1RandLeft = true;
			}

			else if (nxtTkn.tkn_type != EXEC_OPR8R_TKN)	{
				numSeqOperands++;
				// TODO: Uncomment after getting OPR8Rs executing
//				if (prevTknCnt == 1)
//					isResolved = true;

			} else	{
				// How many operands does this OPR8R require?
				Operator opr8r_obj (L"", 0,0,0,0,0);

				if (OK != execTerms.getOpr8rDetails(execTerms.getOpr8rStrFor(nxtTkn._unsigned), opr8r_obj))	{
					failedOnLineNum == 0 ? failedOnLineNum = __LINE__ : 1;
					isFailed = true;

				} else if (numSeqOperands >= opr8r_obj.numReqExecOperands)	{
					// Current OPR8R has what it needs, so do the operation
					// TODO: What about PREFIX and POSTFIX?

					if ((opr8r_obj.type_mask & STATEMENT_ENDER) && opr8r_obj.numReqExecOperands == 0)	{
						failedOnLineNum == 0 ? failedOnLineNum = __LINE__ : 1;
						isFailed = true;

					} else if ((opr8r_obj.type_mask & UNARY) && opr8r_obj.numReqExecOperands == 1)	{
						failedOnLineNum == 0 ? failedOnLineNum = __LINE__ : 1;
						isFailed = true;

					} else if ((opr8r_obj.type_mask & TERNARY_1ST) && opr8r_obj.numReqExecOperands == 1)	{
						if (OK != execTernary1stOp (exprTknStream, currIdx))
							isFailed = true;

					} else if (opr8r_obj.type_mask & TERNARY_2ND)	{
						// TODO: Is this an error condition? Or should there be a state machine or something?
						failedOnLineNum == 0 ? failedOnLineNum = __LINE__ : 1;
						isFailed = true;

					} else if ((opr8r_obj.type_mask & BINARY) && opr8r_obj.numReqExecOperands == 2)	{
						if (OK != execBinaryOp (exprTknStream, currIdx))	{
							failedOnLineNum == 0 ? failedOnLineNum = __LINE__ : 1;
							isFailed = true;
						}

					} else	{
						failedOnLineNum == 0 ? failedOnLineNum = __LINE__ : 1;
						isFailed = true;
					}

					if (!isFailed)	{
						// Operation result stored in Token that previously held the OPR8R. We need to delete any associatd operands
						int delIdx = currIdx - opr8r_obj.numReqExecOperands;
						for (idx = 0; idx < opr8r_obj.numReqExecOperands; idx++)	{
							exprTknStream.erase(exprTknStream.begin() + delIdx);
						}
						currIdx -= opr8r_obj.numReqExecOperands;
					}
				}
			}

			if (currIdx + 1 == exprTknStream.size())
				isEOLreached = true;
			else
				currIdx++;
		}

		// TODO: Debug
		std::wcout << L"********** " << thisSrcFile << L":" << __LINE__ << L" **********" << std::endl;
		for (idx = 0; idx < exprTknStream.size(); idx++)	{
			Token listTkn = exprTknStream[idx];
			std::wcout << L"[" ;
			if (listTkn.tkn_type == EXEC_OPR8R_TKN)
				std::wcout << execTerms.getOpr8rStrFor(listTkn._unsigned);
			else if (listTkn._string.length() > 0)
				std::wcout << listTkn._string;
			else	{
				std::wcout << listTkn._signed;
			}
			std::wcout << L"] ";
		}

		std::wcout << std::endl;


		currTknCnt = exprTknStream.size();

		if (!is1RandLeft && currTknCnt >= prevTknCnt)	{
			// Token stream not reduced; no meaningful work done in this loop
			isFailed = true;
			failedOnLineNum == 0 ? failedOnLineNum = __LINE__ : 1;

		} else if (is1RandLeft)	{
			ret_code = OK;
		}
	}

	if (ret_code != OK)
		std::wcout << L"failedOnLineNum = " << failedOnLineNum << L";" << std::endl;


	return (ret_code);
}

