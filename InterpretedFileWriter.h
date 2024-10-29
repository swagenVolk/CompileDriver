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
#include "Utilities.h"
#include "OpCodes.h"
#include "Operator.h"
#include "CompileExecTerms.h"

#define TMP_STR_BFFR_NUM_BYTES	200

class InterpretedFileWriter {
public:
	InterpretedFileWriter(std::ofstream & interpreted_file, CompileExecTerms & inExecTerms);
	virtual ~InterpretedFileWriter();
	int writeExpressionToFile(ExprTreeNode * rootOfExp);

private:
	std::wstring thisSrcFile;
	std::wstring outFileName;
	std::ofstream * outputStream;
	Utilities util;
	CompileExecTerms * execTerms;
	int writeExpr_12_Opr8r (ExprTreeNode * currBranch);
	int writeAtomicOpCode (uint8_t op_code);
	int writeFlexLenOpCode (uint8_t op_code);
	int writeObjectLen (uint32_t objStartPos, uint32_t objLengthPos);

	int write8BitOpCode (uint8_t op_code, uint8_t payload);
	int write16BitOpCode (uint8_t op_code, uint16_t payload);
	int write32BitOpCode (uint8_t op_code, uint32_t payload);
	int write64BitOpCode (uint8_t op_code, uint64_t payload);
	int writeString (uint8_t op_code, std::wstring tokenStr);
	int writeRawString (std::wstring tokenStr);
	int writeToken (Token * token);

	int writeRawUnsigned (uint64_t  payload, int payloadBitSize);

};

#endif /* INTERPRETEDFILEWRITER_H_ */
