/*
 * BaseLanguageTerms.cpp
 * Base class to hold common fxns that check internal lists for separator, operator and datatype
 * info. Note that the internal lists will have different contents based on what the derived
 * class puts in or removes from the list.
 *
 *  Created on: Jan 4, 2024
 *      Author: Mike Volk
 */

#include "BaseLanguageTerms.h"
#include "OpCodes.h"
#include "Operator.h"
#include "Token.h"
#include <iostream>
#include <string>
#include <vector>

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
	//	Make sure there's no overlap between: valid_data_types reserved_words OPR8Rs SPR8Rs
	//	Check for duplicates in each

	std::map<std::wstring, int> nameReferenceCnt;
	int idx;

	// Reference count valid_data_types
	for (auto itr8r = valid_data_types.begin(); itr8r != valid_data_types.end(); itr8r++)	{
		std::wstring next_type = itr8r->first;
		assert (!next_type.empty());
		auto search = nameReferenceCnt.find(next_type);
		if (search == nameReferenceCnt.end())	{
			nameReferenceCnt.insert(std::pair {next_type, 1});
		
		} else	{
			search->second++;
		}
	}

	// Reference count reserved words
	for (idx = 0; idx < reserved_words.size(); idx++)	{
		std::wstring next_type = reserved_words[idx];
		assert (!next_type.empty());
		auto search = nameReferenceCnt.find(next_type);
		if (search == nameReferenceCnt.end())	{
			nameReferenceCnt.insert(std::pair {next_type, 1});
		
		} else	{
			search->second++;
		}
	}

	// Reference count SPR8Rs
	for (idx = 0; idx < _1char_spr8rs.size(); idx++)	{
		std::wstring nextChar = std::to_wstring(_1char_spr8rs.at(idx));
		assert (!nextChar.empty());
		auto search = nameReferenceCnt.find(nextChar);
		if (search == nameReferenceCnt.end())	{
			nameReferenceCnt.insert(std::pair {nextChar, 1});
		
		} else	{
			search->second++;
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
  		std::wcout << "ERROR: Did not find single char atomic OPR8R " << *o << " in list of valid operators;" << std::endl;
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

			std::wstring opr8r = innr8r->symbol;

  		if (TERNARY_1ST == (innr8r->type_mask & TERNARY_1ST))	{
  			ternary_1st = opr8r;
  			ternary1stCnt++;
  		}

  		if (TERNARY_2ND == (innr8r->type_mask & TERNARY_2ND))	{
  			ternary_2nd = opr8r;
  			ternary2ndCnt++;
  		}

  		if (STATEMENT_ENDER == (innr8r->type_mask & STATEMENT_ENDER))	{
  			statement_ender = opr8r;
  			statementEnderCnt++;
  		}

  		if (nxtDefOpr8r.valid_usage & GNR8D_SRC)	{
  	  	// Add to our map of exec time OPR8Rs
	 			assert (nxtDefOpr8r.op_code != INVALID_OPCODE);
  			assert (0 == execTimeOpr8rMap.count (nxtDefOpr8r.symbol));
  			execTimeOpr8rMap[nxtDefOpr8r.symbol] = nxtDefOpr8r;

				// Reference count USR_SRC OPR8Rs
				assert (!opr8r.empty());
				auto search = nameReferenceCnt.find(opr8r);
				if (search == nameReferenceCnt.end())	{
					nameReferenceCnt.insert(std::pair {opr8r, 1});
				
				} else	{
					search->second++;
				}
			}
  	}
  }

	assert ((ternary1stCnt == 0 && ternary2ndCnt == 0) || (ternary1stCnt == 1 && ternary2ndCnt == 1));
	assert (statementEnderCnt == 1);

	bool isDupesFound = false;
	for (auto refr8r = nameReferenceCnt.begin(); refr8r != nameReferenceCnt.end(); refr8r++)	{
		if (refr8r->second != 1)	{
			std::wcout << L"RESERVED word, data type, operator or separator used more than once [" << refr8r->first << L", " << refr8r->second << L"]" << std::endl;
			isDupesFound = true;
		}
	}

	assert (!isDupesFound);

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
      		if (usage_mode == (usage_mode & nxtOpr8r.valid_usage))	{
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
	  		rand_cnt = nxtOpr8r.numReqSrcOperands;
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
uint8_t BaseLanguageTerms::getOpCodeFor (std::wstring opr8r)	{
	uint8_t op_code = INVALID_OPCODE;

	if (auto search = execTimeOpr8rMap.find(opr8r); search != execTimeOpr8rMap.end())	{
		Operator r8r = search->second;
		op_code = r8r.op_code;
	}

	if (op_code == INVALID_OPCODE)
		std::wcout << L"TODO: DEBUG" << std::endl;

	return (op_code);
}

/* ****************************************************************************
 * Return the OPR8R for the passed in BYTE sized opCode
 * ***************************************************************************/
std::wstring BaseLanguageTerms::getSrcOpr8rStrFor (uint8_t op_code)	{
	std::wstring srcOpr8rStr = L"";
	Operator opr8r;

	if (OK == getExecOpr8rDetails (op_code, opr8r))	{
		std::wstring execOpr8rStr = opr8r.symbol;

		if (auto search = execToSrcOpr8rMap.find(execOpr8rStr); search != execToSrcOpr8rMap.end())	{
			// Corresponding source OPR8R string was found in our special case map used to
			// disambiguate OPR8Rs like [++] [--] which can be PREFIX|POSTFIX
			// and [+] [-] which can be UNARY|BINARY
			srcOpr8rStr = search->second;
		}

		if (srcOpr8rStr.empty())	{
			srcOpr8rStr = opr8r.symbol;
		}
	}

	return (srcOpr8rStr);
}

/* ****************************************************************************
 * Fill in the OPR8R object details, if found
 * ***************************************************************************/
int BaseLanguageTerms::getExecOpr8rDetails (uint8_t op_code, Operator & callers_opr8r)	{
	int ret_code = GENERAL_FAILURE;

	for (auto itr8r = execTimeOpr8rMap.begin(); itr8r != execTimeOpr8rMap.end(); itr8r++)	{
		Operator r8r = itr8r->second;

		if (r8r.op_code == op_code)	{
			callers_opr8r = r8r;
			ret_code = OK;
			break;
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * For the passed ambiguous source OPR8R, search the special case map used
 * for disambiguation and find the exec OPR8R that matches the req_type_mask
 * ***************************************************************************/
std::wstring BaseLanguageTerms::getUniqExecOpr8rStr (std::wstring srcStr, uint8_t req_type_mask)	{
	std::wstring execOpr8rStr = L"";

	std::vector <std::wstring> pssblExecOpr8rs;
	std::vector <Operator> matchingOpr8rs;

	// TODO: decrypt, disambiguate, clarify, uniqify.....
	for (auto itr8r = execToSrcOpr8rMap.begin(); itr8r != execToSrcOpr8rMap.end(); itr8r++)	{
		if (itr8r->second == srcStr)	{
			// Matched on the passed in source OPR8R string
			pssblExecOpr8rs.push_back (itr8r->first);
		}
	}

	if (pssblExecOpr8rs.empty())
		// No disambiguation matches from execToSrcOpr8rMap
		pssblExecOpr8rs.push_back (srcStr);

	std::list<Opr8rPrecedenceLvl>::iterator outr8r;
	std::list<Operator>::iterator innr8r;
	std::vector <std::wstring>::iterator pssblR8r;
	Opr8rPrecedenceLvl precedenceLvl;

	for (outr8r = grouped_opr8rs.begin(); outr8r != grouped_opr8rs.end(); outr8r++){
		precedenceLvl = *outr8r;

		for (innr8r = precedenceLvl.opr8rs.begin(); innr8r != precedenceLvl.opr8rs.end(); ++innr8r){
			Operator nxtOpr8r = *innr8r;

			if (req_type_mask == (req_type_mask & nxtOpr8r.type_mask) && (nxtOpr8r.valid_usage & GNR8D_SRC))	{
				// The current usage mode of this defined OPR8R meets the search criteria
				for (pssblR8r = pssblExecOpr8rs.begin(); pssblR8r != pssblExecOpr8rs.end(); pssblR8r++)	{
					if (0 == nxtOpr8r.symbol.compare(*pssblR8r))	{
						// Operator string directly matches
						matchingOpr8rs.push_back(nxtOpr8r);
						break;
					}
				}
			}
		}
	}

	if (matchingOpr8rs.size() == 1)
		execOpr8rStr = matchingOpr8rs[0].symbol;

	return (execOpr8rStr);
}



/* ****************************************************************************
 * If the passed in USER_WORD is a valid data type, return the associated Token type
 * enum and op_code. Otherwise, return an obviously invalid (hopefully)l pair
 * ***************************************************************************/
std::pair<TokenTypeEnum, uint8_t> BaseLanguageTerms::getDataType_tknEnum_opCode (std::wstring user_word)	{
	std::pair ret_info {START_UNDEF_TKN, INVALID_OPCODE};

	if (auto search = valid_data_types.find(user_word); search != valid_data_types.end())	{
		std::pair tknEnum_opCode = search->second;
		ret_info = tknEnum_opCode;
	}

	return (ret_info);
}

/* ****************************************************************************
 * If the passed in op_code represents a valid data type, a valid datatype string
 * will be returned. Otherwise, return an empty string.
 * ***************************************************************************/
std::wstring BaseLanguageTerms::getDataTypeForOpCode (uint8_t op_code)	{
	std::wstring dataType;

	for (auto itr8r = valid_data_types.begin(); itr8r != valid_data_types.end(); itr8r++)	{
		std::pair tknEnum_opCode = itr8r->second;
		if (tknEnum_opCode.second == op_code)	{
			dataType = itr8r->first;
			break;
		}
	}

	return (dataType);

}

/* ****************************************************************************
 * If the passed in op_code represents a valid data type, a valid datatype string
 * will be returned. Otherwise, return an empty string.
 * ***************************************************************************/
TokenTypeEnum BaseLanguageTerms::getTokenTypeForOpCode (uint8_t op_code)	{
	TokenTypeEnum tknType = START_UNDEF_TKN;

	for (auto itr8r = valid_data_types.begin(); itr8r != valid_data_types.end(); itr8r++)	{
		std::pair tknEnum_opCode = itr8r->second;
		if (tknEnum_opCode.second == op_code)	{
			tknType = tknEnum_opCode.first;
			break;
		}
	}

	return (tknType);

}

/* ****************************************************************************
 *
 * ***************************************************************************/
bool BaseLanguageTerms::isDataType (std::wstring inStr)	{
	bool isValid = false;

	if ( auto search = valid_data_types.find(inStr); search != valid_data_types.end())	{
		isValid = true;
	}

	return isValid;
}

/* ****************************************************************************
 *
 * ***************************************************************************/
bool BaseLanguageTerms::isReservedWord (std::wstring inStr)	{
	bool isValid = false;

		for (auto itr8r = reserved_words.begin(); itr8r != reserved_words.end() && !isValid; itr8r++)	{
			if (0 == inStr.compare(*itr8r))
				isValid = true;
		}

	return isValid;
}

/* ****************************************************************************
 *
 * ***************************************************************************/
bool BaseLanguageTerms::isViableVarName (std::wstring varName)	{
	bool isViable = true;

	if (isReservedWord(varName))	{
		isViable = false;
	
	} else if (isDataType(varName))	{
		isViable = false;

	} else {
		int idx;
		for (idx = 0; idx < varName.length() && isViable; idx++)	{
			wchar_t currChar = varName[idx];

			if (idx == 0 && std::isdigit(currChar))
				// Variable names can't start with numbers
				isViable = false;

			if (currChar != '_' && !std::iswalnum(currChar))
				isViable = false;
		}
	}

	return (isViable);
}
