/*
 * InterpretedFileWriter.cpp
 *
 *  Created on: Oct 14, 2024
 *      Author: Mike Volk
 *
 */

#include "InterpretedFileWriter.h"

InterpretedFileWriter::InterpretedFileWriter(std::ofstream & interpreted_file, CompileExecTerms & inExecTerms) {
	// TODO Auto-generated constructor stub
	outputStream = & interpreted_file;
	execTerms = & inExecTerms;

	// TODO: Are these asserts even necessary when the & operator is used in parameter list?
	assert (outputStream != NULL);
	assert (execTerms != NULL);
	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");

}

InterpretedFileWriter::~InterpretedFileWriter() {
	// TODO Auto-generated destructor stub
}

/* ****************************************************************************
 * writeExpr_12_Opr8r will recursively walk an expression tree in
 * [1st child][2nd child][current] order to create a flat stream of op_code
 * Tokens the Interpreter can use to resolve the expression at execution time.
 *
 * Example C expression
 * (1 + 2 * (3 + 4 * (5 + 6 * 7 * (8 + 9))))
 *
 * And how it will get written to interpreted file.
 * NOTE that we'll go R2L through this list until we find an OPR8R followed by the
 * required # of operands; in this case [8] [9] [+]. Note that in the above C expression,
 * 8 + 9 is in the most deeply nested parentheses and therefore has the highest precedence
 * [1]  [2]  [3]  [4]  [5]  [6]  [7]  [*]  [8]  [9]  [+]  [*]  [+]  [*]  [+]  [*]  [+]
 * [1]  [2]  [3]  [4]  [5]           [42]           [17]  [*]  [+]  [*]  [+]  [*]  [+]
 * [1]  [2]  [3]  [4]  [5]                              [714]  [+]  [*]  [+]  [*]  [+]
 * [1]  [2]  [3]  [4]                                        [719]  [*]  [+]  [*]  [+]
 * [1]  [2]  [3]                                                 [2876]  [+]  [*]  [+]
 * [1]  [2]                                                           [2879]  [*]  [+]
 * [1]                                                                     [5758]  [+]
 *                                                                              [5759]
 *
 * Example nested ternary C expression
 * (count == 1 ? "one" : count == 2 ? "two" : count == 3 ? "three" : count == 4 ? "four" : "MANY")
 *
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]  [:]  [count]  [2]  [==]  [?]  [:]  [count]  [1]  [==]  [?]
 *
 * **************************************************************************************************************************************************************
 * count = 1;
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]  [:]  [count]  [2]  [==]  [?]  [:]  [count]  [1]  [==]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]  [:]  [count]  [2]  [==]  [?]  [:]  [1]  [?]
 *
 * Somehow we've got to skip over all the junk to get to the [one] at the end
 * We're moving BACKWARDS through the list, so L&R for the [:] OPR8R are in reverse order.
 *
 * [one]  [two]  [three]  [four]  [MANY]
 * [:]  [count]  [4]  [==]  [?]  -> pulls off [four] [MANY] because this branch was never reached
 * [:]  [count]  [3]  [==]  [?]  -> pulls off [three] because the false branch was already pulled off above
 * [:]  [count]  [2]  [==]  [?]  -> pulls off [two] because the false branch was already pulled off above
 * [:]  [1]  [?] -> this is finally resolved as [one]
 *
 *
 * **************************************************************************************************************************************************************
 * count = 2;
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]  [:]  [count]  [2]  [==]  [?]  [:]  [count]  [1]  [==]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]  [:]  [count]  [2]  [==]  [?]  [:]  [0]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]  [:]  [1]  [?]
 *
 * [one]  [two]  [three]  [four]  [MANY]
 * [:]  [count]  [4]  [==]  [?]  -> pulls off [four] [MANY] because this branch was never reached
 * [:]  [count]  [3]  [==]  [?]  -> pulls off [three] because the false branch was already pulled off above
 * [:]  [1]  [?]  -> After previous scopes popped, we grab the 1st available -> [two]
 *
 *
 * **************************************************************************************************************************************************************
 * count = 3;
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]  [:]  [count]  [2]  [==]  [?]  [:]  [count]  [1]  [==]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]  [:]  [count]  [2]  [==]  [?]  [:]  [0]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]  [:]  [count]  [2]  [==]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]  [:]  [0]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [1]  [?]
 *
 * [one]  [two]  [three]  [four]  [MANY]
 * [:]  [count]  [4]  [==]  [?]  -> pulls off [four] [MANY] because this branch was never reached
 * [:]  [1]  [?]  -> After previous scopes popped, we grab the 1st available -> [three]
 *
 *
 * **************************************************************************************************************************************************************
 * count = 4;
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]  [:]  [count]  [2]  [==]  [?]  [:]  [count]  [1]  [==]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]  [:]  [count]  [2]  [==]  [?]  [:]  [0]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]  [:]  [0]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [0]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [1]  [?]
 *
 * [one]  [two]  [three]  [four]  [MANY]
 * [:]  [1]  [?] -> There are 2 possibilities for us to grab; Since it's the TRUE path and we're running in REVERSE, pick the 2nd* resolved token [four]
 * 2nd going from R2L, which would in turn be 1st in L2R order for that expression 2-tuple
 *
 *
 * **************************************************************************************************************************************************************
 * count = 5;
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]  [:]  [count]  [2]  [==]  [?]  [:]  [count]  [1]  [==]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]  [:]  [count]  [2]  [==]  [?]  [:]  [0]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]  [:]  [0]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [count]  [3]  [==]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]  [:]  [0]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [count]  [4]  [==]  [?]
 * [one]  [two]  [three]  [four]  [MANY]  [:]  [0]  [?]
 *
 * [one]  [two]  [three]  [four]  [MANY]
 * [:]  [0]  [?] -> There are 2 possibilities for us to grab; Since the FALSE path is valid and we're running in REVERSE, pick the 1st* resolved token [MANY]
 * 1st going from R2L, which would in turn be 2nd in L2R order for that expression 2-tuple
 *
 *
 * ***************************************************************************/
int InterpretedFileWriter::writeExpr_12_Opr8r (ExprTreeNode * currBranch)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;
	bool isTernary1st = false;
	bool isTernary2nd = false;
	bool isTernary1or2 = false;

	if (currBranch != NULL)	{

//		if (currBranch->originalTkn->tkn_type == OPR8R_TKN && currBranch->originalTkn->_string == execTerms->get_ternary_1st())
//			isTernary1st = true;
//
//		else if (currBranch->originalTkn->tkn_type == OPR8R_TKN && currBranch->originalTkn->_string == execTerms->get_ternary_2nd())
//			isTernary2nd = true;
		if (currBranch->originalTkn->tkn_type == SRC_OPR8R_TKN && (currBranch->originalTkn->_string == execTerms->get_ternary_1st()
				|| currBranch->originalTkn->_string == execTerms->get_ternary_2nd()))	{
			isTernary1or2 = true;
		}


		if (currBranch->_1stChild != NULL)	{
			if (OK != writeExpr_12_Opr8r (currBranch->_1stChild))
				isFailed = true;


			if (isTernary1or2)	{
				// TODO: 'Splain yo self
				std::wcout << "[" << currBranch->originalTkn->_string << "] ";
				if (OK != writeToken(currBranch->originalTkn))
					isFailed = true;
			}

			if (!isFailed && currBranch->_2ndChild != NULL)	{
				if (OK != writeExpr_12_Opr8r (currBranch->_2ndChild))
					isFailed = true;
			}
		}

		if (!isFailed)	{
			if (!isTernary1or2)	{
				std::wcout << "[" << currBranch->originalTkn->_string << "] ";
				ret_code = writeToken(currBranch->originalTkn);

			} else	{
				ret_code = OK;
			}
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * Tree that represents an expression will be written out recursively:
 * O12[3] - OPR8R, 1st child, 2nd, and 3rd if it exists
 * Expect that this unified stream will be either acted on in reverse order or
 * possibly reversed prior to pushing it out to the interpreted file.
 * Will need to be very aware of order dependent OPR8Rs
 * (a-b) vs. (b-a) typically have very different results
 * ***************************************************************************/
int InterpretedFileWriter::writeExpressionToFile (ExprTreeNode * rootOfExpr)	{
	int ret_code = GENERAL_FAILURE;

	if (rootOfExpr == NULL)	{
		// TODO: Some kind of error message capture?
		// Who is responsible for reporting out to the user?
  	std::wcout << "INTERNAL ERROR encountered on " << thisSrcFile << ":" << __LINE__ << std::endl;

	} else if (outputStream == NULL)	{
  	std::wcout << "INTERNAL ERROR encountered on " << thisSrcFile << ":" << __LINE__ << std::endl;

	} else	{
		// Make sure we're at END of our output file
		// TODO: Account for nested expressions (???)
		outputStream->seekp(0, std::fstream::end);

		uint32_t startFilePos = outputStream->tellp();

		uint32_t length_pos = writeFlexLenOpCode (EXPRESSION_OPCODE);
		if (0 != length_pos)	{
			// Save off the position where the expression's total length is stored and
			// write 0s to it. It will get filled in later when writing the entire expression out has
			// been completed.

			std::wcout << L"********** writeExpr_12_Opr8r called from " << thisSrcFile << L":" << __LINE__ << L" **********" << std::endl;
			if (OK == writeExpr_12_Opr8r (rootOfExpr))
				ret_code = writeObjectLen (startFilePos, length_pos);

			std::wcout << std::endl;
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

	if (outputStream != NULL)	{

		// Need fill in now known length of this expression
		uint32_t currFilePos = outputStream->tellp();
		uint32_t exprLen = (currFilePos - objStartPos);

		// Write the current object's length
		outputStream->seekp(objLengthPos, std::fstream::beg);
		if (OK != writeRawUnsigned(exprLen, NUM_BITS_IN_DWORD))
			isFailed = true;

		// Now reset file pointer to pos at fxn begin, after the object was entirely written out
		outputStream->seekp(0, std::fstream::end);

		if (currFilePos == outputStream->tellp() && !isFailed)
			ret_code = OK;

	}

	return (ret_code);
}


/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::writeAtomicOpCode (uint8_t op_code)	{
	int ret_code = GENERAL_FAILURE;

	if (outputStream != NULL)	{
		if (op_code >= ATOMIC_OPCODE_RANGE_BEGIN && op_code <= ATOMIC_OPCODE_RANGE_END)	{
			if (op_code <= LAST_VALID_OPR8R_OPCODE)	{
				ret_code = writeRawUnsigned (op_code, NUM_BITS_IN_BYTE);

			} else if (op_code >= FIRST_VALID_DATA_TYPE_OPCODE && op_code <= LAST_VALID_DATA_TYPE_OPCODE)	{
				ret_code = writeRawUnsigned (op_code, NUM_BITS_IN_BYTE);
			}
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

	if (outputStream != NULL)	{
		if (op_code >= FIRST_VALID_FLEX_LEN_OPCODE && op_code <= LAST_VALID_FLEX_LEN_OPCODE)	{
			if (OK == writeRawUnsigned (op_code, NUM_BITS_IN_BYTE))	{
				tmpLenPos = outputStream->tellp();
				if (OK == writeRawUnsigned (tempLen, NUM_BITS_IN_DWORD))
				lengthPos = tmpLenPos;
			}
		}
	}

	return (lengthPos);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::write8BitOpCode (uint8_t op_code, uint8_t  payload)	{
	int ret_code = GENERAL_FAILURE;

	if (outputStream != NULL)	{
		if (op_code >= FIXED_OPCODE_RANGE_BEGIN && op_code <= FIXED_OPCODE_RANGE_END)	{
			if (op_code == UINT8_OPCODE || op_code == INT8_OPCODE)	{
				// Write op_code followed by payload out to file
				if (OK == writeRawUnsigned (op_code, NUM_BITS_IN_BYTE) && OK == writeRawUnsigned (payload, NUM_BITS_IN_BYTE))
					ret_code = OK;
			}
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::write16BitOpCode (uint8_t op_code, uint16_t  payload)	{
	int ret_code = GENERAL_FAILURE;

	if (outputStream != NULL)	{

		if (op_code >= FIXED_OPCODE_RANGE_BEGIN && op_code <= FIXED_OPCODE_RANGE_END)	{
			if (op_code == UINT16_OPCODE || op_code == INT16_OPCODE)	{
				// Write op_code followed by payload out to file
				if (OK == writeRawUnsigned (op_code, NUM_BITS_IN_BYTE) && OK == writeRawUnsigned (payload, NUM_BITS_IN_WORD))
					ret_code = OK;
			}
		}
	}
	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::write32BitOpCode (uint8_t op_code, uint32_t  payload)	{
	int ret_code = GENERAL_FAILURE;

	if (outputStream != NULL)	{
		if (op_code >= FIXED_OPCODE_RANGE_BEGIN && op_code <= FIXED_OPCODE_RANGE_END)	{
			if (op_code == UINT32_OPCODE || op_code == INT32_OPCODE)	{
				// Write op_code followed by payload out to file
				if (OK == writeRawUnsigned (op_code, NUM_BITS_IN_BYTE) && OK == writeRawUnsigned(payload, NUM_BITS_IN_DWORD))
					ret_code = OK;
			}
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::write64BitOpCode (uint8_t op_code, uint64_t  payload)	{
	int ret_code = GENERAL_FAILURE;

	if (outputStream != NULL)	{
		if (op_code >= FIXED_OPCODE_RANGE_BEGIN && op_code <= FIXED_OPCODE_RANGE_END)	{
			if (op_code == UINT64_OPCODE || op_code == INT64_OPCODE)	{
				// Write op_code followed by payload out to file
				if (OK == writeRawUnsigned (op_code, NUM_BITS_IN_BYTE) && OK == writeRawUnsigned (payload, NUM_BITS_IN_QWORD))
					ret_code = OK;
			}
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

	// TODO: Endian-ness is accounted for now; Make this work in other Raw* fxns
	if (outputStream != NULL)	{
		for (int idx = payloadBitSize/NUM_BITS_IN_BYTE; idx > 0; idx--)	{
			maskedQword = (payload & shift_mask);
			maskedQword >>= ((idx - 1) * NUM_BITS_IN_BYTE);
			nextByte = maskedQword;
			outputStream->write(reinterpret_cast<char*>(&nextByte), 1);
			shift_mask >>= NUM_BITS_IN_BYTE;
		}
		ret_code = OK;
	}

	return (ret_code);
}

/* ****************************************************************************
 * TODO: Any kind of check for success?
 * ***************************************************************************/
int InterpretedFileWriter::writeString (uint8_t op_code, std::wstring tokenStr)	{
	int ret_code = GENERAL_FAILURE;

	if (outputStream != NULL)	{

		uint32_t startFilePos = outputStream->tellp();

		uint32_t length_pos = writeFlexLenOpCode (op_code);
		if (0 != length_pos)	{
			// Save off the position where the expression's total length is stored and
			// write 0s to it. It will get filled in later when writing the entire expression out has
			// been completed.

			if (OK == writeRawString (tokenStr))	{
				ret_code = writeObjectLen (startFilePos, length_pos);
			}
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

	if (outputStream != NULL)	{

		if (!tokenStr.empty())	{
			int numBytes = tokenStr.size() * 2;
			const wchar_t * strBffr = tokenStr.data();
			uint16_t nxtWord;
			int idx;

			for (idx = 0; idx < tokenStr.size() && !isFailed; idx++)	{
				nxtWord = strBffr[idx];
				if (OK != writeRawUnsigned (strBffr[idx], NUM_BITS_IN_WORD))
					isFailed = true;
				// outputStream->write(reinterpret_cast<char*>(&nxtWord), NUM_BYTES_IN_WORD);
			}

			if (idx == tokenStr.size() && !isFailed)
				ret_code = OK;

		} else	{
			ret_code = OK;
		}
	}

	return (ret_code);
}


/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::writeToken (Token * token)	{
	int ret_code = GENERAL_FAILURE;

	uint8_t tkn8Val;

	if (outputStream != NULL && token != NULL)	{
		switch(token->tkn_type)	{
		case KEYWORD_TKN :
			ret_code = writeString (VAR_NAME_OPCODE, token->_string);
			break;
		case STRING_TKN :
			ret_code = writeString (STRING_OPCODE, token->_string);
			break;
		case DATETIME_TKN :
			if (OK == writeAtomicOpCode(DATETIME_OPCODE))
				ret_code = writeRawString(token->_string);
			break;
		case UINT8_TKN :
			tkn8Val = token->_unsigned;
			if (OK == writeRawUnsigned (UINT8_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (tkn8Val, NUM_BITS_IN_BYTE);
			break;
		case UINT16_TKN :
			if (OK == writeRawUnsigned (UINT16_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token->_unsigned, NUM_BITS_IN_WORD);
			break;
		case UINT32_TKN :
			if (OK == writeRawUnsigned (UINT32_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token->_unsigned, NUM_BITS_IN_DWORD);
			break;
		case UINT64_TKN :
			if (OK == writeRawUnsigned (UINT64_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token->_unsigned, NUM_BITS_IN_QWORD);
			break;
		case INT8_TKN :
			// TODO: Check
			if (OK == writeRawUnsigned (INT8_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token->_signed, NUM_BITS_IN_BYTE);
			break;
		case INT16_TKN :
			// TODO: Check
			if (OK == writeRawUnsigned (INT16_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token->_signed, NUM_BITS_IN_WORD);
			break;
		case INT32_TKN :
			// TODO: Check
			if (OK == writeRawUnsigned (INT32_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token->_signed, NUM_BITS_IN_DWORD);
			break;
		case INT64_TKN :
			// TODO: Check
			if (OK == writeRawUnsigned (INT64_OPCODE, NUM_BITS_IN_BYTE))
				ret_code = writeRawUnsigned (token->_signed, NUM_BITS_IN_QWORD);
			break;
		case DOUBLE_TKN :
				ret_code = writeString (DOUBLE_OPCODE, token->_string);
			break;
		case SRC_OPR8R_TKN :
			ret_code = writeRawUnsigned (execTerms->getOpCodeFor (token->_string), NUM_BITS_IN_BYTE);
			break;
		case SPR8R_TKN :
		default:
			break;

		}
	}

	return (ret_code);
}
