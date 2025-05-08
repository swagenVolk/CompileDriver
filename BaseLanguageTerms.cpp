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
#include "ExprTreeNode.h"
#include "OpCodes.h"
#include "Operator.h"
#include "Token.h"
#include "common.h"
#include <cstdint>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <vector>

BaseLanguageTerms::BaseLanguageTerms() {
  failed_on_src_line = 0;

}

BaseLanguageTerms::~BaseLanguageTerms() {
}

/* ****************************************************************************
 * Function to ensure no overlap between: valid_data_types reserved_words OPR8Rs SPR8Rs
 * ***************************************************************************/
void BaseLanguageTerms::validityCheck()	{
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

  // Reference count system calls
	for (auto itr8r = system_calls.begin(); itr8r != system_calls.end(); itr8r++)	{
		std::wstring nxt_sys_call = itr8r->first;
		assert (!nxt_sys_call.empty());
		auto search = nameReferenceCnt.find(nxt_sys_call);
		if (search == nameReferenceCnt.end())	{
			nameReferenceCnt.insert(std::pair {nxt_sys_call, 1});
		
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

  if (auto search = valid_data_types.find(check_for_datatype); search != valid_data_types.end())  {
    if (search->second.first != INTERNAL_USE_TKN)
		  is_valid = true;
  }

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
		std::wcout << L"INVALID_OPCODE" << std::endl;

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
    if (search->second.first != INTERNAL_USE_TKN) {
      std::pair tknEnum_opCode = search->second;
      ret_info = tknEnum_opCode;
    }
	}

	return (ret_info);
}

/* ****************************************************************************
 * If the passed in op_code represents a valid data type, a valid datatype string
 * will be returned. Otherwise, return an empty string.
 * ***************************************************************************/
TokenTypeEnum BaseLanguageTerms::getTokenTypeForOpCode (uint8_t op_code)	{
	TokenTypeEnum tknType = START_UNDEF_TKN;

  if (op_code != INVALID_OPCODE)  {
    for (auto itr8r = valid_data_types.begin(); itr8r != valid_data_types.end(); itr8r++)	{
      if (itr8r->second.first != INTERNAL_USE_TKN)  {
        std::pair tknEnum_opCode = itr8r->second;
        if (tknEnum_opCode.second == op_code)	{
          tknType = tknEnum_opCode.first;
          break;
        }
      }
    }
  }

	return (tknType);

}

/* ****************************************************************************
 * Determine if the passed in string is a valid data type or not
 * ***************************************************************************/
bool BaseLanguageTerms::is_valid_user_data_type (std::wstring inStr)	{
	bool isValid = false;

	if ( auto search = valid_data_types.find(inStr); search != valid_data_types.end())	{
		if (search->second.first != INTERNAL_USE_TKN)
      isValid = true;
	}

	return isValid;
}

/* ****************************************************************************
 * Determine if the passed in string is a valid reserved word or not
 * ***************************************************************************/
bool BaseLanguageTerms::is_reserved_word (std::wstring inStr)	{
	bool isValid = false;

		for (auto itr8r = reserved_words.begin(); itr8r != reserved_words.end() && !isValid; itr8r++)	{
			if (0 == inStr.compare(*itr8r))
				isValid = true;
		}

	return isValid;
}

/* ****************************************************************************
 * Determine if the passed in string is a valid system call or not
 * ***************************************************************************/
 bool BaseLanguageTerms::is_system_call (std::wstring inStr)	{
  bool is_valid = false;

  if (auto search = system_calls.find(inStr); search != system_calls.end())  {
      is_valid = true;
  }

  return is_valid;
  
}
  
/* ****************************************************************************
 * Get the parameter list and return type details for this system call
 * std::map <std::wstring, std::pair <std::vector<uint8_t>, TokenTypeEnum>> system_calls;
 * ***************************************************************************/
 int BaseLanguageTerms::get_system_call_details (std::wstring sys_call, std::vector<uint8_t> & param_list, TokenTypeEnum & data_type)  {
  int ret_code = GENERAL_FAILURE;

  if (auto search = system_calls.find(sys_call); search != system_calls.end())  {
    param_list = search->second.first;
    data_type = search->second.second;
    ret_code = OK;
  }

  return ret_code;
}

/* ****************************************************************************
 * Get how many parameters this system call needs before it can be resolved
 * TODO: The declarative part is here, but the actual system calls are 
 * currently in RunTimeInterpreter.  
 * ***************************************************************************/
 int BaseLanguageTerms::get_num_sys_call_parameters (std::wstring sys_call, int & num_params) {
  int ret_code = GENERAL_FAILURE;

  if (auto search = system_calls.find(sys_call); search != system_calls.end())  {
    std::vector<uint8_t> param_list = search->second.first;
    num_params = param_list.size();
    ret_code = OK;
  }
  return ret_code;
}

/* ****************************************************************************
 * Determine if the passed in string meets the requirements for a legit variable
 * name
 * ***************************************************************************/
bool BaseLanguageTerms::is_viable_var_name (std::wstring varName)	{
	bool isViable = true;

	if (is_reserved_word(varName))	{
		isViable = false;
	
	} else if (is_valid_user_data_type(varName))	{
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


/* ****************************************************************************
 * Determine if the passed in Token can be converted to a data type that 
 * corresponds with the passed in op_code
 * ***************************************************************************/
 int BaseLanguageTerms::tkn_type_converts_to_opcode (uint8_t planned_op_code, Token & check_token, std::wstring variable_name, std::wstring & error_msg)  {
  int ret_code = GENERAL_FAILURE;

  Token expected_tkn;
  bool is_case_missed = false;

  switch (planned_op_code)  {
    case DATA_TYPE_UINT8_OPCODE:
      expected_tkn.tkn_type = UINT8_TKN;
      break;
    case DATA_TYPE_UINT16_OPCODE:
      expected_tkn.tkn_type = UINT16_TKN;
      break;
    case DATA_TYPE_UINT32_OPCODE:
      expected_tkn.tkn_type = UINT32_TKN;
      break;
    case DATA_TYPE_UINT64_OPCODE:
      expected_tkn.tkn_type = UINT64_TKN;
      break;
    case DATA_TYPE_INT8_OPCODE:
      expected_tkn.tkn_type = INT8_TKN;
      break;
    case DATA_TYPE_INT16_OPCODE:
      expected_tkn.tkn_type = INT16_TKN;
      break;
    case DATA_TYPE_INT32_OPCODE:
      expected_tkn.tkn_type = INT32_TKN;
      break;
    case DATA_TYPE_INT64_OPCODE:
      expected_tkn.tkn_type = INT64_TKN;
      break;
    case DATA_TYPE_STRING_OPCODE:
      expected_tkn.tkn_type = STRING_TKN;
      break;
    case DATA_TYPE_DATETIME_OPCODE:
      expected_tkn.tkn_type = DATETIME_TKN;
      break;
    case DATA_TYPE_DOUBLE_OPCODE:
      expected_tkn.tkn_type = DOUBLE_TKN;
      break;
    case DATA_TYPE_BOOL_OPCODE:
      expected_tkn.tkn_type = BOOL_TKN;
      break;
    default:
      is_case_missed = true;
      break;

  }

  if (!is_case_missed)
    ret_code = expected_tkn.convertTo(check_token, variable_name, error_msg);
  

  return ret_code;
 }

 /* ****************************************************************************
 *
 * ***************************************************************************/
int BaseLanguageTerms::append_to_flat_tkn_list (std::shared_ptr<ExprTreeNode> tree_node, std::vector<Token> & flatExprTknList)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;

  if (tree_node != NULL)  {
    if (tree_node->originalTkn->tkn_type == SRC_OPR8R_TKN && get_statement_ender() == tree_node->originalTkn->_string)  {
		  ret_code = OK;

    } else if (tree_node->originalTkn->tkn_type == SYSTEM_CALL_TKN) {
      ret_code = append_flattened_system_call (tree_node, flatExprTknList);

    } else {
      switch(tree_node->originalTkn->tkn_type)	{
      case USER_WORD_TKN :
      case STRING_TKN :
      case DATETIME_TKN :
      case BOOL_TKN :
      case UINT8_TKN :
      case UINT16_TKN :
      case UINT32_TKN :
      case UINT64_TKN :
      case INT8_TKN :
      case INT16_TKN :
      case INT32_TKN :
      case INT64_TKN :
      case DOUBLE_TKN :
      case SRC_OPR8R_TKN :
        break;
      default:
        isFailed = true;
        break;
      }

      if (!isFailed)	{
        if (tree_node->originalTkn->tkn_type == SRC_OPR8R_TKN || tree_node->originalTkn->tkn_type == EXEC_OPR8R_TKN)	{
          // Prior to putting OPR8Rs in flattened list, make them EXEC_OPR8R_TKNs
          // since the caller is expected to use the RunTimeInterpreter to resolve
          // the expression
          uint8_t op_code = getOpCodeFor (tree_node->originalTkn->_string);
          tree_node->originalTkn->resetTokenExceptSrc();
          tree_node->originalTkn->tkn_type = EXEC_OPR8R_TKN;
          tree_node->originalTkn->_unsigned = op_code;
          Operator opr8r;
          if (OK == getExecOpr8rDetails (op_code, opr8r))	{
            tree_node->originalTkn->_string = opr8r.symbol;
          }
          if (op_code == INVALID_OPCODE)
            isFailed = true;

        }

        if (!(tree_node->originalTkn->tkn_type == EXEC_OPR8R_TKN && tree_node->originalTkn->_unsigned == TERNARY_2ND_OPR8R_OPCODE))
          // No need to put the [:] OPR8R in the stream
          flatExprTknList.push_back (*tree_node->originalTkn);
        if (!isFailed)
          ret_code = OK;
      }

    }
	}

	return (ret_code);
}

/* ****************************************************************************
 * A system call ExprTreeNode *may* have additional parameter expressions that need to be
 * pushed to the flat list, which cannot be handled by the more simplistic
 * append_to_flat_tkn_list
 * ***************************************************************************/
 int BaseLanguageTerms::append_flattened_system_call (std::shared_ptr<ExprTreeNode> tree_node, std::vector<Token> & flatExprTknList)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;

  flatExprTknList.push_back(*(tree_node->originalTkn));

  int idx = 0;
  for (; idx < tree_node->parameter_list.size() && !isFailed; idx++) {
    if (OK != append_to_flat_tkn_list(tree_node->parameter_list[idx], flatExprTknList))
      isFailed = true;
  }

  if (idx == tree_node->parameter_list.size() && !isFailed)
    ret_code = OK;

  return ret_code;
}

/* ****************************************************************************
 * Take the parse tree supplied by the compiler and recursively turn it into a 
 * flattened expression with a sequence of 
 * [operator][left expression][right expression]
 * NOTE that [left expression] and/or right expression could consist of a single
 * operand, or could contain other nested expressions.
 * TODO: Does this proc belong in this class or in InterpretedFileWriter?
 * ***************************************************************************/
 int BaseLanguageTerms::makeFlatExpr_OLR (std::shared_ptr<ExprTreeNode> currBranch, std::vector<Token> & flatExprTknList)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;

	if (currBranch != NULL)	{

		if (OK != append_to_flat_tkn_list(currBranch, flatExprTknList))
			isFailed = true;

		if (!isFailed && currBranch->_1stChild != NULL)	{
			if (OK != makeFlatExpr_OLR (currBranch->_1stChild, flatExprTknList))
				isFailed = true;

			if (!isFailed && currBranch->_2ndChild != NULL)	{
				if (OK != makeFlatExpr_OLR (currBranch->_2ndChild, flatExprTknList))
					isFailed = true;
			}
		}

		if (!isFailed)	{
			ret_code = OK;
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * Tree that represents an expression will be written out recursively into a
 * flat list for storing in a file stream.  Expression is written out in 
 * [OPR8R][LEFT][RIGHT] order.  The OPR8R goes 1st to enable short-circuiting of
 * [&&], [||] and [?] OPR8Rs.
 * Some example source expressions and their corresponding Token lists that get
 * written out to the interpreted file are shown below.
 * 
 * seven = three + four;
 * [=][seven][B+][three][four]
 *
 * one = 1;
 * [=][one][1]
 *
 * seven * seven + init1++; 
 * [B+][*][seven][seven][1+][init1]
 * 
 * seven * seven + ++init2;
 * [B+][*][seven][seven][+1][init2]
 * 
 * one >= two ? 1 : three <= two ? 2 : three == four ? 3 : six > seven ? 4 : six > (two << two) ? 5 : 12345;
 * [?][>=][one][two][1][?][<=][three][two][2][?][==][three][four][3][?][>][six][seven][4][?][>][six][<<][two][two][5][12345]
 * 
 * count == 1 ? "one" : count == 2 ? "two" : "MANY";
 * [?][==][count][1]["one"][?][==][count][2]["two"]["MANY"]
 * 
 * 3 * 4 / 3 + 4 << 4;
 * [<<][B+][/][*][3][4][3][4][4]
 * 
 * 3 * 4 / 3 + 4 << 4 + 1;
 * [<<][B+][/][*][3][4][3][4][B+][4][1]
 * 
 * (one * two >= three || two * three > six || three * four < seven || four / two < one) && (three % two > 1 || (shortCircuitAnd987 = 654));
 * [&&][||][||][||][>=][*][one][two][three][>][*][two][three][six][<][*][three][four][seven][<][/][four][two][one][||][>][%][three][two][1][=][shortCircuitAnd987][654]
 * TODO: Does this proc belong in this class or in InterpretedFileWriter?
 * ***************************************************************************/
 int BaseLanguageTerms::flattenExprTree (std::shared_ptr<ExprTreeNode> rootOfExpr, std::vector<Token> & flatExprTknList)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;
	int usrSrcLineNum;
	int usrSrcColPos;

	if (rootOfExpr == NULL)
  	SET_FAILED_ON_SRC_LINE;
	else	
		ret_code = makeFlatExpr_OLR (rootOfExpr, flatExprTknList);

	return (ret_code);
}

/* ****************************************************************************
 * System call and associated parameters are bundled up in an ExprTreeNode
 * This proc walks through the structure and generates a flattened sequence
 * that can be consumed & operated on by RunTimeInterpreter.
 * TODO: Does this proc belong in this class or in InterpretedFileWriter?
 * ***************************************************************************/
 int BaseLanguageTerms::flatten_system_call (std::shared_ptr<ExprTreeNode> sys_call_node, std::vector<Token> & flat_tkn_list) {
  int ret_code = GENERAL_FAILURE;

  if (sys_call_node == NULL)  {
  	SET_FAILED_ON_SRC_LINE;
    
  } else {
    flat_tkn_list.push_back(Token (SYSTEM_CALL_TKN, sys_call_node->originalTkn->_string));
    // TODO: Would it be useful to include a parameter count token here?
    int idx = 0;
    for (; idx < sys_call_node->parameter_list.size() && !failed_on_src_line; idx++) {
      if (OK != makeFlatExpr_OLR (sys_call_node->parameter_list[idx], flat_tkn_list))
			  SET_FAILED_ON_SRC_LINE;
    }

    if (idx == sys_call_node->parameter_list.size() && !failed_on_src_line)
      ret_code = OK;

  }

  return ret_code;
}