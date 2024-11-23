/*
 * GeneralParser.cpp
 *
 *  Created on: Nov 13, 2024
 *      Author: Mike Volk
 */

#include "GeneralParser.h"

using namespace std;

GeneralParser::GeneralParser(TokenPtrVector & inTknStream, std::wstring userSrcFileName, CompileExecTerms & inUsrSrcTerms
		, UserMessages & userMessages, std::string object_file_name, std::shared_ptr<VariablesScope> inVarScopeStack)
	: interpretedFileWriter (object_file_name, inUsrSrcTerms, userMessages)
	, interpreter (inUsrSrcTerms, inVarScopeStack, userSrcFileName, userMessages)
	, exprParser (inUsrSrcTerms, inVarScopeStack, userSrcFileName, userMessages)

{
	tknStream = inTknStream;
	this->userSrcFileName = userSrcFileName;
	usrSrcTerms = inUsrSrcTerms;
	this->userMessages = userMessages;
	varScopeStack = inVarScopeStack;
	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");
}

GeneralParser::~GeneralParser() {
	// TODO: Anything to add here?ScopeFrame
	varScopeStack.reset();
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
  	userMessages.logMsg (INTERNAL_ERROR, L"Token stream is unexpectedly empty!", thisSrcFile, __LINE__, 0);
  	isFailed = true;

  } else	{
  	std::pair<TokenTypeEnum, uint8_t> tknTypeEnum_opCode;

		while (!isFailed && !isEOF)	{

			if (!tknStream.empty())	{
				std::shared_ptr <Token> currTkn = tknStream.front();
				tknStream.erase(tknStream.begin());

				std::pair<TokenTypeEnum, uint8_t> enum_opCode = usrSrcTerms.getDataType_tknEnum_opCode (currTkn->_string);
				std::shared_ptr<Token> emptyTkn = std::make_shared<Token>();

				if (currTkn->tkn_type == END_OF_STREAM_TKN)	{
					isEOF = true;

					// This Token is a known data type; handle variable or fxn declaration
				} else if (enum_opCode.second != INVALID_OPCODE)	{
					if (OK != parseVarDeclaration (currTkn->_string, enum_opCode))
						isFailed = true;

				} else if (currTkn->tkn_type == KEYWORD_TKN && OK == varScopeStack->findVar(currTkn->_string, 0, scratchTkn, READ_ONLY, userMessages))	{
					// Put the current Token back; exprParser will need it!
					tknStream.insert(tknStream.begin(), currTkn);

					std::shared_ptr<Token> emptyTkn = std::make_shared<Token>();
					std::shared_ptr<ExprTreeNode> exprTree = std::make_shared<ExprTreeNode> (emptyTkn);
					// TODO: indicate what closed the current expression
					Token exprEnder;
					std::vector<Token> flatExprTkns;
					if (OK != exprParser.makeExprTree (tknStream, exprTree, exprEnder, END_COMMA_NOT_EXPECTED, userMessages))	{
						isFailed = true;

					} else if (OK != interpretedFileWriter.flattenExprTreeWriteToFile(exprTree, flatExprTkns, userMessages))	{
						// ********** writeExpr_12_Opr8r called from InterpretedFileWriter.cpp:213 **********
						// [3] TODO: writeToken: INT8_TKN 3 _signed = 3; on line 3 column 20
						// Should be more like [3][4][+]
						isFailed = true;

					} else if (OK != interpreter.resolveFlattenedExpr(flatExprTkns, userMessages))	{
						isFailed = true;

					} else	{
						// flattenedExpr should have 1 Token left - the result of the expression
						if (flatExprTkns.size() != 1)	{
							userMessages.logMsg(INTERNAL_ERROR, L"Failed to resolve expression starting with " + currTkn->descr_sans_line_num_col()
									, thisSrcFile, __LINE__, 0);
							isFailed = true;

						} else if (OK != varScopeStack->findVar(currTkn->_string, 0, flatExprTkns[0], COMMIT_WRITE, userMessages))	{
							// Don't limit search to current scope
							userMessages.logMsg (INTERNAL_ERROR
									, L"Could not find " + currTkn->_string + L" after parsing initialization expression in " + userSrcFileName
									+ L" on|near " + currTkn->descr_line_num_col(), thisSrcFile, __LINE__, 0);
							isFailed = true;
						}

						// TODO: See the final result
						util.dumpTokenList (flatExprTkns, usrSrcTerms, thisSrcFile, __LINE__);

					}

				} else if (currTkn->tkn_type == KEYWORD_TKN && currTkn->_string == L"if")	{
			  	userMessages.logMsg (INTERNAL_ERROR, L"[if] KEYWORD not supported yet!", thisSrcFile, __LINE__, 0);
			  	isFailed = true;

				} else if (currTkn->tkn_type == KEYWORD_TKN && currTkn->_string == L"else")	{
					// TODO:
			  	userMessages.logMsg (INTERNAL_ERROR, L"[else] KEYWORD not supported yet!", thisSrcFile, __LINE__, 0);
					isFailed = true;

				} else if (currTkn->tkn_type == KEYWORD_TKN && currTkn->_string == L"for")	{
					// TODO:
			  	userMessages.logMsg (INTERNAL_ERROR, L"[for] KEYWORD not supported yet!", thisSrcFile, __LINE__, 0);
					isFailed = true;

				} else if (currTkn->tkn_type == KEYWORD_TKN && currTkn->_string == L"while")	{
					// TODO:
			  	userMessages.logMsg (INTERNAL_ERROR, L"[while] KEYWORD not supported yet!", thisSrcFile, __LINE__, 0);
					isFailed = true;

				} else	{
			  	userMessages.logMsg (INTERNAL_ERROR, L"Unhandled condition!", thisSrcFile, __LINE__, 0);
			  	isFailed = true;
				}
			}
		}
  }

  varScopeStack->displayVariables();

  if (!isFailed && isEOF)
  	ret_code = OK;

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

	uint32_t length_pos = interpretedFileWriter.writeFlexLenOpCode (VARIABLES_DECLARATION_OPCODE, userMessages);
	if (0 != length_pos)	{
		// Save off the position where the expression's total length is stored and
		// write 0s to it. It will get filled in later after the remainder of the
		// declaration has been completed.

		// [op_code][total_length][datatype op_code][[string var_name][init_expression]]+
		//                        ^
		if (tknType_opCode.second == INVALID_OPCODE)	{
			userMessages.logMsg (INTERNAL_ERROR, L"Data type [" + dataTypeStr + L"] -> INVALID_OPCODE!", thisSrcFile, __LINE__, 0);
			isFailed = true;

		} else if (OK != interpretedFileWriter.writeRawUnsigned (tknType_opCode.first, NUM_BITS_IN_BYTE, userMessages))	{
			userMessages.logMsg (INTERNAL_ERROR, L"Failed writing data type op_code to interpreted file.", thisSrcFile, __LINE__, 0);
			isFailed = true;

		} else	{
			varDeclarationState parserState = GET_VAR_NAME;

			std::shared_ptr <Token> currTkn;
			Token currVarNameTkn;

			while (!isFailed && !isDeclarationEnded)	{
				if (!tknStream.empty())	{
					// Grab next Token and remove from stream without destroying
					if (parserState != PARSE_INIT_EXPR)	{
						// uint32 numFruits = 3 + 4, numVeggies = (3 * (1 + 2)), numPizzas = (4 + (2 * 3));
						//                  ^ Consumed on prev loop; consume 3 in PARSE_EXPRESSION logic, not HERE
						currTkn = tknStream.front();
						tknStream.erase(tknStream.begin());
					}

					if (parserState == GET_VAR_NAME)	{
						// bool isBobYerUncle, isFailed = false, isDeclarationEnded = false;
						//      ^              ^                 ^
						if (currTkn->tkn_type != KEYWORD_TKN || currTkn->_string.empty())	{
							userMessages.logMsg (USER_ERROR, L"Expected a KEYWORD for a variable name, but got " + currTkn->descr_sans_line_num_col()
									, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
							isFailed = true;

						} else if (OK != interpretedFileWriter.writeString (VAR_NAME_OPCODE, currTkn->_string, userMessages))	{
							// [op_code][total_length][datatype op_code][[string var_name][init_expression]]+
							//                                            ^ Written out
							userMessages.logMsg (INTERNAL_ERROR
									, L"INTERNAL ERROR: Failed writing out variable name to interpreted file with " + currTkn->descr_sans_line_num_col()
									, thisSrcFile, __LINE__, 0);
							isFailed = true;

						} else if (OK == varScopeStack->findVar(currTkn->_string, 1, scratchTkn, READ_ONLY, userMessages))	{
								userMessages.logMsg (USER_ERROR, L"Variable " + currTkn->_string + L" already exists at current scope."
										, thisSrcFile, currTkn->get_line_number(), currTkn->get_column_pos());
								isFailed = true;

						} else	{
							// Put an uninitialized variable name & Token in the NameSpace
							Token starterTkn (tknType_opCode.first, L"");
							if (OK != varScopeStack->insertNewVarAtCurrScope(currTkn->_string, starterTkn))	{
								userMessages.logMsg (INTERNAL_ERROR, L"Failed to insert " + currTkn->_string + L" into NameSpace AFTER existence check!"
										, thisSrcFile, __LINE__, 0);

								isFailed = true;

							} else	{
								currVarNameTkn = *currTkn;
								parserState = CHECK_FOR_INIT_EXPR;
							}
						}

					} else if (parserState == CHECK_FOR_INIT_EXPR)	{
						// bool isBobYerUncle, isFailed = false, isDeclarationEnded = false, isSueYerAunt;
						//                   ^          ^                           ^                    ^
 						if (currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L",")
							parserState = GET_VAR_NAME;
						else if (currTkn->tkn_type == SRC_OPR8R_TKN && currTkn->_string == assignOpr8r)
							parserState = PARSE_INIT_EXPR;
						else if (currTkn->tkn_type == SRC_OPR8R_TKN && currTkn->_string == usrSrcTerms.get_statement_ender())
							isDeclarationEnded = true;
						else	{
							userMessages.logMsg (USER_ERROR
									, L"Expected a [,] or [" + assignOpr8r + L"] or [" + usrSrcTerms.get_statement_ender() + L"] but got " + currTkn->descr_sans_line_num_col()
									, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
							isFailed = true;
						}

					} else if (parserState == PARSE_INIT_EXPR)	{
						if (OK != resolveVarInitExpr (currVarNameTkn, *currTkn, parserState, isDeclarationEnded))
							isFailed = true;

					} else if (parserState == TIDY_AFTER_EXPR)	{
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
			ret_code = interpretedFileWriter.writeObjectLen (startFilePos, length_pos, userMessages);
	}


	return (ret_code);

}

/* ****************************************************************************
 * Found the beginning of an initialization expression. Resolve the expression
 * and update the variable that was placed in the NameSpace earlier.
 * ***************************************************************************/
int GeneralParser::resolveVarInitExpr (Token & varTkn, Token currTkn, varDeclarationState & parserState, bool & isDeclarationEnded)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;

	// TODO: Add option to build Token List as we go and return it
	std::shared_ptr<Token> emptyTkn = std::make_shared<Token>();
	std::shared_ptr<ExprTreeNode> exprTree = std::make_shared<ExprTreeNode> (emptyTkn);
	// TODO: indicate what closed the current expression
	Token exprEnder;
	std::vector<Token> flatExprTkns;
	if (OK != exprParser.makeExprTree (tknStream, exprTree, exprEnder, END_COMMA_IS_EXPECTED, userMessages))	{
		isFailed = true;

	} else if (OK != interpretedFileWriter.flattenExprTreeWriteToFile(exprTree, flatExprTkns, userMessages))	{
		// ********** writeExpr_12_Opr8r called from InterpretedFileWriter.cpp:213 **********
		// [3] TODO: writeToken: INT8_TKN 3 _signed = 3; on line 3 column 20
		// Should be more like [3][4][+]
		isFailed = true;

	} else if (OK != interpreter.resolveFlattenedExpr(flatExprTkns, userMessages))	{
		isFailed = true;

	} else	{
		// flattenedExpr should have 1 Token left - the result of the expression
		if (flatExprTkns.size() != 1)	{
			userMessages.logMsg (INTERNAL_ERROR, L"Failed to resolve variable initialization expression for " + varTkn.descr_sans_line_num_col()
					, thisSrcFile, __LINE__, 0);
			isFailed = true;

		} else if (OK != varScopeStack->findVar(varTkn._string, 1, flatExprTkns[0], COMMIT_WRITE, userMessages))	{
			// Pass 1 to limit search to the current scope
			userMessages.logMsg (INTERNAL_ERROR, L"Failed to find variable declaration for " + varTkn.descr_line_num_col()
					, thisSrcFile, __LINE__, 0);
			isFailed = true;
		}
	}

	if (!isFailed)	{
		if (exprEnder.tkn_type == SPR8R_TKN && exprEnder._string == L",")
			parserState = GET_VAR_NAME;
		else if (exprEnder.tkn_type == SRC_OPR8R_TKN && exprEnder._string == usrSrcTerms.get_statement_ender())
			isDeclarationEnded = true;
		else
			// Expression didn't close with a [,]. Check if [,] or [;] needs to be consumed.
			parserState = TIDY_AFTER_EXPR;

		ret_code = OK;
	}


	return (ret_code);
}
