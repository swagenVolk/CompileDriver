/*
 * OpCodes.cpp
 *
 *  Created on: Oct 10, 2024
 *      Author: mike
 */

#include "OpCodes.h"

OpCodes::OpCodes() {
	// TODO Auto-generated constructor stub

}

OpCodes::~OpCodes() {
	// TODO Auto-generated destructor stub
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int OpCodes::writeAtomicOpCode (uint8_t op_code)	{
	int ret_code = GENERAL_FAILURE;

	if (op_code >= ATOMIC_OPCODE_RANGE_BEGIN && op_code <= ATOMIC_OPCODE_RANGE_END)	{
		if (op_code <= LAST_VALID_OPR8R_OPCODE)	{
			// TODO: Write it out to file
			ret_code = OK;

		} else if (op_code >= FIRST_VALID_DATA_TYPE_OPCODE && op_code <= LAST_VALID_DATA_TYPE_OPCODE)	{
			// TODO: Write it out to file
			ret_code = OK;
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int OpCodes::write8BitOpCode (uint8_t op_code, uint8_t payload)	{
	int ret_code = GENERAL_FAILURE;

	if (op_code >= FIXED_OPCODE_RANGE_BEGIN && op_code <= FIXED_OPCODE_RANGE_END)	{
		if (op_code == UINT8_OPCODE || op_code == INT8_OPCODE)	{
			// TODO: Write op_code followed by payload out to file
			ret_code = OK;
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int OpCodes::write16BitOpCode (uint8_t op_code, uint16_t payload)	{
	int ret_code = GENERAL_FAILURE;

	if (op_code >= FIXED_OPCODE_RANGE_BEGIN && op_code <= FIXED_OPCODE_RANGE_END)	{
		if (op_code == UINT16_OPCODE || op_code == INT16_OPCODE)	{
			// TODO: Write op_code followed by payload out to file
			ret_code = OK;
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int OpCodes::write32BitOpCode (uint8_t op_code, uint32_t payload)	{
	int ret_code = GENERAL_FAILURE;

	if (op_code >= FIXED_OPCODE_RANGE_BEGIN && op_code <= FIXED_OPCODE_RANGE_END)	{
		if (op_code == UINT32_OPCODE || op_code == INT32_OPCODE)	{
			// TODO: Write op_code followed by payload out to file
			ret_code = OK;
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int OpCodes::write64BitOpCode (uint8_t op_code, uint64_t payload)	{
	int ret_code = GENERAL_FAILURE;

	if (op_code >= FIXED_OPCODE_RANGE_BEGIN && op_code <= FIXED_OPCODE_RANGE_END)	{
		if (op_code == UINT64_OPCODE || op_code == INT64_OPCODE)	{
			// TODO: Write op_code followed by payload out to file
			ret_code = OK;
		}
	}

	return (ret_code);
}


