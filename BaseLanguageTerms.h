/*
 * BaseLanguageTerms.h
 *
 *  Created on: Jan 4, 2024
 *      Author: Mike Volk
 */

#ifndef BASELANGUAGETERMS_H_
#define BASELANGUAGETERMS_H_

#include <cstdint>
#include <string>
#include <list>
#include <iostream>
#include <cassert>
#include <set>
#include <map>
#include <vector>
#include "Token.h"
#include "Operator.h"
#include "Opr8rPrecedenceLvl.h"
#include "OpCodes.h"
#include "ExprTreeNode.h"
#include "Utilities.h"

enum interpreter_modes_enum {
  COMPILE_TIME
  ,INTERPRETER

};

typedef interpreter_modes_enum InterpreterModesType;

class BaseLanguageTerms {

public:
  BaseLanguageTerms();
  virtual ~BaseLanguageTerms();
  bool is_sngl_char_spr8r (wchar_t curr_char);
  bool is_atomic_opr8r (wchar_t curr_char);
  bool is_valid_opr8r (std::wstring pssbl_opr8r, uint8_t usage_mode);
  bool is_valid_datatype (std::wstring pssbl_datatype);
  uint8_t get_type_mask (std::wstring pssbl_opr8r);
  int get_operand_cnt (std::wstring pssbl_opr8r);
  std::wstring get_ternary_1st ();
  std::wstring get_ternary_2nd ();
  std::wstring get_statement_ender();
  bool is_viable_var_name (std::wstring varName);
  bool is_valid_user_data_type (std::wstring inStr);
  bool is_reserved_word (std::wstring inStr);
  bool is_system_call (std::wstring inStr);

  
  // Outer list indicates precedence level.  Multiple OPR8Rs can reside at same precedence level
  std::list<Opr8rPrecedenceLvl> grouped_opr8rs;
  uint8_t getOpCodeFor (std::wstring opr8r);
  std::wstring getSrcOpr8rStrFor (uint8_t op_code);
  int getExecOpr8rDetails (uint8_t op_code, Operator & opr8r);
  std::wstring getUniqExecOpr8rStr (std::wstring srcStr, uint8_t req_type_mask);
  std::wstring getDataTypeForOpCode (uint8_t op_code);
  TokenTypeEnum getTokenTypeForOpCode (uint8_t op_code);
  std::pair<TokenTypeEnum, uint8_t> getDataType_tknEnum_opCode (std::wstring keyword);
  std::wstring getOpr8rsInPrecedenceList();
  int get_system_call_details (std::wstring sys_call, std::vector<uint8_t> & param_list, TokenTypeEnum & data_type);
  int get_num_sys_call_parameters (std::wstring sys_call, int & num_params);
  int tkn_type_converts_to_opcode (uint8_t op_code, Token & check_token, std::wstring variable_name, std::wstring & error_msg);

  void dumpTokenList (std::vector<Token> & tokenStream, std::wstring callersSrcFile, int lineNum);
  void dumpTokenList (std::vector<Token> & tokenStream, int startIdx, std::wstring callersSrcFile, int lineNum);
  void dumpTokenList (TokenPtrVector & tknPtrVector, std::wstring callersSrcFile, int lineNum, bool isShowDetail);
  void dumpTokenList (TokenPtrVector & tknPtrVector, int startIdx, std::wstring callersSrcFile, int lineNum, bool isShowDetail);

  // TODO: Is this the right place for these to live?
  int append_to_flat_tkn_list (std::shared_ptr<ExprTreeNode> tree_node, std::vector<Token> & flatExprTknList);
  int flattenExprTree (std::shared_ptr<ExprTreeNode> rootOfExpr, std::vector<Token> & flatExprTknList);
  int flatten_system_call (std::shared_ptr<ExprTreeNode> sys_call_node, std::vector<Token> & flat_tkn_list);
  int append_flattened_system_call (std::shared_ptr<ExprTreeNode> tree_node, std::vector<Token> & flatExprTknList);

protected:
  int failed_on_src_line;  
  std::wstring this_src_file;
  
  std::wstring atomic_1char_opr8rs;
  std::wstring _1char_spr8rs;
  std::map<std::wstring, std::pair<TokenTypeEnum, uint8_t>> valid_data_types;
  std::vector<std::wstring> reserved_words;
  // system call name, {parameter list, return data_type}
  std::map <std::wstring, std::pair <std::vector<uint8_t>, TokenTypeEnum>> system_calls;
  std::wstring ternary_1st;
  std::wstring ternary_2nd;
  std::wstring statement_ender;
  std::map <std::wstring, Operator> execTimeOpr8rMap;
  std::map <std::wstring, std::wstring> execToSrcOpr8rMap;

  void validityCheck();


private:
  int makeFlatExpr_OLR (std::shared_ptr<ExprTreeNode> currBranch, std::vector<Token> & flatExprTknList);
  Utilities util;

};

#endif /* BASELANGUAGETERMS_H_ */
