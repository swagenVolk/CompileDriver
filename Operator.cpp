/*
 * Operator.cpp
 * Instance of a class holds the characters that make up the operator and some characteristics
 * TODO: How do I ensure each operator is unique? Must be done outside of this class, I assume
 *  Created on: Jun 14, 2024
 *      Author: Mike Volk
 */

#include "Operator.h"
#include <iostream>

Operator::Operator(std::wstring in_symbol, uint8_t in_type_mask, uint8_t in_valid_for_mask, int in_num_operands) {
	// TODO Auto-generated constructor stub
	this->symbol = in_symbol;

	// UNARY doesn't play well with others
	if (in_type_mask & UNARY)
		assert (0 == (in_type_mask & ~UNARY));

	// BINARY doesn't play well with others
	if (in_type_mask & BINARY)
		assert (0 == (in_type_mask & ~BINARY));

	// TERNARY doesn't play well with others
	if (in_type_mask & TERNARY_1ST)
		assert (0 == (in_type_mask & ~TERNARY_1ST));

	if ((in_type_mask & PREFIX) || (in_type_mask & POSTFIX))
		assert (0 == (in_type_mask & ~(PREFIX | POSTFIX)));

	// STATEMENT_ENDER doesn't play well with others
	if (in_type_mask & STATEMENT_ENDER)
		assert (0 == (in_type_mask & ~STATEMENT_ENDER));

	this->type_mask = in_type_mask;

	this->valid_for_mask = in_valid_for_mask;

	this->numOperands = in_num_operands;

}

Operator::~Operator() {
	// TODO Auto-generated destructor stub
}

