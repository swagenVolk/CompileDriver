/*
 * StackOfScopes.h
 *
 *  Created on: Feb 8, 2025
 *      Author: Mike Volk
 */

#ifndef STACKOFSCOPES_H_
#define STACKOFSCOPES_H_

#include <cstdint>
#include <memory>
#include <string>
#include <algorithm>
#include "common.h"
#include "CompileExecTerms.h"
#include "InfoWarnError.h"
#include "ScopeWindow.h"
#include "Token.h"
#include "Utilities.h"
#include "UserMessages.h"
#include "InterpretedFileWriter.h"

enum closeEnum	{
	SCOPE_CLOSED_OK
	,SCOPE_CLOSE_UKNOWN_ERROR
	,NO_SCOPES_OPEN
	,ONLY_ROOT_SCOPE_OPEN
	,SCOPES_OPEN_ABOVE_ROOT
  ,SCOPE_OPCODE_MISMATCH
};

typedef closeEnum closeScopeErr;

class StackOfScopes {
public:
	StackOfScopes();
	virtual ~StackOfScopes();
	void reset();

	int findVar(std::wstring varName, int maxLevels, Token & updateValTkn, ReadOrWrite readOrWrite, std::wstring & errorMsg);
	int insertNewVarAtCurrScope (std::wstring varName, Token varValue);
	void displayVariables();
	int openNewScope (uint8_t openedByOpCode, Token scopenerTkn, uint32_t startScopeFilePos, uint32_t scopeLen);
  int closeTopScope (uint8_t closedScopeOpCode, closeScopeErr & closeErr, bool isRootScope);
	int srcCloseTopScope (InterpretedFileWriter & interpretedFileWriter, uint8_t & closedScopeOpCode, closeScopeErr & closeErr, bool isRootScope);
	int srcCloseTopScope (InterpretedFileWriter & interpretedFileWriter, uint8_t & closedScopeOpCode, closeScopeErr & closeErr);
  bool isInsideLoop (uint32_t & loop_boundary_end_pos, bool is_inc_break_cnt);

	int get_top_opener_tkn (Token & opener_tkn);
	int get_top_opener_opcode (uint8_t & op_code);
	int get_top_boundary_begin_pos (uint32_t & begin_pos);
  int get_top_loop_break_cnt (int & break_cnt);

  int get_top_is_exists_for_loop_cond (bool & is_exists);
  int set_top_is_exists_for_loop_cond (bool & is_exists);

private:
  std::vector<std::shared_ptr<ScopeWindow>> scopeStack;
  Utilities util;
  std::wstring thisSrcFile;


};

#endif /* STACKOFSCOPES_H_ */
