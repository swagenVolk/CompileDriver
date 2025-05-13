/*
 * GeneralParser.cpp
 *
 * High level "executive" that parses through a user's source file and looks for reserved words, variable declarations
 * and the start of expressions and then calls the appropriate procedure to handle the task.
 *
 *  Created on: Nov 13, 2024
 *      Author: Mike Volk
 */

#include "GeneralParser.h"
#include "ExpressionParser.h"
#include "InfoWarnError.h"
#include "InterpretedFileWriter.h"
#include "OpCodes.h"
#include "Token.h"
#include "StackOfScopes.h"
#include "common.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

using namespace std;

GeneralParser::GeneralParser(TokenPtrVector & inTknStream, std::wstring userSrcFileName, CompileExecTerms & inUsrSrcTerms
		, std::shared_ptr<UserMessages> userMessages, std::string object_file_name, std::shared_ptr<StackOfScopes> inVarScopeStack
		, logLvlEnum logLvl)
	: interpretedFileWriter (object_file_name, inUsrSrcTerms, userMessages)
	, interpreter (inUsrSrcTerms, inVarScopeStack, userSrcFileName, userMessages, logLvl)
	, exprParser (inUsrSrcTerms, inVarScopeStack, userSrcFileName, userMessages, logLvl)

{
	tkn_stream = inTknStream;
	this->userSrcFileName = userSrcFileName;
	usrSrcTerms = inUsrSrcTerms;
	this->userMessages = userMessages;
	scopedNameSpace = inVarScopeStack;
	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");
	userErrorLimit = 30;
	logLevel = logLvl;

	ender_and_comma_list.push_back (usrSrcTerms.get_statement_ender());
	ender_and_comma_list.push_back (L",");

	ender_list.push_back (usrSrcTerms.get_statement_ender());

  failed_on_src_line = 0;

}

GeneralParser::~GeneralParser() {
	// TODO: Anything to add here?ScopeWindow
	scopedNameSpace.reset();
	ender_and_comma_list.clear();
	ender_list.clear();

  if (failed_on_src_line > 0 && !userMessages->isExistsInternalError(thisSrcFile, failed_on_src_line))	{
		// Dump out a debugging hint
		std::wcout << L"FAILURE on " << thisSrcFile << L":" << failed_on_src_line << std::endl;
	}

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

		if (!tkn_stream.empty())	{
			std::shared_ptr <Token> currTkn = tkn_stream.front();
			tkn_stream.erase(tkn_stream.begin());

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

  if (tkn_stream.empty())	{
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
	bool isEOF = false;
	bool isExprClosed = false;
	bool isVarDecClosed = false;
	Token tmpTkn;
	uint8_t prevScopeObject = INVALID_OPCODE;
  bool is_expr_static;

  if (tkn_stream.empty())	{
  	userMessages->logMsg (INTERNAL_ERROR, L"Token stream is unexpectedly empty!", thisSrcFile, __LINE__, 0);
  	SET_FAILED_ON_SRC_LINE;
    

  } else	{
  	std::pair<TokenTypeEnum, uint8_t> tknTypeEnum_opCode;
		std::wstring lookUpMsg;
		bool isNewScopened;

		while (!failed_on_src_line && !isEOF)	{

			if (!tkn_stream.empty())	{
				std::shared_ptr <Token> currTkn = tkn_stream.front();
				tkn_stream.erase(tkn_stream.begin());

				std::pair<TokenTypeEnum, uint8_t> enum_opCode = usrSrcTerms.getDataType_tknEnum_opCode (currTkn->_string);
				std::shared_ptr<Token> emptyTkn = std::make_shared<Token>();

				if (currTkn->tkn_type == END_OF_STREAM_TKN)	{
					isEOF = true;

				} else if (currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L"}")	{
					// Indicates we should close the current scope
					closeScopeErr closeErr;

          if (OK != validate_closed_for_loop())  {
            SET_FAILED_ON_SRC_LINE;
          
          } else if (OK != scopedNameSpace->srcCloseTopScope(interpretedFileWriter, prevScopeObject, closeErr))	{
						SET_FAILED_ON_SRC_LINE;

						if (closeErr == ONLY_ROOT_SCOPE_OPEN)
							userMessages->logMsg(USER_ERROR, L"Failure closing scope; possibly unmatched " + currTkn->descr_sans_line_num_col(), userSrcFileName
							, currTkn->get_line_number(), currTkn->get_column_pos());

						else if (closeErr == SCOPE_CLOSE_UKNOWN_ERROR || NO_SCOPES_OPEN)
							userMessages->logMsg (INTERNAL_ERROR, L"Failure closing scope with: " + currTkn->descr_line_num_col(), thisSrcFile, __LINE__, 0);
					}

				} else if (currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L"{")	{
					// Indicates we should OPEN a new but untethered scope.  Kind of unusual, but legal
					if (OK != openFloatyScope(*currTkn))
						SET_FAILED_ON_SRC_LINE;

				} else if (enum_opCode.second != INVALID_OPCODE)	{
					// This Token is a known data type; handle variable or fxn declaration
          int numVars, numInitExpr;
					if (OK != parseVarDeclaration (currTkn->_string, enum_opCode, isVarDecClosed, numVars, numInitExpr))	{
						if (isProgressBlocked())	{
							SET_FAILED_ON_SRC_LINE;

						} else if (!isVarDecClosed)	{
							// Search for a [;] and try compiling again from that point on
							if (OK != chompUntil_infoMsgAfter (ender_list, tmpTkn))
								SET_FAILED_ON_SRC_LINE;
						}
					} else {
						prevScopeObject = VARIABLES_DECLARATION_OPCODE;
					}
				} else if (currTkn->tkn_type == USER_WORD_TKN 
						&& OK != scopedNameSpace->findVar(currTkn->_string, 0, scratchTkn, READ_ONLY, lookUpMsg))	{
			  		userMessages->logMsg (USER_ERROR, L"Unrecognized USER_WORD: " + currTkn->descr_sans_line_num_col()
							, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());

						if (isProgressBlocked ())
							SET_FAILED_ON_SRC_LINE;
						else
						 	prevScopeObject = USER_VAR_OPCODE;

				} else if (currTkn->tkn_type == RESERVED_WORD_TKN && currTkn->_string == L"if")	{
					// Handle [if] block
					if (OK != compile_if_type_block(IF_SCOPE_OPCODE, *currTkn, isNewScopened))
			  		SET_FAILED_ON_SRC_LINE;
					else
						prevScopeObject = IF_SCOPE_OPCODE;

				} else if (currTkn->tkn_type == RESERVED_WORD_TKN && currTkn->_string == L"else")	{
					// Handle [else if]|[else] block
					if (!tkn_stream.empty())	{
						std::shared_ptr <Token> checkForIf = tkn_stream.front();

						if (checkForIf->tkn_type == RESERVED_WORD_TKN && checkForIf->_string == L"if")	{
							tkn_stream.erase(tkn_stream.begin());
							if (prevScopeObject != IF_SCOPE_OPCODE && prevScopeObject != ELSE_IF_SCOPE_OPCODE)	{
								userMessages->logMsg (USER_ERROR, L"Expected [if] or [else if] block before " + currTkn->descr_sans_line_num_col()
									, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
								SET_FAILED_ON_SRC_LINE;
							
							} else if (OK != compile_if_type_block(ELSE_IF_SCOPE_OPCODE, *checkForIf, isNewScopened))	{
								SET_FAILED_ON_SRC_LINE;
							
							} else	{
								prevScopeObject = ELSE_IF_SCOPE_OPCODE;
							}
						
						} else {
							if (prevScopeObject != IF_SCOPE_OPCODE && prevScopeObject != ELSE_IF_SCOPE_OPCODE)	{
								userMessages->logMsg (USER_ERROR, L"Expected [if] or [else if] block before " + currTkn->descr_sans_line_num_col()
									, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
								SET_FAILED_ON_SRC_LINE;
							} else if (OK != compile_if_type_block(ELSE_SCOPE_OPCODE, *currTkn, isNewScopened ))	{
								SET_FAILED_ON_SRC_LINE;
							
							} else	{
								// Invalidate whether new scope opened or else block contains a single statement
								prevScopeObject = INVALID_OPCODE;
							}
						}
					}

				} else if (currTkn->tkn_type == RESERVED_WORD_TKN && currTkn->_string == L"for")	{
			  	if (OK != compile_for_loop_control(*currTkn))
					  SET_FAILED_ON_SRC_LINE;

				} else if (currTkn->tkn_type == RESERVED_WORD_TKN && currTkn->_string == L"while")	{
			  	if (OK != compile_while_loop_control(*currTkn))
					  SET_FAILED_ON_SRC_LINE;

				} else if (currTkn->tkn_type == RESERVED_WORD_TKN && currTkn->_string == L"break")	{
          if (OK != compile_break (*currTkn) && isProgressBlocked ())
            SET_FAILED_ON_SRC_LINE;

        } else if (currTkn->tkn_type == SYSTEM_CALL_TKN)  {
          if (OK != compile_lone_system_call (*currTkn))
            SET_FAILED_ON_SRC_LINE;

        } else if (currTkn->tkn_type != INTERNAL_USE_TKN) {
          // Put the current Token back; exprParser will need it!
					tkn_stream.insert(tkn_stream.begin(), currTkn);
          bool isStopFail;
					
          if (OK != handleExpression(isStopFail, is_expr_static, ENDS_IN_STATEMENT_ENDER))
            SET_FAILED_ON_SRC_LINE;
          else
					  prevScopeObject = EXPRESSION_OPCODE;

        } else	{
					userMessages->logMsg (USER_ERROR, L"Unexpected Token: " + currTkn->descr_sans_line_num_col()
						, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
			  	SET_FAILED_ON_SRC_LINE;
				}
			} else	{
				isEOF = true;
			}
		}
  }

  if (!failed_on_src_line && isEOF)
  	ret_code = OK;

	return (ret_code);

}

/* ****************************************************************************
 * Encountered a [break] statement. Check that it's completed by a [;] and 
 * contained within a [for] loop, or a while loop if supported.
 * ***************************************************************************/
 int GeneralParser::compile_break (Token break_tkn) {
  int ret_code = GENERAL_FAILURE;

  if (tkn_stream.empty())	{
  	userMessages->logMsg (USER_ERROR, L"Expected Token after [break] but stream is empty!", userSrcFileName, break_tkn.get_line_number(), break_tkn.get_column_pos());

  } else {
    std::shared_ptr <Token> currTkn = tkn_stream.front();

    if (currTkn->tkn_type != SRC_OPR8R_TKN || currTkn->_string != usrSrcTerms.get_statement_ender())  {
      userMessages->logMsg (USER_ERROR, L"Expected " + usrSrcTerms.get_statement_ender() + L" but instead got " + currTkn->descr_sans_line_num_col()
        , userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());

    } else {
      tkn_stream.erase(tkn_stream.begin());
      uint32_t loop_boundary_end_pos;

      if (!scopedNameSpace->isInsideLoop(loop_boundary_end_pos, true))  {
        userMessages->logMsg (USER_ERROR, L"break statement only valid when encapsulated in a loop: " + break_tkn.descr_sans_line_num_col()
          , userSrcFileName, break_tkn.get_line_number(), break_tkn.get_column_pos());

      } else {
        ret_code = interpretedFileWriter.writeRawUnsigned(BREAK_OPR8R_OPCODE, NUM_BITS_IN_BYTE);

      }
    }
  }

  return ret_code;
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

	std::shared_ptr <Token> currTkn = tkn_stream.front();

	isClosedByCurly = false;
  bool is_expr_static;

	if ((op_code == IF_SCOPE_OPCODE || op_code == ELSE_IF_SCOPE_OPCODE) && currTkn->tkn_type != SPR8R_TKN && currTkn->_string != L"(")	{
		SET_FAILED_ON_SRC_LINE;
		userMessages->logMsg (USER_ERROR, L"Expected \"(\" after [if] or [else if] but instead got " + currTkn->descr_sans_line_num_col()
			, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());

	} else {
		// Write out the if block structure to interpreted file

		startFilePos = interpretedFileWriter.getWriteFilePos();
		length_pos = interpretedFileWriter.writeFlexLenOpCode (op_code);

		if (0 == length_pos)
			SET_FAILED_ON_SRC_LINE;
	}

	if (!isFailed && (op_code == IF_SCOPE_OPCODE || op_code == ELSE_IF_SCOPE_OPCODE))	{
		// Resolve the conditional expression and write it out
		bool isStopFail;
		if (OK != handleExpression(isStopFail, is_expr_static, ENDS_IN_PARENTHESES))
			// TODO: How to determine if we should go on if return from handleExpression != OK?
			SET_FAILED_ON_SRC_LINE;
	}

	if (!isFailed && !tkn_stream.empty())	{
		// If next Token isn't a scope opening [{], then we need to handle a single statement ONLY.
		std::shared_ptr <Token> checkCurlyTkn = tkn_stream.front();

		if (checkCurlyTkn->tkn_type == SPR8R_TKN && checkCurlyTkn->_string == L"{")	{
			tkn_stream.erase(tkn_stream.begin());
			isClosedByCurly = true;
			ret_code = scopedNameSpace->openNewScope(op_code, openingTkn, startFilePos, 0);

		} else {
			// No opening [{]. Handle single expression
 			// TODO: Can a single expression start with an [(]? If it can, must the expression end with a [;]?
			bool isStopFail;

      if (tkn_stream.empty())  {
        userMessages->logMsg(USER_ERROR, L"Expected more Tokens after " + openingTkn.descr_sans_line_num_col(), userSrcFileName
          , openingTkn.get_line_number(), openingTkn.get_column_pos());
      
      } else {
        std::shared_ptr<Token> check_for_break_tkn = tkn_stream.front();
        if (check_for_break_tkn->tkn_type == RESERVED_WORD_TKN && check_for_break_tkn->_string == L"break") {
          tkn_stream.erase(tkn_stream.begin());
          if (OK == compile_break(*check_for_break_tkn))
            ret_code = interpretedFileWriter.writeObjectLen (startFilePos);
        }

        // TODO: We can have a [break] here.  Should it be handled here outside of handleExpression, or in?
        else if (OK == handleExpression(isStopFail, is_expr_static, ENDS_IN_STATEMENT_ENDER))	{
          ret_code = interpretedFileWriter.writeObjectLen (startFilePos);
        }
      }
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * Found a data type USER_WORD that indicates the beginning of a possibly plural
 * variable declaration.
 * ***************************************************************************/
int GeneralParser::parseVarDeclaration (std::wstring dataTypeStr, std::pair<TokenTypeEnum, uint8_t> tknType_opCode, bool & isDeclarationEnded
  , int & numVarsAdded, int & numInitExpressions)	{

	int ret_code = GENERAL_FAILURE;
	bool isStopFail = false;
	isDeclarationEnded = false;
	Token exprCloserTkn;

  numVarsAdded = 0;
  numInitExpressions = 0;

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
				if (!tkn_stream.empty())	{
					// Grab next Token and remove from stream without destroying
					if (parserState != PARSE_INIT_EXPR)	{
						// uint32 numFruits = 3 + 4, numVeggies = (3 * (1 + 2)), numPizzas = (4 + (2 * 3));
						//                  ^ Consumed on prev loop; consume 3 in PARSE_EXPRESSION logic, not HERE
						currTkn = tkn_stream.front();
						tkn_stream.erase(tkn_stream.begin());
					}

					if (parserState == GET_VAR_NAME)	{
						// bool isBobYerUncle, isFailed = false, isDeclarationEnded = false;
						//      ^              ^                 ^
						if (currTkn->tkn_type != USER_WORD_TKN || currTkn->_string.empty())	{
							userMessages->logMsg (USER_ERROR, L"Expected a USER_WORD for a variable name, but got " + currTkn->descr_sans_line_num_col()
									, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
							// TODO: Is it possible to recover from this and keep compiling?
							isStopFail = true;

						} else if (OK != interpretedFileWriter.write_user_var(currTkn->_string, false))	{
							// [op_code][total_length][datatype op_code][[string var_name][init_expression]]+
							//                                            ^ Written out
							userMessages->logMsg (INTERNAL_ERROR
									, L"INTERNAL ERROR: Failed writing out variable name to interpreted file with " + currTkn->descr_sans_line_num_col()
									, thisSrcFile, __LINE__, 0);
							isStopFail = true;

						} else if (OK == scopedNameSpace->findVar(currTkn->_string, 1, scratchTkn, READ_ONLY, lookUpMsg))	{
								userMessages->logMsg (USER_ERROR, L"Variable " + currTkn->_string + L" already exists at current scope."
										, thisSrcFile, currTkn->get_line_number(), currTkn->get_column_pos());
								// This could be an opportunity to continue compiling
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
                numVarsAdded++;
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
								if (OK != chompUntil_infoMsgAfter (ender_and_comma_list, exprCloserTkn))
									isStopFail = true;
							}
						} else {
              numInitExpressions++;
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

							} else if (OK != chompUntil_infoMsgAfter (ender_and_comma_list, exprCloserTkn))	{
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
 * ***************************************************************************/
int GeneralParser::handleExpression (bool & isStopFail, bool & is_expr_static, expr_ender_type expr_ended_by)	{
	int ret_code = GENERAL_FAILURE;

	if (tkn_stream.empty())	{
		userMessages->logMsg (INTERNAL_ERROR, L"Token stream unexpectedly empty!", thisSrcFile, __LINE__, 0);
		isStopFail = true;

	} else {
		std::shared_ptr <Token> currTkn = tkn_stream.front();

		std::shared_ptr<Token> emptyTkn = std::make_shared<Token>();
		std::shared_ptr<ExprTreeNode> exprTree = std::make_shared<ExprTreeNode> (emptyTkn);
    int expected_ret_tkn_cnt;
    Token exprEnder;
		Token tmpTkn;
		std::vector<Token> flatExprTkns;
		bool isExprClosed;
		int makeTreeRetCode = exprParser.makeExprTree (tkn_stream, exprTree, exprEnder, expr_ended_by, isExprClosed, false, is_expr_static);

		if (OK != makeTreeRetCode && isProgressBlocked())	{
			isStopFail = true;

		} else if (OK != makeTreeRetCode && isExprClosed)	{
			// Error encountered, but expression was closed|completed.  Keep compiling to get more feedback for user

		} else if (OK != makeTreeRetCode && !isExprClosed)	{
			// [;] should close out the expression.  Provide INFO msg to user, try to find the next [;] and proceed from there
			// Search for a [;] and try compiling again from that point on
			if (OK != chompUntil_infoMsgAfter (ender_list, tmpTkn))
				isStopFail = true;

		} else if (OK != usrSrcTerms.flattenExprTree(exprTree, flatExprTkns))	{
			// (3 + 4) -> [3][4][+]
			isStopFail = true;
		
		} else if (OK != interpretedFileWriter.writeFlatExprToFile(flatExprTkns, logLevel >= ILLUSTRATIVE))	{
			// Write out to interpreted file BEFORE we destructively resolve the flat stream of Tokens that make up the expression
			isStopFail = true;

		} else if (OK != interpreter.resolveFlatExpr(flatExprTkns, expected_ret_tkn_cnt))	{
			isStopFail = true;

		} else	{
			// flattenedExpr should have 1 Token left - the result of the expression
			if (flatExprTkns.size() != expected_ret_tkn_cnt)	{
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
	bool isExprClosed = false;

  std::shared_ptr<Token> emptyTkn = std::make_shared<Token>();
	std::shared_ptr<ExprTreeNode> exprTree = std::make_shared<ExprTreeNode> (emptyTkn);

  Token exprEnder;
	std::vector<Token> flatExprTkns;
	closerTkn.resetToken();
  bool is_expr_static;
  int expected_ret_tkn_cnt;

	int makeTreeRetCode = exprParser.makeExprTree (tkn_stream, exprTree, exprEnder, ENDS_IN_COMMA, isExprClosed, true, is_expr_static);
	closerTkn = exprEnder;
	std::wstring lookUpMsg;

	if (OK != makeTreeRetCode)	{
		SET_FAILED_ON_SRC_LINE;

	} else if (!(closerTkn.tkn_type == SPR8R_TKN && closerTkn._string == L",") 
		&& !(closerTkn.tkn_type == SRC_OPR8R_TKN && closerTkn._string == usrSrcTerms.get_statement_ender()))	{
			SET_FAILED_ON_SRC_LINE;
			userMessages->logMsg (INTERNAL_ERROR
					, L"Expected expression to close on [,] or " + usrSrcTerms.get_statement_ender() + L" but got " + closerTkn.descr_sans_line_num_col(), thisSrcFile, __LINE__, 0);

	} else if (OK != usrSrcTerms.flattenExprTree(exprTree, flatExprTkns))	{
		// (3 + 4) -> [3][4][+]
		SET_FAILED_ON_SRC_LINE;
	
	} else if (OK != interpretedFileWriter.writeFlatExprToFile(flatExprTkns, false))	{
		// Write out the expression BEFORE we destructively resolve it
		SET_FAILED_ON_SRC_LINE;

	} else if (OK != interpreter.resolveFlatExpr(flatExprTkns, expected_ret_tkn_cnt))	{
		SET_FAILED_ON_SRC_LINE;

	} else if (!failed_on_src_line)	{
		// flattenedExpr should have 1 Token left - the result of the expression
		if (flatExprTkns.size() != expected_ret_tkn_cnt)	{
			userMessages->logMsg (INTERNAL_ERROR, L"Failed to resolve variable initialization expression for " + varTkn.descr_sans_line_num_col()
					, thisSrcFile, __LINE__, 0);
      SET_FAILED_ON_SRC_LINE;

		} else if (OK != scopedNameSpace->findVar(varTkn._string, 0, flatExprTkns[0], COMMIT_WRITE, lookUpMsg))	{
			// Don't limit search to current scope
			userMessages->logMsg (INTERNAL_ERROR
					, lookUpMsg + L" in " + userSrcFileName + L" on|near " + currTkn.descr_line_num_col(), thisSrcFile, __LINE__, 0);
      SET_FAILED_ON_SRC_LINE;
		}
	}

	if (!failed_on_src_line)	{
		ret_code = OK;
	}

	return (ret_code);
}

/* ****************************************************************************
 * Encounted [for] reserved word.  Need to handle the for loop control structure
 * inside the parentheses.  
 * for (optional init statement; continue if true condition; optional expression)
 * TODO: 
 * Should I include some kind of limit to for loop iterations? 100000? 1000000? More?
 * After compilation has been completed, check for problems?
 * Check if that var in FOR_INIT_IDX is referenced in FOR_ITER_IDX and otherwise warn?
 * Look for same var across ALL of the 3 defined statements and if not found, warn?
 * If there is no conditional, error if ScopeWindow's loop_break_cnt == 0
 * ***************************************************************************/
 int GeneralParser::compile_for_loop_control (Token & openingTkn) {
  int ret_code = GENERAL_FAILURE;
	uint32_t startFilePos;
	uint32_t length_pos;
  bool is_init_expr_filled = false, is_cond_expr_filled = false, is_last_expr_filled = false;
  bool is_init_expr_static, is_cond_expr_static, is_last_expr_static;
  bool is_expr_static;

  startFilePos = interpretedFileWriter.getWriteFilePos();
  length_pos = interpretedFileWriter.writeFlexLenOpCode (FOR_SCOPE_OPCODE);

  if (0 == length_pos) 
    SET_FAILED_ON_SRC_LINE;
  else if (OK != exprParser.check_for_expected_token(tkn_stream, openingTkn, L"(", true))
    SET_FAILED_ON_SRC_LINE;
  
  if (!failed_on_src_line) {
    if (OK != scopedNameSpace->openNewScope(FOR_SCOPE_OPCODE, openingTkn, startFilePos, 0)) {
      // Open up the scope 1st because we could have some variables to insert when compiling the control block
      SET_FAILED_ON_SRC_LINE;
    
    } else if (OK != compile_for_loop_ctrl_expr(0, is_init_expr_filled, is_init_expr_static)) {
      // Init expression can be empty, but closes with ;
      SET_FAILED_ON_SRC_LINE;
    
    } else if (OK != compile_for_loop_ctrl_expr(1, is_cond_expr_filled, is_cond_expr_static))  {
      // Can conditional expression can be empty? Closes with ;
      SET_FAILED_ON_SRC_LINE;

    } else if (OK != compile_for_loop_ctrl_expr(2, is_last_expr_filled, is_last_expr_static))  {
      // Can be empty; closes with [)]
      SET_FAILED_ON_SRC_LINE;

    } else {
      // Setting referred in check AFTER entire loop compiled 
      scopedNameSpace->set_top_is_exists_for_loop_cond(is_cond_expr_filled);

      if (is_cond_expr_filled && is_cond_expr_static) {
        // TODO: May need to revisit isProgressBlocked() usage because I'm doing it differently here.  Might be OK though
        // USER_ERROR or WARNING?
        userMessages->logMsg(USER_ERROR, L"[for] loop control conditional expression is static and will lead to infinite looping: " + openingTkn.descr_sans_line_num_col()
        , userSrcFileName, openingTkn.get_line_number(), openingTkn.get_column_pos());
      }

      if (is_last_expr_filled && is_last_expr_static) {
        // TODO: May need to revisit isProgressBlocked() usage because I'm doing it differently here.  Might be OK though
        // USER_ERROR or WARNING?
        userMessages->logMsg(USER_ERROR, L"[for] loop control last expression is static and will lead to infinite looping: " + openingTkn.descr_sans_line_num_col()
        , userSrcFileName, openingTkn.get_line_number(), openingTkn.get_column_pos());
      }

      if (!tkn_stream.empty())	{
        std::shared_ptr <Token> currTkn = tkn_stream.front();

        if (currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L"{")  {
          // Hit an opening curly, so expect there to be multiple statements
          // within for loop scope to be handled by compileCurrScope
          tkn_stream.erase(tkn_stream.begin());
          ret_code = OK;
        
        } else {
          // Single expression contained in this for loop
          // TODO: It wouldn't make sense, but is it legal to have a [break] as the single statement in a [for] loop?
          closeScopeErr closeErr;
          uint8_t scopeOpCode = FOR_SCOPE_OPCODE;
          bool isStopFail;

          if (OK != handleExpression(isStopFail, is_expr_static, ENDS_IN_STATEMENT_ENDER))
            SET_FAILED_ON_SRC_LINE;  

          else if (OK != scopedNameSpace->srcCloseTopScope(interpretedFileWriter, scopeOpCode, closeErr))	{
            SET_FAILED_ON_SRC_LINE;  

            if (closeErr == ONLY_ROOT_SCOPE_OPEN)
              userMessages->logMsg(USER_ERROR, L"Failure closing scope; possibly unmatched " + currTkn->descr_sans_line_num_col(), userSrcFileName
              , currTkn->get_line_number(), currTkn->get_column_pos());

            else if (closeErr == SCOPE_CLOSE_UKNOWN_ERROR || NO_SCOPES_OPEN)
              userMessages->logMsg (INTERNAL_ERROR, L"Failure closing scope with: " + currTkn->descr_line_num_col(), thisSrcFile, __LINE__, 0);
          
          } else  {
            ret_code = OK;
          }
        }
      } else {
        SET_FAILED_ON_SRC_LINE;  
      }
    }
  }
  
  return ret_code;
}

/* ****************************************************************************
 * Working on control logic of [for] loop. There are up to 3 statements (?) to
 * deal with.
 * for (optional init statement; continue if true condition; optional expression)
 *      ^ exprIdx == 0           ^ exprIdx == 1              ^ exprIdx == 2
 * TODO:
 * Are variable declarations allowed on exprIdx == 2? Or only a single expression?
 * Is it OK to allow multiple statements inside a variable declaration vs. only
 * 1 if there's a "normal" expression instead?
 * FOR_INIT_IDX: Should I check the # of vars that have been initialized? Min of 1?  
 * ***************************************************************************/
 int GeneralParser::compile_for_loop_ctrl_expr (int exprIdx, bool & is_expr_full, bool & is_expr_static) {
  int ret_code = GENERAL_FAILURE;

  is_expr_full = false;

 	uint32_t startFilePos;
	uint32_t length_pos;
  std::wstring lookUpMsg;
  std::vector<Token> emptyFlatExpr;

  if (tkn_stream.empty())	{
    SET_FAILED_ON_SRC_LINE;

  } else {
    std::shared_ptr <Token> currTkn = tkn_stream.front();
    tkn_stream.erase(tkn_stream.begin());
    std::pair<TokenTypeEnum, uint8_t> enum_opCode = usrSrcTerms.getDataType_tknEnum_opCode (currTkn->_string);

    if (exprIdx == FOR_ITER_IDX && currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L")") {
      // It's an empty expression; we're done
      ret_code = interpretedFileWriter.writeFlatExprToFile(emptyFlatExpr, false);
    
    } else if (currTkn->tkn_type == SRC_OPR8R_TKN && currTkn->_string == usrSrcTerms.get_statement_ender())  {
      // It's an empty expression; we're done
      if (exprIdx < FOR_ITER_IDX)  {
        ret_code = interpretedFileWriter.writeFlatExprToFile(emptyFlatExpr, false);
      
      } else if (tkn_stream.empty())	{
        SET_FAILED_ON_SRC_LINE;
      
      } else {
        // if exprIdx == FOR_ITER_IDX, then we need to consume the [)]
        std::shared_ptr <Token> currTkn = tkn_stream.front();
        tkn_stream.erase(tkn_stream.begin());

        if (currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L")")
          ret_code = OK;
        else
          SET_FAILED_ON_SRC_LINE;
      }
      
    } else if (enum_opCode.second != INVALID_OPCODE && exprIdx < FOR_ITER_IDX)  {
      // Data type indicates variable declaration encountered
      // e.g. int16 idx = 0;
      bool isVarDecClosed = false;
      int numVars, numInitExpr;
      if (OK != parseVarDeclaration (currTkn->_string, enum_opCode, isVarDecClosed, numVars, numInitExpr))  {
        SET_FAILED_ON_SRC_LINE;

      } else if (!isVarDecClosed) {
        SET_FAILED_ON_SRC_LINE;

      } else if (exprIdx == FOR_INIT_IDX) {
        // Multiple variable declaration is allowed for init expression
        is_expr_full = true;
        ret_code = OK;
      
      } else if (exprIdx == FOR_CONDITIONAL_IDX)  {
        // Legal to declare variable and set to something, but only 1 is allowed because a conditional must resolve down to 1 TRUE|FALSE
        if (numVars == 1 && numInitExpr == 1) {
          is_expr_full = true;
          ret_code = OK;
        
        } else {
          SET_FAILED_ON_SRC_LINE;
        }
      }
    } else {
      // Put the current Token back; exprParser will need it!
      tkn_stream.insert(tkn_stream.begin(), currTkn);
      expr_ender_type expr_ended_by = ENDS_IN_COMMA;

      bool isStopFail = false;
      if (exprIdx == FOR_ITER_IDX) {
        // Push an opening [(] up front to match expected closing [)]
        std::shared_ptr<Token> openParenTkn = std::make_shared<Token> (SPR8R_TKN, L"(", L"", 0, 0);
        tkn_stream.insert(tkn_stream.begin(), openParenTkn);
        expr_ended_by = ENDS_IN_PARENTHESES;
      }
      is_expr_full = true;
      ret_code = handleExpression(isStopFail, is_expr_static, expr_ended_by);
      if (ret_code != OK)      
        SET_FAILED_ON_SRC_LINE;
    }
  }

  return ret_code;

}

/* ****************************************************************************
 * About to close up a [for] loop. Do some "reasonableness" checks
 * ***************************************************************************/
 int GeneralParser::validate_closed_for_loop() {
  int ret_code = GENERAL_FAILURE;

  uint8_t scopener_op_code;
  bool is_conditional_exists = false;
  Token for_scopener_tkn;

  if (OK == scopedNameSpace->get_top_opener_opcode(scopener_op_code)) {
    if (scopener_op_code != FOR_SCOPE_OPCODE) {
      ret_code = OK;

    } else {
      // We're closing a for loop, so let's do some checking
      int break_cnt;

      if (OK == scopedNameSpace->get_top_loop_break_cnt(break_cnt)) {
        if (break_cnt > 0)  {
          ret_code = OK;

        } else if (OK != scopedNameSpace->get_top_opener_tkn(for_scopener_tkn)) {
          SET_FAILED_ON_SRC_LINE;
        
        } else if (OK != scopedNameSpace->get_top_is_exists_for_loop_cond(is_conditional_exists)) {
          SET_FAILED_ON_SRC_LINE;

        } else if (!is_conditional_exists)  {
          userMessages->logMsg(USER_ERROR, L"[for] loop with empty conditional must have a [break] statement to avoid an infinite loop: " 
            + for_scopener_tkn.descr_sans_line_num_col(), userSrcFileName, for_scopener_tkn.get_line_number(), for_scopener_tkn.get_column_pos());
        } else {
          ret_code = OK;
        }
      }
    }
  }

  return ret_code;
}

/* ****************************************************************************
 * Encounted [while] reserved word.  Need to handle the for loop control structure
 * inside the parentheses.  
 * while (continue if true condition)
 * TODO: 
 * Should I include some kind of limit to while loop iterations? 100000? 1000000? More?
 * ***************************************************************************/
 int GeneralParser::compile_while_loop_control (Token & openingTkn) {
  int ret_code = GENERAL_FAILURE;
  bool isFailed = false;
	uint32_t startFilePos;
	uint32_t length_pos;
  bool is_expr_static;

  startFilePos = interpretedFileWriter.getWriteFilePos();
  length_pos = interpretedFileWriter.writeFlexLenOpCode (WHILE_SCOPE_OPCODE);

  if (0 == length_pos)  {
    SET_FAILED_ON_SRC_LINE;
  
  } else if (OK != handleExpression(isFailed, is_expr_static, ENDS_IN_PARENTHESES))  {
    SET_FAILED_ON_SRC_LINE;

  } else if (OK != scopedNameSpace->openNewScope(WHILE_SCOPE_OPCODE, openingTkn, startFilePos, 0)) {
    // Open up the scope 1st because we could have some variables to insert when compiling the control block
    SET_FAILED_ON_SRC_LINE;

  } else {

    if (is_expr_static) {
      // TODO: May need to revisit isProgressBlocked() usage because I'm doing it differently here.  Might be OK though
      // USER_ERROR or WARNING?
      userMessages->logMsg(USER_ERROR, L"[while] loop control conditional expression is static, which leads to infinite looping: " + openingTkn.descr_sans_line_num_col()
      , userSrcFileName, openingTkn.get_line_number(), openingTkn.get_column_pos());
    }


    if (!tkn_stream.empty())	{
      std::shared_ptr <Token> currTkn = tkn_stream.front();

      if (currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L"{")  {
        // Hit an opening curly, so expect there to be multiple statements
        // within for loop scope to be handled by compileCurrScope
        tkn_stream.erase(tkn_stream.begin());
        ret_code = OK;
      
      } else {
        // Single expression contained in this for loop
        // TODO: It wouldn't make sense, but is it legal to have a [break] as the single statement in a [for] loop?
        closeScopeErr closeErr;
        uint8_t scopeOpCode = WHILE_SCOPE_OPCODE;
        bool isStopFail;
        bool is_body_expr_static;

        if (OK != handleExpression(isStopFail, is_body_expr_static, ENDS_IN_STATEMENT_ENDER))
          SET_FAILED_ON_SRC_LINE;  

        else if (OK != scopedNameSpace->srcCloseTopScope(interpretedFileWriter, scopeOpCode, closeErr))	{
          SET_FAILED_ON_SRC_LINE;  

          if (closeErr == ONLY_ROOT_SCOPE_OPEN)
            userMessages->logMsg(USER_ERROR, L"Failure closing scope; possibly unmatched " + currTkn->descr_sans_line_num_col(), userSrcFileName
            , currTkn->get_line_number(), currTkn->get_column_pos());

          else if (closeErr == SCOPE_CLOSE_UKNOWN_ERROR || NO_SCOPES_OPEN)
            userMessages->logMsg (INTERNAL_ERROR, L"Failure closing scope with: " + currTkn->descr_line_num_col(), thisSrcFile, __LINE__, 0);
        
        } else  {
          ret_code = OK;
        }
      }
    } else {
      SET_FAILED_ON_SRC_LINE;  
    }
  }
  
  return ret_code;
}

/* ****************************************************************************
 * Encounted a stand-alone system call, so call the right ExpressionParser call
 * to do the heavy lifting and then write the expression out to the interpreted
 * file.
 * ***************************************************************************/
 int GeneralParser::compile_lone_system_call (Token & sys_call_name_tkn) {
  int ret_code = GENERAL_FAILURE;

  std::shared_ptr<Token> sys_call_def_tkn = std::make_shared<Token>(sys_call_name_tkn);
  std::shared_ptr<ExprTreeNode> sys_call_node = std::make_shared<ExprTreeNode> (sys_call_def_tkn);
  std::vector<Token> sys_call_tkn_list;

  if (OK == exprParser.compile_system_call(tkn_stream, sys_call_node)) {
    // TODO:

    if (logLevel >= ILLUSTRATIVE)	{
      std::wcout << L"Compiler's Parse Tree for stand-alone system call" << std::endl;
      exprParser.displayParseTree(sys_call_node, 0);
    }
    if (OK != usrSrcTerms.flatten_system_call(sys_call_node, sys_call_tkn_list))  {
      // Turn this system call into a flat expression
      SET_FAILED_ON_SRC_LINE;
    
    // TODO: Exercise expression to test for data type contention
    } else {
      // Write the Token stream out to the interpreted file
      Token tmp_tkn;
      if (OK != interpretedFileWriter.writeFlatExprToFile (sys_call_tkn_list, false))
        SET_FAILED_ON_SRC_LINE;
      else if (OK != exprParser.check_for_expected_token (tkn_stream, tmp_tkn, L";", true))
        SET_FAILED_ON_SRC_LINE;
      else
        ret_code = OK;
    }
  }

  return ret_code;
 }