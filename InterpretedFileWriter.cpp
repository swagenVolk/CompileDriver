/*
 * InterpretedFileWriter.cpp
 *
 *  Created on: Oct 14, 2024
 *      Author: Mike Volk
 *
 *	TODO:
 *	Make sure error messages are captured
 *	Make recursive fxn that only builds a Token list that will be written out separately
 *	so that PREFIX ops can happen BEFORE expression and POSTFIX ops can happen AFTER
 *
 */

#include "InterpretedFileWriter.h"
#include "InfoWarnError.h"
#include "OpCodes.h"
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

InterpretedFileWriter::InterpretedFileWriter(std::string output_file_name, CompileExecTerms & inExecTerms
		, std::shared_ptr<UserMessages> userMessages)
	: outputStream (output_file_name, outputStream.binary | outputStream.out)
{
	// TODO Auto-generated constructor stub
	execTerms = & inExecTerms;
	this->userMessages = userMessages;

	// TODO: Are these asserts even necessary when the & operator is used in parameter list?
	assert (execTerms != NULL);
	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");

	if (!outputStream.is_open()) {
		std::cout << "ERROR: Failed to open output file " << output_file_name << std::endl;
	}

	assert (outputStream.is_open());

}

InterpretedFileWriter::~InterpretedFileWriter() {
	// TODO Auto-generated destructor stub
	if (outputStream.is_open())
		outputStream.close();
}

#if 0
/* ****************************************************************************
 * writeExpr_12_Opr8r will recursively walk an expression tree in
 * [1st child][2nd child][current] order to create a flat stream of op_code
 * Tokens the Interpreter can use to resolve the expression at execution time.
 *
 * **************************************************************************************************************************************************************
 * Example C expression
 * (1 + 2 * (3 + 4 * (5 + 6 * 7 * (8 + 9))))
 *
 * And how it will get written to interpreted file.
 * NOTE that we'll go L2R through this list until we find an OPR8R *preceded* by the
 * required # of operands in this case [8] [9] [+]. Note that in the above C expression,
 * 8 + 9 is in the most deeply nested parentheses and therefore has the highest precedence
 * [1] [2] [3] [4] [5] [42] [8] [9] [+] [*] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [5] [42] [17] [*] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [5] [714] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [719] [*] [+] [*] [+]
 * [1] [2] [3] [2876] [+] [*] [+]
 * [1] [2] [2879] [*] [+]
 * [1] [5758] [+]
 * [5759]
 *
 * **************************************************************************************************************************************************************
 * NOTE the position of the [?] and [:] ternary OPR8Rs their placement differs from other OPR8Rs. At run time, [?] is treated more like a UNARY in that the
 * conditional expression will have already been resolved as [TRUE|FALSE] ahead of time.  The [:] is placed BETWEEN the TRUE and FALSE path expressions as a hint
 * for the Interpreter to know where each path begins and ends. Normal OPR8Rs would be placed AFTER the two expressions. The Interpreter probably *could* execute
 * both expressions but only commit results on the chosen path, but that seems performative and not especially useful.
 *
 * Example nested ternary C expression - TRUE path taken
 * (1 + 2 * (3 + 4 * (5 + 6 * 7 * (count == 1 ? 10 : count == 2 ? 11 : count == 3 ? 12 : count == 4 ? 13 : 33))))
 *
 * count = 1
 * [1] [2] [3] [4] [5] [6] [7] [*] [count] [1] [==] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                      ^^ 1st ^^^ ^^^ 2nd ^^^^^^^^
 * [1] [2] [3] [4] [5] [42] [1] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                      1st 2nd
 * [1] [2] [3] [4] [5] [42] [1] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                           ^^^1st - TRUE path leave expression before [:] (e.g. [10]) in the stream and consume the FALSE path (1 complete sub-expression within)
 * [1] [2] [3] [4] [5] [42] [1] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                              numOperands: 1       2   1    1(?) - but need to consume this nested TERNARY - if this was a BINARY OPR8R (UNARY? POSTFIX? PREFIX? STATEMENT_ENDER?)
 *                                                                   then we'd be done with it. Consume everything up to and including the [:] OPR8R, then consume the next expression
 *
 * [1] [2] [3] [4] [5] [42] [1] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                                                             numOprands: 1       2   1    1(?) ^^^ ^^^  1       2   1   1(?) ^^^ ^^^ 1    ^ Not enough operands for this OPR8R, so
 *                                                                                                                                            we've closed off the nested TERNARYs
 *                                                                                                                                            and need to preserve these OPR8Rs

 * **************************************************************************************************************************************************************
 * Example nested ternary C expression - All FALSE paths taken
 * (1 + 2 * (3 + 4 * (5 + 6 * 7 * (count == 1 ? 10 : count == 2 ? 11 : count == 3 ? 12 : count == 4 ? 13 : 33))))
 *
 * count = 5
 * [1] [2] [3] [4] [5] [6] [7] [*] [count] [1] [==] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [5]        [42]              [0] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [5]        [42]              [0] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                                              ^ FALSE remove^
 * [1] [2] [3] [4] [5] [42] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [5] [42]              [0] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                                       ^ FALSE remove^
 * [1] [2] [3] [4] [5] [42] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [5] [42] [0] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                          ^ FALSE remove^
 * [1] [2] [3] [4] [5] [42] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [5] [42]             [0] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                                      ^ FALSE remove^
 * [1] [2] [3] [4] [5] [42] [33] [*] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [5]           [1386] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4]            		 [1391] [*] [+] [*] [+]
 * [1] [2] [3]             		 		 [5564] [+] [*] [+]
 * [1] [2]              		 		 	 [5567] [*] [+]
 * [1]               		 		 	        [11134] [+]
 * [11135]
 *
 * ***************************************************************************/
int InterpretedFileWriter::writeExpr_12_Opr8r (std::shared_ptr<ExprTreeNode> currBranch, std::vector<std::shared_ptr<Token>> & flatExprTknList)	{
	int ret_code = GENERAL_FAILURE
	bool isFailed = false
	bool isTernary1st = false
	bool isTernary2nd = false
	bool isTernary1or2 = false

	if (currBranch != NULL)	{

		if (currBranch->originalTkn->tkn_type == SRC_OPR8R_TKN && (currBranch->originalTkn->_string == execTerms->get_ternary_1st()
				|| currBranch->originalTkn->_string == execTerms->get_ternary_2nd()))	{
			isTernary1or2 = true
		}

		if (currBranch->_1stChild != NULL)	{
			if (OK != writeExpr_12_Opr8r (currBranch->_1stChild, flatExprTknList))
				isFailed = true

			if (isTernary1or2)	{
				// TODO: 'Splain yo self
				std::wcout << "[" << currBranch->originalTkn->_string << "] "
				if (OK != writeToken(currBranch->originalTkn, flatExprTknList))
					isFailed = true
			}

			if (!isFailed && currBranch->_2ndChild != NULL)	{
				if (OK != writeExpr_12_Opr8r (currBranch->_2ndChild, flatExprTknList))
					isFailed = true
			}
		}

		if (!isFailed)	{
			if (!isTernary1or2)	{
				std::wcout << "[" << currBranch->originalTkn->_string << "] "
				ret_code = writeToken(currBranch->originalTkn, flatExprTknList)

			} else	{
				ret_code = OK
			}
		}
	}

	return (ret_code);
}
#endif

/* ****************************************************************************
 * Tree that represents an expression has already been flattened.  This fxn
 * just needs to write the Token stream out to the interpreted file as a
 * flexible length object.
 * ***************************************************************************/
int InterpretedFileWriter::writeFlatExprToFile (std::vector<Token> & flatExprTknList)	{
	int ret_code = GENERAL_FAILURE;
	int idx;
	bool isFailed = false;

	if (flatExprTknList.empty())	{
  	userMessages->logMsg (INTERNAL_ERROR, L"Flattened expression list is EMPTY!", thisSrcFile, __LINE__, 0);

	} else	{
		// Make sure we're at END of our output file
		// TODO: Account for nested expressions (???)
		outputStream.seekp(0, std::fstream::end);

		uint32_t startFilePos = outputStream.tellp();

		uint32_t length_pos = writeFlexLenOpCode (EXPRESSION_OPCODE);
		if (0 != length_pos)	{
			// Save off the position where the expression's total length is stored and
			// write 0s to it. It will get filled in later when writing the entire expression out has
			// been completed.
			for (idx = 0; idx < flatExprTknList.size() && !isFailed; idx++)	{
				if (OK != writeToken(flatExprTknList[idx]))
					isFailed = true;
			}

			if (!isFailed)
				ret_code = writeObjectLen (startFilePos, length_pos);

		}
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::writeObjectLen (uint32_t objStartPos, uint32_t objLengthPos)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;


	// Need fill in now known length of this expression
	uint32_t currFilePos = outputStream.tellp();
	uint32_t exprLen = (currFilePos - objStartPos);

	// Write the current object's length
	outputStream.seekp(objLengthPos, std::fstream::beg);
	if (OK != writeRawUnsigned(exprLen, NUM_BITS_IN_DWORD))
		isFailed = true;

	// Now reset file pointer to pos at fxn begin, after the object was entirely written out
	outputStream.seekp(0, std::fstream::end);

	if (currFilePos == outputStream.tellp() && !isFailed)
		ret_code = OK;

	return (ret_code);
}


/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::writeAtomicOpCode (uint8_t op_code)	{
	int ret_code = GENERAL_FAILURE;

	if (op_code >= ATOMIC_OPCODE_RANGE_BEGIN && op_code <= ATOMIC_OPCODE_RANGE_END)	{
		if (op_code <= LAST_VALID_OPR8R_OPCODE)	{
			ret_code = writeRawUnsigned (op_code, NUM_BITS_IN_BYTE);

		} else if (op_code >= FIRST_VALID_DATA_TYPE_OPCODE && op_code <= LAST_VALID_DATA_TYPE_OPCODE)	{
			ret_code = writeRawUnsigned (op_code, NUM_BITS_IN_BYTE);
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::writeFlexLenOpCode (uint8_t op_code)	{
	int lengthPos = 0;
	int tmpLenPos = 0;
	uint32_t tempLen = 0x0;

	if (op_code >= FIRST_VALID_FLEX_LEN_OPCODE && op_code <= LAST_VALID_FLEX_LEN_OPCODE)	{
		if (OK == writeRawUnsigned (op_code, NUM_BITS_IN_BYTE))	{
			tmpLenPos = outputStream.tellp();
			if (OK == writeRawUnsigned (tempLen, NUM_BITS_IN_DWORD))
			lengthPos = tmpLenPos;
		}
	}

	return (lengthPos);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::write8BitOpCode (uint8_t op_code, uint8_t  payload)	{
	int ret_code = GENERAL_FAILURE;

	if (op_code == INT8_OPCODE)
		std::wcout << L"TODO: STOP!" << std::endl;

	if (op_code >= FIXED_OPCODE_RANGE_BEGIN && op_code <= FIXED_OPCODE_RANGE_END)	{
		if (op_code == UINT8_OPCODE || op_code == INT8_OPCODE)	{
			// Write op_code followed by payload out to file
			if (OK == writeRawUnsigned (op_code, NUM_BITS_IN_BYTE)
					&& OK == writeRawUnsigned (payload, NUM_BITS_IN_BYTE))
				ret_code = OK;
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::write16BitOpCode (uint8_t op_code, uint16_t  payload)	{
	int ret_code = GENERAL_FAILURE;

	if (op_code >= FIXED_OPCODE_RANGE_BEGIN && op_code <= FIXED_OPCODE_RANGE_END)	{
		if (op_code == UINT16_OPCODE || op_code == INT16_OPCODE)	{
			// Write op_code followed by payload out to file
			if (OK == writeRawUnsigned (op_code, NUM_BITS_IN_BYTE)
					&& OK == writeRawUnsigned (payload, NUM_BITS_IN_WORD))
				ret_code = OK;
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::write32BitOpCode (uint8_t op_code, uint32_t  payload)	{
	int ret_code = GENERAL_FAILURE;

	if (op_code >= FIXED_OPCODE_RANGE_BEGIN && op_code <= FIXED_OPCODE_RANGE_END)	{
		if (op_code == UINT32_OPCODE || op_code == INT32_OPCODE)	{
			// Write op_code followed by payload out to file
			if (OK == writeRawUnsigned (op_code, NUM_BITS_IN_BYTE)
					&& OK == writeRawUnsigned(payload, NUM_BITS_IN_DWORD))
				ret_code = OK;
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::write64BitOpCode (uint8_t op_code, uint64_t  payload)	{
	int ret_code = GENERAL_FAILURE;

	if (op_code >= FIXED_OPCODE_RANGE_BEGIN && op_code <= FIXED_OPCODE_RANGE_END)	{
		if (op_code == UINT64_OPCODE || op_code == INT64_OPCODE)	{
			// Write op_code followed by payload out to file
			if (OK == writeRawUnsigned (op_code, NUM_BITS_IN_BYTE)
					&& OK == writeRawUnsigned (payload, NUM_BITS_IN_QWORD))
				ret_code = OK;
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * TODO: Any kind of check for success?
 * ***************************************************************************/
int InterpretedFileWriter::writeRawUnsigned (uint64_t  payload, int payloadBitSize)	{
	int ret_code = GENERAL_FAILURE;
	uint64_t shift_mask = 0xFF << (payloadBitSize - NUM_BITS_IN_BYTE);
	uint64_t maskedQword;
	uint8_t nextByte;

	assert (payloadBitSize == NUM_BITS_IN_BYTE || payloadBitSize == NUM_BITS_IN_WORD || payloadBitSize == NUM_BITS_IN_DWORD || payloadBitSize == NUM_BITS_IN_QWORD);

	// TODO: Endian-ness is accounted for now Make this work in other Raw* fxns
	for (int idx = payloadBitSize/NUM_BITS_IN_BYTE; idx > 0; idx--)	{
		maskedQword = (payload & shift_mask);
		maskedQword >>= ((idx - 1) * NUM_BITS_IN_BYTE);
		nextByte = maskedQword;
		outputStream.write(reinterpret_cast<char*>(&nextByte), 1);
		shift_mask >>= NUM_BITS_IN_BYTE;
	}
	ret_code = OK;

	return (ret_code);
}

/* ****************************************************************************
 * TODO: Any kind of check for success?
 * ***************************************************************************/
int InterpretedFileWriter::writeString (uint8_t op_code, std::wstring tokenStr)	{
	int ret_code = GENERAL_FAILURE;

	uint32_t startFilePos = outputStream.tellp();

	uint32_t length_pos = writeFlexLenOpCode (op_code);
	if (0 != length_pos)	{
		// Save off the position where the expression's total length is stored and
		// write 0s to it. It will get filled in later when writing the entire expression out has
		// been completed.

		if (OK == writeRawString (tokenStr))	{
			ret_code = writeObjectLen (startFilePos, length_pos);
		}
	}

	return (ret_code);
}


/* ****************************************************************************
 * TODO: Any kind of check for success?
 * TODO: Endian-ness is buggered
 * ***************************************************************************/
int InterpretedFileWriter::writeRawString (std::wstring tokenStr)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;

	if (!tokenStr.empty())	{
		int numBytes = tokenStr.size() * 2;
		const wchar_t * strBffr = tokenStr.data();
		uint16_t nxtWord;
		int idx;

		for (idx = 0; idx < tokenStr.size() && !isFailed; idx++)	{
			nxtWord = strBffr[idx];
			if (OK != writeRawUnsigned (strBffr[idx], NUM_BITS_IN_WORD))
				isFailed = true;
			// outputStream.write(reinterpret_cast<char*>(&nxtWord), NUM_BYTES_IN_WORD)
		}

		if (idx == tokenStr.size() && !isFailed)
			ret_code = OK;

	} else	{
		ret_code = OK;
	}

	return (ret_code);
}


/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::writeToken (Token token)	{
	int ret_code = GENERAL_FAILURE;

	uint8_t tkn8Val;

	if (token.tkn_type == SRC_OPR8R_TKN && execTerms->get_statement_ender() == token._string)
		// TODO: Skip writing [] out not needed
		ret_code = OK;

	else {
		switch(token.tkn_type)	{
		case USER_WORD_TKN :
			ret_code = writeString (VAR_NAME_OPCODE, token._string);
			break;
		case STRING_TKN :
			ret_code = writeString (STRING_OPCODE, token._string);
			break;
		case DATETIME_TKN :
			if (OK == writeAtomicOpCode(DATETIME_OPCODE))
				ret_code = writeRawString(token._string);
			break;
		case UINT8_TKN :
			tkn8Val = token._unsigned;
			if (OK == writeRawUnsigned (UINT8_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (tkn8Val, NUM_BITS_IN_BYTE);
			break;
		case UINT16_TKN :
			if (OK == writeRawUnsigned (UINT16_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token._unsigned, NUM_BITS_IN_WORD);
			break;
		case UINT32_TKN :
			if (OK == writeRawUnsigned (UINT32_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token._unsigned, NUM_BITS_IN_DWORD);
			break;
		case UINT64_TKN :
			if (OK == writeRawUnsigned (UINT64_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token._unsigned, NUM_BITS_IN_QWORD);
			break;
		case INT8_TKN :
			// TODO: Check
			if (OK == writeRawUnsigned (INT8_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token._signed, NUM_BITS_IN_BYTE);
			break;
		case INT16_TKN :
			// TODO: Check
			if (OK == writeRawUnsigned (INT16_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token._signed, NUM_BITS_IN_WORD);
			break;
		case INT32_TKN :
			// TODO: Check
			if (OK == writeRawUnsigned (INT32_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token._signed, NUM_BITS_IN_DWORD);
			break;
		case INT64_TKN :
			// TODO: Check
			if (OK == writeRawUnsigned (INT64_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token._signed, NUM_BITS_IN_QWORD);
			break;
		case DOUBLE_TKN :
				ret_code = writeString (DOUBLE_OPCODE, token._string);
			break;
		case EXEC_OPR8R_TKN :
				if (token._unsigned == POST_INCR_OPR8R_OPCODE || token._unsigned == POST_DECR_OPR8R_OPCODE 
					|| token._unsigned == PRE_INCR_OPR8R_OPCODE || token._unsigned == PRE_DECR_OPR8R_OPCODE)		{
					// Actionable [PRE|POST]FIX OPR8R with a list of variable names to affect
					uint32_t objStartPos = outputStream.tellp();
					uint32_t length_pos = writeFlexLenOpCode (token._unsigned);
					if (0 != length_pos)	{
						// Save off the position where the expression's total length is stored and
						if (OK == writeString (STRING_OPCODE, token._string))	{
							ret_code = writeObjectLen (objStartPos, length_pos);
						}
					}
				} else	{
					ret_code = writeRawUnsigned (token._unsigned, NUM_BITS_IN_BYTE);
				}
			break;
		case SPR8R_TKN :
		case SRC_OPR8R_TKN :
		default:
			break;

		}
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
uint32_t InterpretedFileWriter::getWriteFilePos ()	{

	uint32_t currFilePos = outputStream.tellp();
	return (currFilePos);

}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::addTokenToFlatList (std::shared_ptr<Token> token, std::vector<Token> & flatExprTknList)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;

	if (token->tkn_type == SRC_OPR8R_TKN && execTerms->get_statement_ender() == token->_string)
		// TODO: Skip writing [] out not needed
		ret_code = OK;

	else {
		switch(token->tkn_type)	{
		case USER_WORD_TKN :
		case STRING_TKN :
		case DATETIME_TKN :
		case UINT8_TKN :
		case UINT16_TKN :
		case UINT32_TKN :
		case UINT64_TKN :
		case INT8_TKN :
		case INT16_TKN :
		case INT32_TKN :
		case INT64_TKN :
		case DOUBLE_TKN :
		case SRC_OPR8R_TKN :
			break;
		default:
			isFailed = true;
			break;
		}

		if (!isFailed)	{
			if (token->tkn_type == SRC_OPR8R_TKN || token->tkn_type == EXEC_OPR8R_TKN)	{
				// Prior to putting OPR8Rs in flattened list, make them EXEC_OPR8R_TKNs
				// since the caller is expected to use the RunTimeInterpreter to resolve
				// the expression
				uint8_t op_code = execTerms->getOpCodeFor (token->_string);
				token->resetToken();
				token->tkn_type = EXEC_OPR8R_TKN;
				token->_unsigned = op_code;
				if (op_code == INVALID_OPCODE)
					isFailed = true;

			}

			// Store a copy of this Token in the flattened list
			flatExprTknList.push_back (*token);
			token.reset();
			if (!isFailed)
				ret_code = OK;
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * writeExpr_12_Opr8r will recursively walk an expression tree in
 * [1st child][2nd child][current] order to create a flat stream of op_code
 * Tokens the Interpreter can use to resolve the expression at execution time.
 *
 * **************************************************************************************************************************************************************
 * Example C expression
 * (1 + 2 * (3 + 4 * (5 + 6 * 7 * (8 + 9))))
 *
 * And how it will get written to interpreted file.
 * NOTE that we'll go L2R through this list until we find an OPR8R *preceded* by the
 * required # of operands in this case [8] [9] [+]. Note that in the above C expression,
 * 8 + 9 is in the most deeply nested parentheses and therefore has the highest precedence
 * [1] [2] [3] [4] [5] [42] [8] [9] [+] [*] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [5] [42] [17] [*] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [5] [714] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [719] [*] [+] [*] [+]
 * [1] [2] [3] [2876] [+] [*] [+]
 * [1] [2] [2879] [*] [+]
 * [1] [5758] [+]
 * [5759]
 *
 * **************************************************************************************************************************************************************
 * NOTE the position of the [?] and [:] ternary OPR8Rs their placement differs from other OPR8Rs. At run time, [?] is treated more like a UNARY in that the
 * conditional expression will have already been resolved as [TRUE|FALSE] ahead of time.  The [:] is placed BETWEEN the TRUE and FALSE path expressions as a hint
 * for the Interpreter to know where each path begins and ends. Normal OPR8Rs would be placed AFTER the two expressions. The Interpreter probably *could* execute
 * both expressions but only commit results on the chosen path, but that seems performative and not especially useful.
 *
 * Example nested ternary C expression - TRUE path taken
 * (1 + 2 * (3 + 4 * (5 + 6 * 7 * (count == 1 ? 10 : count == 2 ? 11 : count == 3 ? 12 : count == 4 ? 13 : 33))))
 *
 * count = 1
 * [1] [2] [3] [4] [5] [6] [7] [*] [count] [1] [==] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                      ^^ 1st ^^^ ^^^ 2nd ^^^^^^^^
 * [1] [2] [3] [4] [5] [42] [1] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                      1st 2nd
 * [1] [2] [3] [4] [5] [42] [1] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                           ^^^1st - TRUE path leave expression before [:] (e.g. [10]) in the stream and consume the FALSE path (1 complete sub-expression within)
 * [1] [2] [3] [4] [5] [42] [1] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                              numOperands: 1       2   1    1(?) - but need to consume this nested TERNARY - if this was a BINARY OPR8R (UNARY? POSTFIX? PREFIX? STATEMENT_ENDER?)
 *                                                                   then we'd be done with it. Consume everything up to and including the [:] OPR8R, then consume the next expression
 *
 * [1] [2] [3] [4] [5] [42] [1] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                                                             numOprands: 1       2   1    1(?) ^^^ ^^^  1       2   1   1(?) ^^^ ^^^ 1    ^ Not enough operands for this OPR8R, so
 *                                                                                                                                            we've closed off the nested TERNARYs
 *                                                                                                                                            and need to preserve these OPR8Rs

 * **************************************************************************************************************************************************************
 * Example nested ternary C expression - All FALSE paths taken
 * (1 + 2 * (3 + 4 * (5 + 6 * 7 * (count == 1 ? 10 : count == 2 ? 11 : count == 3 ? 12 : count == 4 ? 13 : 33))))
 *
 * count = 5
 * [1] [2] [3] [4] [5] [6] [7] [*] [count] [1] [==] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [5]        [42]              [0] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [5]        [42]              [0] [?] [10] [:] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                                              ^ FALSE remove^
 * [1] [2] [3] [4] [5] [42] [count] [2] [==] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [5] [42]              [0] [?] [11] [:] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                                       ^ FALSE remove^
 * [1] [2] [3] [4] [5] [42] [count] [3] [==] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [5] [42] [0] [?] [12] [:] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                          ^ FALSE remove^
 * [1] [2] [3] [4] [5] [42] [count] [4] [==] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [5] [42]             [0] [?] [13] [:] [33] [*] [+] [*] [+] [*] [+]
 *                                      ^ FALSE remove^
 * [1] [2] [3] [4] [5] [42] [33] [*] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4] [5]           [1386] [+] [*] [+] [*] [+]
 * [1] [2] [3] [4]            		 [1391] [*] [+] [*] [+]
 * [1] [2] [3]             		 		 [5564] [+] [*] [+]
 * [1] [2]              		 		 	 [5567] [*] [+]
 * [1]               		 		 	        [11134] [+]
 * [11135]
 *
 * ***************************************************************************/
int InterpretedFileWriter::makeFlatExpr_12_Opr8r (std::shared_ptr<ExprTreeNode> currBranch, std::vector<Token> & flatExprTknList)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;
	bool isTernary1st = false;
	bool isTernary2nd = false;
	bool isTernary1or2 = false;

	if (currBranch != NULL)	{

		if (currBranch->originalTkn->tkn_type == SRC_OPR8R_TKN && (currBranch->originalTkn->_string == execTerms->get_ternary_1st()
				|| currBranch->originalTkn->_string == execTerms->get_ternary_2nd()))	{
			isTernary1or2 = true;
		}

		if (currBranch->_1stChild != NULL)	{
			if (OK != makeFlatExpr_12_Opr8r (currBranch->_1stChild, flatExprTknList))
				isFailed = true;

			if (isTernary1or2)	{
				if (OK != addTokenToFlatList(currBranch->originalTkn, flatExprTknList))
					isFailed = true;
			}

			if (!isFailed && currBranch->_2ndChild != NULL)	{
				if (OK != makeFlatExpr_12_Opr8r (currBranch->_2ndChild, flatExprTknList))
					isFailed = true;
			}
		}

		if (!isFailed)	{
			if (!isTernary1or2)	{
				ret_code = addTokenToFlatList(currBranch->originalTkn, flatExprTknList);

			} else	{
				ret_code = OK;
			}
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * Check for PREFIX OPR8Rs and POSTFIX OPR8Rs in the flattened expression. These
 * OPR8Rs will be treated as NO-OPs, so we need to add the "active" PREFIX OPR8Rs
 * BEFORE the main expression, and POSTFIX OPR8Rs AFTER the main expression.  
 * This fxn will check that variables only participate ONCE in a [PRE|POST]FIX
 * operation.
 * TODO: Variable name existence and type checking should have happened ealier
 * by the expression parser, but double check this later.
 * ***************************************************************************/
int InterpretedFileWriter::checkPrePostFix (std::vector<Token> & flatExprTknList, std::vector<std::wstring> & preIncList
	, std::vector<std::wstring> & preDecList, std::vector<std::wstring> & postIncList, std::vector<std::wstring> & postDecList
	, std::wstring userSrcFileName, int usrSrcLineNum, int usrSrcColPos)	{

	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;

	std::map<std::wstring, int> varRefCnt;
	std::set<std::wstring> multiRefVars;

	for (int idx = 1; idx < flatExprTknList.size() && !isFailed; idx++)	{
		// Go through flattened expression list and look for [PRE|POST]FIX OPR8Rs
		// and what should be variable names they affect 
		Token currTkn = flatExprTknList[idx];
		Token prevTkn = flatExprTknList[idx-1];
		bool isInserted = false;
		bool isPrePostFix = false;

		if (currTkn.tkn_type == EXEC_OPR8R_TKN)	{
			if (currTkn._unsigned == POST_INCR_NO_OP_OPCODE)	{
				isPrePostFix = true;
				if (prevTkn.tkn_type != USER_WORD_TKN)
					isFailed = true;
				else	{
					postIncList.push_back(prevTkn._string);
					isInserted = true;
				}

			} else if (currTkn._unsigned == POST_DECR_NO_OP_OPCODE)	{
				isPrePostFix = true;
				if (prevTkn.tkn_type != USER_WORD_TKN)
					isFailed = true;
				else	{
				postDecList.push_back(prevTkn._string);
				isInserted = true;
				}
			} else if (currTkn._unsigned == PRE_INCR_NO_OP_OPCODE)	{
				isPrePostFix = true;
				if (prevTkn.tkn_type != USER_WORD_TKN)
					isFailed = true;
				else	{
					preIncList.push_back(prevTkn._string);
					isInserted = true;
				}
			} else if (currTkn._unsigned == PRE_DECR_NO_OP_OPCODE)	{
				isPrePostFix = true;
				if (prevTkn.tkn_type != USER_WORD_TKN)
					isFailed = true;
				else	{
					preDecList.push_back(prevTkn._string);
					isInserted = true;
				}
			}

			if (isPrePostFix && !isInserted)	{
				isFailed = true;
				userMessages->logMsg(USER_ERROR
				, L"Expected a USER_WORD associated with [" + execTerms->getSrcOpr8rStrFor(currTkn._unsigned) + L"] but instead got " + prevTkn.descr_sans_line_num_col()
				, userSrcFileName, usrSrcLineNum, usrSrcColPos);
			}

			if (isInserted)	{
				if (auto search = varRefCnt.find (prevTkn._string); search != varRefCnt.end())	{
					// Increment the ref count.....est no bueno!
					search->second++;
					multiRefVars.insert(prevTkn._string);
				} else {
					varRefCnt.insert(std::pair {prevTkn._string, 1});
				}
			}
		}
	}

	// No variable should be referenced by a PREFIX|POSTFIX OPR8R more than once
	int multiRefCnt = multiRefVars.size();
	if (multiRefCnt > 0)	{
		std::wstring errMsg = L"A variable can use a single [pre|post]fix operator in an expression these did not: ";
		std::wstring badVars;
		
		for (auto itr8r = multiRefVars.begin(); itr8r != multiRefVars.end(); itr8r++)	{
			if (!badVars.empty())
				badVars.append(L", ");
			badVars.append (*itr8r);
		}
		errMsg.append(badVars);
		userMessages->logMsg(USER_ERROR, errMsg, userSrcFileName, usrSrcLineNum, usrSrcColPos);
		isFailed = true;
	}

	// TODO: Check that all the listed variables exist and are of type SIGNED or UNSIGNED
	if (!isFailed)
		ret_code = OK;
	
	return (ret_code);
}

/* ****************************************************************************
 * Tree that represents an expression will be written out recursively into a
 * flat list for storing in a file stream. Note that if applicable, list(s) of
 * PREFIX OPR8Rs with their respective variable names will be added BEFORE the
 * main expression.  If there is 1 or 2 list(s) of POSTFIX OPR8Rs, these will
 * be written out AFTER the main expression.
 * ***************************************************************************/
int InterpretedFileWriter::flattenExprTree (std::shared_ptr<ExprTreeNode> rootOfExpr, std::vector<Token> & flatExprTknList, std::wstring userSrcFileName)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;
	int usrSrcLineNum;
	int usrSrcColPos;

	if (rootOfExpr == NULL)	{
  	userMessages->logMsg (INTERNAL_ERROR, L"rootOfExpr is NULL!", thisSrcFile, __LINE__, 0);

	} else	{
		usrSrcLineNum = rootOfExpr->originalTkn->get_line_number();
		usrSrcColPos = rootOfExpr->originalTkn->get_column_pos();

		if (OK == makeFlatExpr_12_Opr8r (rootOfExpr, flatExprTknList))	{
			std::vector<std::wstring> preIncList;
			std::vector<std::wstring> preDecList;
			std::vector<std::wstring> postIncList;
			std::vector<std::wstring> postDecList;

			if (OK != checkPrePostFix (flatExprTknList, preIncList, preDecList, postIncList, postDecList, userSrcFileName, usrSrcLineNum, usrSrcColPos))
				isFailed = true;

			// NOTE: [PRE|POST]_[INCR|DECR]_NO_OP_OPCODE still get written out to the expression for disassembly purposes,
			// but are treated as NO-OPs
			std::wstring prefixIncVars = util.joinStrings(preIncList, L",", true);
			std::wstring prefixDecVars = util.joinStrings(preDecList, L",", true);
			std::wstring postfixIncVars = util.joinStrings(postIncList, L",", true);
			std::wstring postfixDecVars = util.joinStrings(postDecList, L",", true);
			// Put PREFIX list BEFORE expression
			if (!prefixIncVars.empty())	{
				Token preIncList (EXEC_OPR8R_TKN, prefixIncVars);
				preIncList._unsigned = PRE_INCR_OPR8R_OPCODE;
				flatExprTknList.insert(flatExprTknList.begin(), preIncList);
			}

			if (!prefixDecVars.empty())	{
				Token preDecList (EXEC_OPR8R_TKN, prefixDecVars);
				preDecList._unsigned = PRE_DECR_OPR8R_OPCODE;
				flatExprTknList.insert(flatExprTknList.begin(), preDecList);
			}

			// Put POSTFIX list AFTER expression
			if (!postfixIncVars.empty())	{
				Token postIncList (EXEC_OPR8R_TKN, postfixIncVars);
				postIncList._unsigned = POST_INCR_OPR8R_OPCODE;
				flatExprTknList.push_back(postIncList);
			}

			if (!postfixDecVars.empty())	{
				Token postDecList (EXEC_OPR8R_TKN, postfixDecVars);
				postDecList._unsigned = POST_DECR_OPR8R_OPCODE;
				flatExprTknList.push_back(postDecList);
			}

			if (!isFailed)
				ret_code = OK;

		}
	}

	return (ret_code);
}
