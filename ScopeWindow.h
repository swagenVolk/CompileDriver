/*
 * ScopeWindow.h
 *
 *  Created on: Nov 11, 2024
 *      Author: Mike Volk
 */

#ifndef SCOPEWINDOW_H_
#define SCOPEWINDOW_H_

#include <cstdint>
#include <map>
#include <string>
#include "common.h"
#include "Token.h"

class ScopeWindow {
public:
	ScopeWindow (uint8_t inOpCode, Token inOpeningTkn, uint32_t inStartFilePos, uint32_t inScopeLen);
	virtual ~ScopeWindow();

	int setBoundaryEndPos (uint32_t end_pos);

	Token openerTkn;																							// When compiling, init with Token that opened scope
	uint8_t opener_opcode;																				// [if] [else if] [else] [for] [while] [function]?
	uint32_t boundary_begin_pos;																	// Where does this scope object begin in the interpreted file?
	uint32_t boundary_end_pos;
	std::map <std::wstring, std::shared_ptr<Token>> variables;		// List of variables defined at this scope
  int loop_break_cnt;                                           // Incremented when a [break] statement is found inside a loop                            

};

#endif /* SCOPEWINDOW_H_ */
