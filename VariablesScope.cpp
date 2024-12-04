/*
 * VariablesScope.cpp
 *
 *  Created on: Nov 18, 2024
 *      Author: Mike Volk
 */

#include "VariablesScope.h"
#include <memory>

VariablesScope::VariablesScope() {
	// TODO Auto-generated constructor stub
	std::shared_ptr<ScopeFrame> rootScope = std::make_shared<ScopeFrame> ();
	scopeStack.push_back(rootScope);
	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");

}

VariablesScope::~VariablesScope() {
	// TODO Auto-generated destructor stub
	thisSrcFile.clear();

	while (scopeStack.size() > 0)	{
		std::shared_ptr<ScopeFrame> top = scopeStack[scopeStack.size() - 1];
		scopeStack.erase(scopeStack.end());
		top.reset();
	}
}

/* ****************************************************************************
 * Look up this variable in our scopeStack|NameSpace. Only update the variable
 * if isCommitUpdate = COMMIT_UPDATE (true)
 * ***************************************************************************/
int VariablesScope::findVar(std::wstring varName, int maxLevels, Token & updateValTkn, bool isCommitUpdate, std::shared_ptr<UserMessages> userMessages)	{
	int ret_code = GENERAL_FAILURE;
	bool isFound = false;

	int scopeTopIdx = scopeStack.size() - 1;
	int endScopeIdx;
	if (maxLevels <= 0)
		endScopeIdx = 0;
	else
		endScopeIdx = scopeTopIdx - maxLevels + 1;

	for (int currIdx = scopeTopIdx; currIdx >= endScopeIdx && !isFound; currIdx--)	{
		std::shared_ptr<ScopeFrame> currScope = scopeStack[currIdx];
		if (auto search = currScope->variables.find(varName); search != currScope->variables.end())	{
			// TODO: existingTkn is probably going to need to be a POINTER for the update to stick
			std::shared_ptr<Token> existingTkn = search->second;
			isFound = true;
			if (isCommitUpdate)	{
				if (OK != existingTkn->convertTo(updateValTkn))	{
					// TODO: What info can I supply to user to resolve src line # etc?
//					callersErrInfo.set(USER_ERROR, thisSrcFile, __LINE__
//							, L"Data type mismatch on assignment to " + varName + L" " + existingTkn->descr_sans_line_num_col());
				} else	{
					existingTkn->isInitialized = true;
					ret_code = OK;
				}
			} else	{
				updateValTkn = *existingTkn;
				ret_code = OK;
			}
		}
	}

	if (isCommitUpdate && !isFound && userMessages != NULL)	{
		userMessages->logMsg (INTERNAL_ERROR, L"Variable " + varName + L" no longer exists at current scope."
				, thisSrcFile, __LINE__, 0);
	}

	return (ret_code);

}

/* ****************************************************************************
 * Insert this variable at the current scope.  Fail if it already exists
 * ***************************************************************************/
int VariablesScope::insertNewVarAtCurrScope (std::wstring varName, Token varValue)	{
	int ret_code = GENERAL_FAILURE;

	int top = scopeStack.size() - 1;

	if (auto search = scopeStack[top]->variables.find(varName); search == scopeStack[top]->variables.end())	{
		std::shared_ptr<Token> newVarTkn = std::make_shared<Token> ();
		*newVarTkn = varValue;
		scopeStack[top]->variables[varName] = newVarTkn;
		ret_code = OK;
	}

	return (ret_code);
}

/* ****************************************************************************
 * Display variables for all scope levels
 * ***************************************************************************/
void VariablesScope::displayVariables()	{
	int scopeTopIdx = scopeStack.size() - 1;

	std::vector<std::wstring> varNames;
	int maxNameLen = 0;

	for (int currIdx = scopeTopIdx; currIdx >= 0; currIdx--)	{
		std::wcout << L"********** BEGIN SCOPE LEVEL " << currIdx << L" **********" << std::endl;
		std::shared_ptr<ScopeFrame> currScope = scopeStack[currIdx];
		varNames.clear();
		for (auto mapr8r = currScope->variables.begin(); mapr8r != currScope->variables.end(); mapr8r++)	{
			std::wstring nxtName = mapr8r->first;
			varNames.push_back (nxtName);
			maxNameLen = (nxtName.size() > maxNameLen ? nxtName.size() : maxNameLen);
		}

		// List variable names for each scope in alphabetical order
		// TODO: Might need to improve on this if we go beyond 1 scope level
		std::sort (varNames.begin(), varNames.end());

		for (auto var8r = varNames.begin(); var8r != varNames.end(); var8r++)	{
			std::shared_ptr<Token> nxtVarTkn = currScope->variables[*var8r];
			std::wstring alignedName = *var8r;
			while (alignedName.size() < maxNameLen)
				alignedName.append (L" ");

			std::wcout << alignedName << L" = " << nxtVarTkn->getValueStr() << L";" << std::endl;
		}

		std::wcout << L"********** <END> SCOPE LEVEL " << currIdx << L" **********" << std::endl;
	}
}
