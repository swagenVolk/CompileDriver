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
	// TODO: Should I used enum instead of string to represent opr8rs in interpreted code?  Would save space.
	std::wstring curr_opr8r;
	int idx = 0;


	// TODO: Break up into multiple precedence levels
	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
	// TODO: Account for this change!
	// grouped_opr8rs.back().opr8rs.push_back( Operator (L"++", (PREFIX|POSTFIX), USR_SRC, 1));   // NOTE: Pre-fix and post-fix precedence is different
	grouped_opr8rs.back().opr8rs.push_back( Operator (L"++", POSTFIX, USR_SRC, 1));   // NOTE: Pre-fix and post-fix precedence is different
	grouped_opr8rs.back().opr8rs.push_back ( Operator (L"--", POSTFIX, USR_SRC, 1));
	grouped_opr8rs.back().opr8rs.push_back ( Operator (POST_INCR_OPR8R, POSTFIX, GNR8D_SRC, 1));
	opr8rOpCodes.emplace (std::pair {POST_INCR_OPR8R, POST_INCR_OPR8R_OPCODE});
	grouped_opr8rs.back().opr8rs.push_back ( Operator (POST_DECR_OPR8R, POSTFIX, GNR8D_SRC, 1));   // NOTE: Pre-fix and post-fix precedence is different
	opr8rOpCodes.emplace (std::pair {POST_DECR_OPR8R, POST_DECR_OPR8R_OPCODE});

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
	//grouped_opr8rs.back().opr8rs.push_back ( Operator (L"--", (PREFIX|POSTFIX), USR_SRC, 1));
	grouped_opr8rs.back().opr8rs.push_back ( Operator (L"++", PREFIX, USR_SRC, 1));
	grouped_opr8rs.back().opr8rs.push_back ( Operator (L"--", PREFIX, USR_SRC, 1));
	grouped_opr8rs.back().opr8rs.push_back ( Operator (PRE_INCR_OPR8R, PREFIX, GNR8D_SRC, 1));   // NOTE: Pre-fix and post-fix precedence is different
	opr8rOpCodes.emplace (std::pair {PRE_INCR_OPR8R, PRE_INCR_OPR8R_OPCODE});
	grouped_opr8rs.back().opr8rs.push_back ( Operator (PRE_DECR_OPR8R, PREFIX, GNR8D_SRC, 1));   // NOTE: Pre-fix and post-fix precedence is different
	opr8rOpCodes.emplace (std::pair {PRE_DECR_OPR8R, PRE_DECR_OPR8R_OPCODE});

	// TODO: Unary +? What's the point? Completeness? UNARY_PLUS_OPR8R_OPCODE
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"-", UNARY, USR_SRC, 1));
  grouped_opr8rs.back().opr8rs.push_back ( Operator (UNARY_MINUS_OPR8R, UNARY, GNR8D_SRC, 1));
	opr8rOpCodes.emplace (std::pair {UNARY_MINUS_OPR8R, UNARY_MINUS_OPR8R_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"!", UNARY, (USR_SRC|GNR8D_SRC),1));
	opr8rOpCodes.emplace (std::pair {L"!", LOGICAL_NOT_OPR8R_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"~", UNARY, (USR_SRC|GNR8D_SRC),1));
	opr8rOpCodes.emplace (std::pair {L"~", BITWISE_NOT_OPR8R_OPCODE});

  //  TODO: ()  Function call
  //  TODO: []  Array subscripting

  // TODO: Unary plus and minus has precedence over binary

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"*", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"*", MULTIPLY_OPR8R_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"/", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"/", DIV_OPR8R_OPCODE});

	grouped_opr8rs.back().opr8rs.push_back ( Operator (L"%", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"%", MOD_OPR8R_OPCODE});

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"+", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"+", BINARY_PLUS_OPR8R_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"-", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"-", BINARY_MINUS_OPR8R_OPCODE});

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"<<", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"<<", LEFT_SHIFT_OPR8R_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L">>", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L">>", RIGHT_SHIFT_OPR8R_OPCODE});

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"<", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"<", LESS_THAN_OPR8R_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"<=", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"<=", LESS_EQUALS_OPR8R8_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L">", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L">", GREATER_THAN_OPR8R_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L">=", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L">=", GREATER_EQUALS_OPR8R8_OPCODE});

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"==", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"==", EQUALITY_OPR8R_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"!=", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"!=", NOT_EQUALS_OPR8R_OPCODE});

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"&", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"&", BITWISE_AND_OPR8R_OPCODE});

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
	grouped_opr8rs.back().opr8rs.push_back ( Operator (L"^", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"^", BITWISE_XOR_OPR8R_OPCODE});

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
	grouped_opr8rs.back().opr8rs.push_back ( Operator (L"|", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"|", BITWISE_OR_OPR8R_OPCODE});

  // TODO: && and || OPR8Rs have the same precedence.  Is it incorrect to order them
  // absolutely like I've done here?  Should I include some kind of precedence level for
  // each OPR8R to account for this?  PROBABLY!!!!
	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"&&", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"&&", LOGICAL_AND_OPR8R_OPCODE});

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"||", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"||", LOGICAL_OR_OPR8R_OPCODE});

  //  TODO: ?:  Ternary conditional[note 3] Right-to-left
  //  TODO: Understand diff between Left-to-right and Right-to-left associativity
  //	TODO: # of expected operands for Ternary? Especially since I'll be pushing a new scope when the "?"
  //	opr8r is encountered.
	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());

  // TODO: Is TERNARY OPR8R the two together, e.g. "?:" ?
	// TODO: How many operands should the constructor get? 3? Or is this OPR8R a special case since it
	// opens up a scope and it'll be treated as such anyway?  If so, shouldn't we pass 0?
	grouped_opr8rs.back().opr8rs.push_back ( Operator (L"?", (BINARY|TERNARY_1ST), (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"?", TERNARY_1ST_OPR8R_OPCODE});
  // TODO: Should this be BINARY instead?

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L":", (BINARY|TERNARY_2ND), (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L":", TERNARY_2ND_OPR8R_OPCODE});

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"=", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"=", ASSIGNMENT_OPR8R_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"+=", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"+=", PLUS_ASSIGN_OPR8R_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"-=", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"-=", MINUS_ASSIGN_OPR8R_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"*=", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"*=", MULTIPLY_ASSIGN_OPR8R_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"/=", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"/=", DIV_ASSIGN_OPR8R_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"%=", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"%=", MOD_ASSIGN_OPR8R_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"<<=", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"<<=", LEFT_SHIFT_ASSIGN_OPR8R_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L">>=", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L">>=", RIGHT_SHIFT_ASSIGN_OPR8R_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"&=", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"&=", BITWISE_AND_ASSIGN_OPR8R_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"^=", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"^=", BITWISE_XOR_ASSIGN_OPR8R_OPCODE});
  grouped_opr8rs.back().opr8rs.push_back ( Operator (L"|=", BINARY, (USR_SRC|GNR8D_SRC), 2));
	opr8rOpCodes.emplace (std::pair {L"|=", BITWISE_OR_ASSIGN_OPR8R_OPCODE});

	grouped_opr8rs.push_back(Opr8rPrecedenceLvl ());
	// TODO: Not listed as an OPR8R on cppreference web page.  Does it matter?
	grouped_opr8rs.back().opr8rs.push_back ( Operator (L";", STATEMENT_ENDER, (USR_SRC|GNR8D_SRC),0));
	opr8rOpCodes.emplace (std::pair {L";",STATEMENT_ENDER_OPR8R_OPCODE});







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

