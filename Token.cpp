
/* ****************************************************************************
 * The Token class will be used to store information on a sequence of charactors
 * parsed out by the file_parser class.  
 * Info can include:
 * Token type
 * Line number the Token started on
 * Column position in the line that the Token started on
 * ***************************************************************************/
#include "Token.h"
#include "TokenCompareResult.h"
#include "locale_strings.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <regex>
#include <variant>

using namespace std;
Token::Token()	{
	resetToken();
}

/* ****************************************************************************
 * Common fxn for a "factory reset" of a Token
 * ***************************************************************************/
void Token::resetToken ()	{
  resetTokenExceptSrc();
  
  src.fileName.clear();
  src.lineNumber = 0;
  src.columnPos = 0;

}

/* ****************************************************************************
 *
 * ***************************************************************************/
void Token::resetTokenExceptSrc ()	{
  tkn_type = START_UNDEF_TKN;
  _string.clear();
  _unsigned = 0;
  _signed = 0;
  _double = 0.0;
  is_Rvalue = false;
  isInitialized = false;

}

/* ****************************************************************************
 *
 * ***************************************************************************/
Token::Token(tkn_type_enum found_type, std::wstring tokenized_str)
: src (L"", 0, 0)	{
	resetToken();
  tkn_type = found_type;
  _string = tokenized_str;

}

/* ****************************************************************************
 *
 * ***************************************************************************/
Token::Token (tkn_type_enum found_type, std::wstring tokenized_str, std::wstring srcFileName, int line_num, int col_pos)
	: src (srcFileName, line_num, col_pos){

  // TODO: Token value not automatically filled in, even though tokenized_str should contain enough info

  tkn_type = found_type;
  _string = tokenized_str;
  _unsigned = 0;
  _signed = 0;
  _double = 0.0;
  // A post-fix operator will turn an object into an rvalue, and it is considered to be a "temporary" object
  // during the evaluation of the current expression, and that temporary object can no longer be
  // modified.
  // The source code
  // while (someVal++ < 37 && otherVal++ < 43) {...}
  // will generate underlying code like so:
  // while (someVal < 37 && otherVal < 43) { someVal += 1; otherVal +=1; <user source code>}
  is_Rvalue = false;


}

Token::~Token()	{

	if (_string.length() > 0)	{
		_string.erase(0, _string.length());
	}
}

Token& Token::operator= (const Token & srcTkn)
{
	// self-assignment check
	if (this == &srcTkn)
		return (*this);

	// if data exists in the current string, delete it
	if (!_string.empty())
		_string.clear();
	_string = srcTkn._string;
	tkn_type = srcTkn.tkn_type;
	_unsigned = srcTkn._unsigned;
	_signed = srcTkn._signed;
	_double = srcTkn._double;
	is_Rvalue = srcTkn.is_Rvalue;
	isInitialized = srcTkn.isInitialized;

	if (srcTkn.src.lineNumber > 0)	{
		// Only overwrite existing src info if new info is good
		src = srcTkn.src;
	}

	src.insertPos = srcTkn.src.insertPos;

	// TODO: I don't understand the comment below
	// return the existing object so we can chain this operator
	return (*this);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
std::wstring Token::get_type_str(bool is_ret_friendly_name)  {
  wstring internal_type_str, friendly_type_str;
  
  switch (tkn_type)  {
    case BRKN_TKN                :
      internal_type_str = L"BRKN_TKN";
      friendly_type_str = L"UNDEFINED";
      break;
    case JUNK_TKN                :
      internal_type_str = L"JUNK_TKN";
      friendly_type_str = L"UNDEFINED";
      break;
    case START_UNDEF_TKN         :
      internal_type_str = L"START_UNDEF_TKN";
      friendly_type_str = L"UNDEFINED";
      break;
    case WHITE_SPACE_TKN         :
      internal_type_str = L"WHITE_SPACE_TKN";
      break;
		case INTERNAL_USE_TKN:
			internal_type_str = L"INTERNAL_USE_TKN";
      friendly_type_str = L"INTERNAL USE";
			break;
  	case RESERVED_WORD_TKN				:
			internal_type_str = L"RESERVED_WORD_TKN";
			break;
		case DATA_TYPE_TKN						:
			internal_type_str = L"DATA_TYPE_TKN";
			break;
    case USER_WORD_TKN             :
      internal_type_str = L"USER_WORD_TKN";
      break;
    case STRING_TKN              :
      internal_type_str = L"STRING_TKN";
      break;
    case DATETIME_TKN            :
      internal_type_str = L"DATETIME_TKN";
      break;
    case OLD_SCHOOL_CMMNT_TKN    :
      internal_type_str = L"OLD_SCHOOL_CMMNT_TKN";
      break;
    case TIL_EOL_CMMNT_TKN       :
      internal_type_str = L"TIL_EOL_CMMNT_TKN";
      break;
		case BOOL_TKN:
		  internal_type_str = L"BOOL_TKN";
      friendly_type_str = L"boolean";
			break;
    case UINT8_TKN								:
			internal_type_str = L"UINT8_TKN";
			break;
    case UINT16_TKN								:
			internal_type_str = L"UINT16_TKN";
			break;
		case UINT32_TKN								:
			internal_type_str = L"UINT32_TKN";
			break;
		case UINT64_TKN								:
			internal_type_str = L"UINT64_TKN";
			break;
		case INT8_TKN								:
			internal_type_str = L"INT8_TKN";
			break;
		case INT16_TKN								:
			internal_type_str = L"INT16_TKN";
			break;
		case INT32_TKN								:
			internal_type_str = L"INT32_TKN";
			break;
		case INT64_TKN								:
			internal_type_str = L"INT64_TKN";
			break;
    case DOUBLE_TKN              :
      // TODO: Support DOUBLE_TKN
      internal_type_str = L"DOUBLE_TKN";
      break;
    case SRC_OPR8R_TKN               :
      internal_type_str = L"SRC_OPR8R_TKN";
      friendly_type_str = L"compile time operator";
      break;
    case EXEC_OPR8R_TKN               :
      internal_type_str = L"EXEC_OPR8R_TKN";
      friendly_type_str = L"interpreted time operator";
      break;
    case SPR8R_TKN               :
      internal_type_str = L"SPR8R_TKN";
      friendly_type_str = L"separator";
      break;
    case END_OF_STREAM_TKN       :
      internal_type_str = L"END_OF_STREAM_TKN";
      break;
    case VOID_TKN:
      internal_type_str = L"VOID_TKN";     
      break; 
    default:
      internal_type_str = L"BRKN_TKN";
      friendly_type_str = L"UNDEFINED";
      break;
  }

  std::wstring ret_string = internal_type_str;
  if (is_ret_friendly_name) {
    if (friendly_type_str.empty())  {
      friendly_type_str = internal_type_str;
      std::wstring _tkn_str = L"_TKN";
      auto pos = friendly_type_str.find_last_of(_tkn_str);
      if (pos != internal_type_str.npos) {
        friendly_type_str.replace(pos - _tkn_str.size() + 1, _tkn_str.size(), L"");
        std::transform(friendly_type_str.begin(), friendly_type_str.end(), friendly_type_str.begin(), ::towlower);

        while (1) {
          pos = friendly_type_str.find (L"_");
          if (pos != friendly_type_str.npos)  {
            friendly_type_str.replace(pos, 1, L" ");
          } else {
            break;
          }
        }
      }
    }
    ret_string = friendly_type_str;

  }

  return (ret_string);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
std::wstring Token::getValueStr ()	{
	std::wstring value;
	std::wstringstream hexStream;

	hexStream.str (L"");
	hexStream << L"0x" << std::hex << _unsigned;
	std::wstring hexStr = hexStream.str();

	// Give STRINGs, DATETIMEs and SPR8Rs some context clues
	if (tkn_type == STRING_TKN || tkn_type == DATETIME_TKN)	{
		value.append (L"\"");
		value.append (_string);
		value.append (L"\"");
	
	} else if (tkn_type == SPR8R_TKN) {
		value.append (L"'");
		value.append (_string);
		value.append (L"'");
	
	} else if (tkn_type == USER_WORD_TKN || tkn_type == DATA_TYPE_TKN || tkn_type == SRC_OPR8R_TKN || tkn_type == RESERVED_WORD_TKN
		|| tkn_type == INTERNAL_USE_TKN)	{
		value.append (_string);

  } else if (tkn_type == SYSTEM_CALL_TKN) {
    value.append (L"sys_call::");
    value.append (_string);

	} else if (tkn_type == BOOL_TKN)	{
		if (_unsigned > 0)
			value.append (TRUE_RESERVED_WORD);
		else
			value.append (FALSE_RESERVED_WORD);
	
	} else if (tkn_type == EXEC_OPR8R_TKN)	{
		if (!_string.empty())	{
			value.append (_string);
		} else {
			value.append (L"EXEC_OPR8R_TKN->");
			value.append (hexStr);
		}
	}

	if (isUnsigned())	{
		value.append (hexStr);

	} else if (isSigned())	{
		value.append (std::to_wstring (_signed));

	} else if (tkn_type == DOUBLE_TKN)	{
		value.append (std::to_wstring (_double));
	}

	if (value.empty())
		// Nothing normal fits, but we need to display SOMETHING
		value.append (get_type_str(false));

	return (value);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
 std::wstring Token::getBracketedValueStr ()	{
	return (L"[" + getValueStr() + L"]");
 }

/* ****************************************************************************
 *
 * ***************************************************************************/
std::wstring Token::descr_sans_line_num_col ()	{
	std::wstring desc = get_type_str(true);

	desc.append(L"(");
	desc.append (isInitialized ? L"I)" : L"U)");
	desc.append (L"->[");
	desc.append(getValueStr());
	desc.append (L"]");

	return (desc);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
std::wstring Token::descr_line_num_col ()	{
	std::wstring desc = descr_sans_line_num_col();

	// TODO: Filename?
	desc.append (L" on line ");
	desc.append (std::to_wstring(src.lineNumber));
	desc.append (L" column ");
	desc.append (std::to_wstring(src.columnPos));

	return (desc);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
TokenCompareResult Token::compare (Token & otherTkn)	{
	TokenCompareResult compareRez;

	if (tkn_type == BOOL_TKN && otherTkn.tkn_type == BOOL_TKN)	{
		if (_unsigned == otherTkn._unsigned)
			compareRez.equals = isTrue;
		else
		 	compareRez.equals = isFalse;

	} else if (tkn_type != BOOL_TKN && isDirectOperand() && otherTkn.tkn_type != BOOL_TKN && otherTkn.isDirectOperand())	{
		// Both types are valid for comparison. Will check later if these 2 types can be compared against one another

		if (tkn_type == otherTkn.tkn_type)	{
			switch (tkn_type)	{
				case STRING_TKN:
					_string > otherTkn._string ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
					_string >= otherTkn._string ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
					_string < otherTkn._string ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
					_string <= otherTkn._string ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
					_string == otherTkn._string ? compareRez.equals = isTrue : compareRez.equals = isFalse;
					break;

				case DATETIME_TKN:
				case UINT8_TKN:
				case UINT16_TKN:
				case UINT32_TKN:
				case UINT64_TKN:
					_unsigned > otherTkn._unsigned ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
					_unsigned >= otherTkn._unsigned ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
					_unsigned < otherTkn._unsigned ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
					_unsigned <= otherTkn._unsigned ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
					_unsigned == otherTkn._unsigned ? compareRez.equals = isTrue : compareRez.equals = isFalse;
					break;

				case INT8_TKN:
				case INT16_TKN:
				case INT32_TKN:
				case INT64_TKN:
					_signed > otherTkn._signed ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
					_signed >= otherTkn._signed ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
					_signed < otherTkn._signed ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
					_signed <= otherTkn._signed ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
					_signed == otherTkn._signed ? compareRez.equals = isTrue : compareRez.equals = isFalse;
					break;

				case DOUBLE_TKN:
					_unsigned > otherTkn._double ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
					_unsigned >= otherTkn._double ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
					_unsigned < otherTkn._double ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
					_unsigned <= otherTkn._double ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
					_unsigned == otherTkn._double ? compareRez.equals = isTrue : compareRez.equals = isFalse;
					break;
				default:
					break;

			}

		} else if (isUnsigned() && otherTkn.isUnsigned()) {
				_unsigned > otherTkn._unsigned ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
				_unsigned >= otherTkn._unsigned ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
				_unsigned < otherTkn._unsigned ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
				_unsigned <= otherTkn._unsigned ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
				_unsigned == otherTkn._unsigned ? compareRez.equals = isTrue : compareRez.equals = isFalse;

		} else if (isSigned() && otherTkn.isSigned()) {
				_signed > otherTkn._signed ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
				_signed >= otherTkn._signed ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
				_signed < otherTkn._signed ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
				_signed <= otherTkn._signed ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
				_signed == otherTkn._signed ? compareRez.equals = isTrue : compareRez.equals = isFalse;

		} else if (isUnsigned() && otherTkn.isSigned()) {
				_unsigned > otherTkn._signed ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
				_unsigned >= otherTkn._signed ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
				_unsigned < otherTkn._signed ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
				_unsigned <= otherTkn._signed ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
				_unsigned == otherTkn._signed ? compareRez.equals = isTrue : compareRez.equals = isFalse;

		} else if (isSigned() && otherTkn.isUnsigned())	{
				_signed > otherTkn._unsigned ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
				_signed >= otherTkn._unsigned ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
				_signed < otherTkn._unsigned ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
				_signed <= otherTkn._unsigned ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
				_signed == otherTkn._unsigned ? compareRez.equals = isTrue : compareRez.equals = isFalse;

		} else if (isUnsigned() && otherTkn.tkn_type == DOUBLE_TKN)	{
			_unsigned > otherTkn._double ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
			_unsigned >= otherTkn._double ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
			_unsigned < otherTkn._double ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
			_unsigned <= otherTkn._double ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
			_unsigned == otherTkn._double ? compareRez.equals = isTrue : compareRez.equals = isFalse;

		} else if (isSigned() && otherTkn.tkn_type == DOUBLE_TKN)	{
			_signed > otherTkn._double ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
			_signed >= otherTkn._double ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
			_signed < otherTkn._double ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
			_signed <= otherTkn._double ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
			_signed == otherTkn._double ? compareRez.equals = isTrue : compareRez.equals = isFalse;
		
		} else if (tkn_type == DOUBLE_TKN && otherTkn.isUnsigned())	{
				_double > otherTkn._unsigned ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
				_double >= otherTkn._unsigned ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
				_double < otherTkn._unsigned ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
				_double <= otherTkn._unsigned ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
				_double == otherTkn._unsigned ? compareRez.equals = isTrue : compareRez.equals = isFalse;

		} else if (tkn_type == DOUBLE_TKN && otherTkn.isSigned()) {
			_double > otherTkn._signed ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
			_double >= otherTkn._signed ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
			_double < otherTkn._signed ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
			_double <= otherTkn._signed ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
			_double == otherTkn._signed ? compareRez.equals = isTrue : compareRez.equals = isFalse;
		}
	}

	return (compareRez);

}

/* ****************************************************************************
 *
 * ***************************************************************************/
bool Token::isDirectOperand ()	{
	bool isRand = isDirectOperand(tkn_type);
	return (isRand);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
bool Token::isDirectOperand (TokenTypeEnum tokenType)	{
	bool isRand = false;

		switch (tokenType)	{
			case BOOL_TKN :
			case STRING_TKN :
			case DATETIME_TKN :
			case UINT8_TKN :
			case UINT16_TKN :
			case UINT32_TKN :
			case UINT64_TKN :
			case INT8_TKN :
			case INT16_TKN :
			case INT32_TKN :
			case INT64_TKN :
			case DOUBLE_TKN :
				isRand = true;
				break;
			default:
				break;
		}


	return isRand;
}

/* ****************************************************************************
 *
 * ***************************************************************************/
 bool Token::is_valid_call_ret_type (TokenTypeEnum token_type) {
  bool is_valid = false;

  if (token_type == VOID_TKN || isDirectOperand(token_type))
    is_valid = true;

  return is_valid;

}

/* ****************************************************************************
 *
 * ***************************************************************************/
bool Token::evalResolvedTokenAsIf ()	{
	bool isTrue = false;

	// USER_WORD_TKN probably means a variable name.  If so, then Token hasn't been resolved yet
	// TODO: Not sure how to handle DATETIME_TKN yet
	assert (tkn_type != USER_WORD_TKN && tkn_type != DATETIME_TKN);

	switch (tkn_type)	{
		case STRING_TKN :
			if (_string.size() > 0)
				isTrue = true;
			break;
		case BOOL_TKN :
		case UINT8_TKN :
		case UINT16_TKN :
		case UINT32_TKN :
		case UINT64_TKN :
			if (_unsigned > 0)
				isTrue = true;
			break;
		case INT8_TKN :
		case INT16_TKN :
		case INT32_TKN :
		case INT64_TKN :
			if (_signed > 0)
				isTrue = true;
			break;
		case DOUBLE_TKN :
			if (_double)
				isTrue = true;
			break;
		default:
			break;
	}
	return (isTrue);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
bool Token::isUnsigned ()	{
	bool isUnsigned = false;

	switch (tkn_type)	{
		case UINT8_TKN:
		case UINT16_TKN:
		case UINT32_TKN:
		case UINT64_TKN:
			isUnsigned = true;
			break;
		default:
			break;
	}

	return (isUnsigned);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
bool Token::isSigned ()	{
	bool isSigned = false;

	switch (tkn_type)	{
		case INT8_TKN:
		case INT16_TKN:
		case INT32_TKN:
		case INT64_TKN:
			isSigned = true;
			break;
		default:
			break;
	}
	return (isSigned);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
void Token::resetToBool (bool isTrue)	{
	resetTokenExceptSrc();
	tkn_type = BOOL_TKN;

	if (isTrue)
		_unsigned = 1;
	else
	 	_unsigned = 0;
}


/* ****************************************************************************
 *
 * ***************************************************************************/
void Token::resetToUnsigned (uint64_t newValue)	{
	resetTokenExceptSrc();
	_unsigned = newValue;

	if (newValue < (0x1 << NUM_BITS_IN_BYTE))
		tkn_type = UINT8_TKN;
	else if (newValue < (0x1 << NUM_BITS_IN_WORD))
		tkn_type = UINT16_TKN;
	else if (newValue < (0x1 << NUM_BITS_IN_DWORD))
		// TODO: Warning!
		tkn_type = UINT32_TKN;
	else
		tkn_type = UINT64_TKN;

}

/* ****************************************************************************
 *
 * ***************************************************************************/
void Token::resetToSigned (int64_t newValue)	{
	resetTokenExceptSrc();
	_signed = newValue;

	int64_t absolute = abs(newValue);

	if (absolute < (0x1 << (NUM_BITS_IN_BYTE - 1)))
		tkn_type = INT8_TKN;
	else if (absolute < (0x1 << (NUM_BITS_IN_WORD - 1)))
		tkn_type = INT16_TKN;
	else if (absolute < (0x1 << (NUM_BITS_IN_DWORD - 1)))
		tkn_type = INT32_TKN;
	else
		tkn_type = INT64_TKN;
}

/* ****************************************************************************
 *
 * ***************************************************************************/
void Token::resetToDouble (double newValue)	{
	resetTokenExceptSrc();
	_double = newValue;
	tkn_type = DOUBLE_TKN;

	// TODO: Any string representation to take care of?
}

/* ****************************************************************************
 *
 * ***************************************************************************/
void Token::resetToString (std::wstring newValue)	{
	resetTokenExceptSrc();

	_string.append (newValue);
	tkn_type = STRING_TKN;
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int Token::convertTo (Token newValTkn, std::wstring variableName, std::wstring & errorMsg)	{
	int ret_code = GENERAL_FAILURE;

	if (tkn_type == newValTkn.tkn_type)	{
		*this = newValTkn;
		ret_code = OK;

	} else if (isUnsigned() && newValTkn.isUnsigned())	{
			// Both are UNSIGNED, but of different sizes
			if (tkn_type >= newValTkn.tkn_type)
				// Keep the user declared larger data_type size
				_unsigned = newValTkn._unsigned;
			else
				resetToUnsigned (newValTkn._unsigned);
			ret_code = OK;

	} else if (isSigned() && newValTkn.isSigned())	{
		// Both are SIGNED, but of diffeent sizes
		if (tkn_type >= newValTkn.tkn_type)
			// Keep the user declared larger data_type size
			_signed = newValTkn._signed;
		else
			resetToSigned (newValTkn._signed);
		ret_code = OK;

	} else if (tkn_type == BOOL_TKN)	{
		// PREV: [BOOL_TKN] 
		// OKGO: [isUnsigned] [isSigned] [DOUBLE_TKN]
		// NOGO: [STRING_TKN] [DATETIME_TKN] 
		if (newValTkn.isUnsigned())	{
			newValTkn._unsigned > 0 ? resetToBool(true) : resetToBool(false);
			ret_code = OK;

		} else if (isSigned())	{
			newValTkn._signed > 0 ? resetToBool(true) : resetToBool(false);
			ret_code = OK;

		} else if (tkn_type == DOUBLE_TKN)	{
			newValTkn._double > 0.0 ? resetToBool(true) : resetToBool(false);
			ret_code = OK;
		}
		
	} else if (isUnsigned())	{
		// PREV: [isUnsigned] 
		// OKGO: [BOOL_TKN] [isSigned]
		// NOGO: [DOUBLE_TKN] [STRING_TKN] [DATETIME_TKN] 
		if (newValTkn.tkn_type == BOOL_TKN)	{
			resetToUnsigned (newValTkn._unsigned);
			ret_code = OK;

		} else if (newValTkn.isSigned())	{
			// TODO: What about negative signed #s?
			_unsigned = (uint64_t)newValTkn._signed;
			ret_code = OK;
		}

	} else if (isSigned())	{
		// PREV: [isSigned]
		// OKGO: [BOOL_TKN] [isUnsigned]  
		// NOGO: [DOUBLE_TKN] [STRING_TKN] [DATETIME_TKN] 
		if (newValTkn.tkn_type == BOOL_TKN)	{
			resetToSigned (newValTkn._unsigned);
			ret_code = OK;

		} else if (newValTkn.isUnsigned())	{
			if (newValTkn.tkn_type <= UINT32_TKN)	{
				_signed = abs((int64_t)newValTkn._unsigned);
				ret_code = OK;
			}
		}


	} else if (tkn_type == DOUBLE_TKN)	{
		// PREV: [DOUBLE_TKN]
		// OKGO: [BOOL_TKN] [isUnsigned] [isSigned]  
		// NOGO: [STRING_TKN] [DATETIME_TKN] 
		if (newValTkn.tkn_type == BOOL_TKN || newValTkn.isUnsigned())	{
			_double = (double)newValTkn._unsigned;
			ret_code = OK;
		
		} else if (newValTkn.isUnsigned())	{
			_double = (double)newValTkn._signed;
			ret_code = OK;
		}
	} 

	if (OK != ret_code)	{
		// TODO:
		errorMsg = L"Failed to convert variable [" + variableName + L"] of type " + get_type_str(true) + L" to " + newValTkn.descr_sans_line_num_col();
	}

	return (ret_code);

}

/* ****************************************************************************
 *
 * ***************************************************************************/
int Token::get_line_number ()	{
	return (src.lineNumber);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int Token::get_column_pos ()	{
	return (src.columnPos);
}
