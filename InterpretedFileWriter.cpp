/*
 * InterpretedFileWriter.cpp
 *
 *  Created on: Oct 14, 2024
 *      Author: Mike Volk
 *
 * TODO:
 * 321Opr8r ????
 * Separate OPR8R and OPERAND streams?
 * Include scope depth on each Token?
 */

#include "InterpretedFileWriter.h"

InterpretedFileWriter::InterpretedFileWriter(std::fstream & interpreted_file, CompileExecTerms & inExecTerms) {
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

	if (currBranch != NULL)	{
		if (currBranch->_1stChild != NULL)	{
			if (OK != writeExpr_12_Opr8r (currBranch->_1stChild))
				isFailed = true;

			if (!isFailed && currBranch->_2ndChild != NULL)	{
				if (OK != writeExpr_12_Opr8r (currBranch->_2ndChild))
					isFailed = true;
			}
		}

		if (!isFailed)	{
			std::wcout << " [" << currBranch->originalTkn->_string << "] ";
			ret_code = OK;
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
		outputStream->seekp(0, std::fstream::end);
		// TODO: Busticated! outputStream->write(reinterpret_cast<char*>(EXPRESSION_OPCODE), sizeof (char));

		// Save off the position where the expression's total length is stored and
		// write 0s to it. It will get filled in later when writing the entire expression out has
		// been completed.
		uint32_t length_pos = outputStream->tellp();
		// TODO: Probably also Busticated! outputStream->write(reinterpret_cast<char*>(0x0), sizeof (uint32_t));

		std::wcout << L"********** writeExpr_12_Opr8r **********" << std::endl;
		ret_code = writeExpr_12_Opr8r (rootOfExpr);
		std::wcout << std::endl;

	}

	return (ret_code);
}
