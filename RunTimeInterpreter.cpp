/*
 * RunTimeInterpreter.cpp
 *
 *  Created on: Jan 4, 2024
 *      Author: Mike Volk
 *
 * TODO:
 * Scope opened for [if] block?
 *
 * A parse tree was created from the expression in the user's source file, but we need
 * to flatten this tree before writing out to the interpreted file since the file is 
 * essentially 2D while a tree is 3D.  The flattened xpression has been written out in 
 * recursive [OPR8R][LEFT][RIGHT] order, with higher precedence operations located deeper 
 * in the tree.  The OPR8R precedes its operands to enable short-circuiting of the 
 * [&&], [||] and [?] OPR8Rs.  Some example source expressions and their corresponding 
 * flattened Token lists that get written out to the interpreted file are shown below. 
 *
 * seven = three + four;				<-- user's C source expression
 * [=][seven][B+][three][four]	<-- Flattened expression for Interpreter
 * ^ Exec [=] OPR8R when we've got the required 2 operands. Can't do that right away because of the 
 * [B+] (binary +) OPR8R after the [seven] operand, but the [B+] OPR8R can be executed right away 
 * because its requirement for 2 operands is met immediately.
 *
 * [=][seven][B+][three][four]
 *           ^ binary +
 * Gets reduced to 
 * [=][seven][7]
 *           
 * one = 1;
 * [=][one][1]
 *
 * seven * seven + init1++; 
 * init1 == 1 at time of this expression
 * [B+][*][seven][seven][1+][init1]
 *                      ^ postfix increment
 * [B+][49]             [1] <-- postfix increment takes place AFTER grabbing the value of [init1]
 * [50]
 * 
 * seven * seven + ++init2;
 * init2 == 2 at time of this expression
 * [B+][*][seven][seven][+1][init2]
 *                      ^ prefix increment
 * [B+][49]             [3] <-- prefix increment takes place BEFORE grabbing the value of [init2]
 * [52]
 * 
 * one >= two ? 1 : three <= two ? 2 : three == four ? 3 : six > seven ? 4 : six > (two << two) ? 5 : 12345;
 * [?][>=][one][two][1][?][<=][three][two][2][?][==][three][four][3][?][>][six][seven][4][?][>][six][<<][two][two][5][12345]
 * 
 * count == 1 ? "one" : count == 2 ? "two" : "MANY";
 * [?][==][count][1]["one"][?][==][count][2]["two"]["MANY"]
 * Example: count == 1
 * [?][==][count][1]["one"][?][==][count][2]["two"]["MANY"]
 * [?][1]           ["one"][?][==][count][2]["two"]["MANY"]
 *    ^ Conditional resolves TRUE; so take TRUE path & discard FALSE path
 * [?][1]           ["one"][?][==][count][2]["two"]["MANY"]
 *                  ^ TRUE ^ FALSE path that gets discarded.
 * ["one"] <-- expression resolved
 * 
 * Example: count == 2
 * [?][==][count][1]["one"][?][==][count][2]["two"]["MANY"]
 * [?][0]           ["one"][?][==][count][2]["two"]["MANY"]
 *    ^ Conditional resolves FALSE; discard TRUE path
 * [?][==][count][2]["two"]["MANY"]
 * [?][1]           ["two"]["MANY"]
 *    ^ Conditional resolves TRUE; take TRUE path & discard FALSE path
 * ["two"] <-- expression resolved
 *
 * Example: count == 3
 * [?][==][count][1]["one"][?][==][count][2]["two"]["MANY"]
 * [?][0]           ["one"][?][==][count][2]["two"]["MANY"]
 *    ^ Conditional resolves FALSE; discard TRUE path
 * [?][==][count][2]["two"]["MANY"]
 * [?][0]           ["two"]["MANY"]
 *    ^ Conditional resolves FALSE; discard TRUE path
 * ["MANY"] <-- expression resolved
 *
 *
 * 3 * 4 / 3 + 4 << 4;
 * [<<][B+][/][*][3][4][3][4][4]
 *            ^ 1st OPR8R followed by required # of operands
 * [<<][B+][/][12]     [3][4][4]
 * [<<][B+][4]            [4][4]
 * [<<][8]                   [4]
 * [128]
 * 
 * 3 * 4 / 3 + 4 << 4 + 1;
 * [<<][B+][/][*][3][4][3][4][B+][4][1]
 *            ^              ^ These 2 OPR8Rs have their operand requirements met
 * [<<][B+][/][12]     [3][4][5]
 * [<<][B+][4]            [4][5]
 * [<<][8]                   [5]
 * [256]
 * 
 * (one * two >= three || two * three > six || three * four < seven || four / two < one) && (three % two > 1 || (shortCircuitAnd987 = 654));
 * [&&][||][||][||][>=][*][one][two][three][>][*][two][three][six][<][*][three][four][seven][<][/][four][two][one][||][>][%][three][two][1][=][shortCircuitAnd987][654]
 *
 * TODO:
 * Error handling on mismatched data types
 * 
 * DONE:
 * Complete assignment operations
 * Will need to be able to recognize complete, yet contained sub-expressions for the untaken TERNARY
 * branch, expressions as fxn|system calls, and potentially for short-circuiting (early break) on the [||] or [&&] OPR8Rs
 * Align comments below with reality
 *
 * Where will fxn calls go: on the operand or opr8r stack? If fxn calls go on the opr8r stack, then
 * the number of arguments the fxn call expects has to be looked up @ run time, OR the # of expected
 * operands could be put on the operand stack.
 *  */

#include "RunTimeInterpreter.h"
#include "BaseLanguageTerms.h"
#include "CompileExecTerms.h"
#include "ExprTreeNode.h"
#include "FileLineCol.h"
#include "OpCodes.h"
#include "Operator.h"
#include "Token.h"
#include "TokenCompareResult.h"
#include <cassert>
#include <climits>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "InfoWarnError.h"
#include "UserMessages.h"
#include "StackOfScopes.h"
#include "common.h"

/* ****************************************************************************
 * This constructor should never get called
 * ***************************************************************************/
RunTimeInterpreter::RunTimeInterpreter() {
  one_tkn = std::make_shared<Token> (UINT64_TKN, L"1");
  // TODO: Token value not automatically filled in currently
  one_tkn->_unsigned = 1;
	one_tkn->isInitialized = true;
  zero_tkn = std::make_shared<Token> (UINT64_TKN, L"0");
  zero_tkn->_unsigned = 0;
	zero_tkn->isInitialized = true;
	this_src_file = util.getLastSegment(util.stringToWstring(__FILE__), L"/");
	failed_on_src_line = 0;
	log_level = SILENT;
	is_illustrative = false;
  assert(0);
}

/* ****************************************************************************
 * This is the COMPILE_TIME constructor call
 * ***************************************************************************/
RunTimeInterpreter::RunTimeInterpreter(CompileExecTerms & execTerms, std::shared_ptr<StackOfScopes> inVarScopeStack
		, std::wstring userSrcFileName, std::shared_ptr<UserMessages> userMessages, logLvlEnum logLvl) {
  one_tkn = std::make_shared<Token> (UINT64_TKN, L"1");
  // TODO: Token value not automatically filled in currently
  one_tkn->_unsigned = 1;
	one_tkn->isInitialized = true;
  zero_tkn = std::make_shared<Token> (UINT64_TKN, L"0");
  zero_tkn->_unsigned = 0;
	zero_tkn->isInitialized = true;

  this->exec_terms = execTerms;
	this_src_file = util.getLastSegment(util.stringToWstring(__FILE__), L"/");
	scope_name_space = inVarScopeStack;
	this->user_messages = userMessages;
	this->usr_src_file_name = userSrcFileName;
	failed_on_src_line = 0;
	log_level = logLvl;
	is_illustrative = false;
	usage_mode = COMPILE_TIME;
}

/* ****************************************************************************
 * This is the INTERPRETER mode constructor call
 * ***************************************************************************/
RunTimeInterpreter::RunTimeInterpreter(std::string interpretedFileName, std::wstring userSrcFileName
	, std::shared_ptr<StackOfScopes> inVarScope,  std::shared_ptr<UserMessages> userMessages, logLvlEnum logLvl)
		: exec_terms ()
		, file_reader (interpretedFileName, exec_terms)	{
  one_tkn = std::make_shared<Token> (UINT64_TKN, L"1");
  // TODO: Token value not automatically filled in currently
  one_tkn->_unsigned = 1;
	one_tkn->isInitialized = true;
  zero_tkn = std::make_shared<Token> (UINT64_TKN, L"0");
  zero_tkn->_unsigned = 0;
	zero_tkn->isInitialized = true;

	this_src_file = util.getLastSegment(util.stringToWstring(__FILE__), L"/");
	scope_name_space = inVarScope;
	this->user_messages = userMessages;
	this->usr_src_file_name = userSrcFileName;
	failed_on_src_line = 0;
	log_level = logLvl;
	is_illustrative = false;
	usage_mode = INTERPRETER;

}

/* ****************************************************************************
 *
 * ***************************************************************************/
RunTimeInterpreter::~RunTimeInterpreter() {
	one_tkn.reset();
	zero_tkn.reset();

	if (failed_on_src_line > 0 && !user_messages->isExistsInternalError(this_src_file, failed_on_src_line))	{
		// Dump out a debugging hint
		std::wcout << L"FAILURE on " << this_src_file << L":" << failed_on_src_line << std::endl;
	}
}

/* ****************************************************************************
 * Analogous to GeneralParser::rootScopeCompile.  
 * This proc gives us an opportunity to handle implementation specific objects 
 * that only appear at the global scope.  Otherwise, call execCurrScope.
 * ***************************************************************************/
int RunTimeInterpreter::execRootScope()	{
	int ret_code = GENERAL_FAILURE;
	uint8_t	root_scope_op_code;
	uint32_t root_scope_len;
  uint32_t break_scope_end_pos;

	if (usage_mode == INTERPRETER)	{
		file_reader.setPos(0);
		if (OK != file_reader.readNextByte(root_scope_op_code) || root_scope_op_code != ANON_SCOPE_OPCODE)
			user_messages->logMsg(INTERNAL_ERROR, L"Failure reading ROOT scope op_code", this_src_file, failed_on_src_line, 0);
		
		else if (OK != file_reader.readNextDword(root_scope_len))
			user_messages->logMsg(INTERNAL_ERROR, L"Failure reading ROOT scope length", this_src_file, failed_on_src_line, 0);

		else {
			ret_code = execCurrScope (file_reader.getPos(), root_scope_len, break_scope_end_pos);
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * Check if we're in the right mode, and be sure we're not nested inside a loop
 * ***************************************************************************/
 bool RunTimeInterpreter::isOkToIllustrate ()  {
  bool isOK = false;
  uint32_t loop_boundary_end_pos;
  
  if (usage_mode == INTERPRETER && log_level >= ILLUSTRATIVE && !scope_name_space->isInsideLoop(loop_boundary_end_pos, false))
    isOK = true;

  return isOK;
}

/* ****************************************************************************
 * Analogous to GeneralParser::compileCurrScope.  
 * Parse through the user's interpreted file, looking for objects such as:
 * Variable declarations
 * Expressions
 * [if] 
 * [else if]
 * [else] 
 * [while] 
 * [for] 
 * [fxn declaration]
 * [fxn call]
 * ***************************************************************************/
int RunTimeInterpreter::execCurrScope (uint32_t exec_start_pos, uint32_t after_scope_bndry, uint32_t & break_scope_end_pos)	{
	int ret_code = GENERAL_FAILURE;
	uint8_t	op_code;
	uint32_t obj_start_pos;
	uint32_t object_len;
	bool is_done = false;
	std::wstringstream hex_stream;
	std::wstringstream obj_start_pos_str;

	if (usage_mode == INTERPRETER)	{

    // Make sure we're starting off at right position
    file_reader.setPos(exec_start_pos);
    break_scope_end_pos = 0;

    while (!is_done && !failed_on_src_line)	{
			obj_start_pos = file_reader.getPos();

			obj_start_pos_str.str(L"");
			obj_start_pos_str << L"0x" << std::hex << obj_start_pos;

			if (file_reader.isEOF())
				is_done = true;

			else if (obj_start_pos == after_scope_bndry)
				// scopeEndPos > 0 means non-global scope and whole scope processed
				is_done = true;

			else if (OK != file_reader.peekNextByte(op_code))	{
				// TODO: isEOF doesn't seem to work. What's the issue?
				is_done = true;
				if (exec_start_pos > 0)	{
					SET_FAILED_ON_SRC_LINE;
					user_messages->logMsg(INTERNAL_ERROR, L"Failed while peeking next op_code in non-global scope"
						, this_src_file, failed_on_src_line, 0);
				}

			} else if (OK != file_reader.readNextByte (op_code))	{
				SET_FAILED_ON_SRC_LINE;
			
      } else if (op_code == BREAK_OPR8R_OPCODE) {
        scope_name_space->isInsideLoop(break_scope_end_pos, true);
        
        if (0 == break_scope_end_pos) {
          // [break] statement MUST be inside a loop and this one wasn't
          SET_FAILED_ON_SRC_LINE;
        
        } else {
          is_done = true;

        }
      } else if (op_code >= FIRST_VALID_FLEX_LEN_OPCODE && op_code <= LAST_VALID_FLEX_LEN_OPCODE)		{
				if (OK != file_reader.readNextDword (object_len))	{
					SET_FAILED_ON_SRC_LINE;
					hex_stream.str(L"");
					hex_stream << L"0x" << std::hex << op_code;
					std::wstring msg = L"Failed to get length of object (opcode = ";
					msg.append(hex_stream.str());
					msg.append(L") starting at ");
					msg.append(obj_start_pos_str.str());
					user_messages->logMsg(INTERNAL_ERROR, msg, this_src_file, failed_on_src_line, 0);

				} else {
					if (op_code == VARIABLES_DECLARATION_OPCODE)	{
						if (OK != execVarDeclaration (obj_start_pos, object_len))	{
							SET_FAILED_ON_SRC_LINE;
						}

					} else if (op_code == EXPRESSION_OPCODE)	{		
						Token result_tkn;	
						is_illustrative = isOkToIllustrate();

						if (is_illustrative)
							std::wcout << L"// ILLUSTRATIVE MODE: Flattened expression resolved below" << std::endl << std::endl;

						
						if (OK != execExpression (obj_start_pos, result_tkn))	{
							SET_FAILED_ON_SRC_LINE;
						}
						is_illustrative = false;
					
					} else if (op_code == IF_SCOPE_OPCODE)	{	
						if (OK != exec_if_block (obj_start_pos, object_len, after_scope_bndry, break_scope_end_pos))	{
							SET_FAILED_ON_SRC_LINE;
						
            } else if (break_scope_end_pos >= after_scope_bndry)  {
              // [break]ing out of the current scope; no need to retain the info
              // TODO: Wouldn't this be a failure?
              is_done = true;
            }

					} else if (op_code == ELSE_IF_SCOPE_OPCODE)	{						
            SET_FAILED_ON_SRC_LINE;
            user_messages->logMsg(INTERNAL_ERROR, L"Floating [else if] block encountered at " + obj_start_pos_str.str(), this_src_file, failed_on_src_line, 0);

					} else if (op_code == ELSE_SCOPE_OPCODE)	{								
            SET_FAILED_ON_SRC_LINE;
            user_messages->logMsg(INTERNAL_ERROR, L"Floating [else] block encountered at " + obj_start_pos_str.str(), this_src_file, failed_on_src_line, 0);

					} else if (op_code == WHILE_SCOPE_OPCODE)	{								
            if (OK != exec_while_loop (obj_start_pos, object_len, after_scope_bndry, break_scope_end_pos))  {
              SET_FAILED_ON_SRC_LINE;

            } else if (break_scope_end_pos >= after_scope_bndry)  {
              // [break]ing out of the current scope; no need to retain the info
              // TODO: Wouldn't this be a failure?
              is_done = true;
              // TODO: break_scope_end_pos = 0;

            }

					} else if (op_code == FOR_SCOPE_OPCODE)	{									
            if (OK != exec_for_loop (obj_start_pos, object_len, after_scope_bndry, break_scope_end_pos))  {
              SET_FAILED_ON_SRC_LINE;

            } else if (break_scope_end_pos >= after_scope_bndry)  {
              // [break]ing out of the current scope; no need to retain the info
              // TODO: Wouldn't this be a failure?
              is_done = true;
              // TODO: break_scope_end_pos = 0;

            }

					} else if (op_code == ANON_SCOPE_OPCODE)	{								
            SET_FAILED_ON_SRC_LINE;
            user_messages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", this_src_file, failed_on_src_line, 0);

					} else if (op_code == USER_FXN_DECLARATION_OPCODE)	{					
            SET_FAILED_ON_SRC_LINE;
            user_messages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", this_src_file, failed_on_src_line, 0);

					} else if (op_code == USER_FXN_CALL_OPCODE)	{									
            SET_FAILED_ON_SRC_LINE;
            user_messages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", this_src_file, failed_on_src_line, 0);

					} else if (op_code == SYSTEM_CALL_OPCODE)	{							
            SET_FAILED_ON_SRC_LINE;
            // TODO: Will stand-alone system calls be wrapped inside an expression?  Probably.....
            user_messages->logMsg(INTERNAL_ERROR, L"NOT SUPPORTED YET!", this_src_file, failed_on_src_line, 0);

					} else {
						SET_FAILED_ON_SRC_LINE;
						hex_stream.str(L"");
						hex_stream << L"0x" << std::hex << op_code;
						std::wstring msg = L"Unknown opcode [";
						msg.append(hex_stream.str());
						msg.append(L"] found at ");
						msg.append(obj_start_pos_str.str());
						user_messages->logMsg(INTERNAL_ERROR, msg, this_src_file, failed_on_src_line, 0);
					}
				}
			}
		}
	}

	if (is_done && !failed_on_src_line)
		ret_code = OK;

	return (ret_code);
}

/* ****************************************************************************
 * General proc to perform the mechanics of evaluating an expression.
 * NOTE that expressions are found in the [if] and [else if] conditionals.
 * Loop control expressions are found in [while] loop conditionals, 
 * and [for] loop init, conditional and end of loop statements.
 * There are also possible expressions used to initialize variables in 
 * variable declarations, and then there are stand alone expressions, typically
 * assignment statements.
 * ***************************************************************************/
int RunTimeInterpreter::execExpression (uint32_t obj_start_pos, Token & result_tkn)	{
	int ret_code = GENERAL_FAILURE;
	std::wstringstream obj_start_pos_str;
	obj_start_pos_str.str(L"");
	obj_start_pos_str << L"0x" << std::hex << obj_start_pos;
  int expected_ret_tkn_cnt;

	if (OK != file_reader.setPos(obj_start_pos))	{
		// Follow on fxn expects to start at beginning of expression
		SET_FAILED_ON_SRC_LINE;
		user_messages->logMsg(INTERNAL_ERROR
			, L"Failed to reset interpreter file position to " + obj_start_pos_str.str(), this_src_file, failed_on_src_line, 0);

	} else {
		std::vector<Token> expr_tkns;
		if (OK != file_reader.readExprIntoList(expr_tkns))	{
			SET_FAILED_ON_SRC_LINE;
			user_messages->logMsg(INTERNAL_ERROR
				, L"Failed to retrieve expression starting at " + obj_start_pos_str.str(), this_src_file, failed_on_src_line, 0);
		
		}	else if (OK != resolveFlatExpr(expr_tkns, expected_ret_tkn_cnt)) {
				SET_FAILED_ON_SRC_LINE;
				user_messages->logMsg(INTERNAL_ERROR
					, L"Failed to resolve flat expression starting at " + obj_start_pos_str.str(), this_src_file, failed_on_src_line, 0);

     } else if (expr_tkns.size() != expected_ret_tkn_cnt)	{
			// TODO: Should not have returned OK!
			// flattenedExpr should have 1 Token left - the result of the expression
			SET_FAILED_ON_SRC_LINE;
			std::wstring dev_msg = L"Failed to resolve at file position " + obj_start_pos_str.str();
			user_messages->logMsg (INTERNAL_ERROR, dev_msg, this_src_file, failed_on_src_line, 0);

		} else {
      if (expected_ret_tkn_cnt == 1)
			  result_tkn = expr_tkns[0];
			ret_code = OK;
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * VARIABLES_DECLARATION_OPCODE		0x6F	
 * [op_code][total_length][datatype op_code][[string var_name][init_expression]]+
 * Examples of user source that would result in a run time call to this fxn:
 *  
 * int8 count = 3;
 * uint8 one, two, three, four, five, six, seven;
 * uint8 init1 = 1, init2 = 2;
 * uint16 twenty = four * five, twentySix = twenty + six, fortyTwo = six * seven, fiftySix = seven * seven + seven;
 * uint16 fifty = seven * seven + init1++;
 * uint16 fiftyTwo = seven * seven + ++init2;
 * int8 result = one >= two ? 1 : three <= two ? 2 : three == four ? 3 : six > seven ? 4 : six > (two << two) ? 5 : 12345; 
 * string countDesc = count == 1 ? "one" : count == 2 ? "two" : count == 3 ? "three" : count == 4 ? "four" : "MANY";
 * bool isShouldBeTrue = one <= two ? true : false;
 * bool isShouldBeFalse = fiftySix <= fiftyTwo ? true : false;
 * string MikeWasHere = "Mike was HERE!!!!";
 * ***************************************************************************/
int RunTimeInterpreter::execVarDeclaration (uint32_t obj_start_pos, uint32_t object_len)	{
	int ret_code = GENERAL_FAILURE;
	uint8_t	op_code;
	bool is_done = false;
	std::wstringstream op_code_hex_str;
	std::wstringstream file_pos_hex_str;
	std::wstring dev_msg;
	uint32_t curr_obj_start_pos;

	if (OK == file_reader.readNextByte(op_code))	{
		// Got the DATA_TYPE_[]_OPCODE
		TokenTypeEnum tkn_type = exec_terms.getTokenTypeForOpCode (op_code);
		if (tkn_type == USER_WORD_TKN || !Token::isDirectOperand (tkn_type))	{
			SET_FAILED_ON_SRC_LINE;
			op_code_hex_str.str(L"");
			op_code_hex_str << L"0x" << std::hex << op_code;
			std::wstring devMsg = L"Expected op_code that would resolve to a datatype but got ";
			devMsg.append(op_code_hex_str.str());
			devMsg.append (L" at file position ");
			file_pos_hex_str.str(L"");
			file_pos_hex_str << L"0x" << std::hex << file_reader.getPos();
			devMsg.append(file_pos_hex_str.str());
			user_messages->logMsg(INTERNAL_ERROR, devMsg, this_src_file, failed_on_src_line, 0);

		} else {
			uint32_t past_limit_file_pos = obj_start_pos + object_len;
			Token var_name_tkn;
			std::vector<Token> tkn_list;
			std::wstring look_up_msg;

			while (!is_done && !failed_on_src_line)	{
				var_name_tkn.resetToString(L"");
				Token var_tkn;
				var_tkn.resetTokenExceptSrc();
				var_tkn.tkn_type = tkn_type;

				if (file_reader.isEOF())	{
					is_done = true;
				
				} else if (past_limit_file_pos <= (curr_obj_start_pos = file_reader.getPos()))	{
					is_done = true;
				
				} else {
						file_pos_hex_str.str(L"");
						file_pos_hex_str << L"0x" << std::hex << curr_obj_start_pos;

					if (OK != file_reader.readNextByte(op_code) || USER_VAR_OPCODE != op_code)	{
						SET_FAILED_ON_SRC_LINE;
						dev_msg = L"Did not get expected VAR_NAME_OPCODE at file position " + file_pos_hex_str.str();
						user_messages->logMsg(INTERNAL_ERROR, dev_msg, this_src_file, failed_on_src_line, 0);
			
					} else if (OK != file_reader.readUserVar (var_name_tkn))	{
						SET_FAILED_ON_SRC_LINE;
						dev_msg = L"Failed reading variable name in declaration after file position " + file_pos_hex_str.str();
						user_messages->logMsg(INTERNAL_ERROR, dev_msg, this_src_file, failed_on_src_line, 0);

					} else if (!exec_terms.is_viable_var_name(var_name_tkn._string))	{
						SET_FAILED_ON_SRC_LINE;
						dev_msg = L"Variable name in declaration is invalid [" + var_name_tkn._string + L"] after file position " + file_pos_hex_str.str();
						user_messages->logMsg(INTERNAL_ERROR, dev_msg, this_src_file, failed_on_src_line, 0);
					
					} else if (OK != scope_name_space->insertNewVarAtCurrScope(var_name_tkn._string, var_tkn))	{
							SET_FAILED_ON_SRC_LINE;
							dev_msg = L"Failed to insert variable into NameSpace [" + var_name_tkn._string + L"] after file position " + file_pos_hex_str.str();
							user_messages->logMsg(INTERNAL_ERROR, dev_msg, this_src_file, failed_on_src_line, 0);
					
					} else if (past_limit_file_pos <= (file_reader.getPos()))	{
						// NOTE: Need to check where we are; variable declaration might not have initialization expression(s)
						is_done = true;

					} else if (OK != file_reader.peekNextByte(op_code))	{
							if (file_reader.isEOF())	{
								is_done = true;
							
							} else {
								SET_FAILED_ON_SRC_LINE;
								user_messages->logMsg(INTERNAL_ERROR, L"Peek failed", this_src_file, failed_on_src_line, 0);
							}
					} else if (op_code == EXPRESSION_OPCODE)	{
						// If there's an initialization expression for this variable in the declaration, then resolve it
						// e.g. uint32 numFruits = 3 + 4, numVeggies = (3 * (1 + 2)), numPizzas = (4 + (2 * 3));
						//                         ^                    ^                         ^
						// Otherwise, go back to top of loop and look for the next variable name
						uint32_t expr_start_pos = file_reader.getPos();
						file_pos_hex_str.str(L"");
						file_pos_hex_str << L"0x" << std::hex << expr_start_pos;
						Token resolved_tkn;

						if (OK != execExpression(expr_start_pos, resolved_tkn))	{
								SET_FAILED_ON_SRC_LINE;

						} else if (OK != scope_name_space->findVar(var_name_tkn._string, 0, resolved_tkn, COMMIT_WRITE, look_up_msg))	{
							// Don't limit search to current scope
							SET_FAILED_ON_SRC_LINE;
							user_messages->logMsg (INTERNAL_ERROR
									, L"After resolving initialization expression starting on|near " + file_pos_hex_str.str() + L": " + look_up_msg
									, this_src_file, failed_on_src_line, 0);
						}
					}
				}
			}
		}
	} else {
			uint32_t currFilePos = file_reader.getPos();
	}

	if (is_done && !failed_on_src_line)
		ret_code = OK;

	return (ret_code);
}

/* ****************************************************************************
 * For the PREFIX OPR8Rs [++()] and [--()], the increment|decrement will happen
 * BEFORE the value is placed on our "stack", so the change will be visible in 
 * the current expression
 * 
 * For the POSTFIX OPR8Rs [()++] and [()--], the increment|decrement will happen
 * AFTER the value is placed on our "stack", so the change will NOT be visible in 
 * the current expression
 * ***************************************************************************/
int RunTimeInterpreter::execPrePostFixOp (std::vector<Token> & expr_tkn_stream, int opr8r_idx)	{
	int ret_code = GENERAL_FAILURE;
	bool is_success = false;
	std::wstring lookup_msg;

	if (opr8r_idx >= 0 && expr_tkn_stream.size() > (opr8r_idx + 1) && expr_tkn_stream[opr8r_idx].tkn_type == EXEC_OPR8R_TKN)	{
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t op_code = expr_tkn_stream[opr8r_idx]._unsigned;
		Operator opr8r;
		exec_terms.getExecOpr8rDetails(op_code, opr8r);

		// Our operand Token *MUST* be a USER_WORD variable name, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		std::wstring var_name1;
		resolveTknOrVar (expr_tkn_stream[opr8r_idx+1], operand1, var_name1);

		if (var_name1.empty())	{
			std::wstring userMsg = L"Failed to execute OPR8R ";
			userMsg.append (opr8r.symbol);
			userMsg.append (L"; ");
			userMsg.append (operand1.descr_line_num_col());
			userMsg.append (L" is an r-value");
			user_messages->logMsg (USER_ERROR, userMsg, this_src_file, __LINE__, 0);

		} else if (!operand1.isSigned() &&  !operand1.isUnsigned())	{
      // TODO: The compiler failed us. How should we handle this?

		} else if (op_code == PRE_INCR_OPR8R_OPCODE || op_code == PRE_DECR_OPR8R_OPCODE)	{
			int addValue = (op_code == PRE_INCR_OPR8R_OPCODE ? 1 : -1);
			operand1.isUnsigned() ? operand1._unsigned += addValue : operand1._signed += addValue;

			// Return altered value to our "stack" for use in the expression
			expr_tkn_stream[opr8r_idx] = operand1;
			is_success = true;

		} else if (op_code == POST_INCR_OPR8R_OPCODE || op_code == POST_DECR_OPR8R_OPCODE)	{
			int addValue = (op_code == POST_INCR_OPR8R_OPCODE ? 1 : -1);

			// Return current value to our "stack" for use in the expression, THEN alter NameSpace value
			expr_tkn_stream[opr8r_idx] = operand1;
			operand1.isUnsigned() ? operand1._unsigned += addValue : operand1._signed += addValue;
			is_success = true;
		}

		if (is_success)	{
			expr_tkn_stream[opr8r_idx].isInitialized = true;
			operand1.isInitialized = true;
			ret_code = scope_name_space->findVar(var_name1, 0, operand1, COMMIT_WRITE, lookup_msg);
			if (OK!= ret_code)
				user_messages->logMsg(INTERNAL_ERROR, lookup_msg, this_src_file, __LINE__, 0);
		}

	} else	{
		user_messages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", this_src_file, __LINE__, 0);
	}

	return (ret_code);
}

/* ****************************************************************************
 * Handle these OPR8Rs:
 * [<] [<=] [>] [>=] [==] [!=]
 * ***************************************************************************/
int RunTimeInterpreter::execEquivalenceOp(std::vector<Token> & expr_tkn_stream, int opr8r_idx)     {
	int ret_code = GENERAL_FAILURE;
	
	if (opr8r_idx >= 0 && expr_tkn_stream.size() > (opr8r_idx + 2) && expr_tkn_stream[opr8r_idx].tkn_type == EXEC_OPR8R_TKN)	{
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t op_code = expr_tkn_stream[opr8r_idx]._unsigned;

		// Either|both of our operand Tokens could be USER_WORD variable names, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		Token operand2;
		std::wstring var_name1;
		std::wstring var_name2;
		resolveTknOrVar (expr_tkn_stream[opr8r_idx + 1], operand1, var_name1);
		resolveTknOrVar (expr_tkn_stream[opr8r_idx + 2], operand2, var_name2);

		TokenCompareResult compare_rez = operand1.compare (operand2);

		switch (op_code)	{
			case LESS_THAN_OPR8R_OPCODE :
				if (compare_rez.lessThan == isTrue)
					expr_tkn_stream[opr8r_idx] = *one_tkn;
				else if (compare_rez.lessThan == isFalse)
					expr_tkn_stream[opr8r_idx] = *zero_tkn;
				else
					SET_FAILED_ON_SRC_LINE;
				break;
			case LESS_EQUALS_OPR8R8_OPCODE :
				if (compare_rez.lessEquals == isTrue)
					expr_tkn_stream[opr8r_idx] = *one_tkn;
				else if (compare_rez.lessEquals == isFalse)
					expr_tkn_stream[opr8r_idx] = *zero_tkn;
				else
					SET_FAILED_ON_SRC_LINE;
				break;
			case GREATER_THAN_OPR8R_OPCODE :
				if (compare_rez.gr8rThan == isTrue)
					expr_tkn_stream[opr8r_idx] = *one_tkn;
				else if (compare_rez.gr8rThan == isFalse)
					expr_tkn_stream[opr8r_idx] = *zero_tkn;
				else
					SET_FAILED_ON_SRC_LINE;
				break;
			case GREATER_EQUALS_OPR8R8_OPCODE :
				if (compare_rez.gr8rEquals == isTrue)
					expr_tkn_stream[opr8r_idx] = *one_tkn;
				else if (compare_rez.gr8rEquals == isFalse)
					expr_tkn_stream[opr8r_idx] = *zero_tkn;
				else
					SET_FAILED_ON_SRC_LINE;
				break;
			case EQUALITY_OPR8R_OPCODE :
				if (compare_rez.equals == isTrue)
					expr_tkn_stream[opr8r_idx] = *one_tkn;
				else if (compare_rez.equals == isFalse)
					expr_tkn_stream[opr8r_idx] = *zero_tkn;
				else
					SET_FAILED_ON_SRC_LINE;
				break;
			case NOT_EQUALS_OPR8R_OPCODE:
				if (compare_rez.equals == isFalse)	
					expr_tkn_stream[opr8r_idx] = *one_tkn;
				else if (compare_rez.equals == isTrue)
					expr_tkn_stream[opr8r_idx] = *zero_tkn;
        else	
				  SET_FAILED_ON_SRC_LINE;
				break;
			default:
				SET_FAILED_ON_SRC_LINE;
				break;
		}

		if (!failed_on_src_line)	{
			expr_tkn_stream[opr8r_idx].isInitialized = true;
			ret_code = OK;
		
		} else	{ 
			std::wstring tmp_str = exec_terms.getSrcOpr8rStrFor(op_code);
			if (tmp_str.empty())
				tmp_str = L"UNKNOWN OP_CODE [" + std::to_wstring(op_code) + L"]";
			user_messages->logMsg (INTERNAL_ERROR, L"Failed to execute " + tmp_str, this_src_file, __LINE__, 0);
		}

	} else	{
		user_messages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", this_src_file, __LINE__, 0);
	}

  return (ret_code);
}

/* ****************************************************************************
 * Handle these OPR8Rs:
 * [+] [-] [*] [/] [%]
 * ***************************************************************************/
int RunTimeInterpreter::execStandardMath (std::vector<Token> & expr_tkn_stream, int opr8r_idx)	{
	int ret_code = GENERAL_FAILURE;
	bool is_params_valid = false;
	bool is_missed_case = false;
	bool is_div_by_0 = false;
	bool is_now_double = false;
	uint64_t tmp_unsigned;
	int64_t tmp_signed;
	double tmp_double;
	std::wstring tmp_str;

	if (opr8r_idx >= 0 && expr_tkn_stream.size() > (opr8r_idx + 2) && expr_tkn_stream[opr8r_idx].tkn_type == EXEC_OPR8R_TKN)	{
		
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t op_code = expr_tkn_stream[opr8r_idx]._unsigned;

		// Either|both of our operand Tokens could be USER_WORD variable names, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		Token operand2;
		std::wstring var_name1;
		std::wstring var_name2;
		resolveTknOrVar (expr_tkn_stream[opr8r_idx+1], operand1, var_name1);
		resolveTknOrVar (expr_tkn_stream[opr8r_idx+2], operand2, var_name2);

		// 1st check for valid passed parameters
		if (op_code == MULTIPLY_OPR8R_OPCODE || op_code == DIV_OPR8R_OPCODE || op_code == BINARY_MINUS_OPR8R_OPCODE)	{
			if ((operand1.isSigned() || operand1.isUnsigned() || operand1.tkn_type == DOUBLE_TKN)
					&& (operand2.isSigned() || operand2.isUnsigned() || operand2.tkn_type == DOUBLE_TKN))
				is_params_valid = true;

		} else if (op_code == MOD_OPR8R_OPCODE)	{
			if ((operand1.isSigned() || operand1.isUnsigned()) && (operand2.isSigned() || operand2.isUnsigned()))
				is_params_valid = true;

		} else if (op_code == BINARY_PLUS_OPR8R_OPCODE)	{
			if ((operand1.isSigned() || operand1.isUnsigned() || operand1.tkn_type == DOUBLE_TKN)
					&& (operand2.isSigned() || operand2.isUnsigned() || operand2.tkn_type == DOUBLE_TKN))
				is_params_valid = true;

			if (operand1.tkn_type == STRING_TKN && operand2.tkn_type == STRING_TKN)
				is_params_valid = true;
		}

		if (is_params_valid)	{
			bool is_adding_strings;
			(op_code == BINARY_PLUS_OPR8R_OPCODE && operand1.tkn_type == STRING_TKN) ? is_adding_strings = true : is_adding_strings = false;


			if (!is_adding_strings && operand1.isSigned() && operand2.isSigned())	{
				// signed vs. signed
				switch (op_code)	{
					case MULTIPLY_OPR8R_OPCODE :
						tmp_signed = operand1._signed * operand2._signed;
						break;
					case DIV_OPR8R_OPCODE :
						if (operand2._signed == 0)
							is_div_by_0 = true;
						else if (0 == operand1._signed % operand2._signed)
							tmp_signed = operand1._signed / operand2._signed;
						else	{
							tmp_double = (double) operand1._signed / (double) operand2._signed;
							is_now_double = true;
						}
						break;
					case BINARY_MINUS_OPR8R_OPCODE :
						tmp_signed = operand1._signed - operand2._signed;
						break;
					case BINARY_PLUS_OPR8R_OPCODE :
						tmp_signed = operand1._signed + operand2._signed;
						break;
					case MOD_OPR8R_OPCODE:
						if (operand2._signed == 0)
							is_div_by_0 = true;
						else
							tmp_signed = operand1._signed % operand2._signed;
						break;
					default:
						is_missed_case = true;
						break;
				}

				if (!is_missed_case &&!is_div_by_0)	{
					if (is_now_double)
						expr_tkn_stream[opr8r_idx].resetToDouble(tmp_double);
					else
						expr_tkn_stream[opr8r_idx].resetToSigned(tmp_signed);
				}

			} else if (!is_adding_strings && operand1.isSigned() && operand2.isUnsigned())	{
				// signed vs. unsigned
				switch (op_code)	{
					case MULTIPLY_OPR8R_OPCODE :
						tmp_signed = operand1._signed * operand2._unsigned;
						break;
					case DIV_OPR8R_OPCODE :
						if (operand2._unsigned == 0)
							is_div_by_0 = true;
						else if (0 == operand1._signed % operand2._unsigned)
							tmp_signed = operand1._signed / operand2._unsigned;
						else	{
							tmp_double = (double) operand1._signed / (double) operand2._unsigned;
							is_now_double = true;
						}
						break;
					case BINARY_MINUS_OPR8R_OPCODE :
						tmp_signed = operand1._signed - operand2._unsigned;
						break;
					case BINARY_PLUS_OPR8R_OPCODE :
						tmp_signed = operand1._signed + operand2._unsigned;
						break;
					case MOD_OPR8R_OPCODE:
						if (operand2._unsigned == 0)
							is_div_by_0 = true;
						else
							tmp_signed = operand1._signed % operand2._unsigned;
						break;
					default:
						is_missed_case = true;
						break;
				}

				if (!is_missed_case &&!is_div_by_0)	{
					if (is_now_double)
						expr_tkn_stream[opr8r_idx].resetToDouble(tmp_double);
					else
						expr_tkn_stream[opr8r_idx].resetToSigned(tmp_signed);
				}

			} else if (!is_adding_strings && operand1.isSigned() && operand2.tkn_type == DOUBLE_TKN)	{
				// signed vs. double
				switch (op_code)	{
					case MULTIPLY_OPR8R_OPCODE :
						tmp_double = operand1._signed * operand2._double;
						break;
					case DIV_OPR8R_OPCODE :
						if (operand2._double == 0)
							is_div_by_0 = true;
						else
							tmp_double = operand1._signed / operand2._double;
						break;
					case BINARY_MINUS_OPR8R_OPCODE :
						tmp_double = operand1._signed - operand2._double;
						break;
					case BINARY_PLUS_OPR8R_OPCODE :
						tmp_double = operand1._signed + operand2._double;
						break;
					default:
						is_missed_case = true;
						break;
				}

				if (!is_missed_case &&!is_div_by_0)
					expr_tkn_stream[opr8r_idx].resetToDouble (tmp_double);

			} else if (!is_adding_strings && operand1.isUnsigned() && operand2.isSigned())	{
				// unsigned vs. signed
				switch (op_code)	{
					case MULTIPLY_OPR8R_OPCODE :
						tmp_signed = operand1._unsigned * operand2._signed;
						break;
					case DIV_OPR8R_OPCODE :
						if (operand2._signed == 0)
							is_div_by_0 = true;
						else	{
							// TODO: Result of integer division used in a floating point context; possible loss of precision
							tmp_double = operand1._unsigned / operand2._signed;
							is_now_double = true;
						}
						break;
					case BINARY_MINUS_OPR8R_OPCODE :
						tmp_signed = operand1._unsigned - operand2._signed;
						break;
					case BINARY_PLUS_OPR8R_OPCODE :
						tmp_signed = operand1._unsigned + operand2._signed;
						break;
					case MOD_OPR8R_OPCODE:
						if (operand2._signed == 0)
							is_div_by_0 = true;
						else
							tmp_signed = operand1._unsigned % operand2._signed;
						break;
					default:
						is_missed_case = true;
						break;
				}

				if (!is_missed_case &&!is_div_by_0)	{
					if (is_now_double)
						expr_tkn_stream[opr8r_idx].resetToDouble(tmp_double);
					else
						expr_tkn_stream[opr8r_idx].resetToSigned (tmp_signed);
				}

			} else if (!is_adding_strings && operand1.isUnsigned() && operand2.tkn_type == DOUBLE_TKN)	{
				// unsigned vs. double
				switch (op_code)	{
					case MULTIPLY_OPR8R_OPCODE :
						tmp_double = operand1._unsigned * operand2._double;
						break;
					case DIV_OPR8R_OPCODE :
						if (operand2._double == 0)
							is_div_by_0 = true;
						else
							tmp_double = operand1._unsigned / operand2._double;
						break;
					case BINARY_MINUS_OPR8R_OPCODE :
						tmp_double = operand1._unsigned - operand2._double;
						break;
					case BINARY_PLUS_OPR8R_OPCODE :
						tmp_double = operand1._unsigned + operand2._double;
						break;
					case MOD_OPR8R_OPCODE:
						if (operand2._signed == 0)
							is_div_by_0 = true;
						else
							tmp_signed = operand1._unsigned % operand2._signed;
						break;
					default:
						is_missed_case = true;
						break;
				}

				if (!is_missed_case &&!is_div_by_0)
					expr_tkn_stream[opr8r_idx].resetToDouble(tmp_double);

			} else if (!is_adding_strings && operand1.isUnsigned() && operand2.isUnsigned())	{
				// unsigned vs. unsigned
				switch (op_code)	{
					case MULTIPLY_OPR8R_OPCODE :
						tmp_unsigned = operand1._unsigned * operand2._unsigned;
						break;
					case DIV_OPR8R_OPCODE :
						if (operand2._unsigned == 0)
							is_div_by_0 = true;
						else
							tmp_unsigned = operand1._unsigned / operand2._unsigned;
						break;
						if (operand2._unsigned == 0)
							is_div_by_0 = true;
						else if (0 == operand1._unsigned % operand2._unsigned)
							tmp_unsigned = operand1._unsigned / operand2._unsigned;
						else	{
							tmp_double = (double) operand1._unsigned / (double) operand2._unsigned;
							is_now_double = true;
						}
						break;
					case BINARY_MINUS_OPR8R_OPCODE :
						tmp_unsigned = operand1._unsigned - operand2._unsigned;
						break;
					case BINARY_PLUS_OPR8R_OPCODE :
						tmp_unsigned = operand1._unsigned + operand2._unsigned;
						break;
					case MOD_OPR8R_OPCODE:
						if (operand2._unsigned == 0)
							is_div_by_0 = true;
						else
							tmp_unsigned = operand1._unsigned % operand2._unsigned;
						break;
					default:
						is_missed_case = true;
						break;
				}

				if (!is_missed_case &&!is_div_by_0)	{
					if (is_now_double)
						expr_tkn_stream[opr8r_idx].resetToDouble(tmp_double);
					else
						expr_tkn_stream[opr8r_idx].resetToUnsigned(tmp_unsigned);
				}

			} else if (!is_adding_strings && operand1.isUnsigned() && operand2.tkn_type == DOUBLE_TKN)	{
				// unsigned vs. double
				switch (op_code)	{
					case MULTIPLY_OPR8R_OPCODE :
						tmp_double = operand1._unsigned * operand2._double;
						break;
					case DIV_OPR8R_OPCODE :
						if (operand2._double == 0)
							is_div_by_0 = true;
						else
							tmp_double = operand1._unsigned / operand2._double;
						break;
					case BINARY_MINUS_OPR8R_OPCODE :
						tmp_double = operand1._unsigned - operand2._double;
						break;
					case BINARY_PLUS_OPR8R_OPCODE :
						tmp_double = operand1._unsigned + operand2._double;
						break;
					default:
						is_missed_case = true;
						break;
				}

				if (!is_missed_case &&!is_div_by_0)
					expr_tkn_stream[opr8r_idx].resetToDouble (tmp_double);

			} else if (!is_adding_strings && operand1.tkn_type == DOUBLE_TKN && operand2.isSigned())	{
				// double vs. signed
				switch (op_code)	{
					case MULTIPLY_OPR8R_OPCODE :
						tmp_double = operand1._double * operand2._signed;
						break;
					case DIV_OPR8R_OPCODE :
						if (operand2._signed == 0)
							is_div_by_0 = true;
						else
							tmp_double = operand1._double / operand2._signed;
						break;
					case BINARY_MINUS_OPR8R_OPCODE :
						tmp_double = operand1._double - operand2._signed;
						break;
					case BINARY_PLUS_OPR8R_OPCODE :
						tmp_double = operand1._double + operand2._signed;
						break;
					default:
						is_missed_case = true;
						break;
				}
				if (!is_missed_case &&!is_div_by_0)
					expr_tkn_stream[opr8r_idx].resetToDouble (tmp_double);

			} else if (!is_adding_strings && operand1.tkn_type == DOUBLE_TKN && operand2.isUnsigned())	{
				// double vs. unsigned
				switch (op_code)	{
					case MULTIPLY_OPR8R_OPCODE :
						tmp_double = operand1._double * operand2._unsigned;
						break;
					case DIV_OPR8R_OPCODE :
						if (operand2._unsigned == 0)
							is_div_by_0 = true;
						else
							tmp_double = operand1._double / operand2._unsigned;
						break;
					case BINARY_MINUS_OPR8R_OPCODE :
						tmp_double = operand1._double - operand2._unsigned;
						break;
					case BINARY_PLUS_OPR8R_OPCODE :
						tmp_double = operand1._double + operand2._unsigned;
						break;
					default:
						is_missed_case = true;
						break;
				}

				if (!is_missed_case &&!is_div_by_0)
					expr_tkn_stream[opr8r_idx].resetToDouble (tmp_double);

			} else if (!is_adding_strings && operand1.tkn_type == DOUBLE_TKN && operand2.tkn_type == DOUBLE_TKN)	{
				// double vs. double
				switch (op_code)	{
				case MULTIPLY_OPR8R_OPCODE :
					tmp_double = operand1._double * operand2._double;
					break;
				case DIV_OPR8R_OPCODE :
					if (operand2._double == 0)
						is_div_by_0 = true;
					else
						tmp_double = operand1._double / operand2._double;
					break;
				case BINARY_MINUS_OPR8R_OPCODE :
					tmp_double = operand1._double - operand2._double;
					break;
				case BINARY_PLUS_OPR8R_OPCODE :
					tmp_double = operand1._double + operand2._double;
					break;
				default:
					is_missed_case = true;
					break;
				}

				if (!is_missed_case &&!is_div_by_0)
					expr_tkn_stream[opr8r_idx].resetToDouble (tmp_double);

			} else if (is_adding_strings && operand1.tkn_type == STRING_TKN && operand2.tkn_type == STRING_TKN)	{
				if (!operand1._string.empty())
					tmp_str = operand1._string;
				if (!operand2._string.empty())
					tmp_str.append(operand2._string);

				expr_tkn_stream[opr8r_idx].resetToString (tmp_str);
			}
		}	else	{
			user_messages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", this_src_file, __LINE__, 0);
		}

		if (is_params_valid && !is_missed_case && !is_div_by_0)	{
			expr_tkn_stream[opr8r_idx].isInitialized = true;
			ret_code = OK;
		
		} else if (is_div_by_0)	{
			// TODO: How can the error msg indicate where this div by zero occurred in the code?
			// Should enclosing scope contain opening line # and possibly source file name
			// void InfoWarnError::set(info_warn_error_type type, std::wstring userSrcFileName, int userSrcLineNum, int userSrcColPos, std::wstring srcFileName, int srcLineNum, std::wstring msgForUser) {
			std::wstring bad_user_msg = L"User code attempted to divide by ZERO! [";
			bad_user_msg.append (var_name1.empty() ? operand1.getValueStr() : var_name1);
			bad_user_msg.append (L" / ");
			bad_user_msg.append (var_name2.empty() ? operand2.getValueStr() : var_name2);
			bad_user_msg.append (L"]");
			user_messages->logMsg (USER_ERROR, bad_user_msg, !usr_src_file_name.empty() ? usr_src_file_name : L"???", 0, 0);

		} else if (is_missed_case)
			user_messages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", this_src_file, __LINE__, 0);
	}

	return (ret_code);
}

/* ****************************************************************************
 * Handle these OPR8Rs:
 * [<<] [>>]
 * ***************************************************************************/
int RunTimeInterpreter::execShift (std::vector<Token> & expr_tkn_stream, int opr8r_idx)	{
	int ret_code = GENERAL_FAILURE;

	if (opr8r_idx >= 0 && expr_tkn_stream.size() > (opr8r_idx + 2) && expr_tkn_stream[opr8r_idx].tkn_type == EXEC_OPR8R_TKN)	{
		// Passed in result_tkn has OPR8R op_code BEFORE it is overwritten by the result
		
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t op_code = expr_tkn_stream[opr8r_idx]._unsigned;

		// Either|both of our operand Tokens could be USER_WORD variable names, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		Token operand2;
		std::wstring var_name1;
		std::wstring var_name2;
		resolveTknOrVar (expr_tkn_stream[opr8r_idx+1], operand1, var_name1);
		resolveTknOrVar (expr_tkn_stream[opr8r_idx+2], operand2, var_name2);

		// Operand #1 must be of type UINT[N] or INT[N]; Operand #2 can be either UINT[N] or INT[N] > 0
		if ((op_code == LEFT_SHIFT_OPR8R_OPCODE || op_code == RIGHT_SHIFT_OPR8R_OPCODE) && (operand1.isUnsigned() || operand1.isSigned())
				&& ((operand2.isUnsigned() && operand2._unsigned >= 0) || operand2.isSigned() && operand2._signed >= 0))	{

			uint64_t tmp_unsigned;
			int64_t tmp_signed;

			if (operand1.isUnsigned())	{
				if (operand2.isUnsigned())	{
					if (op_code == LEFT_SHIFT_OPR8R_OPCODE)	{
						tmp_unsigned = operand1._unsigned << operand2._unsigned;
					} else {
						tmp_unsigned = operand1._unsigned >> operand2._unsigned;
					}
				} else 	{
					if (op_code == LEFT_SHIFT_OPR8R_OPCODE)	{
						tmp_unsigned = operand1._unsigned << operand2._signed;
					} else {
						tmp_unsigned = operand1._unsigned >> operand2._signed;
					}
				}

				expr_tkn_stream[opr8r_idx].resetToUnsigned (tmp_unsigned);

			} else if (operand1._signed > 0)	{
				if (operand2.isUnsigned())	{
					if (op_code == LEFT_SHIFT_OPR8R_OPCODE)	{
						tmp_unsigned = operand1._signed << operand2._unsigned;
					} else {
						tmp_unsigned = operand1._signed >> operand2._unsigned;
					}
				} else	{
					// operand2 is SIGNED
					if (op_code == LEFT_SHIFT_OPR8R_OPCODE)	{
						tmp_unsigned = operand1._signed << operand2._signed;
					} else {
						tmp_unsigned = operand1._signed >> operand2._signed;
					}
				}

				expr_tkn_stream[opr8r_idx].resetToUnsigned (tmp_unsigned);

			} else	{
				// operand1 is negative, so we have to keep the sign bit around
				if (op_code == LEFT_SHIFT_OPR8R_OPCODE)	{
					tmp_signed = operand1._signed << operand2._signed;
				} else {
					tmp_signed = operand1._signed >> operand2._signed;
				}
				expr_tkn_stream[opr8r_idx].resetToSigned (tmp_signed);
			}

			ret_code = OK;
		}
	}	else	{
		user_messages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", this_src_file, __LINE__, 0);
	}

	if (OK != ret_code)
		user_messages->logMsg (INTERNAL_ERROR, L"Function failed!", this_src_file, __LINE__, 0);

	return (ret_code);
}

/* ****************************************************************************
 * Handle these OPR8Rs:
 * [&] [|] [^]
 * ***************************************************************************/
int RunTimeInterpreter::execBitWiseOp (std::vector<Token> & expr_tkn_stream, int opr8r_idx)	{
	int ret_code = GENERAL_FAILURE;
	bool is_params_valid = true;
	bool is_missed_case = false;

	if (opr8r_idx >= 0 && expr_tkn_stream.size() > (opr8r_idx + 2) && expr_tkn_stream[opr8r_idx].tkn_type == EXEC_OPR8R_TKN)	{

		// Passed in result_tkn has OPR8R op_code BEFORE it is overwritten by the result
		// TODO: If these are USER_WORD_TKNs, then do a variable name lookup in our NameSpace
		
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t op_code = expr_tkn_stream[opr8r_idx]._unsigned;

		// Either|both of our operand Tokens could be USER_WORD variable names, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		Token operand2;
		std::wstring var_name1;
		std::wstring var_name2;
		resolveTknOrVar (expr_tkn_stream[opr8r_idx+1], operand1, var_name1);
		resolveTknOrVar (expr_tkn_stream[opr8r_idx+2], operand2, var_name2);

		uint64_t bitwise_result;

		if (op_code == BITWISE_AND_OPR8R_OPCODE || op_code == BITWISE_XOR_OPR8R_OPCODE || op_code == BITWISE_OR_OPR8R_OPCODE)	{
			if (operand1.isUnsigned() && operand2.isUnsigned())	{
				switch (op_code)	{
					case BITWISE_AND_OPR8R_OPCODE :
						bitwise_result = operand1._unsigned & operand2._unsigned;
						break;
					case BITWISE_XOR_OPR8R_OPCODE :
						bitwise_result = operand1._unsigned ^ operand2._unsigned;
						break;
					case BITWISE_OR_OPR8R_OPCODE :
						bitwise_result = operand1._unsigned | operand2._unsigned;
						break;
					default :
						is_missed_case = true;
						break;
				}

			} else if (operand1.isUnsigned() && operand2.isSigned() && operand2._signed >= 0)	{
				switch (op_code)	{
					case BITWISE_AND_OPR8R_OPCODE :
						bitwise_result = operand1._unsigned & operand2._signed;
						break;
					case BITWISE_XOR_OPR8R_OPCODE :
						bitwise_result = operand1._unsigned ^ operand2._signed;
						break;
					case BITWISE_OR_OPR8R_OPCODE :
						bitwise_result = operand1._unsigned | operand2._signed;
						break;
					default :
						is_missed_case = true;
						break;
				}

			} else if (operand1.isSigned() && operand1._signed >= 0 && operand2.isUnsigned())	{
				switch (op_code)	{
					case BITWISE_AND_OPR8R_OPCODE :
						bitwise_result = operand1._signed & operand2._unsigned;
						break;
					case BITWISE_XOR_OPR8R_OPCODE :
						bitwise_result = operand1._signed ^ operand2._unsigned;
						break;
					case BITWISE_OR_OPR8R_OPCODE :
						bitwise_result = operand1._signed | operand2._unsigned;
						break;
					default :
						is_missed_case = true;
						break;
				}

			} else if (operand1.isSigned() && operand1._signed >= 0 && operand2.isSigned() && operand2._signed >= 0)	{
				switch (op_code)	{
					case BITWISE_AND_OPR8R_OPCODE :
						bitwise_result = operand1._signed & operand2._signed;
						break;
					case BITWISE_XOR_OPR8R_OPCODE :
						bitwise_result = operand1._signed ^ operand2._signed;
						break;
					case BITWISE_OR_OPR8R_OPCODE :
						bitwise_result = operand1._signed | operand2._signed;
						break;
					default :
						is_missed_case = true;
						break;
				}

			} else	{
				is_params_valid = false;
			}
		}

		if (is_params_valid && !is_missed_case)	{
			expr_tkn_stream[opr8r_idx].resetToUnsigned(bitwise_result);
			expr_tkn_stream[opr8r_idx].isInitialized = true;
			ret_code = OK;
		
		} else	{
			Operator opr8r;
			exec_terms.getExecOpr8rDetails(op_code, opr8r);
			user_messages->logMsg (INTERNAL_ERROR, L"Failed to execute OPR8R " + opr8r.symbol, this_src_file, __LINE__, 0);
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * Handle these OPR8Rs:
 * [~] [-] [!]
 * ***************************************************************************/
int RunTimeInterpreter::execUnaryOp (std::vector<Token> & expr_tkn_stream, int opr8r_idx)	{
	int ret_code = GENERAL_FAILURE;
	bool isSuccess = false;

	if (opr8r_idx >= 0 && expr_tkn_stream.size() > (opr8r_idx + 1) && expr_tkn_stream[opr8r_idx].tkn_type == EXEC_OPR8R_TKN)	{

		// TODO: If these are USER_WORD_TKNs, then do a variable name lookup in our NameSpace
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t op_code = expr_tkn_stream[opr8r_idx]._unsigned;

		// Our operand Token could be a USER_WORD variable name, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		std::wstring var_name1;
		resolveTknOrVar (expr_tkn_stream[opr8r_idx+1], operand1, var_name1);
		uint64_t unary_result;

		if (op_code == UNARY_PLUS_OPR8R_OPCODE)	{
			// UNARY_PLUS is a NO-OP with the right data types
			if (operand1.isSigned() || operand1.isUnsigned())	{
				expr_tkn_stream[opr8r_idx] = operand1;
				ret_code = OK;
			}

		} else if (op_code == UNARY_MINUS_OPR8R_OPCODE)	{
			if (operand1.isSigned())	{
				expr_tkn_stream[opr8r_idx] = operand1;
				expr_tkn_stream[opr8r_idx]._signed = (0 - operand1._signed);
				ret_code = OK;

			} else if (operand1.isUnsigned())	{
				int64_t tmpInt64 = expr_tkn_stream[opr8r_idx]._unsigned;
				expr_tkn_stream[opr8r_idx].resetToSigned (-tmpInt64);
				ret_code = OK;

			} else if (operand1.tkn_type == DOUBLE_TKN)	{
				double tmp_double = expr_tkn_stream[opr8r_idx]._double;
				expr_tkn_stream[opr8r_idx].resetToDouble (-tmp_double);
				ret_code = OK;
			}

		} else if (op_code == LOGICAL_NOT_OPR8R_OPCODE)	{
			if (operand1.isUnsigned() || operand1.tkn_type == BOOL_TKN)	{
				if (operand1._unsigned == 0)
					expr_tkn_stream[opr8r_idx] = *one_tkn;
				else
					expr_tkn_stream[opr8r_idx] = *zero_tkn;

				if (operand1.tkn_type == BOOL_TKN)
					// Preserve the data type
					expr_tkn_stream[opr8r_idx].tkn_type = BOOL_TKN;
				ret_code = OK;

			} else if (operand1.isSigned())	{
				if (operand1._signed == 0)
					expr_tkn_stream[opr8r_idx] = *one_tkn;
				else
					expr_tkn_stream[opr8r_idx] = *zero_tkn;
				ret_code = OK;

			} else if (operand1.tkn_type == DOUBLE_TKN)	{
				if (operand1._double == 0.0)
					expr_tkn_stream[opr8r_idx] = *one_tkn;
				else
					expr_tkn_stream[opr8r_idx] = *zero_tkn;
				ret_code = OK;
			}
		} else if (op_code == BITWISE_NOT_OPR8R_OPCODE && operand1.isUnsigned())	{
			// Need to guard against uncalled for overflow
			// TODO: Chicken|egg problem with warning user about overflow
			uint64_t mask = ~(0x0);
			if (operand1.tkn_type == UINT8_TKN)
				mask = 0xFF;
			else if (operand1.tkn_type == UINT16_TKN)
				mask = 0xFFFF;
			else if (operand1.tkn_type == UINT32_TKN)
				mask = 0xFFFFFFFF;
			
			operand1._unsigned = ~(operand1._unsigned) & mask;
			expr_tkn_stream[opr8r_idx] = operand1;
			ret_code = OK;
		}

		if (OK == ret_code) {
			expr_tkn_stream[opr8r_idx].isInitialized = true;
		
		} else	{
			Operator opr8r;
			exec_terms.getExecOpr8rDetails(op_code, opr8r);
			user_messages->logMsg (INTERNAL_ERROR, L"Failed to execute OPR8R " + opr8r.symbol, this_src_file, __LINE__, 0);
		}

	} else	{
		user_messages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", this_src_file, __LINE__, 0);
	}

	return (ret_code);
}

/* ****************************************************************************
 * Handle these assignment OPR8Rs:
 * [=] [+=] [-=] [*=] [/=] [%=] [<<=] [>>=] [&=] [|=] [^=]
 * ***************************************************************************/
int RunTimeInterpreter::execAssignmentOp(std::vector<Token> & expr_tkn_stream, int opr8r_idx)     {
	int ret_code = GENERAL_FAILURE;
	bool isSuccess = false;
	std::wstring lookUpMsg;
	
	if (opr8r_idx >= 0 && expr_tkn_stream.size() > (opr8r_idx + 2) && expr_tkn_stream[opr8r_idx].tkn_type == EXEC_OPR8R_TKN)	{
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t original_op_code = expr_tkn_stream[opr8r_idx]._unsigned;

		// Either|both of our operand Tokens could be USER_WORD variable names, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		Token operand2;
		std::wstring var_name1;
		std::wstring var_name2;
		resolveTknOrVar (expr_tkn_stream[opr8r_idx + 1], operand1, var_name1, false);
		resolveTknOrVar (expr_tkn_stream[opr8r_idx + 2], operand2, var_name2);

		bool is_op_success = false;

		std::wstring bgn_notta_var_msg = L"Left operand of an assignment operator must be a named variable: ";
		Token opr8rTkn = expr_tkn_stream[opr8r_idx];
		
		if (var_name1.empty() && usage_mode == COMPILE_TIME)	{
			user_messages->logMsg(USER_ERROR
				, bgn_notta_var_msg + opr8rTkn.descr_sans_line_num_col() + L" Assignment operation may need to be enclosed in parentheses."
				, usr_src_file_name, opr8rTkn.get_line_number(), opr8rTkn.get_column_pos());

		} else if (var_name1.empty() && usage_mode == INTERPRETER)	{
			user_messages->logMsg(INTERNAL_ERROR, bgn_notta_var_msg + opr8rTkn.descr_sans_line_num_col()
				, this_src_file, __LINE__, 0);

		} else	{
			switch (original_op_code)	{
				case ASSIGNMENT_OPR8R_OPCODE :
					if (OK == scope_name_space->findVar(var_name1, 0, operand2, COMMIT_WRITE, lookUpMsg))	{
						// We've updated the NS Variable Token; now overwrite the OPR8R with the result also
						expr_tkn_stream[opr8r_idx] = operand2;
						is_op_success = true;
						ret_code = OK;
					
					} else if (!lookUpMsg.empty())	{
						user_messages->logMsg(INTERNAL_ERROR, lookUpMsg, this_src_file, __LINE__, 0);
					}
					break;
				case PLUS_ASSIGN_OPR8R_OPCODE :
					expr_tkn_stream[opr8r_idx]._unsigned = BINARY_PLUS_OPR8R_OPCODE;
					if (OK == execBinaryOp (expr_tkn_stream, opr8r_idx))
						is_op_success = true;
					break;
				case MINUS_ASSIGN_OPR8R_OPCODE :
					expr_tkn_stream[opr8r_idx]._unsigned = BINARY_MINUS_OPR8R_OPCODE;
					if (OK == execBinaryOp (expr_tkn_stream, opr8r_idx))
						is_op_success = true;
					break;
				case MULTIPLY_ASSIGN_OPR8R_OPCODE :
					expr_tkn_stream[opr8r_idx]._unsigned = MULTIPLY_OPR8R_OPCODE;
					if (OK == execBinaryOp (expr_tkn_stream, opr8r_idx))
						is_op_success = true;
					break;
				case DIV_ASSIGN_OPR8R_OPCODE :
					expr_tkn_stream[opr8r_idx]._unsigned = DIV_OPR8R_OPCODE;
					if (OK == execBinaryOp (expr_tkn_stream, opr8r_idx))
						is_op_success = true;
					break;
				case MOD_ASSIGN_OPR8R_OPCODE :
					expr_tkn_stream[opr8r_idx]._unsigned = MOD_OPR8R_OPCODE;
					if (OK == execBinaryOp (expr_tkn_stream, opr8r_idx))
						is_op_success = true;
					break;
				case LEFT_SHIFT_ASSIGN_OPR8R_OPCODE :
					expr_tkn_stream[opr8r_idx]._unsigned = LEFT_SHIFT_OPR8R_OPCODE;
					if (OK == execBinaryOp (expr_tkn_stream, opr8r_idx))
						is_op_success = true;
					break;
				case RIGHT_SHIFT_ASSIGN_OPR8R_OPCODE :
					expr_tkn_stream[opr8r_idx]._unsigned = RIGHT_SHIFT_OPR8R_OPCODE;
					if (OK == execBinaryOp (expr_tkn_stream, opr8r_idx))
						is_op_success = true;
					break;
				case BITWISE_AND_ASSIGN_OPR8R_OPCODE :
					expr_tkn_stream[opr8r_idx]._unsigned = BITWISE_AND_OPR8R_OPCODE;
					if (OK == execBinaryOp (expr_tkn_stream, opr8r_idx))
						is_op_success = true;
					break;
				case BITWISE_XOR_ASSIGN_OPR8R_OPCODE :
					expr_tkn_stream[opr8r_idx]._unsigned = BITWISE_XOR_OPR8R_OPCODE;
					if (OK == execBinaryOp (expr_tkn_stream, opr8r_idx))
						is_op_success = true;
					break;
				case BITWISE_OR_ASSIGN_OPR8R_OPCODE :
					expr_tkn_stream[opr8r_idx]._unsigned = BITWISE_OR_OPR8R_OPCODE;
					if (OK == execBinaryOp (expr_tkn_stream, opr8r_idx))
						is_op_success = true;
					break;
				default:
					break;
			}
		}

		if (is_op_success)	{
			expr_tkn_stream[opr8r_idx].isInitialized = true;
			if (original_op_code != ASSIGNMENT_OPR8R_OPCODE
					&& OK == scope_name_space->findVar(var_name1, 0, expr_tkn_stream[opr8r_idx], COMMIT_WRITE, lookUpMsg))	{
				// Commit the result to the stored NS variable. OPR8R Token (previously @ opr8r_idx) has already been overwritten with result 
				ret_code = OK;
			} else if (!lookUpMsg.empty())	{
				user_messages->logMsg(INTERNAL_ERROR, lookUpMsg, this_src_file, __LINE__, 0);
			}
		}
	}  else	{
		user_messages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", this_src_file, __LINE__, 0);
	}

	return (ret_code);
}

/* ****************************************************************************
 * Jump gate for handling BINARY OPR8Rs
 * ***************************************************************************/
int RunTimeInterpreter::execBinaryOp(std::vector<Token> & expr_tkn_stream, int opr8r_idx)     {
	int ret_code = GENERAL_FAILURE;

	if (opr8r_idx >= 0 && expr_tkn_stream.size() > (opr8r_idx + 2) && expr_tkn_stream[opr8r_idx].tkn_type == EXEC_OPR8R_TKN)	{
		// Snarf up OPR8R op_code BEFORE it is overwritten by the result
		uint8_t op_code = expr_tkn_stream[opr8r_idx]._unsigned;
		Operator opr8r;
		exec_terms.getExecOpr8rDetails(op_code, opr8r);

		// Either|both of our operand Tokens could be USER_WORD variable names, requiring a NameSpace look up to get the actual value
		// TODO: Figure out how to log errors but continue on when compiling
		Token operand1;
		Token operand2;
		std::wstring var_name1;
		std::wstring var_name2;
		resolveTknOrVar (expr_tkn_stream[opr8r_idx+1], operand1, var_name1, op_code == ASSIGNMENT_OPR8R_OPCODE ?  false : true);
		resolveTknOrVar (expr_tkn_stream[opr8r_idx+2], operand2, var_name2);

		switch (op_code)	{
			case MULTIPLY_OPR8R_OPCODE :
			case DIV_OPR8R_OPCODE :
			case MOD_OPR8R_OPCODE :
			case BINARY_PLUS_OPR8R_OPCODE :
			case BINARY_MINUS_OPR8R_OPCODE :
				ret_code = execStandardMath (expr_tkn_stream, opr8r_idx);
				break;

			case LEFT_SHIFT_OPR8R_OPCODE :
			case RIGHT_SHIFT_OPR8R_OPCODE :
				ret_code = execShift (expr_tkn_stream, opr8r_idx);
				break;

			case LESS_THAN_OPR8R_OPCODE :
			case LESS_EQUALS_OPR8R8_OPCODE :
			case GREATER_THAN_OPR8R_OPCODE :
			case GREATER_EQUALS_OPR8R8_OPCODE :
			case EQUALITY_OPR8R_OPCODE :
			case NOT_EQUALS_OPR8R_OPCODE :
				ret_code = execEquivalenceOp (expr_tkn_stream, opr8r_idx);
				break;

			case BITWISE_AND_OPR8R_OPCODE :
			case BITWISE_XOR_OPR8R_OPCODE :
			case BITWISE_OR_OPR8R_OPCODE :
				ret_code = execBitWiseOp (expr_tkn_stream, opr8r_idx);
				break;

			case LOGICAL_AND_OPR8R_OPCODE :
				if (operand1.evalResolvedTokenAsIf() && operand2.evalResolvedTokenAsIf())
					expr_tkn_stream[opr8r_idx] = *one_tkn;
				else
					expr_tkn_stream[opr8r_idx] = *zero_tkn;
				ret_code = OK;

				break;

			case LOGICAL_OR_OPR8R_OPCODE :
				if (operand1.evalResolvedTokenAsIf() || operand2.evalResolvedTokenAsIf())
					expr_tkn_stream[opr8r_idx] = *one_tkn;
				else
					expr_tkn_stream[opr8r_idx] = *zero_tkn;
				ret_code = OK;
				break;

			case ASSIGNMENT_OPR8R_OPCODE :
			case PLUS_ASSIGN_OPR8R_OPCODE :
			case MINUS_ASSIGN_OPR8R_OPCODE :
			case MULTIPLY_ASSIGN_OPR8R_OPCODE :
			case DIV_ASSIGN_OPR8R_OPCODE :
			case MOD_ASSIGN_OPR8R_OPCODE :
			case LEFT_SHIFT_ASSIGN_OPR8R_OPCODE :
			case RIGHT_SHIFT_ASSIGN_OPR8R_OPCODE :
			case BITWISE_AND_ASSIGN_OPR8R_OPCODE :
			case BITWISE_XOR_ASSIGN_OPR8R_OPCODE :
			case BITWISE_OR_ASSIGN_OPR8R_OPCODE :
				ret_code = execAssignmentOp (expr_tkn_stream, opr8r_idx);
				break;
			default:
				break;
		}


		if (ret_code == OK)
			expr_tkn_stream[opr8r_idx].isInitialized = true;

	} else	{
		user_messages->logMsg (INTERNAL_ERROR, L"Incorrect parameters|count", this_src_file, __LINE__, 0);
	}

	return (ret_code);
}

/* ****************************************************************************
 * The [&&] OPR8R can be short-circuited if the 1st [operand|expression] can be 
 * resolved to FALSE - evaluating the 2nd [operand|expression] is redundant
 * ***************************************************************************/
int RunTimeInterpreter::exec_logical_and (std::vector<Token> & expr_tkn_stream, int opr8r_idx)	{
	int ret_code = GENERAL_FAILURE;
  int expected_ret_tkn_cnt;
	
	if (OK != execFlatExpr_OLR(expr_tkn_stream, opr8r_idx + 1, expected_ret_tkn_cnt))	{
		// Resolve the 1st expression
		SET_FAILED_ON_SRC_LINE;
	
	} else if (expr_tkn_stream[opr8r_idx + 1].evalResolvedTokenAsIf())	{
		// 1st expression evaluated as TRUE
		bool is_2nd_true = false;

		if (OK != execFlatExpr_OLR(expr_tkn_stream, opr8r_idx + 2, expected_ret_tkn_cnt))	{
			// Resolve the 2nd expression
			SET_FAILED_ON_SRC_LINE;
		
		} else if (expr_tkn_stream[opr8r_idx + 2].evalResolvedTokenAsIf())	{
			// 2nd expression evaluated as TRUE
			is_2nd_true = true;
		} 

		if (!failed_on_src_line)	{
		 	expr_tkn_stream.erase(expr_tkn_stream.begin() + opr8r_idx + 1, expr_tkn_stream.begin() + opr8r_idx + 3);
			if (is_2nd_true)
				expr_tkn_stream[opr8r_idx] = *one_tkn;
			else
				expr_tkn_stream[opr8r_idx] = *zero_tkn;
		}

	} else {
		// 1st|Left [operand|expression] evaluated FALSE; short-circuit 2nd|Right side
		int last_idx_of_sub_expr;
		if (OK != getEndOfSubExprIdx (expr_tkn_stream, opr8r_idx + 2, last_idx_of_sub_expr))	{
			SET_FAILED_ON_SRC_LINE;
		} else	{
			if (last_idx_of_sub_expr == opr8r_idx + 1)
				expr_tkn_stream.erase (expr_tkn_stream.begin() + last_idx_of_sub_expr);
			else
				expr_tkn_stream.erase(expr_tkn_stream.begin() + opr8r_idx + 1, expr_tkn_stream.begin() + last_idx_of_sub_expr + 1);
			expr_tkn_stream[opr8r_idx] = *zero_tkn;

		}
	}

	if (!failed_on_src_line)	{
		expr_tkn_stream[opr8r_idx].isInitialized = true;
		ret_code = OK;
	}

	return (ret_code);
}

/* ****************************************************************************
 * The [||] OPR8R can be short-circuited if the 1st [operand|expression] can be 
 * resolved to TRUE - evaluating the 2nd [operand|expression] is redundant
 * ***************************************************************************/
int RunTimeInterpreter::exec_logical_or (std::vector<Token> & expr_tkn_stream, int opr8r_idx)	{
	int ret_code = GENERAL_FAILURE;
  int expected_ret_tkn_cnt;

	if (OK != execFlatExpr_OLR(expr_tkn_stream, opr8r_idx + 1, expected_ret_tkn_cnt))	{
		// Resolve the 1st expression
		SET_FAILED_ON_SRC_LINE;
	
	} else {
		bool is_left_true = expr_tkn_stream[opr8r_idx + 1].evalResolvedTokenAsIf();
		// Erase the 1st|Left operand; we've already resolved it and have the result
		expr_tkn_stream.erase (expr_tkn_stream.begin() + opr8r_idx + 1);

		if (is_left_true)	{
			// 1st|Left [operand|expression] evaluated TRUE; short-circuit 2nd|Right side

			int last_idx_sub_expr;
			if (OK != getEndOfSubExprIdx (expr_tkn_stream, opr8r_idx + 1, last_idx_sub_expr))	{
				SET_FAILED_ON_SRC_LINE;
			} else	{
				// Erase the 2nd|Right expression|operand
				if (last_idx_sub_expr == opr8r_idx + 1)
					expr_tkn_stream.erase(expr_tkn_stream.begin() + last_idx_sub_expr);
				else
					expr_tkn_stream.erase(expr_tkn_stream.begin() + opr8r_idx + 1, expr_tkn_stream.begin() + last_idx_sub_expr + 1);
				expr_tkn_stream[opr8r_idx] = *one_tkn;
			}
		
		} else {
			// Evaluate 2nd|Right [operand|expression] for final [TRUE|FALSE]
			bool is_right_true = false;

			if (OK != execFlatExpr_OLR(expr_tkn_stream, opr8r_idx + 1, expected_ret_tkn_cnt))	{
				// Resolve the 2nd expression
				SET_FAILED_ON_SRC_LINE;
			
			} else if (expr_tkn_stream[opr8r_idx + 1].evalResolvedTokenAsIf())	{
				// 2nd expression evaluated as TRUE
				is_right_true = true;
			} 

			if (!failed_on_src_line)	{
				expr_tkn_stream.erase(expr_tkn_stream.begin() + opr8r_idx + 1);
				if (is_right_true)
					expr_tkn_stream[opr8r_idx] = *one_tkn;
				else
					expr_tkn_stream[opr8r_idx] = *zero_tkn;
			}
		}
	}

	if (!failed_on_src_line)	{
		expr_tkn_stream[opr8r_idx].isInitialized = true;
		ret_code = OK;
	}

	return (ret_code);

}


/* ****************************************************************************
 * ? (ternaryConditional) (truePath) (falsePath)
 * Resolve the ? conditional; determine which of [TRUE|FALSE] path will be taken
 * and short-circuit the other path
 * ***************************************************************************/
int RunTimeInterpreter::execTernary1stOp(std::vector<Token> & flat_expr_tkns, int opr8r_idx)     {
	int ret_code = GENERAL_FAILURE;
  int expected_ret_tkn_cnt;

	if (OK != execFlatExpr_OLR(flat_expr_tkns, opr8r_idx + 1, expected_ret_tkn_cnt))	{
		// Resolve the conditional
		SET_FAILED_ON_SRC_LINE;
	
	} else {
		illustrativeB4op (flat_expr_tkns, opr8r_idx);

		bool is_tern_cond_true = flat_expr_tkns[opr8r_idx + 1].evalResolvedTokenAsIf();
		// Remove TERNARY_1ST and conditional result from list
		flat_expr_tkns.erase(flat_expr_tkns.begin() + opr8r_idx, flat_expr_tkns.begin() + opr8r_idx + 2);
		int last_idx_sub_expr;

		if (is_tern_cond_true)	{
			if (flat_expr_tkns[opr8r_idx].tkn_type == EXEC_OPR8R_TKN && OK != execFlatExpr_OLR(flat_expr_tkns, opr8r_idx, expected_ret_tkn_cnt))	
				// Must resolve the TRUE path 
				SET_FAILED_ON_SRC_LINE;
		
			if (!failed_on_src_line && OK != getEndOfSubExprIdx (flat_expr_tkns, opr8r_idx + 1, last_idx_sub_expr))
				// Determine the end of the FALSE path
				SET_FAILED_ON_SRC_LINE;

			else if (!failed_on_src_line)	{
				// Short-circuit the FALSE path
				if (last_idx_sub_expr == opr8r_idx + 1)
					flat_expr_tkns.erase(flat_expr_tkns.begin() + last_idx_sub_expr);
				else
					flat_expr_tkns.erase(flat_expr_tkns.begin() + opr8r_idx + 1, flat_expr_tkns.begin() + last_idx_sub_expr + 1);
			}

		} else {
			// Determine the end of the TRUE path sub-expression. NOTE: [?] and [conditional] have already been removed
			if (OK != getEndOfSubExprIdx (flat_expr_tkns, opr8r_idx, last_idx_sub_expr))
				SET_FAILED_ON_SRC_LINE;
			else	{
				// Short-circuit the TRUE path
				flat_expr_tkns.erase(flat_expr_tkns.begin() + opr8r_idx, flat_expr_tkns.begin() + last_idx_sub_expr + 1);
				if (flat_expr_tkns[opr8r_idx].tkn_type == EXEC_OPR8R_TKN)	{
					if (OK != execFlatExpr_OLR(flat_expr_tkns, opr8r_idx, expected_ret_tkn_cnt))
						// Resolving the FALSE path failed
						SET_FAILED_ON_SRC_LINE;
				}
			}
		}
	}

	if (!failed_on_src_line)	{
		illustrativeAfterOp(flat_expr_tkns);
		ret_code = OK;
	}

	return (ret_code);
}

/* ****************************************************************************
 * Figure out where this sub-expression ends by tallying up OPR8Rs and operands.
 * The stream of Tokens that represent the expression follow [OPR8R][1][2] where
 * [1] and [2] are expressions. If either starts with something that can be
 * evaluated right away (e.g. NOT an EXEC_OPR8R_TKN), then there's no nested
 * expressions that need to be resolved 1st. Since we know how many operands each
 * OPR8R requires, we can determine when all the nested expressions have bottomed
 * out.  This fxn allows us to short circuit sub-expressions that don't need to
 * be evaluated.
 * ***************************************************************************/
int RunTimeInterpreter::getEndOfSubExprIdx (std::vector<Token> & expr_tkn_stream, int start_idx, int & last_idx_sub_expr)	{
	int ret_code = GENERAL_FAILURE;

	int fnd_rand_cnt = 0;
	int req_rand_cnt = 0;
	int idx;
	last_idx_sub_expr = -1;
	std::vector<int> opr8rReqStack;
	std::vector<int> operandStack;

/* 	if (start_idx >= 0 && start_idx < expr_tkn_stream.size() && expr_tkn_stream[start_idx].tkn_type != EXEC_OPR8R_TKN)
		// Starting off with a variable, so this sub-expression is ALREADY complete
		last_idx_sub_expr = start_idx;
 */
	for (idx = start_idx; idx < expr_tkn_stream.size() && !failed_on_src_line && last_idx_sub_expr == -1; idx++)	{
		Token curr_tkn = expr_tkn_stream[idx];

		if (curr_tkn.tkn_type == EXEC_OPR8R_TKN)	{
			// Get details on this OPR8R to determine how it affects resolvedRandCnt
			Operator opr8r_deets;
			if (OK != exec_terms.getExecOpr8rDetails(curr_tkn._unsigned, opr8r_deets))	{
				SET_FAILED_ON_SRC_LINE;

			} else  if (opr8r_deets.type_mask & TERNARY_2ND)	{
				// TERNARY_2ND was *NOT* expected!
				SET_FAILED_ON_SRC_LINE;

			} else	{
				opr8rReqStack.push_back(opr8r_deets.numReqExecOperands);
			}
		} else {
			// TODO: Will have to deal with fxn calls at some point
			operandStack.push_back(1);

			bool is_all_ops_done = false;

			while (!is_all_ops_done)	{
				if (opr8rReqStack.empty())	{
					is_all_ops_done = true;
				
				} else	{
					int top_idx = opr8rReqStack.size() - 1;
					int num_req_rands = opr8rReqStack[top_idx];

					if (operandStack.size() >= num_req_rands)	{
						// Remove the OPR8R
						opr8rReqStack.pop_back();
						for (int edx = 0; edx < num_req_rands; edx++)	{
							// Remove Operands associated with this OPR8R
							operandStack.pop_back();
						}
						// Need a RESULT Operand marker put back
						operandStack.push_back(1);
					
					} else {
						is_all_ops_done = true;
					}
				}
			}

			if (opr8rReqStack.empty())	{
				if (operandStack.size() == 1)
					last_idx_sub_expr = idx;
				else
				 	SET_FAILED_ON_SRC_LINE;
			} 
		}
	}

	if (last_idx_sub_expr >= 0 && last_idx_sub_expr < expr_tkn_stream.size())	{
		ret_code = OK;
	}

	return (ret_code);
}

/* ****************************************************************************
 * TODO: Might need to differentiate between compile and interpret mode.  Depends
 * on where|when I'm doing final type and other bounds checking.
 * ***************************************************************************/
int RunTimeInterpreter::execOperation (Operator opr8r, int opr8r_idx, std::vector<Token> & flat_expr_tkns)	{
	int ret_code = GENERAL_FAILURE;

	if (opr8r.op_code == POST_INCR_OPR8R_OPCODE || opr8r.op_code == POST_DECR_OPR8R_OPCODE 
		|| opr8r.op_code == PRE_INCR_OPR8R_OPCODE || opr8r.op_code == PRE_DECR_OPR8R_OPCODE)	{
		ret_code = execPrePostFixOp (flat_expr_tkns, opr8r_idx);

	} else if ((opr8r.type_mask & UNARY) && opr8r.numReqExecOperands == 1)	{
		ret_code = execUnaryOp (flat_expr_tkns, opr8r_idx); 
		if (OK != ret_code)
			user_messages->logMsg (INTERNAL_ERROR, L"Failed executing UNARY OPR8R [" + opr8r.symbol + L"]"
				, this_src_file, __LINE__, 0);

	} else if (opr8r.type_mask & TERNARY_2ND)	{
		user_messages->logMsg (INTERNAL_ERROR, L"Unexpected TERNARY_2ND", this_src_file, __LINE__, 0);

	} else if ((opr8r.type_mask & BINARY) && opr8r.numReqExecOperands == 2)	{
		ret_code = execBinaryOp (flat_expr_tkns, opr8r_idx);
	} else	{
		user_messages->logMsg (INTERNAL_ERROR, L"", this_src_file, __LINE__, 0);
	}

	return (ret_code);

}

/* ****************************************************************************
 * For ILLUSTRATIVE display purposes. Before performing an operation, show the 
 * current Token list with a caret [^] below that line showing which OPR8R will
 * be executed next.
 * ***************************************************************************/
 void RunTimeInterpreter::illustrativeB4op (std::vector<Token> & flat_expr_tkns, int opr8r_idx)	{
	int caret_pos = -1;
	std::wstring tmp_str;

	if (is_illustrative)	{
		if (tkns_illustrative_str.empty())	{
			tkns_illustrative_str = util.getTokenListStr(flat_expr_tkns, opr8r_idx, caret_pos);
			std::wcout << tkns_illustrative_str << std::endl;
			caret_pos >= 0 ? caret_pos++ : 1;

		} else {
			// Calc caret pos on Token list displayed after previous operation
			int nxt_brkt_pos = -1;
			int brkt_cnt = 0;
			int curr_pos = 0;
			
			while (caret_pos < 0)	{
				nxt_brkt_pos = tkns_illustrative_str.find(L"[", curr_pos);
				if (nxt_brkt_pos != std::string::npos)	{
					brkt_cnt++;
					curr_pos = nxt_brkt_pos + 1;
				}
				else	{
				 	break;
				}
				
				if (brkt_cnt == opr8r_idx + 1)
					caret_pos = nxt_brkt_pos + 1;
			}
		}

		if (caret_pos >= 0)	{
			// Put the ^ underneath the target OPR8R from the previous line
			tmp_str.insert (0, caret_pos, ' ');
			tmp_str.append(L"^ ");
			if (opr8r_idx >= 0 && opr8r_idx < flat_expr_tkns.size())	{
				Operator opr8r;
				if (OK == exec_terms.getExecOpr8rDetails (flat_expr_tkns[opr8r_idx]._unsigned, opr8r))	{
					int rand_cnt = opr8r.numReqExecOperands;
					if (!opr8r.description.empty())
						tmp_str.append (opr8r.description);
          else
            tmp_str.append (L"Use");

					tmp_str.append (L" next ");
					tmp_str.append (std::to_wstring(rand_cnt));
					tmp_str.append (L" sequential operand(s); replace w/ result");

					if (opr8r.op_code == TERNARY_1ST_OPR8R_OPCODE)	{
						tmp_str.append (L"; [Conditional][TRUE path][FALSE path]");
					}
				} else if (flat_expr_tkns[opr8r_idx].tkn_type == SYSTEM_CALL_TKN) {
          tmp_str.append (L"system call consumes next ");
          std::vector<uint8_t> param_list;
          TokenTypeEnum data_type;
          exec_terms.get_system_call_details(flat_expr_tkns[opr8r_idx]._string, param_list, data_type);
          tmp_str.append (std::to_wstring(param_list.size()));
					tmp_str.append (L" sequential operand(s); replace w/ result (");
          Token tmp_type_tkn (data_type, L"");
          tmp_str.append (tmp_type_tkn.get_type_str(true));
          tmp_str.append (L")");
        }
			}
			std::wcout << tmp_str << std::endl;
		}
	}
}

/* ****************************************************************************
 * 
 * ***************************************************************************/
 void RunTimeInterpreter::illustrativeAfterOp (std::vector<Token> & flat_expr_tkns)	{

	if (is_illustrative)	{
		int caret_pos = 0;
		std::wstring prevStr = tkns_illustrative_str;
		tkns_illustrative_str = util.getTokenListStr(flat_expr_tkns, 0, caret_pos);

		if (tkns_illustrative_str != prevStr)	{
			if (flat_expr_tkns.size() == 1)
				tkns_illustrative_str.append(L" expression resolved");
			std::wcout << tkns_illustrative_str << std::endl;
		}
	}
}

/* ****************************************************************************
 * Determine if the element at the current index in our flat expression list is
 * either an operator or a system call with all the resolved, sequential operands
 * it needs to do something useful
 * ***************************************************************************/
 int RunTimeInterpreter::check_expr_element_is_ready (std::vector<Token> & flat_expr_tkns, int curr_idx, bool & is_ready) {
  int ret_code = GENERAL_FAILURE;

  int num_req_seq_rands = INT_MAX;
  bool is_actor = false;

  if (flat_expr_tkns[curr_idx].tkn_type == EXEC_OPR8R_TKN) {
    Operator opr8r;
    if (OK != exec_terms.getExecOpr8rDetails (flat_expr_tkns[curr_idx]._unsigned, opr8r))	{
      SET_FAILED_ON_SRC_LINE;
    
    } else if (opr8r.op_code == TERNARY_1ST_OPR8R_OPCODE)	{
      // [?] is a special case, AND there is the potential for short-circuiting
      is_ready = true;
      ret_code = OK;

    } else if (opr8r.op_code == LOGICAL_AND_OPR8R_OPCODE)	{
      // Special case this OPR8R in case there is short circuiting
      is_ready = true;
      ret_code = OK;

    } else if (opr8r.op_code == LOGICAL_OR_OPR8R_OPCODE)	{
      is_ready = true;
      ret_code = OK;

    } else {
      is_actor = true;
      num_req_seq_rands = opr8r.numReqExecOperands;
    }
  
  } else if (flat_expr_tkns[curr_idx].tkn_type == SYSTEM_CALL_TKN) {
    if (OK != exec_terms.get_num_sys_call_parameters (flat_expr_tkns[curr_idx]._string, num_req_seq_rands))  {
      SET_FAILED_ON_SRC_LINE;
    
    } else  {
     is_actor = true;
    }
  } else if (flat_expr_tkns[curr_idx].isDirectOperand() || flat_expr_tkns[curr_idx].tkn_type == USER_WORD_TKN) {
    ret_code = OK;
  
  } else {
    SET_FAILED_ON_SRC_LINE;
  }

  if (is_actor && !is_ready) {
    // Now check to see if we have enough resolved operands to execute the operator|system call|user_fxn_call (future)
    if (curr_idx + num_req_seq_rands >= flat_expr_tkns.size())  {
      SET_FAILED_ON_SRC_LINE;
      // TODO:
      exec_terms.dumpTokenList(flat_expr_tkns, this_src_file, __LINE__);

    } else {
      int num_fnd_seq_rands;
      for (num_fnd_seq_rands = 0; !failed_on_src_line && num_fnd_seq_rands < num_req_seq_rands;) {
        Token nxt_tkn = flat_expr_tkns[(curr_idx + 1) + num_fnd_seq_rands];
        if (nxt_tkn.tkn_type == EXEC_OPR8R_TKN || nxt_tkn.tkn_type == SYSTEM_CALL_TKN)
          break;
        else
          num_fnd_seq_rands++;
      }
  
      if (num_fnd_seq_rands == num_req_seq_rands)
        is_ready = true;

      ret_code = OK;
    }
  }

  return ret_code;
}

/* ****************************************************************************
 * Execute the operation, system call or user defined fxn in our flat expression
 * list.
 * ***************************************************************************/
 int RunTimeInterpreter::exec_flat_expr_list_element (std::vector<Token> & flat_expr_tkns, int exec_idx) {
  int ret_code = GENERAL_FAILURE;

  TokenTypeEnum exec_tkn_type = flat_expr_tkns[exec_idx].tkn_type;
  // Make a string with a caret to point to the OPR8R that's going to get executed....search for N "[" for placement
  illustrativeB4op (flat_expr_tkns, exec_idx);

  if (exec_tkn_type == EXEC_OPR8R_TKN)  {
    Operator opr8r;
    if (OK != exec_terms.getExecOpr8rDetails (flat_expr_tkns[exec_idx]._unsigned, opr8r))	{
      SET_FAILED_ON_SRC_LINE;
    
    } else if (opr8r.op_code == TERNARY_1ST_OPR8R_OPCODE)	{
      // [?] is a special case, AND there is the potential for short-circuiting
      if (OK != execTernary1stOp (flat_expr_tkns, exec_idx))
        SET_FAILED_ON_SRC_LINE;
      else
        ret_code = OK;
    
    } else if (opr8r.op_code == LOGICAL_AND_OPR8R_OPCODE)	{
      // Special case this OPR8R in case there is short circuiting
      if (OK != exec_logical_and (flat_expr_tkns, exec_idx))
        SET_FAILED_ON_SRC_LINE;
      else
        ret_code = OK;

    } else if (opr8r.op_code == LOGICAL_OR_OPR8R_OPCODE)	{
      if (OK != exec_logical_or (flat_expr_tkns, exec_idx))
        SET_FAILED_ON_SRC_LINE;
      else
        ret_code = OK;

    } else {
      if (OK != execOperation (opr8r, exec_idx, flat_expr_tkns))	{
        SET_FAILED_ON_SRC_LINE;
      } else	{
        // Operation result stored in Token that previously held the OPR8R. We need to delete any associatd operands
        flat_expr_tkns.erase(flat_expr_tkns.begin() + exec_idx + 1, flat_expr_tkns.begin() + exec_idx + opr8r.numReqExecOperands + 1);
        ret_code = OK;
      }
    }
  } else if (exec_tkn_type == SYSTEM_CALL_TKN)	{
    if (OK != exec_system_call(flat_expr_tkns, exec_idx))
      SET_FAILED_ON_SRC_LINE;
    else
      ret_code = OK;

  } else {
    SET_FAILED_ON_SRC_LINE;
  }

  if (ret_code == OK)
    illustrativeAfterOp (flat_expr_tkns);

  return ret_code;
}

/* ****************************************************************************
 * Fxn to evaluate expressions.  
 * NOTE the start_idx parameter. Fxn can also be called recursively to resolve
 * sub-expressions further down the expression stream. Useful for resolving the
 * TERNARY_1ST conditional, and short-circuiting either the TERNARY [TRUE|FALSE]
 * paths after the conditional is resolved.  Also useful for short-circuiting
 * the 2nd expression in the [&&] and [||] OPR8Rs.
 * ***************************************************************************/
int RunTimeInterpreter::execFlatExpr_OLR(std::vector<Token> & flat_expr_tkns, int start_idx, int & expected_tkn_cnt)     {
	int ret_code = GENERAL_FAILURE;
  expected_tkn_cnt = 1;
	bool is_1_rand_left = false;
	int prev_tkn_cnt = flat_expr_tkns.size();
	int currTknCnt;
	int fnd_seq_rands = 0;
	int sub_expr_completed_line = 0;

	if (start_idx >= prev_tkn_cnt)	{
		SET_FAILED_ON_SRC_LINE;
		user_messages->logMsg (INTERNAL_ERROR, L"Parameter start_idx goes beyond Token stream", this_src_file, failed_on_src_line, 0);

	} else if (flat_expr_tkns[start_idx].tkn_type != EXEC_OPR8R_TKN && flat_expr_tkns[start_idx].tkn_type != SYSTEM_CALL_TKN)	{
		// 1st thing we hit is an Operand, so we're done! 
		sub_expr_completed_line = __LINE__;

	} else 	{

    if (flat_expr_tkns[start_idx].tkn_type == SYSTEM_CALL_TKN)  {
      // Check expected return data type. If it's a void, let our caller know by adjusting
      // expected_tkn_cnt
      TokenTypeEnum ret_data_type;
      std::vector<uint8_t> param_list;
      if (OK != exec_terms.get_system_call_details (flat_expr_tkns[start_idx]._string, param_list, ret_data_type))
        SET_FAILED_ON_SRC_LINE;
      else if (ret_data_type == VOID_TKN)
        expected_tkn_cnt = 0;
    }
		int opr8r_cnt = 0;
		bool is_out_of_tkns = false;
		int caret_pos;

		while (!is_out_of_tkns && !failed_on_src_line && !sub_expr_completed_line)	{
			prev_tkn_cnt = flat_expr_tkns.size();
			bool is_1_opr8r_done = false;
			int curr_idx = start_idx;
      bool is_ready_to_exec = false;

 			while (!is_out_of_tkns && !is_1_opr8r_done && !failed_on_src_line)	{
				if (curr_idx >= flat_expr_tkns.size() - 1) {
					is_out_of_tkns = true;
          break;
        }

        if (OK != check_expr_element_is_ready (flat_expr_tkns, curr_idx, is_ready_to_exec)) {
          SET_FAILED_ON_SRC_LINE;
        
        } else if (is_ready_to_exec)  {
          if (OK != exec_flat_expr_list_element (flat_expr_tkns, curr_idx))
            SET_FAILED_ON_SRC_LINE;
          else
            is_1_opr8r_done = true;
        
        } else {
					curr_idx++;
					if (curr_idx > flat_expr_tkns.size())
						SET_FAILED_ON_SRC_LINE;
        }
			}

			if (!failed_on_src_line && curr_idx == start_idx)	{
				// We completed the sub-expression we were tasked with
				sub_expr_completed_line = __LINE__;
			
			} else if (!failed_on_src_line && prev_tkn_cnt <= flat_expr_tkns.size())	{
				// Inner while made ZERO forward progress.....time to quit and think about what we havn't done
				SET_FAILED_ON_SRC_LINE;
				std::wstring devMsg = L"Inner while did NOT make forward progress; start_idx = ";
				devMsg.append(std::to_wstring(start_idx));
				devMsg.append (L"; currIdx = ");
				devMsg.append (std::to_wstring(curr_idx));
				devMsg.append (L";");
				user_messages->logMsg (INTERNAL_ERROR, devMsg, this_src_file, failed_on_src_line, 0);
				exec_terms.dumpTokenList (flat_expr_tkns, this_src_file, __LINE__);
			}
		}
	}

	if (sub_expr_completed_line && !failed_on_src_line && flat_expr_tkns[start_idx].tkn_type == USER_WORD_TKN)	{
		// SUCCESS IFF we can resolve this variable from our NameSpace to its final value
		Token resolved_tkn;
		std::wstring lookUpMsg;
		if (OK == scope_name_space->findVar(flat_expr_tkns[start_idx]._string, 0, resolved_tkn, READ_ONLY, lookUpMsg))	{
			flat_expr_tkns[start_idx] = resolved_tkn;
			flat_expr_tkns[start_idx].isInitialized = true;
			ret_code = OK;
		
		} else if (!lookUpMsg.empty())	{
			user_messages->logMsg(INTERNAL_ERROR, lookUpMsg, this_src_file, __LINE__, 0);
		}
	} else if (sub_expr_completed_line && !failed_on_src_line && flat_expr_tkns[start_idx].tkn_type != EXEC_OPR8R_TKN)	{
		if (flat_expr_tkns[start_idx].isDirectOperand())
			flat_expr_tkns[start_idx].isInitialized = true;

    ret_code = OK;
	}

	if (ret_code != OK && !failed_on_src_line)	{
		user_messages->logMsg (INTERNAL_ERROR, L"Unhandled error!", this_src_file, __LINE__, 0);
		// See the final result
		exec_terms.dumpTokenList (flat_expr_tkns, this_src_file, __LINE__);
	}

	return (ret_code);
}

/* ****************************************************************************
 * Publicly facing fxn
 * ***************************************************************************/
int RunTimeInterpreter::resolveFlatExpr(std::vector<Token> & flat_expr_tkns, int & expected_tkn_cnt)     {
	int ret_code = GENERAL_FAILURE;

	if (0 == flat_expr_tkns.size())	{
		user_messages->logMsg (INTERNAL_ERROR, L"Token stream unexpectedly EMPTY!", this_src_file, __LINE__, 0);
	
	} else	{
		tkns_illustrative_str.clear();
		ret_code = execFlatExpr_OLR(flat_expr_tkns, 0, expected_tkn_cnt);
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::resolveTknOrVar (Token & original_tkn, Token & resolved_tkn, std::wstring & var_name, bool is_check_init)	{
	int ret_code = GENERAL_FAILURE;

	if (original_tkn.tkn_type == USER_WORD_TKN)	{
		var_name = original_tkn._string;
		std::wstring lookup_msg;
		if (OK != scope_name_space->findVar(var_name, 0, resolved_tkn, READ_ONLY, lookup_msg))	{
			user_messages->logMsg(INTERNAL_ERROR, lookup_msg, this_src_file, __LINE__, 0);

		} else	{
			if (is_check_init && usage_mode == COMPILE_TIME && !resolved_tkn.isInitialized)	{
				user_messages->logMsg(WARNING, L"Uninitialized variable used - " + original_tkn.descr_sans_line_num_col()
					, usr_src_file_name, original_tkn.get_line_number(), original_tkn.get_column_pos());
			}
			ret_code = OK;
		}
	} else	{
		resolved_tkn = original_tkn;
		ret_code = OK;
	}


	return (ret_code);

}


/* ****************************************************************************
 *
 * ***************************************************************************/
int RunTimeInterpreter::resolveTknOrVar (Token & original_tkn, Token & resolved_tkn, std::wstring & varName)	{
	return resolveTknOrVar(original_tkn, resolved_tkn, varName, true);
}

/* ****************************************************************************
 * Encountered the IF_SCOPE_OPCODE. Evaluate the conditional to determine if 
 * the enclosed block will be executed.  Check for follow on [else if] and|or
 * [else] blocks at the same scope. We'll need to know where the scope that
 * encloses this [if] block ends.
 * [op_code][total_length][conditional EXPRESSION][code block]
 * ***************************************************************************/
int RunTimeInterpreter::exec_if_block (uint32_t scope_start_pos, uint32_t if_scope_len
	, uint32_t after_parent_scope_pos, uint32_t & break_scope_end_pos)	{
	
	int ret_code = GENERAL_FAILURE;
  break_scope_end_pos = 0;

	bool is_else_blocks_done = false;
	bool is_if_cond_true = false;
	uint32_t pos_after_else_scope = 0;
	uint8_t curr_obj_op_code = INVALID_OPCODE;
	uint32_t curr_obj_start_pos = 0;
	uint32_t curr_obj_len = 0;
	uint32_t nxt_obj_start_pos = 0;
	Token if_cond_tkn;

	if (OK != execExpression(scope_start_pos + OPCODE_NUM_BYTES + NUM_BYTES_IN_DWORD, if_cond_tkn))	{
		SET_FAILED_ON_SRC_LINE;
	
	} else if (if_cond_tkn.evalResolvedTokenAsIf())	{
		// [if] condition is TRUE, so execute the code block
		is_if_cond_true = true;
		if (OK != execCurrScope(file_reader.getPos(), scope_start_pos + if_scope_len, break_scope_end_pos))
			SET_FAILED_ON_SRC_LINE;


	} else {
		// [if] condition is FALSE, so jump around the [if] block
		file_reader.setPos(scope_start_pos + if_scope_len);
	}

	if (!failed_on_src_line)	{
		bool is_skip_block;

		while (!is_else_blocks_done && !failed_on_src_line)	{
			// Consume any [else if][else] blocks after our initial [if] block
			curr_obj_start_pos = file_reader.getPos();
			is_skip_block = false;

			if (curr_obj_start_pos == after_parent_scope_pos)	{
				// We've just gone past the parent scope that contains the [if] block
				// and any chained [else if]+, [else] blocks
				is_else_blocks_done = true;

			} else if (OK != file_reader.peekNextByte(curr_obj_op_code))	{
				SET_FAILED_ON_SRC_LINE;

			} else if (curr_obj_op_code != ELSE_IF_SCOPE_OPCODE && curr_obj_op_code != ELSE_SCOPE_OPCODE)	{
				is_else_blocks_done = true;

			} else 	{
				if (OK != file_reader.readNextByte(curr_obj_op_code))	
					SET_FAILED_ON_SRC_LINE;
				else if (OK != file_reader.readNextDword(curr_obj_len))
					SET_FAILED_ON_SRC_LINE;

				if (!failed_on_src_line && curr_obj_op_code == ELSE_IF_SCOPE_OPCODE)	{
					is_skip_block = is_if_cond_true;

					if (!is_skip_block)	{
						// Test the conditional
						Token elseIfConditional;
						if (OK != execExpression(curr_obj_start_pos + OPCODE_NUM_BYTES + NUM_BYTES_IN_DWORD, elseIfConditional))	{
							SET_FAILED_ON_SRC_LINE;
						
						} else if (elseIfConditional.evalResolvedTokenAsIf())	{
							// [else if] condition is TRUE, so execute the code block
							is_if_cond_true = true;
							if (OK != execCurrScope(file_reader.getPos(), curr_obj_start_pos + curr_obj_len, break_scope_end_pos))
								SET_FAILED_ON_SRC_LINE;
						
						} else	{
							is_skip_block = true;
						}
					}
				
				} else if (!failed_on_src_line && curr_obj_op_code == ELSE_SCOPE_OPCODE)	{
					is_else_blocks_done = true;
					if (is_if_cond_true)	{
						is_skip_block = true;
					
					} else {
						if (OK != execCurrScope(file_reader.getPos(), curr_obj_start_pos + curr_obj_len, break_scope_end_pos))
							SET_FAILED_ON_SRC_LINE;
					}
				}
			}

			if (!failed_on_src_line && is_skip_block)	{
				nxt_obj_start_pos = curr_obj_start_pos + curr_obj_len;
				if (nxt_obj_start_pos >= after_parent_scope_pos)	
					is_else_blocks_done = true;
				if (OK != file_reader.setPos(nxt_obj_start_pos))
					// TODO: What about EOF? Or past scope?
					SET_FAILED_ON_SRC_LINE;

			}
		}
	}

	if (!failed_on_src_line && is_else_blocks_done)
		ret_code = OK;

	// TODO: Why was this here?  Did I miss a position check?
  // uint32_t finalFilePos = file_reader.getPos();

	return (ret_code);
}

/* ****************************************************************************
 * Extract expression from variable declaration
 * VARIABLES_DECLARATION_OPCODE    0x6F  
 * [op_code][total_length][datatype op_code][[string var_name][init_expression]]+
 * ***************************************************************************/
int RunTimeInterpreter::get_expr_from_var_declaration (uint32_t start_pos, std::vector<Token> & expr_tkn_list)  {
  int ret_code = GENERAL_FAILURE;
  uint32_t var_name_len = 0;

  //                                        [op_code]          [total_length]       [datatype op_code][[string var_name][init_expression]]+
  uint32_t var_name_start_pos = start_pos + OPCODE_NUM_BYTES + NUM_BYTES_IN_DWORD + OPCODE_NUM_BYTES;
  if (OK != file_reader.setPos(var_name_start_pos + OPCODE_NUM_BYTES) || OK != file_reader.readNextDword(var_name_len)) {
    SET_FAILED_ON_SRC_LINE;
  
  } else if (OK != file_reader.setPos(var_name_start_pos + var_name_len)) {
    SET_FAILED_ON_SRC_LINE;

  } else if (OK != file_reader.readExprIntoList(expr_tkn_list))  {
    SET_FAILED_ON_SRC_LINE;

  } else {
    ret_code = OK;
  }

  return ret_code;
}

/* ****************************************************************************
 * 
 * ***************************************************************************/
 int RunTimeInterpreter::exec_cached_expr (std::vector<Token> expr_tkn_list, bool & is_result_true) {
  int ret_code = GENERAL_FAILURE;
  Token result_tkn;
  int expected_ret_tkn_cnt;

  if (OK == resolveFlatExpr(expr_tkn_list, expected_ret_tkn_cnt) && expected_ret_tkn_cnt == 1
    && expr_tkn_list.size() == 1) {
    is_result_true = expr_tkn_list[0].evalResolvedTokenAsIf();
    ret_code = OK;
  }

  return ret_code;

}

/* ****************************************************************************
 * FOR_SCOPE_OPCODE 0x6D
 * [op_code][total_length][init_expression][conditional_expression][last_expression][code_block]
 * ***************************************************************************/
 int RunTimeInterpreter::exec_for_loop (uint32_t for_scope_start, uint32_t for_scope_len, uint32_t after_parent_scope_pos
  , uint32_t & break_scope_end_pos)  {
  
  int ret_code = GENERAL_FAILURE;
  break_scope_end_pos = 0;
  uint8_t op_code;
  uint32_t init_expr_pos = for_scope_start + OPCODE_NUM_BYTES + NUM_BYTES_IN_DWORD;
  uint32_t cond_expr_pos, cond_expr_len, last_expr_pos, last_expr_len;
  Token conditional_result, empty_tkn;
  uint32_t init_expr_len;
  std::vector<Token> cond_expr_tkn_list;
  std::vector<Token> last_expr_tkn_list;
  bool is_for_scopened = false;
  int num_for_loops_done = 0;

  if (OK == file_reader.setPos(init_expr_pos) 
    && OK == file_reader.readNextByte(op_code)) {
    // Exec initialization expression OR variable declaration

    if (OK == scope_name_space->openNewScope(FOR_SCOPE_OPCODE, empty_tkn, for_scope_start, for_scope_len))
      is_for_scopened = true;
    else
      SET_FAILED_ON_SRC_LINE;
    
    if (!failed_on_src_line && op_code == VARIABLES_DECLARATION_OPCODE)  { 
      // Need to put variables into for loop's scope
      if (OK != file_reader.readNextDword(init_expr_len))
        SET_FAILED_ON_SRC_LINE;
      else if (OK != execVarDeclaration (init_expr_pos, init_expr_len))
        SET_FAILED_ON_SRC_LINE;

    } else if (!failed_on_src_line && op_code == EXPRESSION_OPCODE)  {
      
      if (OK != file_reader.readNextDword(init_expr_len)) 
        SET_FAILED_ON_SRC_LINE;
      
      else if (OK != execExpression (init_expr_pos, conditional_result))
        SET_FAILED_ON_SRC_LINE;
    }
  }

  if (!failed_on_src_line) {
    // Get conditional expression or single variable declaration with init expression and store it in a copy list
    cond_expr_pos = init_expr_pos + init_expr_len;
 
    if (OK != file_reader.setPos(cond_expr_pos) || OK != file_reader.readNextByte(op_code)) {
      SET_FAILED_ON_SRC_LINE;

    } else if (op_code == VARIABLES_DECLARATION_OPCODE)  { 
        // Need to put what should be *SINGLE* variable into for loop's scope
        if (OK != file_reader.readNextDword(cond_expr_len))  {
          SET_FAILED_ON_SRC_LINE;
        
        }  else if (OK != execVarDeclaration (cond_expr_pos, cond_expr_len))  {
          SET_FAILED_ON_SRC_LINE;

        } else if (OK != get_expr_from_var_declaration (cond_expr_pos, cond_expr_tkn_list))  {
          // Extracting conditional expression from VARIABLES_DECLARATION failed
          SET_FAILED_ON_SRC_LINE;
        }

    } else if (op_code == EXPRESSION_OPCODE)	{
      if (OK != file_reader.setPos(cond_expr_pos + OPCODE_NUM_BYTES))
        SET_FAILED_ON_SRC_LINE;
      else if (OK != file_reader.readNextDword(cond_expr_len))
        SET_FAILED_ON_SRC_LINE;
      else if (OK != file_reader.setPos(cond_expr_pos))
        SET_FAILED_ON_SRC_LINE;
      else if (OK != file_reader.readExprIntoList(cond_expr_tkn_list))
        SET_FAILED_ON_SRC_LINE;
  
    } else {
      SET_FAILED_ON_SRC_LINE;
    }
  }

  if (!failed_on_src_line) {
    // Get iterative, end of loop expression into a copy list, cached for execution
    last_expr_pos = cond_expr_pos + cond_expr_len;
    if (OK != file_reader.setPos(last_expr_pos + OPCODE_NUM_BYTES))
      SET_FAILED_ON_SRC_LINE;

    else if (OK != file_reader.readNextDword(last_expr_len))
      // Grab this object's length  
      SET_FAILED_ON_SRC_LINE;

    else if (OK != file_reader.setPos(last_expr_pos))
      SET_FAILED_ON_SRC_LINE;
  
    else if (OK != file_reader.readExprIntoList(last_expr_tkn_list))
      // Grab the last expression and cache it in our copy list
      SET_FAILED_ON_SRC_LINE;

  }

  if (!failed_on_src_line) {
    // Determine location of 1st expression AFTER the for loop control constructs
    uint32_t code_block_start_pos = last_expr_pos + last_expr_len;
    bool is_for_cond_true = true, tmp_bool;
    uint32_t for_scope_end_boundary_pos = for_scope_start + for_scope_len;


    while (is_for_cond_true && !failed_on_src_line) {
      // Execute the conditional expression at top of loop
      if (cond_expr_tkn_list.empty())  {
        // An empty conditional expression is OK. Hopefully the compiler checked for a [break] statement inside the loop
        is_for_cond_true = true;
      
      } else if (OK != exec_cached_expr (cond_expr_tkn_list, is_for_cond_true))  {
        SET_FAILED_ON_SRC_LINE;

      } else if (is_for_cond_true) {
        if (OK != execCurrScope(code_block_start_pos, for_scope_end_boundary_pos, break_scope_end_pos)) {
          SET_FAILED_ON_SRC_LINE;
        
        } else if (break_scope_end_pos == for_scope_end_boundary_pos) {
          // We're [break]ing out of this current [for] loop; no need to bubble up
          is_for_cond_true = false;
          break_scope_end_pos = 0;

        } else if (break_scope_end_pos > for_scope_end_boundary_pos)  {
          // [break] out of enclosing loop; preserve break_scope_end_pos to bubble info up
          is_for_cond_true = false;
        
        } else {
          // Execute last/iteration expression at end of loop
          if (!last_expr_tkn_list.empty() && OK != exec_cached_expr(last_expr_tkn_list, tmp_bool))
            SET_FAILED_ON_SRC_LINE;

          num_for_loops_done++;
        }
      }
    }
  }

  // Close scope when done
  closeScopeErr closeErr;
  if (is_for_scopened && OK != scope_name_space->closeTopScope (FOR_SCOPE_OPCODE, closeErr, false)) 
    SET_FAILED_ON_SRC_LINE;

  if (!failed_on_src_line)
    ret_code = OK;
 
  return ret_code;
}


/* ****************************************************************************
 * WHILE_SCOPE_OPCODE 0x6C  
 * [op_code][total_length][conditional EXPRESSION][code block]
 * ***************************************************************************/
 int RunTimeInterpreter::exec_while_loop (uint32_t while_scope_start, uint32_t while_scope_len, uint32_t after_parent_scope_pos
  , uint32_t & break_scope_end_pos)  {
  
  int ret_code = GENERAL_FAILURE;
  break_scope_end_pos = 0;
  uint8_t op_code;
  uint32_t cond_expr_pos = while_scope_start + OPCODE_NUM_BYTES + NUM_BYTES_IN_DWORD;
  uint32_t cond_expr_len;
  Token conditional_result, empty_tkn;
  std::vector<Token> cond_expr_tkn_list;
  bool is_while_scopened = false;
  int num_while_loops_done = 0;

  if (OK == file_reader.setPos(cond_expr_pos) 
    && OK == file_reader.readNextByte(op_code)) {
    // Exec initialization expression OR variable declaration

    if (OK == scope_name_space->openNewScope(WHILE_SCOPE_OPCODE, empty_tkn, while_scope_start, while_scope_len))
      is_while_scopened = true;
    else
      SET_FAILED_ON_SRC_LINE;
    
    if (!failed_on_src_line && op_code == EXPRESSION_OPCODE)  {
      
      if (OK != file_reader.setPos(cond_expr_pos + OPCODE_NUM_BYTES))
        SET_FAILED_ON_SRC_LINE;
      else if (OK != file_reader.readNextDword(cond_expr_len))
        SET_FAILED_ON_SRC_LINE;
      else if (OK != file_reader.setPos(cond_expr_pos))
        SET_FAILED_ON_SRC_LINE;
      else if (OK != file_reader.readExprIntoList(cond_expr_tkn_list))
        SET_FAILED_ON_SRC_LINE;

    }
  }

  if (!failed_on_src_line) {
    // Determine location of 1st expression AFTER the while loop control constructs
    uint32_t code_block_start_pos = cond_expr_pos + cond_expr_len;
    bool is_while_cond_true = true, tmp_bool;
    uint32_t while_scope_end_boundary_pos = while_scope_start + while_scope_len;

    while (is_while_cond_true && !failed_on_src_line) {
      // Execute the conditional expression at top of loop
      if (OK != exec_cached_expr (cond_expr_tkn_list, is_while_cond_true))  {
        SET_FAILED_ON_SRC_LINE;

      } else if (is_while_cond_true) {
        if (OK != execCurrScope(code_block_start_pos, while_scope_end_boundary_pos, break_scope_end_pos)) {
          SET_FAILED_ON_SRC_LINE;
        
        } else if (break_scope_end_pos == while_scope_end_boundary_pos) {
          // We're [break]ing out of this current [while] loop; no need to bubble up
          is_while_cond_true = false;
          break_scope_end_pos = 0;

        } else if (break_scope_end_pos > while_scope_end_boundary_pos)  {
          // [break] out of enclosing loop; preserve break_scope_end_pos to bubble info up
          is_while_cond_true = false;
        
        } else {
          num_while_loops_done++;
        
        }
      }
    }
  }

  // Close scope when done
  closeScopeErr closeErr;
  if (is_while_scopened && OK != scope_name_space->closeTopScope (WHILE_SCOPE_OPCODE, closeErr, false)) 
    SET_FAILED_ON_SRC_LINE;

  if (!failed_on_src_line)
    ret_code = OK;

 
  return ret_code;
}

/* ****************************************************************************
 * This fxn acts as a "routing table" to get the proper system call invoked
 * This fxn is a great candidate for extending with derived classes to add 
 * more functionality. 
 * Will re-visit this if there is a future need
 * ***************************************************************************/
 int RunTimeInterpreter::exec_system_call (std::vector<Token> & flat_expr_tkns, int sys_call_idx)  {

  int ret_code = GENERAL_FAILURE;

  if (sys_call_idx >= 0 && sys_call_idx < flat_expr_tkns.size())  {
    std::wstring sys_call = flat_expr_tkns[sys_call_idx]._string;

    if (sys_call == SYS_CALL_STR) {
      ret_code = exec_sys_call_str (flat_expr_tkns, sys_call_idx);
    
    } else if (sys_call == SYS_CALL_PRINT_LINE) {
      ret_code = exec_sys_call_print_line(flat_expr_tkns, sys_call_idx);
    
    }
  }

  return ret_code;

 }

/* ****************************************************************************
 * System call str() will return a string representation for a Tokens of any 
 * valid data type 
 * ***************************************************************************/
 int RunTimeInterpreter::exec_sys_call_str (std::vector<Token> & flat_expr_tkns, int sys_call_idx)  {

  int ret_code = GENERAL_FAILURE;
  std::wstring token_str;

  if (flat_expr_tkns.size() >= sys_call_idx + 2) {
    Token param_tkn = flat_expr_tkns[sys_call_idx + 1];

    if (param_tkn.tkn_type == USER_WORD_TKN) {
			std::wstring lookUpMsg;
  		if (OK != scope_name_space->findVar(param_tkn._string, 0, scratch_tkn, READ_ONLY, lookUpMsg))	{
					user_messages->logMsg (INTERNAL_ERROR, L"Variable " + param_tkn._string + L" was not declared"
						, usr_src_file_name, param_tkn.get_line_number(), param_tkn.get_column_pos());
        SET_FAILED_ON_SRC_LINE;
  		
      } else {
        token_str = scratch_tkn.getValueStr();
      }

    } else {
      token_str = param_tkn.getValueStr();
    }

    if (0 == failed_on_src_line)  {
      flat_expr_tkns[sys_call_idx].resetToString(token_str);
      // Only need to delete 1 item from the list - the single parameter passed to the SYS_CALL_STR
      flat_expr_tkns.erase(flat_expr_tkns.begin() + sys_call_idx + 1);
      ret_code = OK;      
    }
  }

  return ret_code;

 }

 /* ****************************************************************************
 * System call print_line() doesn't have a return value (void), but takes a
 * resolved STRING_TKN and prints it out.
 * ***************************************************************************/
 int RunTimeInterpreter::exec_sys_call_print_line (std::vector<Token> & flat_expr_tkns, int sys_call_idx)  {

  int ret_code = GENERAL_FAILURE;
  std::wstring token_str;

  if (flat_expr_tkns.size() >= sys_call_idx + 2) {
    Token param_tkn = flat_expr_tkns[sys_call_idx + 1];

    if (param_tkn.tkn_type == STRING_TKN) {
      std::wcout << param_tkn._string << std::endl;
      // Delete 2 items from the list - print_line sys_call and the single parameter passed to it
      flat_expr_tkns.erase(flat_expr_tkns.begin() + sys_call_idx, flat_expr_tkns.begin() + sys_call_idx + 2);
      ret_code = OK;      
    }
  }

  return ret_code;

 }
