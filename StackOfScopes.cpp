/*
 * StackOfScopes.cpp
 *
 *  Created on: Feb 8, 2025
 *      Author: Mike Volk
 * 
 * Conceptually this is a NameSpace that holds variables in their respective scope levels
 */

#include "StackOfScopes.h"
#include "InfoWarnError.h"
#include "InterpretedFileWriter.h"
#include "OpCodes.h"
#include "common.h"
#include <cstdint>
#include <iostream>
#include <memory>

StackOfScopes::StackOfScopes() {
  Token rootScopeTkn (INTERNAL_USE_TKN, L"__ROOT_SCOPE");
  std::shared_ptr<ScopeWindow> rootScope = std::make_shared<ScopeWindow> (INVALID_OPCODE, rootScopeTkn, 0, 0);
  scopeStack.push_back(rootScope);
  thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");

}

StackOfScopes::~StackOfScopes() {
  thisSrcFile.clear();
  reset();
}

void StackOfScopes::reset() {
  while (scopeStack.size() > 0) {
    std::shared_ptr<ScopeWindow> top = scopeStack[scopeStack.size() - 1];
    scopeStack.erase(scopeStack.end());
    top.reset();
  }

}

/* ****************************************************************************
 * Look up this variable in our scopeStack|NameSpace. Only update the variable
 * if isCommitUpdate = COMMIT_UPDATE (true)
 * ***************************************************************************/
int StackOfScopes::findVar(std::wstring varName, int maxLevels, Token & updateValTkn
  , ReadOrWrite readOrWrite, std::wstring & errorMsg) {
  int ret_code = GENERAL_FAILURE;
  bool isFound = false;
  errorMsg.clear();

  int scopeTopIdx = scopeStack.size() - 1;
  int endScopeIdx;
  if (maxLevels <= 0)
    endScopeIdx = 0;
  else
    endScopeIdx = scopeTopIdx - maxLevels + 1;

  for (int currIdx = scopeTopIdx; currIdx >= endScopeIdx && !isFound; currIdx--)  {
    std::shared_ptr<ScopeWindow> currScope = scopeStack[currIdx];
    if (auto search = currScope->variables.find(varName); search != currScope->variables.end()) {
      // TODO: existingTkn is probably going to need to be a POINTER for the update to stick
      std::shared_ptr<Token> existingTkn = search->second;
      isFound = true;
      if (COMMIT_WRITE == readOrWrite)  {
        std::wstring errMsg;

        if (OK == existingTkn->convertTo(updateValTkn, varName, errorMsg))  {
          // TODO: What info can I supply to user to resolve src line # etc?
          existingTkn->isInitialized = true;
          ret_code = OK;
        }
      } else  {
        updateValTkn = *existingTkn;
        ret_code = OK;
      }
    }
  }

  return (ret_code);

}

/* ****************************************************************************
 * Insert this variable at the current scope.  Fail if it already exists
 * ***************************************************************************/
int StackOfScopes::insertNewVarAtCurrScope (std::wstring varName, Token varValue) {
  int ret_code = GENERAL_FAILURE;

  int top = scopeStack.size() - 1;

  if (auto search = scopeStack[top]->variables.find(varName); search == scopeStack[top]->variables.end()) {
    std::shared_ptr<Token> newVarTkn = std::make_shared<Token> ();
    *newVarTkn = varValue;
    scopeStack[top]->variables[varName] = newVarTkn;
    ret_code = OK;
  }

  return (ret_code);
}

/* ****************************************************************************
 * Open a new scope with info that indicates where it started
 * ***************************************************************************/
int StackOfScopes::openNewScope (uint8_t openedByOpCode, Token scopenerTkn, uint32_t startScopeFilePos, uint32_t scopeLen)  {
  int ret_code = GENERAL_FAILURE;

  std::shared_ptr<ScopeWindow> openedScope = std::make_shared<ScopeWindow> (openedByOpCode, scopenerTkn, startScopeFilePos, scopeLen);
  openedScope->openerTkn = scopenerTkn;
  scopeStack.push_back(openedScope);
  
  ret_code = OK;

  return (ret_code);
}

/* ****************************************************************************
 * Close the current top level, non-ROOT level scope
 * ***************************************************************************/
int StackOfScopes::srcCloseTopScope (InterpretedFileWriter & interpretedFileWriter, uint8_t & closedScopeOpCode, closeScopeErr & closeErr)  {
  return (srcCloseTopScope(interpretedFileWriter, closedScopeOpCode, closeErr, false));
}

/* ****************************************************************************
 * Close the current top level scope
 * TODO: I should probably call this in GeneralParser.cpp for root scope compile
 * ***************************************************************************/
int  StackOfScopes::srcCloseTopScope (InterpretedFileWriter & interpretedFileWriter, uint8_t & closedScopeOpCode
  , closeScopeErr & closeErr, bool isRootScope) {
  int ret_code = GENERAL_FAILURE;
  closeErr = SCOPE_CLOSE_UKNOWN_ERROR;
  bool isDeleteReady = false;

  if (scopeStack.size() == 0) 
    closeErr = NO_SCOPES_OPEN;

  else if (!isRootScope && scopeStack.size() == 1)  
    closeErr = ONLY_ROOT_SCOPE_OPEN;

  else if (isRootScope && scopeStack.size() > 1)  
    closeErr = SCOPES_OPEN_ABOVE_ROOT;

  else if (!isRootScope && scopeStack.size() > 1) 
    isDeleteReady = true;

  else if (isRootScope && scopeStack.size() == 1) 
    isDeleteReady = true;

  if (isDeleteReady)  {
    std::shared_ptr<ScopeWindow> top = scopeStack[scopeStack.size() - 1];

    uint32_t scopeObjFilePos = top->boundary_begin_pos;
    closedScopeOpCode = top->opener_opcode;

    scopeStack.erase(scopeStack.end());
    top.reset();

    if (isRootScope)
      ret_code = OK;
    else
      ret_code =  interpretedFileWriter.writeObjectLen (scopeObjFilePos);

    if (ret_code == OK)
      closeErr = SCOPE_CLOSED_OK;
  }

  return (ret_code);
}

/* ****************************************************************************
 * Close the current top level scope
 * ***************************************************************************/
 int StackOfScopes::closeTopScope (uint8_t closedScopeOpCode, closeScopeErr & closeErr, bool isRootScope) {
  int ret_code = GENERAL_FAILURE;
  closeErr = SCOPE_CLOSE_UKNOWN_ERROR;
  bool isDeleteReady = false;

  if (scopeStack.size() == 0) 
    closeErr = NO_SCOPES_OPEN;

  else if (!isRootScope && scopeStack.size() == 1)  
    closeErr = ONLY_ROOT_SCOPE_OPEN;

  else if (isRootScope && scopeStack.size() > 1)  
    closeErr = SCOPES_OPEN_ABOVE_ROOT;

  else if (!isRootScope && scopeStack.size() > 1) 
    isDeleteReady = true;

  else if (isRootScope && scopeStack.size() == 1) 
    isDeleteReady = true;

  if (isDeleteReady)  {
    std::shared_ptr<ScopeWindow> top = scopeStack[scopeStack.size() - 1];

    if (top->opener_opcode != closedScopeOpCode)  {
      closeErr = SCOPE_OPCODE_MISMATCH;

    } else {
      scopeStack.erase(scopeStack.end());
      top.reset();
      ret_code = OK;
      closeErr = SCOPE_CLOSED_OK;
    }

  }

  return (ret_code);
}

/* ****************************************************************************
 * Are we currently nested inside a loop?
 * ***************************************************************************/
bool StackOfScopes::isInsideLoop (uint32_t & loop_boundary_end_pos, bool is_inc_break_cnt)  {
  bool isInLoop = false;
  loop_boundary_end_pos = 0;
  
  for (int idx = scopeStack.size() - 1; idx >= 0 && !isInLoop; idx--)  {
    if (scopeStack[idx]->opener_opcode == FOR_SCOPE_OPCODE || scopeStack[idx]->opener_opcode == WHILE_SCOPE_OPCODE) {
      loop_boundary_end_pos = scopeStack[idx]->boundary_end_pos;
      isInLoop = true;
      if (is_inc_break_cnt)
        scopeStack[idx]->loop_break_cnt++;
    }
  }

  return isInLoop;
}

/* ****************************************************************************
 * Display variables for all scope levels
 * ***************************************************************************/
void StackOfScopes::displayVariables()  {
  int scopeTopIdx = scopeStack.size() - 1;

  std::vector<std::wstring> varNames;
  int maxNameLen = 0;
  std::wcout << L"/* ********** <SHOW VARIABLES & VALUES> ********** */" << std::endl;

  for (int currIdx = scopeTopIdx; currIdx >= 0; currIdx--)  {
    std::wcout << L"// ********** <SCOPE LEVEL " << currIdx << L"> ********** " << std::endl;
    std::shared_ptr<ScopeWindow> currScope = scopeStack[currIdx];
    if (currScope->openerTkn.tkn_type != START_UNDEF_TKN)
      std::wcout << L"// Scope opened by: " << currScope->openerTkn.descr_line_num_col() << std::endl;

    varNames.clear();
    for (auto mapr8r = currScope->variables.begin(); mapr8r != currScope->variables.end(); mapr8r++)  {
      std::wstring nxtName = mapr8r->first;
      varNames.push_back (nxtName);
      maxNameLen = (nxtName.size() > maxNameLen ? nxtName.size() : maxNameLen);
    }

    // List variable names for each scope in alphabetical order
    // TODO: Might need to improve on this if we go beyond 1 scope level
    std::sort (varNames.begin(), varNames.end());

    for (auto var8r = varNames.begin(); var8r != varNames.end(); var8r++) {
      std::shared_ptr<Token> nxtVarTkn = currScope->variables[*var8r];
      std::wstring alignedName = *var8r;
      while (alignedName.size() < maxNameLen)
        alignedName.append (L" ");

      std::wcout << alignedName << L" = " << nxtVarTkn->getValueStr() << L";" << std::endl;
    }

    std::wcout << L"// ********** </SCOPE LEVEL " << currIdx << L"> ********** " << std::endl;
  }

  std::wcout << L"/* ********** </SHOW VARIABLES & VALUES> ********** */" << std::endl;
}


/* ****************************************************************************
 * 
 * ***************************************************************************/
 int StackOfScopes::get_top_opener_tkn (Token & opener_tkn)  {
  int ret_code = GENERAL_FAILURE;

  int top_idx = scopeStack.size() - 1;

  if (top_idx >= 0) {
    opener_tkn = scopeStack[top_idx]->openerTkn;
    ret_code = OK;
  }

  return ret_code;
}

/* ****************************************************************************
 * 
 * ***************************************************************************/
 int StackOfScopes::get_top_opener_opcode (uint8_t & op_code)  {
  int ret_code = GENERAL_FAILURE;
  op_code = INVALID_OPCODE;

  int top_idx = scopeStack.size() - 1;

  if (top_idx >= 0) {
    op_code = scopeStack[top_idx]->opener_opcode;    
    ret_code = OK;
  }

  return ret_code;

}

/* ****************************************************************************
 * 
 * ***************************************************************************/
 int StackOfScopes::get_top_boundary_begin_pos (uint32_t & begin_pos)  {
  int ret_code = GENERAL_FAILURE;
  begin_pos = 0;

  int top_idx = scopeStack.size() - 1;

  if (top_idx >= 0) {
    begin_pos = scopeStack[top_idx]->boundary_begin_pos;
    ret_code = OK;
  }

  return ret_code;

}

/* ****************************************************************************
 * 
 * ***************************************************************************/
 int StackOfScopes::get_top_loop_break_cnt (int & break_cnt) {
  int ret_code = GENERAL_FAILURE;

  break_cnt = 0;
  int top_idx = scopeStack.size() - 1;

  if (top_idx >= 0) {
    break_cnt = scopeStack[top_idx]->loop_break_cnt;
    ret_code = OK;
  }

  return ret_code;

}

/* ****************************************************************************
 * 
 * ***************************************************************************/
 int StackOfScopes::get_top_is_exists_for_loop_cond (bool & is_exists) {
  int ret_code = GENERAL_FAILURE;

  int top_idx = scopeStack.size() - 1;

  if (top_idx >= 0) {
    is_exists = scopeStack[top_idx]->is_exists_for_loop_cond;
    ret_code = OK;
  }

  return ret_code;

}

/* ****************************************************************************
 * 
 * ***************************************************************************/
 int StackOfScopes::set_top_is_exists_for_loop_cond (bool & is_exists) {
  int ret_code = GENERAL_FAILURE;

  int top_idx = scopeStack.size() - 1;

  if (top_idx >= 0) {
    scopeStack[top_idx]->is_exists_for_loop_cond = is_exists;
    ret_code = OK;
  }

  return ret_code;

}
