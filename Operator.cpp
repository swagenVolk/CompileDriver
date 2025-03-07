/*
 * Operator.cpp
 * Instance of a class holds the characters that make up the operator and some characteristics
 *  Created on: Jun 14, 2024
 *      Author: Mike Volk
 */

#include "Operator.h"
#include <iostream>
#include "OpCodes.h"

Operator::Operator()	{
	symbol = L"";
	type_mask = 0;
	valid_usage = 0;
	numReqSrcOperands = 0;
	numReqExecOperands = 0;
	op_code = INVALID_OPCODE;
	description = L"";

}

Operator::Operator(std::wstring in_symbol, uint8_t in_type_mask, uint8_t inValidUsage, int in_num_src_operands, int in_num_exec_operands
	, uint8_t in_op_code, std::wstring inDescr) {
	symbol = in_symbol;

	// UNARY doesn't play well with others
	if (in_type_mask & UNARY)
		assert (0 == (in_type_mask & ~UNARY));

	// BINARY doesn't play well with MOST others
	if (in_type_mask & BINARY)
		assert (0 == (in_type_mask & ~BINARY) || TERNARY_1ST == (in_type_mask & ~BINARY) || TERNARY_2ND == (in_type_mask & ~BINARY));

	// TERNARY doesn't play well with MOST others
	if (in_type_mask & TERNARY_1ST)
		assert (0 == (in_type_mask & ~TERNARY_1ST) || (BINARY == (in_type_mask & ~TERNARY_1ST)));

	if (in_type_mask & TERNARY_2ND)
		assert (0 == (in_type_mask & ~TERNARY_2ND) || (BINARY == (in_type_mask & ~TERNARY_2ND)));

	if ((in_type_mask & PREFIX) || (in_type_mask & POSTFIX))
		assert (0 == (in_type_mask & ~(PREFIX | POSTFIX)));

	// STATEMENT_ENDER doesn't play well with others
	if (in_type_mask & STATEMENT_ENDER)
		assert (0 == (in_type_mask & ~STATEMENT_ENDER));

	type_mask = in_type_mask;
	valid_usage = inValidUsage;
	numReqSrcOperands = in_num_src_operands;
	numReqExecOperands = in_num_exec_operands;
	op_code = in_op_code;
	description = inDescr;

}

Operator::~Operator() {
}

Operator& Operator::operator= (const Operator& src_opr8r)
{
	// self-assignment check
	if (this == &src_opr8r)
		return *this;

	// if data exists in the current string, delete it
	symbol.clear();
	symbol = src_opr8r.symbol;
	type_mask = src_opr8r.type_mask;
	valid_usage = src_opr8r.valid_usage;
	numReqSrcOperands = src_opr8r.numReqSrcOperands;
	numReqExecOperands = src_opr8r.numReqExecOperands;
	op_code = src_opr8r.op_code;
	description.clear();
	description = src_opr8r.description;

	// TODO: I don't understand the comment below
	// return the existing object so we can chain this operator
	return *this;
}
