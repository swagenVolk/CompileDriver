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
	std::wstring outFileName;
	std::fstream * outputStream;
	Utilities util;
	CompileExecTerms * execTerms;
	int recursiveWriteExpression (ExprTreeNode * currBranch);
	int writeExpr_Opr8r_123 (ExprTreeNode * currBranch);
	int writeExpr_123_Opr8r (ExprTreeNode * currBranch);
	int writeExpr_Depth1st_123_Opr8r (ExprTreeNode * currBranch);
	int writeExpr_1_Opr8r_23 (ExprTreeNode * currBranch);
	int writeExpr_321_Opr8r (ExprTreeNode * currBranch);
};

#endif /* INTERPRETEDFILEWRITER_H_ */
