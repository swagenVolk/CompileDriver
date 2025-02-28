/*
 * GeneralParser.cpp
 *
 *  Created on: Nov 13, 2024
 *      Author: Mike Volk
 */

#include "GeneralParser.h"
#include "InfoWarnError.h"
#include "OpCodes.h"
#include "Token.h"
#include "StackOfScopes.h"
#include "common.h"
#include <cstdint>
#include <iostream>

using namespace std;

GeneralParser::GeneralParser(TokenPtrVector & inTknStream, std::wstring userSrcFileName, CompileExecTerms & inUsrSrcTerms
		, std::shared_ptr<UserMessages> userMessages, std::string object_file_name, std::shared_ptr<StackOfScopes> inVarScopeStack
		, logLvlEnum logLvl)
	: interpretedFileWriter (object_file_name, inUsrSrcTerms, userMessages)
	, interpreter (inUsrSrcTerms, inVarScopeStack, userSrcFileName, userMessages, logLvl)
	, exprParser (inUsrSrcTerms, inVarScopeStack, userSrcFileName, userMessages, logLvl)

{
	tknStream = inTknStream;
	this->userSrcFileName = userSrcFileName;
	usrSrcTerms = inUsrSrcTerms;
	this->userMessages = userMessages;
	scopedNameSpace = inVarScopeStack;
	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");
	userErrorLimit = 30;
	logLevel = logLvl;

	ender_comma_list.push_back (usrSrcTerms.get_statement_ender());
	ender_comma_list.push_back (L",");

	ender_list.push_back (usrSrcTerms.get_statement_ender());
}

GeneralParser::~GeneralParser() {
	// TODO: Anything to add here?ScopeWindow
	scopedNameSpace.reset();
	ender_comma_list.clear();
	ender_list.clear();

}

/* ****************************************************************************
 * Have we accumulated enough user errors to stop compilation?
 * ***************************************************************************/
bool GeneralParser::isProgressBlocked ()	{
	bool isBlocked = false;
	int numUnique;
	int numTotal;

	userMessages->getInternalErrorCnt(numUnique, numTotal);

	if (numUnique > 0)
		isBlocked = true;
	
	else	{
		userMessages->getUserErrorCnt(numUnique, numTotal);

		if (numUnique >= userErrorLimit)
			isBlocked = true;
	}

	return (isBlocked);
}

/* ****************************************************************************
 * Chew through the Token stream until we find the Token we're looking for.
 * Typically used when Compiler hits an error in the middle of an expression.
 * This fxn will be used to find the expected closing Token, and then
 * compilation will pick up from there.
 * ***************************************************************************/
int GeneralParser::chompUntil_infoMsgAfter (std::vector<std::wstring> searchStrings, Token & closerTkn)	{
	int ret_code = GENERAL_FAILURE;
	bool isTknFound = false;
	bool isEOF = false;

	while (!isTknFound && !isEOF)	{

		if (!tknStream.empty())	{
			std::shared_ptr <Token> currTkn = tknStream.front();
			tknStream.erase(tknStream.begin());

			for (int idx = 0; idx < searchStrings.size() && !isTknFound; idx++)	{
				if (currTkn->_string == searchStrings[idx])	{
					isTknFound = true;
					closerTkn = *currTkn;
					ret_code = OK;
					userMessages->logMsg(INFO, RESUME_COMPILATION + currTkn->descr_sans_line_num_col()
							, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
				}
			}

			// Free up the memory
			currTkn.reset();

		} else	{
			isEOF = true;
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * Parse through the user's source file, looking for USER_WORDs such as:
 * data type USER_WORDs that either indicate a variable or a fxn declaration
 * [if] [else if] [else] [while] [for] [?fxn]
 * the current expression and if it's well formed, commit it to
 * the interpreted stream. If it's not well formed, generate a clear error
 * message to the user.
 * TODO:
 * Good place to check for scope objects that are only legal at the global scope,
 * if|when I get to that point of customization.
 * ***************************************************************************/
int GeneralParser::compileRootScope () 	{
	int ret_code = GENERAL_FAILURE;

  if (tknStream.empty())	{
  	userMessages->logMsg (INTERNAL_ERROR, L"Token stream is unexpectedly empty!", thisSrcFile, __LINE__, 0);

  } else	{
		uint32_t length_pos = interpretedFileWriter.writeFlexLenOpCode (ANON_SCOPE_OPCODE);
		// TODO: Do I want to add a checksum for ANON_SCOPEs? Would only be calculated for ROOT
		// TODO: Remove ROOT scope creation from StackOfScopes?
		if (OK == compileCurrScope())	{
			if (OK != interpretedFileWriter.writeObjectLen (0))
  			userMessages->logMsg (INTERNAL_ERROR, L"Could not back fill ROOT scope length!", thisSrcFile, __LINE__, 0);
			else
				ret_code = OK;
		}
  }

	return (ret_code);
}

/* ****************************************************************************
 * Parse through the user's source file, looking for USER_WORDs such as:
 * data type USER_WORDs that either indicate a variable or a fxn declaration
 * [if] [else if] [else] [while] [for] [?fxn]
 * the current expression and if it's well formed, commit it to
 * the interpreted stream. If it's not well formed, generate a clear error
 * message to the user.
 * ***************************************************************************/
int GeneralParser::compileCurrScope () 	{
	int ret_code = GENERAL_FAILURE;
	bool isStopFail = false;
	bool isEOF = false;
	bool isExprClosed = false;
	bool isVarDecClosed = false;
	Token tmpTkn;
	uint8_t prevScopeObject = INVALID_OPCODE;

  if (tknStream.empty())	{
  	userMessages->logMsg (INTERNAL_ERROR, L"Token stream is unexpectedly empty!", thisSrcFile, __LINE__, 0);
  	isStopFail = true;

  } else	{
  	std::pair<TokenTypeEnum, uint8_t> tknTypeEnum_opCode;
		std::wstring lookUpMsg;
		bool isNewScopened;

		while (!isStopFail && !isEOF)	{

			if (!tknStream.empty())	{
				std::shared_ptr <Token> currTkn = tknStream.front();
				tknStream.erase(tknStream.begin());

				std::pair<TokenTypeEnum, uint8_t> enum_opCode = usrSrcTerms.getDataType_tknEnum_opCode (currTkn->_string);
				std::shared_ptr<Token> emptyTkn = std::make_shared<Token>();

				if (currTkn->tkn_type == END_OF_STREAM_TKN)	{
					isEOF = true;

				} else if (currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L"}")	{
					// Indicates we should close the current scope
					closeScopeErr closeErr;

 					if (OK != scopedNameSpace->closeTopScope(interpretedFileWriter, prevScopeObject, closeErr))	{
						isStopFail = true;

						if (closeErr == ONLY_ROOT_SCOPE_OPEN)
							userMessages->logMsg(USER_ERROR, L"Failure closing scope; possibly unmatched " + currTkn->descr_sans_line_num_col(), userSrcFileName
							, currTkn->get_line_number(), currTkn->get_column_pos());

						else if (closeErr == SCOPE_CLOSE_UKNOWN_ERROR || NO_SCOPES_OPEN)
							userMessages->logMsg (INTERNAL_ERROR, L"Failure closing scope with: " + currTkn->descr_line_num_col(), thisSrcFile, __LINE__, 0);
					}

				} else if (currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L"{")	{
					// Indicates we should OPEN a new but untethered scope.  Kind of unusual, but legal
					if (OK != openFloatyScope(*currTkn))
						isStopFail = true;

				} else if (enum_opCode.second != INVALID_OPCODE)	{
					// This Token is a known data type; handle variable or fxn declaration
					if (OK != parseVarDeclaration (currTkn->_string, enum_opCode, isVarDecClosed))	{
						if (isProgressBlocked())	{
							isStopFail = true;

						} else if (!isVarDecClosed)	{
							// Search for a [;] and try compiling again from that point on
							if (OK != chompUntil_infoMsgAfter (ender_list, tmpTkn))
								isStopFail = true;
						}
					} else {
						prevScopeObject = VARIABLES_DECLARATION_OPCODE;
					}
				} else if (currTkn->tkn_type == USER_WORD_TKN 
						&& OK != scopedNameSpace->findVar(currTkn->_string, 0, scratchTkn, READ_ONLY, lookUpMsg))	{
			  		userMessages->logMsg (USER_ERROR, L"Unrecognized USER_WORD: " + currTkn->descr_sans_line_num_col()
							, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());

						if (isProgressBlocked ())
							isStopFail = true;
						else
						 	prevScopeObject = VAR_NAME_OPCODE;

				} else if ((currTkn->tkn_type == USER_WORD_TKN && OK == scopedNameSpace->findVar(currTkn->_string, 0, scratchTkn, READ_ONLY, lookUpMsg))
					|| currTkn->tkn_type == SRC_OPR8R_TKN && (currTkn->_string == usrSrcTerms.getSrcOpr8rStrFor (PRE_INCR_OPR8R_OPCODE)
					|| currTkn->_string == usrSrcTerms.getSrcOpr8rStrFor (PRE_DECR_OPR8R_OPCODE)))	{
					// Put the current Token back; exprParser will need it!
					tknStream.insert(tknStream.begin(), currTkn);
					handleExpression(isStopFail);
					prevScopeObject = EXPRESSION_OPCODE;

				} else if (currTkn->tkn_type == RESERVED_WORD_TKN && currTkn->_string == L"if")	{
					// Handle [if] block
					if (OK != compile_if_type_block(IF_SCOPE_OPCODE, *currTkn, isNewScopened))
			  		isStopFail = true;
					else
						prevScopeObject = IF_SCOPE_OPCODE;

				} else if (currTkn->tkn_type == RESERVED_WORD_TKN && currTkn->_string == L"else")	{
					// Handle [else if]|[else] block
					if (!tknStream.empty())	{
						std::shared_ptr <Token> checkForIf = tknStream.front();

						if (checkForIf->tkn_type == RESERVED_WORD_TKN && checkForIf->_string == L"if")	{
							tknStream.erase(tknStream.begin());
							if (prevScopeObject != IF_SCOPE_OPCODE && prevScopeObject != ELSE_IF_SCOPE_OPCODE)	{
								userMessages->logMsg (USER_ERROR, L"Expected [if] or [else if] block before " + currTkn->descr_sans_line_num_col()
									, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
								isStopFail = true;
							
							} else if (OK != compile_if_type_block(ELSE_IF_SCOPE_OPCODE, *checkForIf, isNewScopened))	{
								isStopFail = true;
							
							} else	{
								prevScopeObject = ELSE_IF_SCOPE_OPCODE;
							}
						
						} else {
							if (prevScopeObject != IF_SCOPE_OPCODE && prevScopeObject != ELSE_IF_SCOPE_OPCODE)	{
								userMessages->logMsg (USER_ERROR, L"Expected [if] or [else if] block before " + currTkn->descr_sans_line_num_col()
									, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
								isStopFail = true;
							} else if (OK != compile_if_type_block(ELSE_SCOPE_OPCODE, *currTkn, isNewScopened ))	{
								isStopFail = true;
							
							} else	{
								// Invalidate whether new scope opened or else block contains a single statement
								prevScopeObject = INVALID_OPCODE;
							}
						}
					}

				} else if (currTkn->tkn_type == RESERVED_WORD_TKN && currTkn->_string == L"for")	{
					// TODO:
			  	userMessages->logMsg (INTERNAL_ERROR, L"[for] RESERVED_WORD not supported yet!", thisSrcFile, __LINE__, 0);
					isStopFail = true;

				} else if (currTkn->tkn_type == RESERVED_WORD_TKN && currTkn->_string == L"while")	{
					// TODO:
			  	userMessages->logMsg (INTERNAL_ERROR, L"[while] RESERVED_WORD not supported yet!", thisSrcFile, __LINE__, 0);
					isStopFail = true;
				} else	{
					userMessages->logMsg (USER_ERROR, L"Unexpected Token: " + currTkn->descr_sans_line_num_col()
						, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
			  	isStopFail = true;
				}
			} else	{
				isEOF = true;
			}
		}
  }

  if (!isStopFail && isEOF)
  	ret_code = OK;

	return (ret_code);

}

/* ****************************************************************************
 * Encountered a floating [{] that opens up a scope.  Weird, but legal!
 * Use INVALID_OPCODE to indicate what opened the scope up.  
 * Will use INVALID_OPCODE also to open up ROOT scope, but we should be able to
 * differentiate with that object starting at file position 0.
 * ***************************************************************************/
int GeneralParser::openFloatyScope(Token openScopeTkn)	{
	int ret_code = GENERAL_FAILURE;

	uint32_t startFilePos = interpretedFileWriter.getWriteFilePos();
	uint32_t length_pos = interpretedFileWriter.writeFlexLenOpCode (ANON_SCOPE_OPCODE);

	if (OK != interpretedFileWriter.writeRawUnsigned(INVALID_OPCODE, NUM_BITS_IN_BYTE))	{
		userMessages->logMsg (INTERNAL_ERROR, L"Failure writing scope type", thisSrcFile, __LINE__, 0);

	} else if (OK != scopedNameSpace->openNewScope(INVALID_OPCODE, openScopeTkn, startFilePos, 0))	{
		userMessages->logMsg (INTERNAL_ERROR, L"Failure opening scope with: " + openScopeTkn.descr_line_num_col(), thisSrcFile, __LINE__, 0);

	} else {
		ret_code = OK;
	}

	return (ret_code);

}

/* ****************************************************************************
 * [if][else if][else] KEYWORD has been encountered at the current scope and consumed.
 * For [if] and [else if] blocks, a conditional expression enclosed in parentheses is 
 * expected after this. If an opening [{] is encountered, then a new scope is opened 
 * by the compiler and 0 or more expressions, variable declarations, etc. will follow.  
 * Otherwise, only 1 expression is expected and the scope for this block is immediately 
 * closed by this proc in the eyes of both the compiler and the interpreter.
 * [op_code][total_length][conditional EXPRESSION][code block]
 * ***************************************************************************/
int GeneralParser::compile_if_type_block (uint8_t op_code, Token & openingTkn, bool & isClosedByCurly)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;
	uint32_t startFilePos;
	uint32_t length_pos;

	std::shared_ptr <Token> currTkn = tknStream.front();

	isClosedByCurly = false;

	if ((op_code == IF_SCOPE_OPCODE || op_code == ELSE_IF_SCOPE_OPCODE) && currTkn->tkn_type != SPR8R_TKN && currTkn->_string != L"(")	{
		isFailed = true;
		userMessages->logMsg (USER_ERROR, L"Expected \"(\" after [if] or [else if] but instead got " + currTkn->descr_sans_line_num_col()
			, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());

	} else {
		// Write out the if block structure to interpreted file

		startFilePos = interpretedFileWriter.getWriteFilePos();
		length_pos = interpretedFileWriter.writeFlexLenOpCode (op_code);

		if (0 == length_pos)
			isFailed = true;
	}

	if (!isFailed && (op_code == IF_SCOPE_OPCODE || op_code == ELSE_IF_SCOPE_OPCODE))	{
		// Resolve the conditional expression and write it out
		bool isStopFail;
		if (OK != handleExpression(isStopFail))
			// TODO: How to determine if we should go on if return from handleExpression != OK?
			isFailed = true;
	}

	if (!isFailed && !tknStream.empty())	{
		// If next Token isn't a scope opening [{], then we need to handle a single statement ONLY.
		std::shared_ptr <Token> checkCurlyTkn = tknStream.front();

		if (checkCurlyTkn->tkn_type == SPR8R_TKN && checkCurlyTkn->_string == L"{")	{
			tknStream.erase(tknStream.begin());
			isClosedByCurly = true;
			ret_code = scopedNameSpace->openNewScope(op_code, openingTkn, startFilePos, 0);

		} else {
			// No opening [{]. Handle single expression
 			// TODO: Can a single expression start with an [(]? If it can, must the expression end with a [;]?
			bool isStopFail;
			if (OK == handleExpression(isStopFail))	{
				ret_code = interpretedFileWriter.writeObjectLen (startFilePos);
			}
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * Found a data type USER_WORD that indicates the beginning of a possibly plural
 * variable declaration.
 * ***************************************************************************/
int GeneralParser::parseVarDeclaration (std::wstring dataTypeStr, std::pair<TokenTypeEnum, uint8_t> tknType_opCode, bool & isDeclarationEnded)	{
	int ret_code = GENERAL_FAILURE;
	bool isStopFail = false;
	isDeclarationEnded = false;
	Token exprCloserTkn;

	std::wstring assignOpr8r = usrSrcTerms.getSrcOpr8rStrFor(ASSIGNMENT_OPR8R_OPCODE);

	uint32_t startFilePos = interpretedFileWriter.getWriteFilePos();
	uint32_t length_pos = interpretedFileWriter.writeFlexLenOpCode (VARIABLES_DECLARATION_OPCODE);

	if (0 != length_pos)	{
		// Save off the position where the expression's total length is stored and
		// write 0s to it. It will get filled in later after the remainder of the
		// declaration has been completed.

		// [op_code][total_length][datatype op_code][[string var_name][init_expression]]+
		//                        ^
		if (tknType_opCode.second == INVALID_OPCODE)	{
			userMessages->logMsg (INTERNAL_ERROR, L"Data type [" + dataTypeStr + L"] -> INVALID_OPCODE!", thisSrcFile, __LINE__, 0);
			isStopFail = true;

		} else if (OK != interpretedFileWriter.writeRawUnsigned (tknType_opCode.second, NUM_BITS_IN_BYTE))	{
			userMessages->logMsg (INTERNAL_ERROR, L"Failed writing data type op_code to interpreted file.", thisSrcFile, __LINE__, 0);
			isStopFail = true;

		} else	{
			varDeclarationState parserState = GET_VAR_NAME;

			std::shared_ptr <Token> currTkn;
			Token currVarNameTkn;
			std::wstring lookUpMsg;

			while (!isStopFail && !isDeclarationEnded)	{
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
						if (currTkn->tkn_type != USER_WORD_TKN || currTkn->_string.empty())	{
							userMessages->logMsg (USER_ERROR, L"Expected a USER_WORD for a variable name, but got " + currTkn->descr_sans_line_num_col()
									, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
							// TODO: Is it possible to recover from this and keep compiling?
							isStopFail = true;

						} else if (OK != interpretedFileWriter.writeString (VAR_NAME_OPCODE, currTkn->_string))	{
							// [op_code][total_length][datatype op_code][[string var_name][init_expression]]+
							//                                            ^ Written out
							userMessages->logMsg (INTERNAL_ERROR
									, L"INTERNAL ERROR: Failed writing out variable name to interpreted file with " + currTkn->descr_sans_line_num_col()
									, thisSrcFile, __LINE__, 0);
							isStopFail = true;

						} else if (OK == scopedNameSpace->findVar(currTkn->_string, 1, scratchTkn, READ_ONLY, lookUpMsg))	{
								userMessages->logMsg (USER_ERROR, L"Variable " + currTkn->_string + L" already exists at current scope."
										, thisSrcFile, currTkn->get_line_number(), currTkn->get_column_pos());
								// TODO: This could be an opportunity to continue compiling
								if (isProgressBlocked())	{
									isStopFail = true;

								} else	{
									currVarNameTkn = *currTkn;
									parserState = CHECK_FOR_INIT_EXPR;
								}

						} else	{
							// Put an uninitialized variable name & Token in the NameSpace
							Token starterTkn (tknType_opCode.first, L"");
							if (OK != scopedNameSpace->insertNewVarAtCurrScope(currTkn->_string, starterTkn))	{
								userMessages->logMsg (INTERNAL_ERROR, L"Failed to insert " + currTkn->_string + L" into NameSpace AFTER existence check!"
										, thisSrcFile, __LINE__, 0);

								isStopFail = true;

							} else	{
								currVarNameTkn = *currTkn;
								parserState = CHECK_FOR_INIT_EXPR;
							}
						}

					} else if (parserState == CHECK_FOR_INIT_EXPR)	{
						// bool isBobYerUncle, isFailed = false, isDeclarationEnded = false, isSueYerAunt;
						//                   ^          ^                           ^                    ^
 						if (currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L",")	{
							parserState = GET_VAR_NAME;
						
						} else if (currTkn->tkn_type == SRC_OPR8R_TKN && currTkn->_string == assignOpr8r)	{
							parserState = PARSE_INIT_EXPR;
						
						} else if (currTkn->tkn_type == SRC_OPR8R_TKN && currTkn->_string == usrSrcTerms.get_statement_ender())	{
							isDeclarationEnded = true;
						
						} else	{
							userMessages->logMsg (USER_ERROR
									, L"Expected a [,] or [" + assignOpr8r + L"] or [" + usrSrcTerms.get_statement_ender() + L"] but got " + currTkn->descr_sans_line_num_col()
									, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
							// TODO: What to look for to keep compiling? Do we look for the next , or USER_WORD, [=] or [;]?
							isStopFail = true;
						}

					} else if (parserState == PARSE_INIT_EXPR)	{
						if (OK != resolveVarInitExpr (currVarNameTkn, *currTkn, exprCloserTkn, isDeclarationEnded))	{
							if (isProgressBlocked())	{
								isStopFail = true;
							} else if (exprCloserTkn.tkn_type == START_UNDEF_TKN)	{
								// Our closer wasn't found....so look for either a [,] or a [;]
								if (OK != chompUntil_infoMsgAfter (ender_comma_list, exprCloserTkn))
									isStopFail = true;
							}
						}

						if (!isStopFail)	{
							if (exprCloserTkn.tkn_type == SPR8R_TKN && exprCloserTkn._string == L",")
								parserState = GET_VAR_NAME;
							else if (exprCloserTkn.tkn_type == SRC_OPR8R_TKN && exprCloserTkn._string == usrSrcTerms.get_statement_ender())
								isDeclarationEnded = true;
							else
								// Expression didn't close with a [,]. Check if [,] or [;] needs to be consumed.
								parserState = TIDY_AFTER_EXPR;
						}


					} else if (parserState == TIDY_AFTER_EXPR)	{
						if (currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L",")
							parserState = GET_VAR_NAME;
						else if (currTkn->tkn_type == SRC_OPR8R_TKN && currTkn->_string == usrSrcTerms.get_statement_ender())
							isDeclarationEnded = true;
						else	{
							userMessages->logMsg (USER_ERROR
									, L"Expected a [,] or [" + usrSrcTerms.get_statement_ender() + L"] but got " + currTkn->descr_sans_line_num_col()
									, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
							if (isProgressBlocked())	{
								isStopFail = true;

							} else if (OK != chompUntil_infoMsgAfter (ender_comma_list, exprCloserTkn))	{
								// We found neither a [,] or a [;]
								isStopFail = true;
							}
						}
					}
				}
			}
		}

		if (!isStopFail)
			ret_code = interpretedFileWriter.writeObjectLen (startFilePos);
	}


	return (ret_code);

}

/* ****************************************************************************
 * All the logic to parse & compile an expression and write it out to the
 * interpreted file is in this proc.
 * TODO: Figure out if I can use this proc for variable declarations also.
 * ***************************************************************************/
int GeneralParser::handleExpression (bool & isStopFail)	{
	int ret_code = GENERAL_FAILURE;

	if (tknStream.empty())	{
		userMessages->logMsg (INTERNAL_ERROR, L"Token stream unexpectedly empty!", thisSrcFile, __LINE__, 0);
		isStopFail = true;

	} else {
		std::shared_ptr <Token> currTkn = tknStream.front();

		std::shared_ptr<Token> emptyTkn = std::make_shared<Token>();
		std::shared_ptr<ExprTreeNode> exprTree = std::make_shared<ExprTreeNode> (emptyTkn);
		// TODO: indicate what closed the current expression
		Token exprEnder;
		Token tmpTkn;
		std::vector<Token> flatExprTkns;
		bool isExprClosed;
		int makeTreeRetCode = exprParser.makeExprTree (tknStream, exprTree, exprEnder, END_COMMA_NOT_EXPECTED, isExprClosed, false);

		if (OK != makeTreeRetCode && isProgressBlocked())	{
			isStopFail = true;

		} else if (OK != makeTreeRetCode && isExprClosed)	{
			// Error encountered, but expression was closed|completed.  Keep compiling to get more feedback for user

		} else if (OK != makeTreeRetCode && !isExprClosed)	{
			// [;] should close out the expression.  Provide INFO msg to user, try to find the next [;] and proceed from there
			// TODO: Check for some upper limit of user errors to see if we should bail.
			// Search for a [;] and try compiling again from that point on
			if (OK != chompUntil_infoMsgAfter (ender_list, tmpTkn))
				isStopFail = true;

		} else if (OK != interpretedFileWriter.flattenExprTree(exprTree, flatExprTkns, userSrcFileName))	{
			// (3 + 4) -> [3][4][+]
			isStopFail = true;
		
		} else if (OK != interpretedFileWriter.writeFlatExprToFile(flatExprTkns, logLevel >= ILLUSTRATIVE))	{
			// Write out to interpreted file BEFORE we destructively resolve the flat stream of Tokens that make up the expression
			isStopFail = true;

		} else if (OK != interpreter.resolveFlatExpr(flatExprTkns))	{
			isStopFail = true;

		} else	{
			// flattenedExpr should have 1 Token left - the result of the expression
			if (flatExprTkns.size() != 1)	{
				userMessages->logMsg(INTERNAL_ERROR, L"Failed to resolve expression starting with " + currTkn->descr_sans_line_num_col()
						, thisSrcFile, __LINE__, 0);
				isStopFail = true;
			
			} else {
				ret_code = OK;
			}
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * Found the beginning of an initialization expression. Resolve the expression
 * and update the variable that was placed in the NameSpace earlier.
 * ***************************************************************************/
int GeneralParser::resolveVarInitExpr (Token & varTkn, Token currTkn, Token & closerTkn, bool & isDeclarationEnded)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;
	bool isExprClosed = false;

	// TODO: Add option to build Token List as we go and return it
	std::shared_ptr<Token> emptyTkn = std::make_shared<Token>();
	std::shared_ptr<ExprTreeNode> exprTree = std::make_shared<ExprTreeNode> (emptyTkn);
	// TODO: indicate what closed the current expression
	Token exprEnder;
	std::vector<Token> flatExprTkns;
	closerTkn.resetToken();

	int makeTreeRetCode = exprParser.makeExprTree (tknStream, exprTree, exprEnder, END_COMMA_IS_EXPECTED, isExprClosed, true);
	closerTkn = exprEnder;
	std::wstring lookUpMsg;

	if (OK != makeTreeRetCode)	{
		isFailed = true;

	} else if (!(closerTkn.tkn_type == SPR8R_TKN && closerTkn._string == L",") 
		&& !(closerTkn.tkn_type == SRC_OPR8R_TKN && closerTkn._string == usrSrcTerms.get_statement_ender()))	{
			isFailed = true;
			userMessages->logMsg (INTERNAL_ERROR
					, L"Expected expression to close on [,] or " + usrSrcTerms.get_statement_ender() + L" but got " + closerTkn.descr_sans_line_num_col(), thisSrcFile, __LINE__, 0);

	} else if (OK != interpretedFileWriter.flattenExprTree(exprTree, flatExprTkns, userSrcFileName))	{
		// (3 + 4) -> [3][4][+]
		isFailed = true;
	
	} else if (OK != interpretedFileWriter.writeFlatExprToFile(flatExprTkns, false))	{
		// Write out the expression BEFORE we destructively resolve it
		isFailed = true;

	} else if (OK != interpreter.resolveFlatExpr(flatExprTkns))	{
		isFailed = true;

	} else if (!isFailed)	{
		// flattenedExpr should have 1 Token left - the result of the expression
		if (flatExprTkns.size() != 1)	{
			userMessages->logMsg (INTERNAL_ERROR, L"Failed to resolve variable initialization expression for " + varTkn.descr_sans_line_num_col()
					, thisSrcFile, __LINE__, 0);
			isFailed = true;

		} else if (OK != scopedNameSpace->findVar(varTkn._string, 0, flatExprTkns[0], COMMIT_WRITE, lookUpMsg))	{
			// Don't limit search to current scope
			userMessages->logMsg (INTERNAL_ERROR
					, L"Could not find " + varTkn._string + L" after parsing initialization expression in " + userSrcFileName
					+ L" on|near " + currTkn.descr_line_num_col(), thisSrcFile, __LINE__, 0);
			isFailed = true;
		}
	}

	if (!isFailed)	{
		ret_code = OK;
	}

	return (ret_code);
}