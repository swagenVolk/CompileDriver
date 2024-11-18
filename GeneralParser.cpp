/*
 * GeneralParser.cpp
 *
 *  Created on: Nov 13, 2024
 *      Author: Mike Volk
 */

#include "GeneralParser.h"

using namespace std;

GeneralParser::GeneralParser(TokenPtrVector & inTknStream, CompileExecTerms & inUsrSrcTerms, std::string object_file_name)
	: interpretedFileWriter (object_file_name, inUsrSrcTerms)
	, interpreter (inUsrSrcTerms)
	, exprParser (inUsrSrcTerms)

{
	tknStream = inTknStream;
	usrSrcTerms = inUsrSrcTerms;
	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");
	errorInfo.set1stInSrcStack (thisSrcFile);
}

GeneralParser::~GeneralParser() {
	// TODO: Anything to add here?ScopeFrame
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
	bool isEOF = false;

  if (tknStream.empty())	{
  	errorInfo.set (INTERNAL_ERROR, thisSrcFile, __LINE__, L"Token stream is unexpectedly empty!");
  	isFailed = true;

  } else	{
  	std::shared_ptr<ScopeFrame> rootScope = std::make_shared<ScopeFrame> ();
  	scopeStack.push_back(rootScope);

  	std::pair<TokenTypeEnum, uint8_t> tknTypeEnum_opCode;

		while (!isFailed && !isEOF)	{

			if (!tknStream.empty())	{
				std::shared_ptr <Token> currTkn = { tknStream.front() };
				tknStream.erase(tknStream.begin());

				std::pair<TokenTypeEnum, uint8_t> enum_opCode = usrSrcTerms.getDataType_tknEnum_opCode (currTkn->_string);

				if (currTkn->tkn_type == END_OF_STREAM_TKN)	{
					isEOF = true;

					// This Token is a known data type; handle variable or fxn declaration
				} else if (enum_opCode.second != INVALID_OPCODE)	{
					if (OK != parseVarDeclaration (currTkn->_string, enum_opCode))
						isFailed = true;

				} else if (currTkn->tkn_type == KEYWORD_TKN && currTkn->_string == L"if")	{
			  	errorInfo.set (INTERNAL_ERROR, thisSrcFile, __LINE__, L"[if] KEYWORD not supported yet!");
			  	isFailed = true;

				} else if (currTkn->tkn_type == KEYWORD_TKN && currTkn->_string == L"else")	{
					// TODO:
			  	errorInfo.set (INTERNAL_ERROR, thisSrcFile, __LINE__, L"[else] KEYWORD not supported yet!");
					isFailed = true;

				} else if (currTkn->tkn_type == KEYWORD_TKN && currTkn->_string == L"for")	{
					// TODO:
			  	errorInfo.set (INTERNAL_ERROR, thisSrcFile, __LINE__, L"[for] KEYWORD not supported yet!");
					isFailed = true;

				} else if (currTkn->tkn_type == KEYWORD_TKN && currTkn->_string == L"while")	{
					// TODO:
			  	errorInfo.set (INTERNAL_ERROR, thisSrcFile, __LINE__, L"[while] KEYWORD not supported yet!");
					isFailed = true;

				} else	{
			  	errorInfo.set (INTERNAL_ERROR, thisSrcFile, __LINE__, L"Unhandled condition!");
			  	isFailed = true;
				}
			}
		}
  }

  if (!isFailed && isEOF)
  	ret_code = OK;

  if (OK != ret_code)	{
  	std::wcout << errorInfo.getFormattedMsg() << std::endl;
  }

	return (ret_code);

}

/* ****************************************************************************
 * Found a data type KEYWORD that indicates the beginning of a possibly plural
 * variable declaration.
 * ***************************************************************************/
int GeneralParser::parseVarDeclaration (std::wstring dataTypeStr, std::pair<TokenTypeEnum, uint8_t> tknType_opCode)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;
	bool isDeclarationEnded = false;

	uint32_t startFilePos = interpretedFileWriter.getWriteFilePos();
	std::wstring assignOpr8r = usrSrcTerms.getSrcOpr8rStrFor(ASSIGNMENT_OPR8R_OPCODE);

	uint32_t length_pos = interpretedFileWriter.writeFlexLenOpCode (VARIABLES_DECLARATION_OPCODE, errorInfo);
	if (0 != length_pos)	{
		// Save off the position where the expression's total length is stored and
		// write 0s to it. It will get filled in later after the remainder of the
		// declaration has been completed.

		// [op_code][total_length][datatype op_code][[string var_name][init_expression]]+
		//                        ^
		if (tknType_opCode.second == INVALID_OPCODE)	{
			errorInfo.set (INTERNAL_ERROR, thisSrcFile, __LINE__, L"Data type [" + dataTypeStr + L"] -> INVALID_OPCODE!");
			isFailed = true;

		} else if (OK != interpretedFileWriter.writeRawUnsigned (tknType_opCode.first, NUM_BITS_IN_BYTE, errorInfo))	{
			errorInfo.set (INTERNAL_ERROR, thisSrcFile, __LINE__, L"Failed writing data type op_code to interpreted file.");
			isFailed = true;

		} else	{
			varDeclarationState parserState = GET_VAR_NAME;

			std::shared_ptr <Token> currTkn;
			Token currVarName;

			while (!isFailed && !isDeclarationEnded)	{
				if (!tknStream.empty())	{
					// Grab next Token and remove from stream without destroying
					if (parserState != PARSE_EXPRESSION)	{
						// uint32 numFruits = 3 + 4, numVeggies = (3 * (1 + 2)), numPizzas = (4 + (2 * 3));
						//                  ^ Consumed on prev loop; consume 3 in PARSE_EXPRESSION logic, not HERE
						currTkn = tknStream.front();
						tknStream.erase(tknStream.begin());
					}

					if (parserState == GET_VAR_NAME)	{
						// bool isBobYerUncle, isFailed = false, isDeclarationEnded = false;
						//      ^              ^                 ^
						if (currTkn->tkn_type != KEYWORD_TKN || currTkn->_string.empty())	{
							errorInfo.set (USER_ERROR, thisSrcFile, __LINE__
									, L"Expected a KEYWORD for a variable name, but got " + currTkn->description());
							isFailed = true;

						} else if (OK != interpretedFileWriter.writeString (VAR_NAME_OPCODE, currTkn->_string, errorInfo))	{
							// [op_code][total_length][datatype op_code][[string var_name][init_expression]]+
							//                                            ^ Written out
							errorInfo.set(INTERNAL_ERROR, thisSrcFile, __LINE__
									, L"INTERNAL ERROR: Failed writing out variable name to interpreted file with " + currTkn->description());
							isFailed = true;
						} else	{
							std::shared_ptr<ScopeFrame> currScope = { scopeStack[scopeStack.size() - 1] };
							if (auto search = currScope->variables.find(currTkn->_string); search != currScope->variables.end())	{
								errorInfo.set(USER_ERROR, thisSrcFile, __LINE__
										, L"Variable " + currTkn->_string + L" already exists at current scope.");
								isFailed = true;

							} else	{
							// Put an uninitialized variable name & Token in the NameSpace
								Token starterTkn (tknType_opCode.first, L"", currTkn->line_number, currTkn->column_pos);
								currScope->variables[currTkn->_string] = starterTkn;
								currVarName = *currTkn;
								parserState = CHECK_FOR_EXPRESSION;
							}
							// Release temporary pointer
							currScope.reset();
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
									, L"Expected a [,] or [" + assignOpr8r + L"] or [" + usrSrcTerms.get_statement_ender() + L"] but got " + currTkn->description());
							isFailed = true;
						}

					} else if (parserState == PARSE_EXPRESSION)	{
						// TODO: Add option to build Token List as we go and return it
						std::shared_ptr<Token> emptyTkn = std::make_shared<Token>();
						std::shared_ptr<ExprTreeNode> exprTree = std::make_shared<ExprTreeNode> (emptyTkn);
						// TODO: indicate what closed the current expression
						Token exprEnder;
						std::vector<Token> flatExprTkns;
						if (OK != exprParser.parseExpression (tknStream, exprTree, exprEnder, END_COMMA_IS_EXPECTED, errorInfo))	{
							isFailed = true;

						} else if (OK != interpretedFileWriter.writeExpressionToFile(exprTree, flatExprTkns, errorInfo))	{
							// ********** writeExpr_12_Opr8r called from InterpretedFileWriter.cpp:213 **********
							// [3] TODO: writeToken: INT8_TKN 3 _signed = 3; on line 3 column 20
							// Should be more like [3][4][+]
							// TODO: turnClosedScopeIntoTree is only called when parens are closed!
							isFailed = true;

						} else if (OK != interpreter.resolveFlattenedExpr(flatExprTkns, errorInfo))	{
							// TODO:
							isFailed = true;

						} else	{
							// flattenedExpr should have 1 Token left - the result of the expression
							if (flatExprTkns.size() != 1)	{
								errorInfo.set(INTERNAL_ERROR, thisSrcFile, __LINE__
										, L"Failed to resolve variable initialization expression for " + currVarName.description());
								isFailed = true;

							} else	{
								// Update the relevant variable at the current scope
								std::shared_ptr<ScopeFrame> currScope = { scopeStack[scopeStack.size() - 1] };
								if (auto search = currScope->variables.find(currVarName._string); search != currScope->variables.end())	{
									Token existingTkn = search->second;
									if (OK != existingTkn.convertTo(flatExprTkns[0]))	{
										errorInfo.set(USER_ERROR, thisSrcFile, __LINE__
												, L"Data type mismatch between variable and its initialization on " + currVarName.description());
										isFailed = true;
									}
								} else	{
								// WTF?! This should *NEVER* happen
									errorInfo.set(INTERNAL_ERROR, thisSrcFile, __LINE__
											, L"Variable " + currTkn->_string + L" no longer exists at current scope.");
									isFailed = true;
								}
								// Release temporary pointer
								currScope.reset();
							}
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
			ret_code = interpretedFileWriter.writeObjectLen (startFilePos, length_pos, errorInfo);
	}


	return (ret_code);

}

