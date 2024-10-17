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
}

InterpretedFileWriter::~InterpretedFileWriter() {
	// TODO Auto-generated destructor stub
}

/* ****************************************************************************
 * 1[2][3]O
 * Example expression: (1 + 2 * (3 + 4 * (5 + 6 * 7 * (8 + 9))))
 * Gets turned into an expression tree. Expression tree gets flattened and
 * written out to interpreted file as:
 * [1] [2] [3] [4] [5] [6] [7] [*] [8] [9] [+] [*] [+] [*] [+] [*] [+]
 *
 * Interpreter will work BACKWARDS through this expression list to resolve it
 * [1] [2] [3] [4] [5] [6] [7] [*] [8] [9] [+] [*] [+] [*] [+] [*] [+]
 *                                 ^^^^^^^^^^^
 * [1] [2] [3] [4] [5] [6] [7] [*] [17] [*] [+] [*] [+] [*] [+]
 *                     ^^^^^^^^^^^
 * [1] [2] [3] [4] [5] [42] [17] [*] [+] [*] [+] [*] [+]
 *                     ^^^^^^^^^^^^^
 * [1] [2] [3] [4] [5] [714] [+] [*] [+] [*] [+]
 *                 ^^^^^^^^^^^^^
 * [1] [2] [3] [4] [719] [*] [+] [*] [+]
 *             ^^^^^^^^^^^^^
 * [1] [2] [3] [2876] [+] [*] [+]
 *         ^^^^^^^^^^^^^^
 * [1] [2] [2879] [*] [+]
 *     ^^^^^^^^^^^^^^
 * [1] [5758] [+]
 * [5759]
 * ***************************************************************************/
int InterpretedFileWriter::writeExpr_Depth1st_123_Opr8r (ExprTreeNode * currBranch)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;

	if (currBranch != NULL)	{

		if (currBranch->_1stChild != NULL)	{
			if (OK != writeExpr_Depth1st_123_Opr8r (currBranch->_1stChild))
				isFailed = true;

			if (!isFailed && currBranch->_2ndChild != NULL)	{
				if (OK != writeExpr_Depth1st_123_Opr8r (currBranch->_2ndChild))
					isFailed = true;
			}

			if (!isFailed && currBranch->_3rdChild != NULL)	{
				if (currBranch->originalTkn->tkn_type == OPR8R_TKN && (TERNARY_1ST & execTerms->get_type_mask(currBranch->originalTkn->_string)))
					std::wcout << " [:] ";

				if (OK != writeExpr_Depth1st_123_Opr8r (currBranch->_3rdChild))
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
 * Ways to do it:
 * O1[2][3] - and write the token stream out backwards?
 * 1[2][3]O
 * ***************************************************************************/
int InterpretedFileWriter::writeExpr_Opr8r_123 (ExprTreeNode * currBranch)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;

	if (currBranch != NULL)	{
		std::wcout << " [" << currBranch->originalTkn->_string << "] ";

		if (currBranch->_1stChild != NULL)	{
			if (OK != writeExpr_123_Opr8r (currBranch->_1stChild))
				isFailed = true;

			if (!isFailed && currBranch->_2ndChild != NULL)	{
				if (OK != writeExpr_123_Opr8r (currBranch->_2ndChild))
					isFailed = true;
			}

			if (!isFailed && currBranch->_3rdChild != NULL)	{
				if (currBranch->originalTkn->tkn_type == OPR8R_TKN && (TERNARY_1ST & execTerms->get_type_mask(currBranch->originalTkn->_string)))
					std::wcout << " [:] ";

				if (OK != writeExpr_123_Opr8r (currBranch->_3rdChild))
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
 * Ways to do it:
 * O1[2][3] - and write the token stream out backwards?
 * 1[2][3]O
 * ***************************************************************************/
int InterpretedFileWriter::writeExpr_123_Opr8r (ExprTreeNode * currBranch)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;

	if (currBranch != NULL)	{
		if (currBranch->_1stChild != NULL)	{
			if (OK != writeExpr_123_Opr8r (currBranch->_1stChild))
				isFailed = true;

			if (!isFailed && currBranch->_2ndChild != NULL)	{
				if (OK != writeExpr_123_Opr8r (currBranch->_2ndChild))
					isFailed = true;
			}

			if (!isFailed && currBranch->_3rdChild != NULL)	{
				if (currBranch->originalTkn->tkn_type == OPR8R_TKN && (TERNARY_1ST & execTerms->get_type_mask(currBranch->originalTkn->_string)))
					std::wcout << " [:] ";

				if (OK != writeExpr_123_Opr8r (currBranch->_3rdChild))
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
 * Ways to do it:
 * O1[2][3] - and write the token stream out backwards?
 * 1[2][3]O
 * ***************************************************************************/
int InterpretedFileWriter::writeExpr_321_Opr8r (ExprTreeNode * currBranch)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;

	if (currBranch != NULL)	{
		if (!isFailed && currBranch->_3rdChild != NULL)	{
			if (OK != writeExpr_321_Opr8r (currBranch->_3rdChild))
				isFailed = true;
			else if (currBranch->originalTkn->tkn_type == OPR8R_TKN && (TERNARY_1ST & execTerms->get_type_mask(currBranch->originalTkn->_string)))
				std::wcout << " [:] ";
		}

		if (!isFailed && currBranch->_2ndChild != NULL)	{
			if (OK != writeExpr_321_Opr8r (currBranch->_2ndChild))
				isFailed = true;
		}

		if (currBranch->_1stChild != NULL)	{
			if (OK != writeExpr_321_Opr8r (currBranch->_1stChild))
				isFailed = true;
		}

		if (!isFailed)	{
			std::wcout << " [" << currBranch->originalTkn->_string << "] ";
			ret_code = OK;
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * Ways to do it:
 * O1[2][3] - and write the token stream out backwards?
 * 1[2][3]O
 * ***************************************************************************/
int InterpretedFileWriter::writeExpr_1_Opr8r_23 (ExprTreeNode * currBranch)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;

	if (currBranch != NULL)	{
		if (currBranch->_1stChild != NULL)	{
			if (OK != writeExpr_1_Opr8r_23 (currBranch->_1stChild))
				isFailed = true;
		}

		if (!isFailed)	{
			std::wcout << "[" << currBranch->originalTkn->_string << "] ";
		}

		if (!isFailed && currBranch->_2ndChild != NULL)	{
			if (OK != writeExpr_1_Opr8r_23 (currBranch->_2ndChild))
				isFailed = true;
		}

		if (!isFailed && currBranch->_3rdChild != NULL)	{
			if (currBranch->originalTkn->tkn_type == OPR8R_TKN && (TERNARY_1ST & execTerms->get_type_mask(currBranch->originalTkn->_string)))
				std::wcout << "[:] ";

			if (OK != writeExpr_1_Opr8r_23 (currBranch->_3rdChild))
				isFailed = true;
		}
	}

	if (!isFailed)	{
		ret_code = OK;
	}

	return (ret_code);
}

/* ****************************************************************************
 * Ways to do it:
 * O1[2][3] - and write the token stream out backwards?
 * 1[2][3]O
 * ***************************************************************************/
int InterpretedFileWriter::recursiveWriteExpression (ExprTreeNode * currBranch)	{
	int ret_code = GENERAL_FAILURE;

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
  	std::wcout << "INTERNAL ERROR encountered on " << util.getLastSegment (util.stringToWstring(__FILE__), L"/") << ":" << __LINE__ << std::endl;

	} else if (outputStream == NULL)	{
  	std::wcout << "INTERNAL ERROR encountered on " << util.getLastSegment (util.stringToWstring(__FILE__), L"/") << ":" << __LINE__ << std::endl;

	} else	{
		// Make sure we're at END of our output file
		outputStream->seekp(0, std::fstream::end);
		// TODO: Busticated! outputStream->write(reinterpret_cast<char*>(EXPRESSION_OPCODE), sizeof (char));

		// Save off the position where the expression's total length is stored and
		// write 0s to it. It will get filled in later when writing the entire expression out has
		// been completed.
		uint32_t length_pos = outputStream->tellp();
		// TODO: Probably also Busticated! outputStream->write(reinterpret_cast<char*>(0x0), sizeof (uint32_t));

		std::wcout << L"********** writeExpr_123_Opr8r **********" << std::endl;
		ret_code = writeExpr_123_Opr8r (rootOfExpr);
		std::wcout << std::endl;

		std::wcout << L"********** writeExpr_Opr8r_123 **********" << std::endl;
		ret_code = writeExpr_Opr8r_123 (rootOfExpr);
		std::wcout << std::endl;

		std::wcout << L"********** writeExpr_321_Opr8r **********" << std::endl;
		ret_code = writeExpr_321_Opr8r (rootOfExpr);
		std::wcout << std::endl;

		std::wcout << L"********** writeExpr_Depth1st_123O **********" << std::endl;
		ret_code = writeExpr_Depth1st_123_Opr8r (rootOfExpr);
		std::wcout << std::endl;

//		std::wcout << L"********** writeExpr_1_Opr8r_23 **********" << std::endl;
//		ret_code = writeExpr_1_Opr8r_23 (rootOfExpr);
//		std::wcout << std::endl;

	}

	return (ret_code);
}
