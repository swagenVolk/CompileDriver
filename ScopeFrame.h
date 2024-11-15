/*
 * ScopeFrame.h
 *
 *  Created on: Nov 11, 2024
 *      Author: Mike Volk
 */

#ifndef SCOPEFRAME_H_
#define SCOPEFRAME_H_

#include <cstdint>
#include <map>
#include "common.h"
#include "Token.h"

class ScopeFrame {
public:
	ScopeFrame();
	virtual ~ScopeFrame();

	uint8_t type;																// [if] [else if] [else] [for] [while] [function]?
	uint32_t begin_file_pos;										// Where does this scope object begin in the interpreted file?
	uint32_t end_file_pos;											// Where does this scope object END in the interpreted file?
	std::map <std::wstring, Token> variables;		// List of variables defined at this scope

};

#endif /* SCOPEFRAME_H_ */
