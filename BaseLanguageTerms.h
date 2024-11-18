/*
 * BaseOperators.h
 *
 *  Created on: Jan 4, 2024
 *      Author: Mike Volk
 */

#ifndef BASELANGUAGETERMS_H_
#define BASELANGUAGETERMS_H_

#include <string>
#include <list>
#include <iostream>
#include <cassert>
#include <set>
#include <map>
#include "common.h"
#include "Token.h"
#include "Operator.h"
#include "Opr8rPrecedenceLvl.h"
#include "OpCodes.h"

// typedef std::map <std::wstring, std::int> r8rKey_reqRandCntVal;


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

  // Outer list indicates precedence level.  Multiple OPR8Rs can reside at same precedence level
  std::list<Opr8rPrecedenceLvl> grouped_opr8rs;
  uint8_t getOpCodeFor (std::wstring opr8r);
  std::wstring getSrcOpr8rStrFor (uint8_t op_code);
  int getExecOpr8rDetails (uint8_t op_code, Operator & opr8r);
  std::wstring getUniqExecOpr8rStr (std::wstring srcStr, uint8_t req_type_mask);
  std::wstring getDataTypeForOpCode (uint8_t op_code);
  std::pair<TokenTypeEnum, uint8_t> getDataType_tknEnum_opCode (std::wstring keyword);

protected:
  void validityCheck();
  std::wstring atomic_1char_opr8rs;
  std::wstring _1char_spr8rs;
  std::map<std::wstring, std::pair<TokenTypeEnum, uint8_t>> valid_data_types;
  std::wstring ternary_1st;
  std::wstring ternary_2nd;
  std::wstring statement_ender;
  std::map <std::wstring, Operator> execTimeOpr8rMap;
  std::map <std::wstring, std::wstring> execToSrcOpr8rMap;
};

#endif /* BASELANGUAGETERMS_H_ */
