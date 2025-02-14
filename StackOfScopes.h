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
#include "ScopeLevel.h"
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
	int closeTopScope (InterpretedFileWriter & interpretedFileWriter, uint8_t & closedScopeOpCode, closeScopeErr & closeErr, bool isRootScope);
	int closeTopScope (InterpretedFileWriter & interpretedFileWriter, uint8_t & closedScopeOpCode, closeScopeErr & closeErr);


private:
  std::vector<std::shared_ptr<ScopeLevel>> scopeStack;
  Utilities util;
  std::wstring thisSrcFile;

};

#endif /* STACKOFSCOPES_H_ */
