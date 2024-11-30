/*
 * InterpretedFileWriter.h
 *
 *  Created on: Oct 14, 2024
 *      Author: Mike Volk
 */

#ifndef INTERPRETEDFILEWRITER_H_
#define INTERPRETEDFILEWRITER_H_

#include <string>
#include <ostream>
#include <iostream>
#include <fstream>
#include <memory>
#include "common.h"
#include "ExprTreeNode.h"
#include "ExpressionParser.h"
#include "Utilities.h"
#include "OpCodes.h"
#include "Operator.h"
#include "CompileExecTerms.h"
#include "UserMessages.h"

class InterpretedFileWriter {
public:
	InterpretedFileWriter(std::string output_file_name, CompileExecTerms & inExecTerms, UserMessages & userMessages);
	virtual ~InterpretedFileWriter();
	int writeFlatExprToFile(std::shared_ptr<ExprTreeNode> rootOfExpr, std::vector<Token> & flatExprTknList, UserMessages & userMessages);

	// TODO: Is making these "public" legit?
	int writeFlexLenOpCode (uint8_t op_code, UserMessages & userMessages);
	int writeObjectLen (uint32_t objStartPos, uint32_t objLengthPos, UserMessages & userMessages);
	int writeRawUnsigned (uint64_t  payload, int payloadBitSize, UserMessages & userMessages);
	int writeString (uint8_t op_code, std::wstring tokenStr, UserMessages & userMessages);
	uint32_t getWriteFilePos ();
	int flattenExprTree (std::shared_ptr<ExprTreeNode> rootOfExpr, std::vector<Token> & flatExprTknList, std::wstring userSrcFileName);

private:
	std::wstring thisSrcFile;
	std::wstring outFileName;
	Utilities util;
	CompileExecTerms * execTerms;
	std::ofstream outputStream;
	UserMessages userMessages;

	int writeAtomicOpCode (uint8_t op_code);
	int write8BitOpCode (uint8_t op_code, uint8_t payload);
	int write16BitOpCode (uint8_t op_code, uint16_t payload);
	int write32BitOpCode (uint8_t op_code, uint32_t payload);
	int write64BitOpCode (uint8_t op_code, uint64_t payload);
	int writeRawString (std::wstring tokenStr);
	int writeToken (std::shared_ptr<Token>, std::vector<Token> & flatExprTknList);
  int makeFlatExpr_12_Opr8r (std::shared_ptr<ExprTreeNode> currBranch, std::vector<Token> & flatExprTknList);
	int addTokenToFlatList (std::shared_ptr<Token> token, std::vector<Token> & flatExprTknList);
};

#endif /* INTERPRETEDFILEWRITER_H_ */
