/*
 * ScopeLevel.cpp
 *
 *  Created on: Nov 11, 2024
 *      Author: Mike Volk
 */

#include "ScopeLevel.h"
#include "common.h"
#include <cstdint>

ScopeLevel::ScopeLevel(uint8_t inOpCode, Token inOpeningTkn, uint32_t inStartFilePos, uint32_t inScopeLen) {
	// TODO Auto-generated constructor stub
	opener_opcode = inOpCode;
	openerTkn = inOpeningTkn;
	boundary_begin_pos = inStartFilePos;

	// scopeLen will be 0 during compilation, because the object has just been encountered, but not compiled yet
	inScopeLen > 0 ? boundary_end_pos = inStartFilePos + inScopeLen : boundary_end_pos = 0;
}

int ScopeLevel::setBoundaryEndPos (uint32_t end_pos)	{
	int ret_code = GENERAL_FAILURE;

	if (end_pos > boundary_begin_pos)	{
		boundary_end_pos = end_pos;
		ret_code = OK;
	}

	return (ret_code);
}

ScopeLevel::~ScopeLevel() {
	// TODO Auto-generated destructor stub
}

