/*
 * GeneratedCodeTerms.cpp
 *
 *  Created on: Jan 4, 2024
 *      Author: Mike Volk
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

#include "RunTimeExecutor.h"

#include "Token.h"
#include "TokenCompareResult.h"
#include <iterator>

RunTimeExecutor::RunTimeExecutor() {
	// TODO Auto-generated constructor stub
  // TODO: Add special case operators to handle the result of the ":" opr8r?
  // e.g. TRUE - Keep 1st expression|operand on the stack, but consume the 2nd
  // FALSE - Consume the 1st expression|operand and leave the result from the 2nd expression|operand on the stack

  oneTkn = new Token(INT64_TKN, L"1", 0, 0);
  zeroTkn = new Token(INT64_TKN, L"0", 0, 0);

}

RunTimeExecutor::~RunTimeExecutor() {
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
int RunTimeExecutor::execOperation (Token * opr8r, TokenPtrVector & operands, TokenPtrVector & resultStack) {
  int ret_code = GENERAL_FAILURE;

  if (opr8r != NULL)	{
  	std::wstring opr8rStr = opr8r->_string;

  	int numReqRands = get_operand_cnt(opr8r->_string);

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
  			ret_code = execGr8rThan (compareRez, resultStack);

  		} else if (0 == opr8r->_string.compare (L">="))	{
  			ret_code = execGr8rThanEqual(compareRez, resultStack);

  		} else if (0 == opr8r->_string.compare (L"=="))	{
  			ret_code = execEquivalenceOp(compareRez, resultStack);

  		} else if (0 == opr8r->_string.compare (L"!="))	{
  			ret_code = execNotEqualsOp(compareRez, resultStack);

  		} else if (0 == opr8r->_string.compare (L"<"))	{
    			ret_code = execLessThan (compareRez, resultStack);

    	} else if (0 == opr8r->_string.compare (L"<="))	{
    			ret_code = execLessThanEqual(compareRez, resultStack);

  		} else if (0 == opr8r->_string.compare (L"&&"))	{
  			ret_code = execLogicalAndOp(leftRez, rightRez, resultStack);

  		} else if (0 == opr8r->_string.compare (L"||"))	{
  			ret_code = execLogicalOrOp(leftRez, rightRez, resultStack);
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
void RunTimeExecutor::dumpTokenStream (TokenPtrVector tokenStream)	{
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
bool RunTimeExecutor::isShortCircuit (Token * opr8rTkn, int operandIdx, Token * operand)	{
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
int RunTimeExecutor::consumeShortCircuit (TokenPtrVector & unifiedStream, int startIdx)	{
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
				int numReqRands = get_operand_cnt(currTkn->_string);

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


/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeExecutor::consumeUnifiedStream (TokenPtrVector & unifiedStream, int startIdx)	{
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

				TokenPtrVector opResult;

				if (operands.size() != numReqRands)	{
					isFailed = true;
					std::wcout << "isFailed on line " << __LINE__ << "; currIdx = " << currIdx << "; " << std::endl;
					// isFailed on line 432; currIdx = 6;
					// ||,1,&&,0,&&,0,&&,1,1
					// TODO:  Nothing was put on the operands stack because there were recursive calls to resolve OPR8Rs
					// Need to check back when we get here, because && @ currIdx == 6 *has* 2 operands to work with: 1,1
				}

 				if (!isFailed && OK != execOperation(currTkn, operands, opResult))	{
					isFailed = true;
					std::wcout << "isFailed on line " << __LINE__ << std::endl;
				}

 				if (!isFailed && opResult.size() != 1)	{
					isFailed = true;
					std::wcout << "isFailed on line " << __LINE__ << std::endl;
					std::wcout << "TODO insert: currIdx = " << currIdx << "; unifiedStream.size = " << unifiedStream.size() << "; opResult.size = " << opResult.size() << "; " << std::endl;
					std::wcout << currTkn->_string << "; line # " << currTkn->line_number << "; oprndIdx = " << oprndIdx << "; " << std::endl;
				}

 				if (!isFailed) {
					// After performing the operation, clear out all associated tokens (1 OPR8R + [0-3] operands
					// and put the operation result back on the stack @ the right location
					// TODO: Do I need to explicitly call delete on the Token pointers, or will erase handle it for me?
					unifiedStream.erase(unifiedStream.begin() + currIdx, unifiedStream.begin() + currIdx + operands.size() + 1);
					unifiedStream.insert(unifiedStream.begin() + currIdx, opResult.back());
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


/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeExecutor::execGr8rThan(TokenCompareResult compareRez, TokenPtrVector & resultStack)     {
	int ret_code = GENERAL_FAILURE;

	if (compareRez.gr8rThan == isTrue)	{
		Token * trueTkn = new Token (INT64_TKN, L"1", 0, 0);
		trueTkn->_signed = 1;
		resultStack.push_back(trueTkn);
		ret_code = OK;

	} else if (compareRez.gr8rThan == isFalse) {
		Token * falseTkn = new Token (INT64_TKN, L"0", 0, 0);
		falseTkn->_signed = 0;
		resultStack.push_back(falseTkn);
		ret_code = OK;
	}

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeExecutor::execGr8rThanEqual(TokenCompareResult compareRez, TokenPtrVector & resultStack)     {
	int ret_code = GENERAL_FAILURE;

	if (compareRez.gr8rEquals == isTrue)	{
		Token * trueTkn = new Token (INT64_TKN, L"1", 0, 0);
		trueTkn->_signed = 1;
		resultStack.push_back(trueTkn);
		ret_code = OK;

	} else if (compareRez.gr8rEquals == isFalse) {
		Token * falseTkn = new Token (INT64_TKN, L"0", 0, 0);
		falseTkn->_signed = 0;
		resultStack.push_back(falseTkn);
		ret_code = OK;
	}

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeExecutor::execEquivalenceOp(TokenCompareResult compareRez, TokenPtrVector & resultStack)     {
	int ret_code = GENERAL_FAILURE;

	if (compareRez.equals == isTrue)	{
		Token * trueTkn = new Token (INT64_TKN, L"1", 0, 0);
		trueTkn->_signed = 1;
		resultStack.push_back(trueTkn);
		ret_code = OK;

	} else if (compareRez.equals == isFalse) {
		Token * falseTkn = new Token (INT64_TKN, L"0", 0, 0);
		falseTkn->_signed = 0;
		resultStack.push_back(falseTkn);
		ret_code = OK;
	}

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeExecutor::execNotEqualsOp(TokenCompareResult compareRez, TokenPtrVector & resultStack)     {
	int ret_code = GENERAL_FAILURE;

	if (compareRez.equals == isFalse)	{
		Token * trueTkn = new Token (INT64_TKN, L"1", 0, 0);
		trueTkn->_signed = 1;
		resultStack.push_back(trueTkn);
		ret_code = OK;

	} else if (compareRez.equals == isTrue) {
		Token * falseTkn = new Token (INT64_TKN, L"0", 0, 0);
		falseTkn->_signed = 0;
		resultStack.push_back(falseTkn);
		ret_code = OK;
	}

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeExecutor::execLessThan(TokenCompareResult compareRez, TokenPtrVector & resultStack)     {
	int ret_code = GENERAL_FAILURE;

	if (compareRez.lessThan == isTrue)	{
		Token * trueTkn = new Token (INT64_TKN, L"1", 0, 0);
		trueTkn->_signed = 1;
		resultStack.push_back(trueTkn);
		ret_code = OK;

	} else if (compareRez.lessThan == isFalse) {
		Token * falseTkn = new Token (INT64_TKN, L"0", 0, 0);
		falseTkn->_signed = 0;
		resultStack.push_back(falseTkn);
		ret_code = OK;
	}

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeExecutor::execLessThanEqual(TokenCompareResult compareRez, TokenPtrVector & resultStack)     {
	int ret_code = GENERAL_FAILURE;

	if (compareRez.lessEquals == isTrue)	{
		Token * trueTkn = new Token (INT64_TKN, L"1", 0, 0);
		trueTkn->_signed = 1;
		resultStack.push_back(trueTkn);
		ret_code = OK;

	} else if (compareRez.lessEquals == isFalse) {
		Token * falseTkn = new Token (INT64_TKN, L"0", 0, 0);
		falseTkn->_signed = 0;
		resultStack.push_back(falseTkn);
		ret_code = OK;
	}

  return (ret_code);
}


/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeExecutor::execLogicalAndOp(TokenCompareResult leftRez, TokenCompareResult rightRez, TokenPtrVector & resultStack)     {
	int ret_code = GENERAL_FAILURE;

	if (leftRez.gr8rEquals == isTrue && rightRez.gr8rEquals == isTrue)	{
		Token * trueTkn = new Token (INT64_TKN, L"1", 0, 0);
		trueTkn->_signed = 1;
		resultStack.push_back(trueTkn);
		ret_code = OK;

	} else if (leftRez.gr8rEquals != compareFailed && rightRez.gr8rEquals != compareFailed) {
		Token * falseTkn = new Token (INT64_TKN, L"0", 0, 0);
		falseTkn->_signed = 0;
		resultStack.push_back(falseTkn);
		ret_code = OK;
	}

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeExecutor::execLogicalOrOp(TokenCompareResult leftRez, TokenCompareResult rightRez, TokenPtrVector & resultStack)     {
	int ret_code = GENERAL_FAILURE;

	if (leftRez.gr8rEquals != compareFailed && rightRez.gr8rEquals != compareFailed) {
		if (leftRez.gr8rEquals == isTrue || rightRez.gr8rEquals == isTrue)	{
			Token * trueTkn = new Token (INT64_TKN, L"1", 0, 0);
			trueTkn->_signed = 1;
			resultStack.push_back(trueTkn);
			ret_code = OK;

		} else {
			Token * falseTkn = new Token (INT64_TKN, L"0", 0, 0);
			falseTkn->_signed = 0;
			resultStack.push_back(falseTkn);
			ret_code = OK;
		}
	}


  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeExecutor::execColonOp () {
  int ret_code = GENERAL_FAILURE;

  // TODO:
  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeExecutor::execTernaryTrue () {
	int ret_code = GENERAL_FAILURE;

	// TODO:
	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeExecutor::execTernaryFalse () {
	int ret_code = GENERAL_FAILURE;

	// TODO:
	return (ret_code);
}

