/*
 * BaseOperators.cpp
 * Base class to hold common fxns that check internal lists for separator, operator and datatype
 * info. Note that the internal lists will have different contents based on what the derived
 * class puts in or removes from the list.
 *
 *  Created on: Jan 4, 2024
 *      Author: Mike Volk
 */

#include "BaseLanguageTerms.h"

BaseLanguageTerms::BaseLanguageTerms() {
	// TODO Auto-generated constructor stub

}

BaseLanguageTerms::~BaseLanguageTerms() {
	// TODO Auto-generated destructor stub
}

/* ****************************************************************************
 *
 * ***************************************************************************/
void BaseLanguageTerms::validityCheck()	{
  // TODO - Check ordering of base class / derived class constructors.  May need to move the code below out of the constructor
	// Double check caller for any OPR8Rs in SPR8Rs
  for(std::wstring::iterator o = atomic_1char_opr8rs.begin(); o != atomic_1char_opr8rs.end(); ++o) {
    if (_1char_spr8rs.find(*o) != std::wstring::npos) {
      std::wcout << "ERROR: file_parser::file_parser: Found OPR8R " << *o << " in SPR8R list " << _1char_spr8rs << "; " << std::endl;
      assert (_1char_spr8rs.find(*o) == std::wstring::npos);
    }
  }

  // Double check caller for any SPR8Rs in OPR8Rs
  for(std::wstring::iterator s = _1char_spr8rs.begin(); s != _1char_spr8rs.end(); ++s) {
    if (atomic_1char_opr8rs.find(*s) != std::wstring::npos) {
      std::wcout << "ERROR: file_parser::file_parser: Found SPR8R " << *s << " in OPR8R list " << atomic_1char_opr8rs << "; " << std::endl;
      assert (atomic_1char_opr8rs.find(*s) == std::wstring::npos);
    }
  }

	std::wstring opr8r;
  std::list<Opr8rPrecedenceLvl>::iterator outr8r;
  std::list<Operator>::iterator innr8r;
  Opr8rPrecedenceLvl precedenceLvl;

  for(std::wstring::iterator o = atomic_1char_opr8rs.begin(); o != atomic_1char_opr8rs.end(); ++o) {
  	opr8r.clear();
  	opr8r.push_back(*o);
    bool isFound = false;

    for (outr8r = grouped_opr8rs.begin(); outr8r != grouped_opr8rs.end() && !isFound; outr8r++){
    	precedenceLvl = *outr8r;

      for (innr8r = precedenceLvl.opr8rs.begin(); innr8r != precedenceLvl.opr8rs.end(); ++innr8r){
    		Operator nxtDefOpr8r = *innr8r;
    		if (0 == opr8r.compare(nxtDefOpr8r.symbol))	{
    			isFound = true;
    			break;
    		}
    	}
    }

  	if (!isFound) {
  		std::wcout << "ERROR: file_parser::file_parser: Did not find single char atomic OPR8R " << *o << " in list of valid operators;" << std::endl;
  		assert (_1char_spr8rs.find(*o) == std::wstring::npos);
  	}
  }

  int ternary1stCnt = 0;
  int ternary2ndCnt = 0;
  int statementEnderCnt = 0;


  for (outr8r = grouped_opr8rs.begin(); outr8r != grouped_opr8rs.end(); outr8r++){
  	precedenceLvl = *outr8r;

  	for (innr8r = precedenceLvl.opr8rs.begin(); innr8r != precedenceLvl.opr8rs.end(); ++innr8r){
  		Operator nxtDefOpr8r = *innr8r;
  		if (TERNARY_1ST == (innr8r->type_mask & TERNARY_1ST))	{
  			ternary_1st = innr8r->symbol;
  			ternary1stCnt++;
  		}

  		if (TERNARY_2ND == (innr8r->type_mask & TERNARY_2ND))	{
  			ternary_2nd = innr8r->symbol;
  			ternary2ndCnt++;
  		}

  		if (STATEMENT_ENDER == (innr8r->type_mask & STATEMENT_ENDER))	{
  			statement_ender = innr8r->symbol;
  			statementEnderCnt++;
  		}
  	}
  }

	assert ((ternary1stCnt == 0 && ternary2ndCnt == 0) || (ternary1stCnt == 1 && ternary2ndCnt == 1));
	assert (statementEnderCnt == 1);

}

/* ****************************************************************************
 * Determine if curr_char is a single charactor separator or not.
 * ***************************************************************************/
bool BaseLanguageTerms::is_sngl_char_spr8r (wchar_t curr_char) {
  bool is_spr8r = false;

  if (_1char_spr8rs.find(curr_char) != std::wstring::npos)
    is_spr8r = true;

  return (is_spr8r);
}

/* ****************************************************************************
 * Determine if curr_char is a single charactor operator or not.
 * NOTE: Most OPR8Rs will contain multiple characters, but the statements below
 * num_tomatoes = 3;;;;;;;
 * are legal, but the extra ; will pop an empty stack
 * ***************************************************************************/
bool BaseLanguageTerms::is_atomic_opr8r (wchar_t curr_char) {
  bool is_opr8r = false;

  if (atomic_1char_opr8rs.find(curr_char) != std::wstring::npos)
    is_opr8r = true;

  return (is_opr8r);
}

/* ****************************************************************************
 * Check if term is a valid OPR8R or not.
 * ***************************************************************************/
bool BaseLanguageTerms::is_valid_opr8r (std::wstring check_for_opr8r, uint8_t usage_mode) {
  bool is_valid = false;


  std::list<Opr8rPrecedenceLvl>::iterator outr8r;
  std::list<Operator>::iterator innr8r;
  Opr8rPrecedenceLvl precedenceLvl;

    for (outr8r = grouped_opr8rs.begin(); outr8r != grouped_opr8rs.end() && !is_valid; outr8r++){
    	precedenceLvl = *outr8r;

      for (innr8r = precedenceLvl.opr8rs.begin(); innr8r != precedenceLvl.opr8rs.end() && !is_valid; ++innr8r){
    		Operator nxtOpr8r = *innr8r;

      	if (0 == nxtOpr8r.symbol.compare(check_for_opr8r))	{
      		// Operator string matches
      		if (usage_mode == (usage_mode & nxtOpr8r.valid_for_mask))	{
      			// And the current usage_mode is acceptable also
      			is_valid = true;
      		}
      		break;
      	}
      }
    }

  return is_valid;
}

/* ****************************************************************************
 * Check if term is a valid datatype
 * ***************************************************************************/
bool BaseLanguageTerms::is_valid_datatype (std::wstring check_for_datatype)	{
  bool is_valid = false;

  if (auto search = valid_data_types.find(check_for_datatype); search != valid_data_types.end())
		is_valid = true;

//  for (int idx = 0; idx < valid_data_types.size() && !is_valid; idx++)	{
//  	std::wstring nxtDatatype = valid_data_types[idx];
//  	if (0 == nxtDatatype.compare(check_for_datatype))	{
//  		is_valid = true;
//  		break;
//  	}
//  }

  return is_valid;
}

/* ****************************************************************************
 * Check if this OPR8R has the type_mask the caller expects
 * ***************************************************************************/
uint8_t BaseLanguageTerms::get_type_mask (std::wstring pssbl_opr8r)	{
	uint8_t opr8r_mask = 0x0;

  std::list<Opr8rPrecedenceLvl>::iterator outr8r;
  std::list<Operator>::iterator innr8r;
  Opr8rPrecedenceLvl precedenceLvl;

	for (outr8r = grouped_opr8rs.begin(); outr8r != grouped_opr8rs.end() && opr8r_mask == 0x0; outr8r++){
		precedenceLvl = *outr8r;

		for (innr8r = precedenceLvl.opr8rs.begin(); innr8r != precedenceLvl.opr8rs.end(); ++innr8r){
			Operator nxtOpr8r = *innr8r;
			if (0 == nxtOpr8r.symbol.compare(pssbl_opr8r))	{
				opr8r_mask = nxtOpr8r.type_mask;
				break;
			}
		}
	}

	return opr8r_mask;
}



/* ****************************************************************************
 * Get the number of operands this OPR8R requires
 * ***************************************************************************/
int BaseLanguageTerms::get_operand_cnt (std::wstring pssbl_opr8r)	{
	int rand_cnt = -1;

  std::list<Opr8rPrecedenceLvl>::iterator outr8r;
  std::list<Operator>::iterator innr8r;
  Opr8rPrecedenceLvl precedenceLvl;

	for (outr8r = grouped_opr8rs.begin(); outr8r != grouped_opr8rs.end() && rand_cnt == -1; outr8r++){
		precedenceLvl = *outr8r;

		for (innr8r = precedenceLvl.opr8rs.begin(); innr8r != precedenceLvl.opr8rs.end(); ++innr8r){
			Operator nxtOpr8r = *innr8r;
	  	if (0 == nxtOpr8r.symbol.compare(pssbl_opr8r))	{
	  		rand_cnt = nxtOpr8r.numOperands;
	  		break;
	  	}
		}
	}

	return rand_cnt;
}

/* ****************************************************************************
 * Return the starting ternary opr8r string
 * ***************************************************************************/
std::wstring BaseLanguageTerms::get_ternary_1st ()	{
	return ternary_1st;
}

/* ****************************************************************************
 * Return the middle ternary opr8r string
 * ***************************************************************************/
std::wstring BaseLanguageTerms::get_ternary_2nd ()	{
	return ternary_2nd;
}

/* ****************************************************************************
 * Return the 1 & only STATEMENT_ENDER opr8r string
 * ***************************************************************************/
std::wstring BaseLanguageTerms::get_statement_ender()	{
	return statement_ender;
}

/* ****************************************************************************
 * Return the BYTE sized opCode for this OPR8R
 * ***************************************************************************/
uint8_t BaseLanguageTerms::getOpr8rOpCode (std::wstring opr8r)	{
	uint8_t opCode = 	INVALID_OPCODE;

	if (auto search = opr8rOpCodes.find(opr8r); search != opr8rOpCodes.end())
		opCode = search->second;

	return (opCode);
}
