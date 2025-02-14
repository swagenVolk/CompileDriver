/*
 * ScopeLevel.h
 *
 *  Created on: Nov 11, 2024
 *      Author: Mike Volk
 */

#ifndef SCOPELEVEL_H_
#define SCOPELEVEL_H_

#include <cstdint>
#include <map>
#include <string>
#include "common.h"
#include "Token.h"

class ScopeLevel {
public:
	ScopeLevel (uint8_t inOpCode, Token inOpeningTkn, uint32_t inStartFilePos, uint32_t inScopeLen);
	virtual ~ScopeLevel();

	int setBoundaryEndPos (uint32_t end_pos);

	Token openerTkn;																							// When compiling, init with Token that opened scope
	uint8_t opener_opcode;																				// [if] [else if] [else] [for] [while] [function]?
	uint32_t boundary_begin_pos;																	// Where does this scope object begin in the interpreted file?
	uint32_t boundary_end_pos;
	std::map <std::wstring, std::shared_ptr<Token>> variables;		// List of variables defined at this scope

};

#endif /* SCOPELEVEL_H_ */
