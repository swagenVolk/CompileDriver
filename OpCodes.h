/*
 * OpCodes.h
 *
 * This file defines OpCodes that will be produced by the Compiler and consumed by the Interpreter.
 *
 *  Created on: Oct 10, 2024
 *      Author: Mike Volk
 */

#ifndef OPCODES_H_
#define OPCODES_H_

#include "common.h"
#include <cstdint>

// TODO: This is the "compressed" approach.  Do I need to have _OPCODE on the end?
// Will there be any confusion with other types of opcodes?
// [0x1-0x2F] is reserved for self-contained, single 8-bit BYTE OPR8R op_codes
#define OPCODE_NUM_BYTES								1
#define INVALID_OPCODE									0x0
#define ATOMIC_OPCODE_RANGE_BEGIN				0x1
#define ATOMIC_OPCODE_RANGE_END					0x2F


#define	POST_INCR_OPR8R_OPCODE					0x1			// PREFIX & POSTFIX OPR8Rs are executed before|after the expression
#define	POST_DECR_OPR8R_OPCODE					0x2			// that contains them. We still want to keep a reference in the Interpreted
#define	PRE_INCR_OPR8R_OPCODE					  0x3			// expression stream for when the Interpreted file get disassembled.
#define	PRE_DECR_OPR8R_OPCODE					  0x4

#define UNARY_PLUS_OPR8R_OPCODE					0x5
#define	UNARY_MINUS_OPR8R_OPCODE				0x6
#define	LOGICAL_NOT_OPR8R_OPCODE				0x7			// "!"
#define	BITWISE_NOT_OPR8R_OPCODE				0x8			// "~"
#define	MULTIPLY_OPR8R_OPCODE					  0x9			// "*"
#define	DIV_OPR8R_OPCODE						    0xA			// "/"
#define	MOD_OPR8R_OPCODE						    0xB			// "%"
#define	BINARY_PLUS_OPR8R_OPCODE				0xC 		// "+"
#define	BINARY_MINUS_OPR8R_OPCODE				0xD 		// "-"
#define	LEFT_SHIFT_OPR8R_OPCODE					0xE 		// "<<"
#define	RIGHT_SHIFT_OPR8R_OPCODE				0xF 		// ">>"
#define	LESS_THAN_OPR8R_OPCODE					0x10		// "<"
#define	LESS_EQUALS_OPR8R8_OPCODE				0x11		// "<="
#define	GREATER_THAN_OPR8R_OPCODE				0x12		// ">"
#define	GREATER_EQUALS_OPR8R8_OPCODE		0x13		// ">="
#define	EQUALITY_OPR8R_OPCODE					  0x14		// "=="
#define	NOT_EQUALS_OPR8R_OPCODE					0x15		// "!="
#define	BITWISE_AND_OPR8R_OPCODE				0x16		// "&"
#define	BITWISE_XOR_OPR8R_OPCODE				0x17		// "^"
#define	BITWISE_OR_OPR8R_OPCODE					0x18		// "|"
#define	LOGICAL_AND_OPR8R_OPCODE				0x19		// "&&"
#define	LOGICAL_OR_OPR8R_OPCODE					0x1A		// "||"
#define	TERNARY_1ST_OPR8R_OPCODE				0x1B		// "?", TERNARY_1ST
#define	TERNARY_2ND_OPR8R_OPCODE				0x1C		// ":", TERNARY_2ND
#define	ASSIGNMENT_OPR8R_OPCODE					0x1D		// "="
#define	PLUS_ASSIGN_OPR8R_OPCODE				0x1E		// "+="
#define	MINUS_ASSIGN_OPR8R_OPCODE				0x1F		// "-="
#define	MULTIPLY_ASSIGN_OPR8R_OPCODE		0x20		// "*="
#define	DIV_ASSIGN_OPR8R_OPCODE					0x21		// "/="
#define	MOD_ASSIGN_OPR8R_OPCODE					0x22		// "%="
#define	LEFT_SHIFT_ASSIGN_OPR8R_OPCODE	0x23		// "<<="
#define	RIGHT_SHIFT_ASSIGN_OPR8R_OPCODE	0x24		// ">>="
#define	BITWISE_AND_ASSIGN_OPR8R_OPCODE	0x25		// "&="
#define	BITWISE_XOR_ASSIGN_OPR8R_OPCODE	0x26		// "^="
#define	BITWISE_OR_ASSIGN_OPR8R_OPCODE	0x27		// "|="
#define	STATEMENT_ENDER_OPR8R_OPCODE		0x28		// ";", STATEMENT_ENDER
#define LAST_VALID_OPR8R_OPCODE					0x28		// Change this value if new op_codes in this range are created

// [0x30-0x3F] is reserved for self-contained, single 8-bit BYTE data type op_codes
#define FIRST_VALID_DATA_TYPE_OPCODE		0x30
#define DATA_TYPE_UINT8_OPCODE					0x30
#define DATA_TYPE_UINT16_OPCODE					0x31
#define DATA_TYPE_UINT32_OPCODE					0x32
#define DATA_TYPE_UINT64_OPCODE					0x33
#define DATA_TYPE_INT8_OPCODE						0x34
#define DATA_TYPE_INT16_OPCODE					0x35
#define DATA_TYPE_INT32_OPCODE					0x36
#define DATA_TYPE_INT64_OPCODE					0x37
#define DATA_TYPE_STRING_OPCODE					0x38
#define DATA_TYPE_DATETIME_OPCODE				0x39
#define DATA_TYPE_DOUBLE_OPCODE					0x3A
#define DATA_TYPE_BOOL_OPCODE           0x3B
#define LAST_VALID_DATA_TYPE_OPCODE			0x3B		// Change this value if new data_types in this range are created

// Opcodes [0x40-0x47] have an 8-bit payload
#define FIXED_OPCODE_RANGE_BEGIN				0x40
#define FIXED_OPCODE_RANGE_END					0x5F

#define UINT8_OPCODE										0x40	// [op_code][8-bit #]
#define INT8_OPCODE											0x42	// [op_code][8-bit #]
#define BOOL_DATA_OPCODE                0x43

// Opcodes [0x48-0x4F] have a 16-bit payload
#define UINT16_OPCODE										0x48	// [op_code][16-bit #]
#define INT16_OPCODE										0x49	// [op_code][16-bit #]

// Opcodes [0x50-0x57] have a 32-bit payload
#define UINT32_OPCODE										0x50	// [op_code][32-bit #]
#define INT32_OPCODE										0x51	// [op_code][32-bit #]

// Opcodes [0x58-0x5F] have a 64-bit payload
#define UINT64_OPCODE										0x58	// [op_code][64-bit #]
#define INT64_OPCODE										0x59	// [op_code][64-bit #]


// Opcodes [0x60-0x7F(?)] are guaranteed to have a [DWORD] sized total_length field directly
// following the op_code. This allows the Interpreter to rapidly jump to the next object without
// chomping through all the internals of this op_code.
#define FLEX_OP_LEN_FLD_NUM_BYTES				4
#define FIRST_VALID_FLEX_LEN_OPCODE			0x60
#define STRING_OPCODE										0x60	// [op_code][total_length][string]
#define VAR_NAME_OPCODE									0x61	// [op_code][total_length][var name string]
// TODO: ARRAY_VARIABLE_OPCODE?  Or is that more of a parsing problem @ run time?
// TODO: Should I store doubles as something other than a literal string?
#define DATETIME_OPCODE									0x62	// [op_code][total_length][datetime string]
#define DOUBLE_OPCODE										0x63	// [op_code][total_length][double string]

// TODO: Should code_blocks also have their own total_length field?
// TODO: Could these code_blocks be considered unnamed scopes instead?
// TODO: Should scope openers and closers also contain the line # and column of the Token that opened or closed them?

#define EXPRESSION_OPCODE								0x68	// [op_code][total_length][expression stream]
#define IF_BLOCK_OPCODE									0x69	// [op_code][total_length][conditional -> [op_code][total_length][expression stream]][code_block]
#define ELSE_IF_BLOCK_OPCODE						0x6A	// [op_code][total_length][conditional -> [op_code][total_length][expression stream]][code_block]
#define ELSE_BLOCK_OPCODE								0x6B	// [op_code][total_length][code_block]
#define WHILE_LOOP_OPCODE								0x6C	// [op_code][total_length][conditional -> [op_code][total_length][expression stream]][code_block]
#define FOR_LOOP_OPCODE									0x6D	// [op_code][total_length][init_expression][conditional_expression][last_expression][code_block]
#define SCOPE_OPEN_OPCODE								0x6E	// [op_code][total_length][]
#define VARIABLES_DECLARATION_OPCODE		0x6F	// [op_code][total_length][datatype op_code][[string var_name][init_expression]]+
#define FXN_DECLARATION_OPCODE					0x70	// [op_code][total_length][string fxn_name][parameter type list][parameter name list]
#define FXN_CALL_OPCODE									0x71	// [op_code][total_length][string fxn_name][expression list]
#define SYSTEM_CALL_OPCODE							0x72	// [op_code][total_length][string fxn_name][expression list]
#define LAST_VALID_FLEX_LEN_OPCODE			0x72	// Change this value if new flexible length op_codes in this range are created

// TODO: What about SPR8Rs?
// this->_1char_spr8rs = L"()[]{}"; [ASCII - 0x28,0x29,0x5B,0x5D,0x7B,0x7D]
// So there is overlap between the op_codes defined above and the SPR8R's ASCII representation
// Probably doesn't matter since (,),{ and } are syntactic sugar that melts away
// If at all necessary, could create an appropriate 8-bit payload op_code that holds a character
// TODO: Is a comma a spr8r?  What about the '\' character, for paths?

#endif /* OPCODES_H_ */
