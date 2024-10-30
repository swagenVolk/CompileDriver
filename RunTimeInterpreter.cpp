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
 *  */

#include "RunTimeInterpreter.h"

#include "Token.h"
#include "TokenCompareResult.h"
#include <iterator>

RunTimeInterpreter::RunTimeInterpreter(CompileExecTerms & execTerms) {
	// TODO Auto-generated constructor stub
  oneTkn = new Token(INT64_TKN, L"1", 0, 0);
  // TODO: Token value not automatically filled in currently
  oneTkn->_signed = 1;
  zeroTkn = new Token(INT64_TKN, L"0", 0, 0);
  zeroTkn->_signed = 0;
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
int RunTimeInterpreter::execEquivalenceOp(Token & operand1, Token & operand2, Token & resultTkn)     {
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;
	uint8_t op_code = resultTkn._unsigned;
	TokenCompareResult compareRez = operand1.compare (operand2);

	switch (op_code)	{
		case LESS_THAN_OPR8R_OPCODE :
			if (compareRez.lessThan == isTrue)
				resultTkn = *oneTkn;
			else if (compareRez.lessThan == isFalse)
				resultTkn = *zeroTkn;
			else
				isFailed = true;
			break;
		case LESS_EQUALS_OPR8R8_OPCODE :
			if (compareRez.lessEquals == isTrue)
				resultTkn = *oneTkn;
			else if (compareRez.lessEquals == isFalse)
				resultTkn = *zeroTkn;
			else
				isFailed = true;
			break;
		case GREATER_THAN_OPR8R_OPCODE :
			if (compareRez.gr8rThan == isTrue)
				resultTkn = *oneTkn;
			else if (compareRez.gr8rThan == isFalse)
				resultTkn = *zeroTkn;
			else
				isFailed = true;
			break;
		case GREATER_EQUALS_OPR8R8_OPCODE :
			if (compareRez.gr8rEquals == isTrue)
				resultTkn = *oneTkn;
			else if (compareRez.gr8rEquals == isFalse)
				resultTkn = *zeroTkn;
			else
				isFailed = true;
			break;
		case EQUALITY_OPR8R_OPCODE :
			if (compareRez.equals == isTrue)
				resultTkn = *oneTkn;
			else if (compareRez.equals == isFalse)
				resultTkn = *zeroTkn;
			else
				isFailed = true;
			break;
		case NOT_EQUALS_OPR8R_OPCODE:
			if (compareRez.equals == isFalse)
				resultTkn = *oneTkn;
			else if (compareRez.equals == isTrue)
				resultTkn = *zeroTkn;
			else
				isFailed = true;
		default:
			isFailed = true;
			break;
	}

	if (!isFailed)
		ret_code = OK;

  return (ret_code);
}


/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execStandardMath (Token & operand1, Token & operand2, Token & resultTkn)	{
	int ret_code = GENERAL_FAILURE;
	bool isParamsValid = false;
	bool isMissedCase = false;
	uint64_t tmpUnsigned;
	int64_t tmpSigned;
	double tmpDouble;
	std::wstring tmpStr;

	// Passed in resultTkn has OPR8R op_code BEFORE it is overwritten by the result
	uint8_t op_code = resultTkn._unsigned;

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
					tmpSigned = operand1._signed / operand2._signed;
					break;
				case BINARY_MINUS_OPR8R_OPCODE :
					tmpSigned = operand1._signed - operand2._signed;
					break;
				case BINARY_PLUS_OPR8R_OPCODE :
					tmpSigned = operand1._signed + operand2._signed;
					break;
				case MOD_OPR8R_OPCODE:
					tmpSigned = operand1._signed % operand2._signed;
					break;
				default:
					isMissedCase = true;
					break;
			}

			if (!isMissedCase)
				resultTkn.resetToSigned(tmpSigned);

		} else if (!isAddingStrings && operand1.isSigned() && operand2.isUnsigned())	{
			// signed vs. unsigned
			switch (op_code)	{
				case MULTIPLY_OPR8R_OPCODE :
					tmpSigned = operand1._signed * operand2._unsigned;
					break;
				case DIV_OPR8R_OPCODE :
					tmpSigned = operand1._signed / operand2._unsigned;
					break;
				case BINARY_MINUS_OPR8R_OPCODE :
					tmpSigned = operand1._signed - operand2._unsigned;
					break;
				case BINARY_PLUS_OPR8R_OPCODE :
					tmpSigned = operand1._signed + operand2._unsigned;
					break;
				case MOD_OPR8R_OPCODE:
					tmpSigned = operand1._signed % operand2._unsigned;
					break;
				default:
					isMissedCase = true;
					break;
			}

			if (!isMissedCase)
				resultTkn.resetToSigned(tmpSigned);

		} else if (!isAddingStrings && operand1.isSigned() && operand2.tkn_type == DOUBLE_TKN)	{
			// signed vs. double
			switch (op_code)	{
				case MULTIPLY_OPR8R_OPCODE :
					tmpDouble = operand1._signed * operand2._double;
					break;
				case DIV_OPR8R_OPCODE :
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

			if (!isMissedCase)
				resultTkn.resetToDouble (tmpDouble);

		} else if (!isAddingStrings && operand1.isUnsigned() && operand2.isSigned())	{
			// unsigned vs. double
			switch (op_code)	{
				case MULTIPLY_OPR8R_OPCODE :
					tmpDouble = operand1._unsigned * operand2._double;
					break;
				case DIV_OPR8R_OPCODE :
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

			if (!isMissedCase)
				resultTkn.resetToDouble(tmpDouble);

		} else if (!isAddingStrings && operand1.isUnsigned() && operand2.isUnsigned())	{
			// unsigned vs. unsigned
			switch (op_code)	{
				case MULTIPLY_OPR8R_OPCODE :
					tmpUnsigned = operand1._unsigned * operand2._unsigned;
					break;
				case DIV_OPR8R_OPCODE :
					tmpUnsigned = operand1._unsigned / operand2._unsigned;
					break;
				case BINARY_MINUS_OPR8R_OPCODE :
					tmpUnsigned = operand1._unsigned - operand2._unsigned;
					break;
				case BINARY_PLUS_OPR8R_OPCODE :
					tmpUnsigned = operand1._unsigned + operand2._unsigned;
					break;
				case MOD_OPR8R_OPCODE:
					tmpUnsigned = operand1._unsigned % operand2._unsigned;
					break;
				default:
					isMissedCase = true;
					break;
			}

			if (!isMissedCase)
				resultTkn.resetToUnsigned(tmpUnsigned);

		} else if (!isAddingStrings && operand1.isUnsigned() && operand2.tkn_type == DOUBLE_TKN)	{
			// unsigned vs. double
			switch (op_code)	{
				case MULTIPLY_OPR8R_OPCODE :
					tmpDouble = operand1._unsigned * operand2._double;
					break;
				case DIV_OPR8R_OPCODE :
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

			if (!isMissedCase)
				resultTkn.resetToDouble (tmpDouble);

		} else if (!isAddingStrings && operand1.tkn_type == DOUBLE_TKN && operand2.isSigned())	{
			// double vs. signed
			switch (op_code)	{
				case MULTIPLY_OPR8R_OPCODE :
					tmpDouble = operand1._double * operand2._signed;
					break;
				case DIV_OPR8R_OPCODE :
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
			if (!isMissedCase)
				resultTkn.resetToDouble (tmpDouble);

		} else if (!isAddingStrings && operand1.tkn_type == DOUBLE_TKN && operand2.isUnsigned())	{
			// double vs. unsigned
			switch (op_code)	{
				case MULTIPLY_OPR8R_OPCODE :
					tmpDouble = operand1._double * operand2._unsigned;
					break;
				case DIV_OPR8R_OPCODE :
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

			if (!isMissedCase)
				resultTkn.resetToDouble (tmpDouble);

		} else if (!isAddingStrings && operand1.tkn_type == DOUBLE_TKN && operand2.tkn_type == DOUBLE_TKN)	{
			// double vs. double
			switch (op_code)	{
			case MULTIPLY_OPR8R_OPCODE :
				tmpDouble = operand1._double * operand2._double;
				break;
			case DIV_OPR8R_OPCODE :
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

			if (!isMissedCase)
				resultTkn.resetToDouble (tmpDouble);

		} else if (isAddingStrings && operand1.tkn_type == STRING_TKN && operand2.tkn_type == STRING_TKN)	{
			if (!operand1._string.empty())
				tmpStr = operand1._string;
			if (!operand2._string.empty())
				tmpStr.append(operand2._string);

			resultTkn.resetToString (tmpStr);
		}
	}

	if (isParamsValid && !isMissedCase)
		ret_code = OK;

	return (ret_code);
}


/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execShift (Token & operand1, Token & operand2, Token & resultTkn)	{
	int ret_code = GENERAL_FAILURE;

	// Passed in resultTkn has OPR8R op_code BEFORE it is overwritten by the result
	uint8_t op_code = resultTkn._unsigned;

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

			resultTkn.resetToUnsigned (tmpUnsigned);

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

			resultTkn.resetToUnsigned (tmpUnsigned);

		} else	{
			// operand1 is negative, so we have to keep the sign bit around
			if (op_code == LEFT_SHIFT_OPR8R_OPCODE)	{
				tmpSigned = operand1._signed << operand2._signed;
			} else {
				tmpSigned = operand1._signed >> operand2._signed;
			}
			resultTkn.resetToSigned (tmpSigned);
		}

		ret_code = OK;
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execBitWiseOp (Token & operand1, Token & operand2, Token & resultTkn)	{
	int ret_code = GENERAL_FAILURE;
	bool isParamsValid = true;
	bool isMissedCase = false;

	// Passed in resultTkn has OPR8R op_code BEFORE it is overwritten by the result
	uint8_t op_code = resultTkn._unsigned;
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
		resultTkn.resetToUnsigned(bitWiseResult);
		ret_code = OK;
	}


	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::execBinaryOp(std::vector<Token> & exprTknStream, int & callersIdx)     {
	int ret_code = GENERAL_FAILURE;
	bool isSuccess = false;
	int opr8rIdx = callersIdx;

	if (exprTknStream.size() > opr8rIdx && exprTknStream.size() >= 3 && opr8rIdx >= 2 && exprTknStream[opr8rIdx].tkn_type == EXEC_OPR8R_TKN)	{
		uint8_t op_code = exprTknStream[opr8rIdx]._unsigned;

		// TODO: If these are KEYWORD_TKNs, then do a variable name lookup in our NameSpace
		Token operand1 = exprTknStream[opr8rIdx-2];
		Token operand2 = exprTknStream[opr8rIdx-1];

		switch (op_code)	{
			case MULTIPLY_OPR8R_OPCODE :
			case DIV_OPR8R_OPCODE :
			case MOD_OPR8R_OPCODE :
			case BINARY_PLUS_OPR8R_OPCODE :
			case BINARY_MINUS_OPR8R_OPCODE :
				if (OK == execStandardMath (operand1, operand2, exprTknStream[opr8rIdx]))
					isSuccess = true;

				break;
			case LEFT_SHIFT_OPR8R_OPCODE :
			case RIGHT_SHIFT_OPR8R_OPCODE :
				if (OK == execShift (operand1, operand2, exprTknStream[opr8rIdx]))
					isSuccess = true;
				break;

			case LESS_THAN_OPR8R_OPCODE :
			case LESS_EQUALS_OPR8R8_OPCODE :
			case GREATER_THAN_OPR8R_OPCODE :
			case GREATER_EQUALS_OPR8R8_OPCODE :
			case EQUALITY_OPR8R_OPCODE :
			case NOT_EQUALS_OPR8R_OPCODE :
				if (OK == execEquivalenceOp (operand1, operand2, exprTknStream[opr8rIdx]))
					isSuccess = true;
				break;

			case BITWISE_AND_OPR8R_OPCODE :
			case BITWISE_XOR_OPR8R_OPCODE :
			case BITWISE_OR_OPR8R_OPCODE :
				if (OK == execBitWiseOp (operand1, operand2, exprTknStream[opr8rIdx]))
					isSuccess = true;
				break;

			case LOGICAL_AND_OPR8R_OPCODE :
				if (operand1.evalResolvedTokenAsIf() && operand2.evalResolvedTokenAsIf())
					exprTknStream[opr8rIdx] = *oneTkn;
				else
					exprTknStream[opr8rIdx] = *zeroTkn;
				isSuccess = true;
				break;

			case LOGICAL_OR_OPR8R_OPCODE :
				if (operand1.evalResolvedTokenAsIf() || operand2.evalResolvedTokenAsIf())
					exprTknStream[opr8rIdx] = *oneTkn;
				else
					exprTknStream[opr8rIdx] = *zeroTkn;
				isSuccess = true;
				break;

			case ASSIGNMENT_OPR8R_OPCODE :
				break;

			// TODO: Might be able to leverage earlier work (e.g. BINARY_PLUS_OPR8R_OPCODE)
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

		if (isSuccess)	{
			// Last parameter for .erase does not get deleted
			exprTknStream.erase (exprTknStream.begin() + (opr8rIdx-2), exprTknStream.begin() + (opr8rIdx));
			callersIdx -= 2;
			ret_code = OK;
		}
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
int RunTimeInterpreter::takeTernaryTrue (std::vector<Token> & exprTknStream, int & callersIdx)     {
	int ret_code = GENERAL_FAILURE;
	int ternary1stIdx = callersIdx;
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
				callersIdx = (ternary1stIdx + 1) - 2;
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
int RunTimeInterpreter::takeTernaryFalse (std::vector<Token> & exprTknStream, int & callersIdx)     {
	int ret_code = GENERAL_FAILURE;

	int ternary1stIdx = callersIdx;
	int idx;
	int ternary2ndIdx = findNextTernary2ndIdx (exprTknStream, ternary1stIdx);

	if (ternary2ndIdx > ternary1stIdx && ternary2ndIdx < exprTknStream.size())	{
		// Remove ternary conditional, TERNARY_1ST and up to and including TERNARY_2ND
		exprTknStream.erase(exprTknStream.begin() + (ternary1stIdx - 1), exprTknStream.begin() + ternary2ndIdx + 1);

		// Adjust callersIdx by accounting for deletion of [Conditional Operand][?]
		callersIdx = ternary1stIdx - 2;
		ret_code = OK;
	}

	return (ret_code);
}

/* ****************************************************************************
 * (ternaryConditional) ? (truePath) : (falsePath)
 *          1st ternary ^ 2nd ternary^
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
int RunTimeInterpreter::getEndOfSubExprIdx (std::vector<Token> & exprTknStream, int startIdx, int & lastIdxOfExpr)	{
	int ret_code = GENERAL_FAILURE;

	int resolvedRandCnt = 0;
	int idx;
	bool isFailed = false;
	lastIdxOfExpr = 0;

	for (idx = startIdx; idx < exprTknStream.size() && !isFailed && lastIdxOfExpr == 0; idx++)	{
		Token currTkn = exprTknStream[idx];

		if (currTkn.tkn_type != EXEC_OPR8R_TKN)	{
			resolvedRandCnt++;

		} else	{
			// Get details on this OPR8R to determine how it affects resolvedRandCnt
			Operator opr8rDeets;
			if (OK != execTerms.getExecOpr8rDetails(currTkn._unsigned, opr8rDeets))	{
				isFailed = true;

			} else if (opr8rDeets.type_mask & TERNARY_1ST)	{
				// Find the corresponding TERNARY_2ND, and get the ending index of the TERNARY FALSE path sub-expression
				int ternary2ndIdx = findNextTernary2ndIdx (exprTknStream, idx);

				if (ternary2ndIdx == 0 || ternary2ndIdx >= exprTknStream.size())	{
					isFailed = true;

				} else if (OK != getEndOfSubExprIdx (exprTknStream, ternary2ndIdx + 1, lastIdxOfExpr))	{
					isFailed = true;

				} else if (lastIdxOfExpr == 0 || lastIdxOfExpr > exprTknStream.size())	{
					isFailed = true;
				}

			} else if (opr8rDeets.type_mask & TERNARY_2ND)	{
				// TERNARY_2ND was *NOT* expected!
				isFailed = true;

			} else if (opr8rDeets.type_mask & BINARY)	{
				if (resolvedRandCnt >= 2)	{
					// If this OPR8R were executed, 2 Operands would get consumed and 1 result Operand would be left
					resolvedRandCnt--;

				} else if (resolvedRandCnt == 1 && idx > 0)	{
					// There must be at least 1 term in an expression!
					// [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
					//                                     ^idx
					lastIdxOfExpr = idx - 1;

				} else	{
					isFailed = true;
				}

				// TODO: BINARY and resolvedRandCnt == 1
			} else if (opr8rDeets.type_mask & UNARY)	{
				// TODO: Handle this case
				isFailed = true;
			}
		}
	}

	if (lastIdxOfExpr != 0)	{
		ret_code = OK;

	} else if (!isFailed && resolvedRandCnt == 1 && idx == exprTknStream.size())	{
		// A single resolved term at the very end makes for a valid sub-expression
		lastIdxOfExpr = exprTknStream.size() - 1;
		ret_code = OK;
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
				Operator opr8r_obj;

				if (OK != execTerms.getExecOpr8rDetails(nxtTkn._unsigned, opr8r_obj))	{
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
						else	{
							// Deletions made; Break out of loop so currIdx -> 0
							// TODO: Is there a way to be SMRT about this?
							break;
						}

					} else if (opr8r_obj.type_mask & TERNARY_2ND)	{
						// TODO: Is this an error condition? Or should there be a state machine or something?
						failedOnLineNum == 0 ? failedOnLineNum = __LINE__ : 1;
						isFailed = true;

					} else if ((opr8r_obj.type_mask & BINARY) && opr8r_obj.numReqExecOperands == 2)	{
						if (OK != execBinaryOp (exprTknStream, currIdx))	{
							failedOnLineNum == 0 ? failedOnLineNum = __LINE__ : 1;
							isFailed = true;
						} else	{
							// Deletions made; Break out of loop so currIdx -> 0
							// TODO: Is there a way to be SMRT about this?
							break;
						}
					} else	{
						failedOnLineNum == 0 ? failedOnLineNum = __LINE__ : 1;
						isFailed = true;
					}

					if (!isFailed)	{
						// Operation result stored in Token that previously held the OPR8R. We need to delete any associatd operands
						int StartDelIdx = currIdx - opr8r_obj.numReqExecOperands;
						exprTknStream.erase(exprTknStream.begin() + StartDelIdx, exprTknStream.begin() + StartDelIdx + 1);
						currIdx -= opr8r_obj.numReqExecOperands;
					}
				}
			}

			if (currIdx + 1 == exprTknStream.size())
				isEOLreached = true;
			else
				currIdx++;
		}

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

	// TODO: See the final result
	dumpTokenList (exprTknStream, thisSrcFile, __LINE__);

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
		Token * listTkn = tokenStream[idx];
		std::wcout << L"[" ;
		if (listTkn->tkn_type == EXEC_OPR8R_TKN)
			std::wcout << execTerms.getOpr8rStrFor(listTkn->_unsigned);
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
void RunTimeInterpreter::dumpTokenList (std::vector<Token> & tokenStream, std::wstring callersSrcFile, int lineNum)	{
	std::wstring tknStrmStr = L"";

	std::wcout << L"********** dumpTokenList called from " << callersSrcFile << L":" << lineNum << L" **********" << std::endl;
	int idx;
	for (idx = 0; idx < tokenStream.size(); idx++)	{
		Token listTkn = tokenStream[idx];
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

}
