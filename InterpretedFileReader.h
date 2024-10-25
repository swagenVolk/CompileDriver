/*
 * InterpretedFileReader.h
 *
 *  Created on: Oct 24, 2024
 *      Author: Mike Volk
 */

#ifndef INTERPRETEDFILEREADER_H_
#define INTERPRETEDFILEREADER_H_

#include <string>
#include <istream>
#include <iostream>
#include <fstream>
#include <memory>
#include "common.h"
#include "ExprTreeNode.h"
#include "Utilities.h"
#include "OpCodes.h"
#include "Operator.h"
#include "CompileExecTerms.h"
#include "Token.h"


class InterpretedFileReader {
public:
	InterpretedFileReader(std::ifstream & interpreted_file, CompileExecTerms & inExecTerms);
	virtual ~InterpretedFileReader();
	int evalExpr();

protected:
	uint8_t readOpCode ();
	int readFileByte (uint8_t & nextByte);
	int readNextWord (uint16_t & nextWord);
	int readNextDword (uint32_t & nextDword);
	int readNextQword (uint64_t & nextQword);
	int readRawUnsigned (uint64_t & payload, int payloadByteSize);
	int readRawString (std::wstring & rawString, int strLen);

	int snagOpr8r (uint8_t op_code, Token nxtTkn, std::vector<Token> & exprTknStream);
	int snagFixedRange (uint8_t op_code, Token nxtTkn, std::vector<Token> & exprTknStream);
	int snagString (uint8_t op_code, Token nxtTkn, std::vector<Token> & exprTknStream);


private:
	std::wstring thisSrcFile;
	std::wstring inFileName;
	std::ifstream * inputStream;
	Utilities util;
	CompileExecTerms * execTerms;

};

#endif /* INTERPRETEDFILEREADER_H_ */
