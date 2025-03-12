/*
 * InterpretedFileWriter.cpp
 *
 *  Created on: Oct 14, 2024
 *      Author: Mike Volk
 *
 * Utility class used by the Compiler to write objects out to the Interpreted file.
 *
 */

#include "InterpretedFileWriter.h"
#include "InfoWarnError.h"
#include "OpCodes.h"
#include "Token.h"
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
	if (outputStream.is_open())
		outputStream.close();
}

/* ****************************************************************************
 * Tree that represents an expression has already been flattened.  This fxn
 * just needs to write the Token stream out to the interpreted file as a
 * flexible length object.
 * ***************************************************************************/
int InterpretedFileWriter::writeFlatExprToFile (std::vector<Token> & flatExprTknList, bool isIllustrative)	{
	int ret_code = GENERAL_FAILURE;
	int idx;
	bool isFailed = false;

	if (flatExprTknList.empty())	{
  	userMessages->logMsg (INTERNAL_ERROR, L"Flattened expression list is EMPTY!", thisSrcFile, __LINE__, 0);

	} else	{
		// Make sure we're at END of our output file
		outputStream.seekp(0, std::fstream::end);

		uint32_t startFilePos = outputStream.tellp();

		uint32_t length_pos = writeFlexLenOpCode (EXPRESSION_OPCODE);
		if (0 != length_pos)	{
			// Save off the position where the expression's total length is stored and
			// write 0s to it. It will get filled in later when writing the entire expression out has
			// been completed.
			for (idx = 0; idx < flatExprTknList.size() && !isFailed; idx++)	{
				if (OK != writeToken(flatExprTknList[idx]))	{
					isFailed = true;
				 	userMessages->logMsg (INTERNAL_ERROR, L"Failure writing Token in flat expression list out to interpreted file"
						, thisSrcFile, __LINE__, 0);
				}
			}

			if (!isFailed)
				ret_code = writeObjectLen (startFilePos);

      if (OK == ret_code && isIllustrative) {
        int caretPos;
        std::wcout << L"\nParse tree flattened and written out to interpreted file" << std::endl;
        std::wcout << util.getTokenListStr(flatExprTknList, 0, caretPos) << std::endl;
      }

		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * Called after an entire object has been written out and the length now needs
 * to be filled in.  This fxn assumes that the position for the output interpreted
 * file is correct and at then end of the current object.
 * ***************************************************************************/
int InterpretedFileWriter::writeObjectLen (uint32_t objStartPos)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;


	// Need fill in now known length of this expression
	uint32_t currFilePos = outputStream.tellp();
	uint32_t exprLen = (currFilePos - objStartPos);

	assert (exprLen > 0);

	// Write the current object's length
	outputStream.seekp(objStartPos + OPCODE_NUM_BYTES, std::fstream::beg);
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

	// Account for endian-ness 
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
			ret_code = writeObjectLen (startFilePos);
		}
	}

	return (ret_code);
}


/* ****************************************************************************
 * TODO: Any kind of check for success?
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
		case BOOL_TKN :
			if (OK == writeRawUnsigned (BOOL_DATA_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token._unsigned > 0 ? 1 : 0, NUM_BITS_IN_BYTE);
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
			if (OK == writeRawUnsigned (INT8_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token._signed, NUM_BITS_IN_BYTE);
			break;
		case INT16_TKN :
			if (OK == writeRawUnsigned (INT16_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token._signed, NUM_BITS_IN_WORD);
			break;
		case INT32_TKN :
			if (OK == writeRawUnsigned (INT32_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token._signed, NUM_BITS_IN_DWORD);
			break;
		case INT64_TKN :
			if (OK == writeRawUnsigned (INT64_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token._signed, NUM_BITS_IN_QWORD);
			break;
		case DOUBLE_TKN :
				ret_code = writeString (DOUBLE_OPCODE, token._string);
			break;
		case EXEC_OPR8R_TKN :
			ret_code = writeRawUnsigned (token._unsigned, NUM_BITS_IN_BYTE);
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
		ret_code = OK;

	else {
		switch(token->tkn_type)	{
		case USER_WORD_TKN :
		case STRING_TKN :
		case DATETIME_TKN :
		case BOOL_TKN :
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
				token->resetTokenExceptSrc();
				token->tkn_type = EXEC_OPR8R_TKN;
				token->_unsigned = op_code;
				Operator opr8r;
				if (OK == execTerms->getExecOpr8rDetails (op_code, opr8r))	{
					token->_string = opr8r.symbol;
				}
				if (op_code == INVALID_OPCODE)
					isFailed = true;

			}

			if (!(token->tkn_type == EXEC_OPR8R_TKN && token->_unsigned == TERNARY_2ND_OPR8R_OPCODE))
				// No need to put the [:] OPR8R in the stream
				flatExprTknList.push_back (*token);
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
int InterpretedFileWriter::makeFlatExpr_LRO (std::shared_ptr<ExprTreeNode> currBranch, std::vector<Token> & flatExprTknList)	{
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
			if (OK != makeFlatExpr_LRO (currBranch->_1stChild, flatExprTknList))
				isFailed = true;

			if (isTernary1or2)	{
				if (OK != addTokenToFlatList(currBranch->originalTkn, flatExprTknList))
					isFailed = true;
			}

			if (!isFailed && currBranch->_2ndChild != NULL)	{
				if (OK != makeFlatExpr_LRO (currBranch->_2ndChild, flatExprTknList))
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
 * Take the parse tree supplied by the compiler and recursively turn it into a 
 * flattened expression with a sequence of 
 * [operator][left expression][right expression]
 * NOTE that [left expression] and/or right expression could consist of a single
 * operand, or could contain other nested expressions.
 * ***************************************************************************/
int InterpretedFileWriter::makeFlatExpr_OLR (std::shared_ptr<ExprTreeNode> currBranch, std::vector<Token> & flatExprTknList)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;
	bool isTernary2nd = false;

	if (currBranch != NULL)	{

		if (OK != addTokenToFlatList(currBranch->originalTkn, flatExprTknList))
			isFailed = true;

		if (!isFailed && currBranch->_1stChild != NULL)	{
			if (OK != makeFlatExpr_OLR (currBranch->_1stChild, flatExprTknList))
				isFailed = true;

			if (!isFailed && currBranch->_2ndChild != NULL)	{
				if (OK != makeFlatExpr_OLR (currBranch->_2ndChild, flatExprTknList))
					isFailed = true;
			}
		}

		if (!isFailed)	{
			ret_code = OK;
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * Tree that represents an expression will be written out recursively into a
 * flat list for storing in a file stream.  Expression is written out in 
 * [OPR8R][LEFT][RIGHT] order.  The OPR8R goes 1st to enable short-circuiting of
 * [&&], [||] and [?] OPR8Rs.
 * Some example source expressions and their corresponding Token lists that get
 * written out to the interpreted file are shown below.
 * 
 * seven = three + four;
 * [=][seven][B+][three][four]
 *
 * one = 1;
 * [=][one][1]
 *
 * seven * seven + init1++; 
 * [B+][*][seven][seven][1+][init1]
 * 
 * seven * seven + ++init2;
 * [B+][*][seven][seven][+1][init2]
 * 
 * one >= two ? 1 : three <= two ? 2 : three == four ? 3 : six > seven ? 4 : six > (two << two) ? 5 : 12345;
 * [?][>=][one][two][1][?][<=][three][two][2][?][==][three][four][3][?][>][six][seven][4][?][>][six][<<][two][two][5][12345]
 * 
 * count == 1 ? "one" : count == 2 ? "two" : "MANY";
 * [?][==][count][1]["one"][?][==][count][2]["two"]["MANY"]
 * 
 * 3 * 4 / 3 + 4 << 4;
 * [<<][B+][/][*][3][4][3][4][4]
 * 
 * 3 * 4 / 3 + 4 << 4 + 1;
 * [<<][B+][/][*][3][4][3][4][B+][4][1]
 * 
 * (one * two >= three || two * three > six || three * four < seven || four / two < one) && (three % two > 1 || (shortCircuitAnd987 = 654));
 * [&&][||][||][||][>=][*][one][two][three][>][*][two][three][six][<][*][three][four][seven][<][/][four][two][one][||][>][%][three][two][1][=][shortCircuitAnd987][654]
 * ***************************************************************************/
int InterpretedFileWriter::flattenExprTree (std::shared_ptr<ExprTreeNode> rootOfExpr, std::vector<Token> & flatExprTknList, std::wstring userSrcFileName)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;
	int usrSrcLineNum;
	int usrSrcColPos;

	if (rootOfExpr == NULL)
  	userMessages->logMsg (INTERNAL_ERROR, L"rootOfExpr is NULL!", thisSrcFile, __LINE__, 0);
	else	
		ret_code = makeFlatExpr_OLR (rootOfExpr, flatExprTknList);

	return (ret_code);
}
