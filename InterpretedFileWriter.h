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
#include <fstream>
#include "common.h"
#include "ExprTreeNode.h"
#include "Utilities.h"
#include "OpCodes.h"
#include "Operator.h"
#include "CompileExecTerms.h"

class InterpretedFileWriter {
public:
	InterpretedFileWriter(std::fstream & interpreted_file, CompileExecTerms & inExecTerms);
	virtual ~InterpretedFileWriter();
	int writeExpressionToFile(ExprTreeNode * rootOfExp);

private:
	std::wstring thisSrcFile;
	std::wstring outFileName;
	std::fstream * outputStream;
	Utilities util;
	CompileExecTerms * execTerms;
	int recursiveWriteExpression (ExprTreeNode * currBranch);
	int writeExpr_12_Opr8r (ExprTreeNode * currBranch);
};

#endif /* INTERPRETEDFILEWRITER_H_ */
