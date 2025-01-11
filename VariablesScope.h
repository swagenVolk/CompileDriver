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
#include "CompileExecTerms.h"
#include "InfoWarnError.h"
#include "ScopeFrame.h"
#include "Token.h"
#include "Utilities.h"
#include "UserMessages.h"

class VariablesScope {
public:
	VariablesScope();
	virtual ~VariablesScope();
	void reset();

	int findVar(std::wstring varName, int maxLevels, Token & updateValTkn, ReadOrWrite readOrWrite, std::wstring & errorMsg);
	int insertNewVarAtCurrScope (std::wstring varName, Token varValue);
	void displayVariables();

private:
  std::vector<std::shared_ptr<ScopeFrame>> scopeStack;
  Utilities util;
  std::wstring thisSrcFile;

};

#endif /* VARIABLESSCOPE_H_ */
