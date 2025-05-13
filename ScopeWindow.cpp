/*
 * ScopeWindow.cpp
 *
 *  Created on: Nov 11, 2024
 *      Author: Mike Volk
 *
 *  Class represents a scope instance and contains variables valid at this scope along with positional info
 *  for where this scope starts in the interpreted file when used by the interpreter.
 *  Another class will be used to hold multiple nested and currently open scopes, and close scopes when
 *  appropriate e.g. the compiler goes past a '}' that closes a scope in source, or at exec time when the
 *  interpreter goes past the file limit position of a scope and has to close it.
 */

#include "ScopeWindow.h"
#include "common.h"
#include <cstdint>

ScopeWindow::ScopeWindow(uint8_t inOpCode, Token inOpeningTkn, uint32_t inStartFilePos, uint32_t inScopeLen) {
  opener_opcode = inOpCode;
  openerTkn = inOpeningTkn;
  boundary_begin_pos = inStartFilePos;

  // scopeLen will be 0 during compilation, because the object has just been encountered, but not compiled yet
  if (inScopeLen > 0)
    boundary_end_pos = inStartFilePos + inScopeLen;
  else
    boundary_end_pos = 0;

  loop_break_cnt = 0;    
  is_exists_for_loop_cond = false;
}

int ScopeWindow::setBoundaryEndPos (uint32_t end_pos) {
  int ret_code = GENERAL_FAILURE;

  if (end_pos > boundary_begin_pos) {
    boundary_end_pos = end_pos;
    ret_code = OK;
  }

  return (ret_code);
}

ScopeWindow::~ScopeWindow() {
}

