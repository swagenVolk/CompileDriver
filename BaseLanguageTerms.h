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
  uint8_t getOpr8rOpCode (std::wstring opr8r);

protected:
  void validityCheck();
  std::wstring atomic_1char_opr8rs;
  std::wstring _1char_spr8rs;
  std::set<std::wstring> valid_data_types;
  std::wstring ternary_1st;
  std::wstring ternary_2nd;
  std::wstring statement_ender;
  std::map <std::wstring, int> opr8rOpCodes;
};

#endif /* BASELANGUAGETERMS_H_ */
