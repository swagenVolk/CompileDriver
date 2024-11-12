/*
 * InterpretedFileReader.cpp
 *
 *  Created on: Oct 24, 2024
 *      Author: Mike Volk
 *
 * TODO: Does it make sense to change the endian-ness of the file?
 */

#include "InterpretedFileReader.h"

/* ****************************************************************************
 *
 * ***************************************************************************/
InterpretedFileReader::InterpretedFileReader(std::ifstream & interpreted_file, CompileExecTerms & inExecTerms) {
	// TODO Auto-generated constructor stub
	inputStream = & interpreted_file;
	execTerms = & inExecTerms;

	// TODO: Are these asserts even necessary when the & operator is used in parameter list?
	assert (inputStream != NULL);
	assert (execTerms != NULL);

	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");

}

/* ****************************************************************************
 *
 * ***************************************************************************/
InterpretedFileReader::~InterpretedFileReader() {
	// TODO Auto-generated destructor stub
}

/* ****************************************************************************
 * TODO: Check for EOF!
 * ***************************************************************************/
int InterpretedFileReader::readFileByte (uint8_t & nextByte)	{
	int ret_code = GENERAL_FAILURE;

	if (inputStream != NULL)	{
		nextByte = inputStream->get();
		ret_code = OK;
	}

	if (ret_code != OK) { std::wcout << L"FAILED on line " << __LINE__ << std::endl; }	// TODO
	return (ret_code);
}

/* ****************************************************************************
 * TODO: Check for EOF!
 * ***************************************************************************/
int InterpretedFileReader::readNextWord (uint16_t & nextWord)	{
	int ret_code = GENERAL_FAILURE;
	uint64_t qword;

	ret_code = readRawUnsigned (qword, NUM_BYTES_IN_WORD);
	nextWord = qword;

	if (ret_code != OK) { std::wcout << L"FAILED on line " << __LINE__ << std::endl; }	// TODO
	return (ret_code);
}

/* ****************************************************************************
 * TODO: Check for EOF!
 * ***************************************************************************/
int InterpretedFileReader::readNextDword (uint32_t & nextDword)	{
	int ret_code = GENERAL_FAILURE;
	uint64_t qword;

	ret_code = readRawUnsigned (qword, NUM_BYTES_IN_DWORD);
	nextDword = qword;

	if (ret_code != OK) { std::wcout << L"FAILED on line " << __LINE__ << std::endl; }	// TODO
	return (ret_code);
}

/* ****************************************************************************
 * TODO: Check for EOF!
 * ***************************************************************************/
int InterpretedFileReader::readNextQword (uint64_t & nextQword)	{
	int ret_code = GENERAL_FAILURE;

	ret_code = readRawUnsigned (nextQword, NUM_BYTES_IN_QWORD);

	if (ret_code != OK) { std::wcout << L"FAILED on line " << __LINE__ << std::endl; }	// TODO
	return (ret_code);
}


/* ****************************************************************************
 * TODO: Any kind of check for success?
 * ***************************************************************************/
int InterpretedFileReader::readRawUnsigned (uint64_t & payload, int payloadByteSize)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;
	uint8_t nextByte;
	payload = 0x0;

	assert (payloadByteSize == 1 || payloadByteSize == NUM_BYTES_IN_WORD || payloadByteSize == NUM_BYTES_IN_DWORD || payloadByteSize == NUM_BYTES_IN_QWORD);

	// TODO: Endian-ness is accounted for now; Make this work in other Raw* fxns
	if (inputStream != NULL)	{
		for (int idx = 0; idx < payloadByteSize && !isFailed; idx++)	{
			if (OK != readFileByte(nextByte))	{
				isFailed = true;

			} else	{
				if (idx > 0)
					payload <<= NUM_BITS_IN_BYTE;

				payload |= nextByte;
			}
		}
		if (!isFailed)
			ret_code = OK;
	}

	if (ret_code != OK) { std::wcout << L"FAILED on line " << __LINE__ << std::endl; }	// TODO
	return (ret_code);
}

/* ****************************************************************************
 * TODO: Check for EOF!
 * ***************************************************************************/
int InterpretedFileReader::snagOpr8r (uint8_t op_code, Token nxtTkn, std::vector<Token> & exprTknStream)	{
	int ret_code = GENERAL_FAILURE;

	Operator chkOpr8r;

	if (OK == execTerms->getExecOpr8rDetails(op_code, chkOpr8r))	{
		nxtTkn.tkn_type = EXEC_OPR8R_TKN;
		nxtTkn._unsigned = op_code;
		exprTknStream.push_back (nxtTkn);
		ret_code = OK;
	}

	if (ret_code != OK) {
		std::wcout << L"op_code = 0x" << std::hex << op_code << L"; FAILED on line " << __LINE__ << std::endl;
	}	// TODO

	return (ret_code);
}

/* ****************************************************************************
 * TODO: Check for EOF!
 * ***************************************************************************/
int InterpretedFileReader::snagFixedRange (uint8_t op_code, Token nxtTkn, std::vector<Token> & exprTknStream)	{
	int ret_code = GENERAL_FAILURE;
	uint8_t byte;
	uint16_t word;
	uint32_t dword;
	uint64_t qword;
	bool isFailed = false;

	switch (op_code)	{
		case UINT8_OPCODE:
			// [op_code][8-bit #]
			if (OK == readFileByte (byte))	{
				nxtTkn.tkn_type = UINT8_TKN;
				nxtTkn._unsigned = byte;
			}
			break;
		case INT8_OPCODE:
			// [op_code][8-bit #]
			// TODO: Any conversion for signed INTs?
			if (OK == readFileByte (byte))	{
				nxtTkn.tkn_type = INT8_TKN;
				nxtTkn._signed = byte;
			}
			break;
		case UINT16_OPCODE:
			// [op_code][16-bit #]
			if (OK == readNextWord (word))	{
				nxtTkn.tkn_type = UINT16_TKN;
				nxtTkn._unsigned = word;
			}
			break;
		case INT16_OPCODE:
			// [op_code][16-bit #]
			// TODO: Any conversion for signed INTs?
			if (OK == readNextWord (word))	{
				nxtTkn.tkn_type = INT16_TKN;
				nxtTkn._signed = word;
			}
			break;
			break;
		case UINT32_OPCODE:
			// [op_code][32-bit #]
			if (OK == readNextDword (dword))	{
				nxtTkn.tkn_type = UINT32_TKN;
				nxtTkn._unsigned = dword;
			}
			break;
		case INT32_OPCODE:
			// [op_code][32-bit #]
			// TODO: Any conversion for signed INTs?
			if (OK == readNextDword (dword))	{
				nxtTkn.tkn_type = INT32_TKN;
				nxtTkn._signed = dword;
			}
			break;
		case UINT64_OPCODE:
			// [op_code][64-bit #]
			if (OK == readNextQword (qword))	{
				nxtTkn.tkn_type = UINT64_TKN;
				nxtTkn._unsigned = qword;
			}
			break;
		case INT64_OPCODE:
			// [op_code][64-bit #]
			// TODO: Any conversion for signed INTs?
			if (OK == readNextQword (qword))	{
				nxtTkn.tkn_type = INT64_TKN;
				nxtTkn._signed = qword;
			}
			break;
		default:
			isFailed = true;
			break;
	}

	if (!isFailed)	{
		
		exprTknStream.push_back (nxtTkn);
		ret_code = OK;
	}

	if (ret_code != OK) { std::wcout << L"FAILED on line " << __LINE__ << std::endl; }	// TODO
	return (ret_code);
}

/* ****************************************************************************
 * TODO: Check for EOF!
 * ***************************************************************************/
int InterpretedFileReader::snagString (uint8_t op_code, Token nxtTkn, std::vector<Token> & exprTknStream)	{
	int ret_code = GENERAL_FAILURE;
	uint32_t objLen;
	bool isFailed = false;
	std::wstring tknStr;
	uint16_t nxtWideChar;

	if (inputStream != NULL)	{
		uint32_t initPos = inputStream->tellg();

		if (OK != readNextDword (objLen))	{
			isFailed = true;
		} else	{
			// initPos is right after the 1-byte op_code (so -1)
			uint32_t nxtObjStartPos = (initPos - 1) + objLen;
			uint32_t numWideChars = (objLen - (OPCODE_BYTE_LEN + FLEX_LEN_FLD_BYTE_LEN)) / NUM_BYTES_IN_WORD;
			int idx;

			for (idx = 0; idx < numWideChars && !isFailed; idx++)	{
				if (OK != readNextWord (nxtWideChar))	{
					isFailed = true;

				} else if (!std::isprint(nxtWideChar))	{
					isFailed = true;

				} else	{
					tknStr.push_back(nxtWideChar);
				}
			}

			if (!isFailed)	{
				switch (op_code)	{
					case STRING_OPCODE:
						nxtTkn.tkn_type = STRING_TKN;
						break;
					case VAR_NAME_OPCODE:
						nxtTkn.tkn_type = KEYWORD_TKN;
						break;
					case DATETIME_OPCODE:
						nxtTkn.tkn_type = DATETIME_TKN;
						break;
					case DOUBLE_OPCODE:
						nxtTkn.tkn_type = DOUBLE_TKN;
						break;
					default:
						isFailed = true;
						break;
				}

				if (!isFailed && (nxtTkn.tkn_type == STRING_TKN || !tknStr.empty()))	{
					nxtTkn._string = tknStr;
								
					exprTknStream.push_back (nxtTkn);
					ret_code = OK;
				}
			}
		}
	}
	if (ret_code != OK) { std::wcout << L"FAILED on line " << __LINE__ << std::endl; }	// TODO
	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileReader::readExprIntoList (std::vector<Token> & exprTknStream)	{
	int ret_code = GENERAL_FAILURE;
	uint32_t exprStartPos;
	uint8_t op_code;
	uint32_t exprLen;

	if (inputStream != NULL)	{
		exprStartPos = inputStream->tellg();

		if (OK != readFileByte(op_code))	{
			// TODO: In future, op_code will have already been read in
			std::wcout << "TODO: Failure on line " << __LINE__ << std::endl;
		} else if (op_code != EXPRESSION_OPCODE)	{
			std::wcout << "TODO: Failure on line " << __LINE__ << std::endl;

		} else if (OK != readNextDword (exprLen))	{
			std::wcout << "TODO: Failure on line " << __LINE__ << std::endl;

		} else	{
			// OK to consume expression and create a Token list out of it
			uint32_t nxtObjStartPos = exprStartPos + exprLen;
			bool isItTheEnd = false;
			bool isFailed = false;
			uint32_t currFilePos;

			while (!isItTheEnd && !isFailed)	{
				currFilePos = inputStream->tellg();

				if (currFilePos == std::istream::traits_type::eof())	{
					isItTheEnd = true;

				} else if (currFilePos == nxtObjStartPos)	{
					isItTheEnd = true;

				} else if (OK != readFileByte (op_code)) {
					isFailed = true;

				} else	{
					// Handle next Token
					// TODO: Will this work without doing a malloc of some kind?  If so, what's
					// happening behind the scenes?
					Token nxtTkn (START_UNDEF_TKN, L"", 0, 0);

					if (op_code >= ATOMIC_OPCODE_RANGE_BEGIN && op_code <= LAST_VALID_OPR8R_OPCODE)	{
						if (OK != snagOpr8r (op_code, nxtTkn, exprTknStream))
							isFailed = true;

					} else if (op_code >= FIRST_VALID_DATA_TYPE_OPCODE && op_code <= LAST_VALID_DATA_TYPE_OPCODE)	{
						// These aren't valid in an expression
						isFailed = true;

					} else if (op_code >= FIXED_OPCODE_RANGE_BEGIN && op_code <= FIXED_OPCODE_RANGE_END)	{
						if (OK != snagFixedRange (op_code, nxtTkn, exprTknStream))
							isFailed = true;

					} else if (op_code == STRING_OPCODE || op_code == VAR_NAME_OPCODE || op_code == DATETIME_OPCODE || op_code == DOUBLE_OPCODE)	{
						if (OK != snagString (op_code, nxtTkn, exprTknStream))
							isFailed = true;

					} else	{
						isFailed = true;
					}
				}
			}

			if (!isFailed && isItTheEnd)
				ret_code = OK;

		}
	}

	if (ret_code != OK) { std::wcout << L"FAILED on line " << __LINE__ << std::endl; }	// TODO

	return (ret_code);
}








