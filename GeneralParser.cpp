/*
 * GeneralParser.cpp
 *
 *  Created on: Nov 13, 2024
 *      Author: Mike Volk
 */

#include "GeneralParser.h"

using namespace std;

GeneralParser::GeneralParser(TokenPtrVector & inTknStream, CompileExecTerms & inUsrSrcTerms, std::string object_file_name) {
	tknStream = inTknStream;
	usrSrcTerms = inUsrSrcTerms;
	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");
	std::shared_ptr<InterpretedFileWriter> interpretedFileWriter (new InterpretedFileWriter (object_file_name, usrSrcTerms));
	std::shared_ptr<RunTimeInterpreter> interpreter (new RunTimeInterpreter (usrSrcTerms));
	std::unique_ptr<ExpressionParser> exprParser (new ExpressionParser (usrSrcTerms, tknStream));
}

GeneralParser::~GeneralParser() {
	// Free up memory of the shared_ptrs
	interpretedFileWriter.reset();
	interpreter.reset();

}

/* ****************************************************************************
 * Parse through the user's source file, looking for KEYWORDs such as:
 * data type KEYWORDs that either indicate a variable or a fxn declaration
 * [if] [else if] [else] [while] [for] [?fxn]
 * the current expression and if it's well formed, commit it to
 * the interpreted stream. If it's not well formed, generate a clear error
 * message to the user.
 * ***************************************************************************/
int GeneralParser::findKeyWordObjects () 	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;

  if (tknStream.empty())	{
  	errorInfo.set (INTERNAL_ERROR, thisSrcFile, __LINE__, L"Token stream is unexpectedly empty!");
  	isFailed = true;

  } else	{
  	bool isEOF = false;
  	uint8_t dataTypeOpCode;

		while (!isFailed && !isEOF)	{

			if (!tknStream.empty())	{
				Token *currTkn = tknStream.front();

				dataTypeOpCode = usrSrcTerms.getDataTypeOpCode (currTkn->_string);

				if (currTkn->tkn_type == END_OF_STREAM_TKN)
					isEOF = true;

				else if (dataTypeOpCode != INVALID_OPCODE)	{
					// This Token is a known data type; handle variable or fxn declaration
					if (OK != parseVarDeclaration (dataTypeOpCode))
						isFailed = true;

				} else if (currTkn->tkn_type == KEYWORD_TKN && currTkn->_string == L"if")	{
					// TODO:
					isFailed = true;

				} else if (currTkn->tkn_type == KEYWORD_TKN && currTkn->_string == L"else")	{
					// TODO:
					isFailed = true;

				} else if (currTkn->tkn_type == KEYWORD_TKN && currTkn->_string == L"for")	{
					// TODO:
					isFailed = true;

				} else if (currTkn->tkn_type == KEYWORD_TKN && currTkn->_string == L"while")	{
					// TODO:
					isFailed = true;

				} else	{
					isFailed = true;
				}
			}
		}
  }

	return (ret_code);

}

/* ****************************************************************************
 * Found a data type KEYWORD that indicates the beginning of a possibly plural
 * variable declaration.
 * ***************************************************************************/
int GeneralParser::parseVarDeclaration (uint8_t dataTypeOpCode)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;
	bool isDeclarationEnded = false;

	uint32_t startFilePos = interpretedFileWriter->getWriteFilePos();
	std::wstring assignOpr8r = usrSrcTerms.getSrcOpr8rStrFor(ASSIGNMENT_OPR8R_OPCODE);

	uint32_t length_pos = interpretedFileWriter->writeFlexLenOpCode (VARIABLES_DECLARATION_OPCODE);
	if (0 != length_pos)	{
		// Save off the position where the expression's total length is stored and
		// write 0s to it. It will get filled in later after the remainder of the
		// declaration has been completed.

		// [op_code][total_length][datatype op_code][[string var_name][init_expression]]+
		//                        ^
		if (OK != interpretedFileWriter->writeRawUnsigned (dataTypeOpCode, NUM_BITS_IN_BYTE))	{
			errorInfo.set (INTERNAL_ERROR, thisSrcFile, __LINE__, L"Failed writing data type op_code to interpreted file.");
			isFailed = true;

		} else	{
			varDeclarationState parserState = GET_VAR_NAME;

			while (!isFailed && !isDeclarationEnded)	{
				if (!tknStream.empty())	{
					// Grab next Token and remove from stream without destroying
					Token *currTkn = tknStream.front();
					tknStream.erase(tknStream.begin());

					if (parserState == GET_VAR_NAME)	{
						// bool isBobYerUncle, isFailed = false, isDeclarationEnded = false;
						//      ^              ^                 ^
						if (currTkn->tkn_type != KEYWORD_TKN || currTkn->_string.empty())	{
							errorInfo.set (USER_ERROR, thisSrcFile, __LINE__
									, L"Expected a KEYWORD for a variable name, but got " + currTkn->description());
							isFailed = true;

						} else if (OK != interpretedFileWriter->writeString (VAR_NAME_OPCODE, currTkn->_string))	{
							// [op_code][total_length][datatype op_code][[string var_name][init_expression]]+
							//                                            ^ Written out
							errorInfo.set(INTERNAL_ERROR, thisSrcFile, __LINE__
									, L"INTERNAL ERROR: Failed writing out variable name to interpreted file with " + currTkn->description());
							isFailed = true;
						} else	{
							parserState = CHECK_FOR_EXPRESSION;
							// TODO: Put a Token in the NameSpace
						}

					} else if (parserState == CHECK_FOR_EXPRESSION)	{
						// bool isBobYerUncle, isFailed = false, isDeclarationEnded = false, isSueYerAunt;
						//                   ^          ^                           ^                    ^
 						if (currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L",")
							parserState = GET_VAR_NAME;
						else if (currTkn->tkn_type == SRC_OPR8R_TKN && currTkn->_string == assignOpr8r)
							parserState = PARSE_EXPRESSION;
						else if (currTkn->tkn_type == SRC_OPR8R_TKN && currTkn->_string == usrSrcTerms.get_statement_ender())
							isDeclarationEnded = true;
						else	{
							errorInfo.set(USER_ERROR, thisSrcFile, __LINE__
									, L"Expected a \",\" or " + assignOpr8r + L" or " + usrSrcTerms.get_statement_ender() + L", but got " + currTkn->description());
							isFailed = true;
						}

					} else if (parserState == PARSE_EXPRESSION)	{
						// TODO: Add option to build Token List as we go and return it
						ExprTreeNode * exprTree = NULL;
						// TODO: indicate what closed the current expression
						Token exprEnder;
						std::vector<Token> flatExprTkns;
						if (OK != exprParser->parseExpression (&exprTree, exprEnder, END_COMMA_IS_EXPECTED))	{
							isFailed = true;

						} else if (OK != interpretedFileWriter->writeExpressionToFile(exprTree, flatExprTkns))	{
							isFailed = true;

						} else if (OK != interpreter->resolveFlattenedExpr(flatExprTkns))	{
							// TODO:
							std::wcout << L"resolveExpression FAILED!" << std::endl;
							isFailed = true;

						} else	{
							// flattenedExpr should have 1 Token left - the result of the expression
							// TODO: Type check and update the relevant NSO

						}

						if (exprEnder.tkn_type == SPR8R_TKN && exprEnder._string == L",")
							parserState = GET_VAR_NAME;
						else if (exprEnder.tkn_type == SRC_OPR8R_TKN && exprEnder._string == usrSrcTerms.get_statement_ender())
							isDeclarationEnded = true;
						else
							// Expression didn't close with a [,]. Check if [,] or [;] needs to be consumed.
							parserState = TIDY_AFTER_EXPRESSION;

					} else if (parserState == TIDY_AFTER_EXPRESSION)	{
						if (currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L",")
							parserState = GET_VAR_NAME;
						else if (currTkn->tkn_type == SRC_OPR8R_TKN && currTkn->_string == usrSrcTerms.get_statement_ender())
							isDeclarationEnded = true;
						else
							isFailed = true;
					}
				}
			}
		}

		if (!isFailed)
			ret_code = interpretedFileWriter->writeObjectLen (startFilePos, length_pos);
	}


	return (ret_code);

}

