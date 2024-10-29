/*
 * UserSourceTerms.cpp
 * Overrides BaseLanguageTerms class and additionally holds OPR8R definitions and their
 * precedence in relation to one another.
 *
 *  Created on: Jan 4, 2024
 *      Author: Mike Volk
 */

#include "CompileExecTerms.h"

CompileExecTerms::CompileExecTerms() {
	// TODO Auto-generated constructor stub
  // C operator precedence from https://en.cppreference.com/w/c/language/operator_precedence
	std::wstring curr_opr8r;
	int idx = 0;


	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
	grouped_opr8rs.back().opr8rs.push_back( Operator (L"++", POSTFIX, USR_SRC, 1, 0, INVALID_OPCODE));   // NOTE: Pre-fix and post-fix precedence is different
	grouped_opr8rs.back().opr8rs.push_back ( Operator (L"--", POSTFIX, USR_SRC, 1, 0, INVALID_OPCODE));
	grouped_opr8rs.back().opr8rs.push_back ( Operator (POST_INCR_OPR8R, POSTFIX, GNR8D_SRC, 1, 1, POST_INCR_OPR8R_OPCODE));
	grouped_opr8rs.back().opr8rs.push_back ( Operator (POST_DECR_OPR8R, POSTFIX, GNR8D_SRC, 1, 1, POST_DECR_OPR8R_OPCODE));   // NOTE: Pre-fix and post-fix precedence is different

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
	grouped_opr8rs.back().opr8rs.push_back ( Operator (L"++", PREFIX, USR_SRC, 1, 0, INVALID_OPCODE));
	grouped_opr8rs.back().opr8rs.push_back ( Operator (L"--", PREFIX, USR_SRC, 1, 0, INVALID_OPCODE));
	grouped_opr8rs.back().opr8rs.push_back ( Operator (PRE_INCR_OPR8R, PREFIX, GNR8D_SRC, 1, 1, PRE_INCR_OPR8R_OPCODE));   // NOTE: Pre-fix and post-fix precedence is different
	grouped_opr8rs.back().opr8rs.push_back ( Operator (PRE_DECR_OPR8R, PREFIX, GNR8D_SRC, 1, 1, PRE_DECR_OPR8R_OPCODE));   // NOTE: Pre-fix and post-fix precedence is different

	// TODO: Unary +? What's the point? Completeness? UNARY_PLUS_OPR8R_OPCODE
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"-", UNARY, USR_SRC, 1, 0, INVALID_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (UNARY_MINUS_OPR8R, UNARY, GNR8D_SRC, 1, 1, UNARY_MINUS_OPR8R_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"!", UNARY, (USR_SRC|GNR8D_SRC), 1, 1, LOGICAL_NOT_OPR8R_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"~", UNARY, (USR_SRC|GNR8D_SRC), 1, 1, BITWISE_NOT_OPR8R_OPCODE));

  //  TODO: ()  Function call
  //  TODO: []  Array subscripting

  // TODO: Unary plus and minus has precedence over binary

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"*", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, MULTIPLY_OPR8R_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"/", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, DIV_OPR8R_OPCODE));

	grouped_opr8rs.back().opr8rs.push_back ( Operator (L"%", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, MOD_OPR8R_OPCODE));

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"+", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, BINARY_PLUS_OPR8R_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"-", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, BINARY_MINUS_OPR8R_OPCODE));

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"<<", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, LEFT_SHIFT_OPR8R_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L">>", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, RIGHT_SHIFT_OPR8R_OPCODE));

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"<", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, LESS_THAN_OPR8R_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"<=", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, LESS_EQUALS_OPR8R8_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L">", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, GREATER_THAN_OPR8R_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L">=", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, GREATER_EQUALS_OPR8R8_OPCODE));

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"==", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, EQUALITY_OPR8R_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"!=", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, NOT_EQUALS_OPR8R_OPCODE));

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"&", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, BITWISE_AND_OPR8R_OPCODE));

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
	grouped_opr8rs.back().opr8rs.push_back ( Operator (L"^", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, BITWISE_XOR_OPR8R_OPCODE));

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
	grouped_opr8rs.back().opr8rs.push_back ( Operator (L"|", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, BITWISE_OR_OPR8R_OPCODE));

  // TODO: && and || OPR8Rs have the same precedence.  Is it incorrect to order them
  // absolutely like I've done here?  Should I include some kind of precedence level for
  // each OPR8R to account for this?  PROBABLY!!!!
	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"&&", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, LOGICAL_AND_OPR8R_OPCODE));

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"||", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, LOGICAL_OR_OPR8R_OPCODE));

  //  TODO: ?:  Ternary conditional[note 3] Right-to-left
  //  TODO: Understand diff between Left-to-right and Right-to-left associativity
  //	TODO: # of expected operands for Ternary? Especially since I'll be pushing a new scope when the "?"
  //	opr8r is encountered.
	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());

  // TODO: Is TERNARY OPR8R the two together, e.g. "?:" ?
	// TODO: How many operands should the constructor get? 3? Or is this OPR8R a special case since it
	// opens up a scope and it'll be treated as such anyway?  If so, shouldn't we pass 0?
	// TODO: TERNARY_1ST expects 1 resolved operand for its conditional at run time. Should compile time
	// vs. run time characteristics get stored differently?
	// TODO: Could just special case this instead of adding *another* member variable
	grouped_opr8rs.back().opr8rs.push_back ( Operator (L"?", (BINARY|TERNARY_1ST), (USR_SRC|GNR8D_SRC), 2, 1, TERNARY_1ST_OPR8R_OPCODE));
  // TODO: Should this be BINARY instead?

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
	// TERNARY_2ND OPR8R doesn't follow same pattern in interpreted expression stream. It sits in between and DIVIDES 2 paths/sub-expressions
	// where normal BINARY OPR8Rs will appear AFTER their 2 required operands
	// [TRUE path expression][:][FALSE path expression]
	// [operand1][operand2][OPR8R]
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L":", (BINARY|TERNARY_2ND), (USR_SRC|GNR8D_SRC), 2, 2, TERNARY_2ND_OPR8R_OPCODE));

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"=", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, ASSIGNMENT_OPR8R_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"+=", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, PLUS_ASSIGN_OPR8R_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"-=", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, MINUS_ASSIGN_OPR8R_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"*=", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, MULTIPLY_ASSIGN_OPR8R_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"/=", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, DIV_ASSIGN_OPR8R_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"%=", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, MOD_ASSIGN_OPR8R_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"<<=", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, LEFT_SHIFT_ASSIGN_OPR8R_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L">>=", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, RIGHT_SHIFT_ASSIGN_OPR8R_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"&=", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, BITWISE_AND_ASSIGN_OPR8R_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"^=", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, BITWISE_XOR_ASSIGN_OPR8R_OPCODE));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"|=", BINARY, (USR_SRC|GNR8D_SRC), 2, 2, BITWISE_OR_ASSIGN_OPR8R_OPCODE));

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
	// TODO: Not listed as an OPR8R on cppreference web page.  Does it matter?
	grouped_opr8rs.back().opr8rs.push_back ( Operator (L";", STATEMENT_ENDER, (USR_SRC|GNR8D_SRC), 0, 0, STATEMENT_ENDER_OPR8R_OPCODE));

  // TODO: 15  , Comma

  // NOTE:  ;;; is 3 valid statements in a row, but if we interpreted it as 1 thing,
  // it would be an error
  this->atomic_1char_opr8rs = L";";

  this->_1char_spr8rs = L"()[]{}";
  // TODO: Is a comma a spr8r?  What about the '\' character, for paths?

  this->valid_data_types.insert (L"uint8");
  this->valid_data_types.insert (L"uint16");
  this->valid_data_types.insert (L"uint32");
  this->valid_data_types.insert (L"uint64");
  this->valid_data_types.insert (L"int8");
  this->valid_data_types.insert (L"int16");
  this->valid_data_types.insert (L"int32");
  this->valid_data_types.insert (L"int64");
  this->valid_data_types.insert (L"string");
  this->valid_data_types.insert (L"datetime");
  this->valid_data_types.insert (L"double");

  // TODO: What is the right way to do this?
  validityCheck();
}

CompileExecTerms::~CompileExecTerms() {
	// TODO Auto-generated destructor stub
}

/* ****************************************************************************
 * Return internal use string for PREFIX OPR8R that is unambiguous and unique
 * NOTE: PREFIX and POSTFIX OPR8Rs in source code are the same (++,--), but
 * position relative to their operand determines whether they're PREFIX or
 * POSTFIX.  The internal use string allows for carrying this info forward.
 * ***************************************************************************/
std::wstring CompileExecTerms::getUniqPrefixOpr8r (std::wstring srcStr) {
	std::wstring internalStr = srcStr;

	if (srcStr == L"++")
		internalStr = PRE_INCR_OPR8R;
	else if (srcStr == L"--")
		internalStr = PRE_DECR_OPR8R;

	return (internalStr);
}

/* ****************************************************************************
 * Return internal use string for UNARY OPR8R that is unambiguous and unique
 * NOTE: UNARY OPR8Rs in source code have the same string (+,-), as some
 * BINARY OPR8Rs, and expression context is used to determine whether they're
 * UNARY or BINARY. The internal use string allows for carrying this info forward.
 * ***************************************************************************/
std::wstring CompileExecTerms::getUniqUnaryOpr8r (std::wstring srcStr) {
	std::wstring internalStr = srcStr;

	if (srcStr == L"+")
		internalStr = UNARY_PLUS_OPR8R;
	else if (srcStr == L"-")
		internalStr = UNARY_MINUS_OPR8R;

	return (internalStr);
}

/* ****************************************************************************
 * Return internal use string for POSTFIX OPR8R that is unambiguous and unique
 * NOTE: PREFIX and POSTFIX OPR8Rs in source code are the same (++,--), but
 * position relative to their operand determines whether they're PREFIX or
 * POSTFIX.  The internal use string allows for carrying this info forward.
 * ***************************************************************************/
std::wstring CompileExecTerms::getUniqPostfixOpr8r (std::wstring srcStr) {
	std::wstring internalStr = srcStr;

	if (srcStr == L"++")
		internalStr = POST_INCR_OPR8R;
	else if (srcStr == L"--")
		internalStr = POST_DECR_OPR8R;

	return (internalStr);
}

