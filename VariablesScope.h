/*
 * VariablesScope.h
 *
 *  Created on: Nov 18, 2024
 *      Author: Mike Volk
 */

#ifndef VARIABLESSCOPE_H_
#define VARIABLESSCOPE_H_

#include <memory>
#include <string>
#include <algorithm>
#include "common.h"
#include "InfoWarnError.h"
#include "ScopeFrame.h"
#include "Token.h"
#include "Utilities.h"
#include "UserMessages.h"

#define COMMIT_WRITE true
#define READ_ONLY false

enum caller_mode_enum	{
	COMPILE_TIME
	,EXEC_TIME
};

typedef caller_mode_enum VarScopeCallerMode;


class VariablesScope {
public:
	VariablesScope();
	virtual ~VariablesScope();
	void reset();

	int findSourceVar(std::wstring varName, int maxLevels, Token & readOrWriteTkn, bool isWrite, std::shared_ptr<UserMessages> userMessages);
	int findExecVar(std::wstring varName, int maxLevels, Token & readOrWriteTkn, bool isWrite, std::shared_ptr<UserMessages> userMessages);
	int insertNewVarAtCurrScope (std::wstring varName, Token varValue);
	void displayVariables();

private:
	int findVar(std::wstring varName, int maxLevels, Token & readOrWriteTkn, bool isWrite, std::shared_ptr<UserMessages> userMessages, VarScopeCallerMode mode);
  std::vector<std::shared_ptr<ScopeFrame>> scopeStack;
  Utilities util;
  std::wstring thisSrcFile;

};

#endif /* VARIABLESSCOPE_H_ */
