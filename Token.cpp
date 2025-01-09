
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
#include <iostream>

using namespace std;
Token::Token()	{
	resetToken();
}

/* ****************************************************************************
 *
 * ***************************************************************************/
void Token::resetToken ()	{
  tkn_type = START_UNDEF_TKN;
  _string.clear();
  src.fileName.clear();
  src.lineNumber = 0;
  src.columnPos = 0;
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

	src = srcTkn.src;

	// TODO: I don't understand the comment below
	// return the existing object so we can chain this operator
	return (*this);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
std::wstring Token::get_type_str()  {
  wstring ret_string;

  switch (tkn_type)  {

    case BRKN_TKN                :
      ret_string = L"BRKN_TKN";
      break;
    case JUNK_TKN                :
      ret_string = L"JUNK_TKN";
      break;
    case START_UNDEF_TKN         :
      ret_string = L"START_UNDEF_TKN";
      break;
    case WHITE_SPACE_TKN         :
      ret_string = L"WHITE_SPACE_TKN";
      break;
  	case RESERVED_WORD_TKN				:
			ret_string = L"RESERVED_WORD_TKN";
			break;
		case DATA_TYPE_TKN						:
			ret_string = L"DATA_TYPE_TKN";
			break;
    case USER_WORD_TKN             :
      ret_string = L"USER_WORD_TKN";
      break;
    case STRING_TKN              :
      ret_string = L"STRING_TKN";
      break;
    case DATETIME_TKN            :
      ret_string = L"DATETIME_TKN";
      break;
    case OLD_SCHOOL_CMMNT_TKN    :
      ret_string = L"OLD_SCHOOL_CMMNT_TKN";
      break;
    case TIL_EOL_CMMNT_TKN       :
      ret_string = L"TIL_EOL_CMMNT_TKN";
      break;
		case BOOL_TKN:
		ret_string = L"BOOL_TKN";
			break;
    case UINT8_TKN								:
			ret_string = L"UINT8_TKN";
			break;
    case UINT16_TKN								:
			ret_string = L"UINT16_TKN";
			break;
		case UINT32_TKN								:
			ret_string = L"UINT32_TKN";
			break;
		case UINT64_TKN								:
			ret_string = L"UINT64_TKN";
			break;
		case INT8_TKN								:
			ret_string = L"INT8_TKN";
			break;
		case INT16_TKN								:
			ret_string = L"INT16_TKN";
			break;
		case INT32_TKN								:
			ret_string = L"INT32_TKN";
			break;
		case INT64_TKN								:
			ret_string = L"INT64_TKN";
			break;
    case DOUBLE_TKN              :
        // TODO: Support DOUBLE_TKN
        ret_string = L"DOUBLE_TKN";
        break;
    case SRC_OPR8R_TKN               :
      ret_string = L"SRC_OPR8R_TKN";
      break;
    case EXEC_OPR8R_TKN               :
      ret_string = L"EXEC_OPR8R_TKN";
      break;
    case SPR8R_TKN               :
      ret_string = L"SPR8R_TKN";
      break;
    case END_OF_STREAM_TKN       :
      ret_string = L"END_OF_STREAM_TKN";
      break;
    default:
      ret_string = L"BRKN_TKN";
      break;
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
	
	} else if (tkn_type == USER_WORD_TKN || tkn_type == DATA_TYPE_TKN || tkn_type == SRC_OPR8R_TKN)	{
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
		value.append (get_type_str());

	return (value);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
std::wstring Token::descr_sans_line_num_col ()	{
	std::wstring desc = get_type_str();

	desc.append(L"(");
	desc.append (isInitialized ? L"I)" : L"U)");
	desc.append (L" ");
	desc.append(getValueStr());
	desc.append (L";");

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
	resetToken();
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
	resetToken();
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
	resetToken();
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
	resetToken();
	_double = newValue;
	tkn_type = DOUBLE_TKN;

	// TODO: Any string representation to take care of?
}

/* ****************************************************************************
 *
 * ***************************************************************************/
void Token::resetToString (std::wstring newValue)	{
	resetToken();

	_string.append (newValue);
	tkn_type = STRING_TKN;
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int Token::convertTo (Token newValTkn)	{
	int ret_code = GENERAL_FAILURE;

	if (tkn_type == newValTkn.tkn_type)	{
		*this = newValTkn;
		ret_code = OK;

	} else if (isUnsigned() && newValTkn.tkn_type == BOOL_TKN)	{
		if (_unsigned > 0)
			resetToBool(true);
		else
		 	resetToBool(false);

		ret_code = OK;

	} else if (isSigned() && newValTkn.tkn_type == BOOL_TKN)	{
		if (_signed > 0)
			resetToBool(true);
		else
		 	resetToBool(false);

		ret_code = OK;

	} else if (isUnsigned() && newValTkn.isUnsigned())	{
		// Both are UNSIGNED, but of diffeent sizes
		if (tkn_type >= newValTkn.tkn_type)
			// Keep the user declared larger data_type size
			_unsigned = newValTkn._unsigned;
		else
			resetToUnsigned (newValTkn._unsigned);
		ret_code = OK;

	}	else if (isSigned() && newValTkn.isSigned())	{
		// Both are SIGNED, but of diffeent sizes
		if (tkn_type >= newValTkn.tkn_type)
			// Keep the user declared larger data_type size
			_signed = newValTkn._signed;
		else
			resetToSigned (newValTkn._signed);
		ret_code = OK;

	} else if (tkn_type == DOUBLE_TKN && newValTkn.isSigned())	{
		// DOUBLE SIGNED
		_double = newValTkn._signed;
		ret_code = OK;

	} else if (tkn_type == DOUBLE_TKN && newValTkn.isUnsigned())	{
		// DOUBLE UNSIGNED
		_double = newValTkn._unsigned;
		ret_code = OK;

	} else if (isSigned() && newValTkn.tkn_type == DOUBLE_TKN)	{
		// SIGNED DOUBLE
    double beforeDecPt;
    double afterDecPt = std::modf(newValTkn._double, &beforeDecPt);
    if (afterDecPt == 0.0 && beforeDecPt <= INT64_MAX)	{
			_signed = floor(beforeDecPt);
    	ret_code = OK;
    }

	} else if (isSigned() && newValTkn.isUnsigned())	{
		// SIGNED UNSIGNED
		if (newValTkn.tkn_type <= UINT32_TKN)	{
			_signed = (int64_t)newValTkn._unsigned;
			ret_code = OK;
		}

	} else if (isUnsigned() && newValTkn.tkn_type == DOUBLE_TKN)	{
		// UNSIGNED DOUBLE
		if (newValTkn._double >= 0)	{
			double beforeDecPt;
			double afterDecPt = std::modf(newValTkn._double, &beforeDecPt);
			if (afterDecPt == 0.0 && beforeDecPt <= UINT64_MAX)	{
				_unsigned = floor(beforeDecPt);
				ret_code = OK;
			}
		}

	} else if (isUnsigned() && newValTkn.isSigned())	{
		// UNSIGNED SIGNED
		if (newValTkn._signed >= 0)	{
			_unsigned = newValTkn._signed;
			ret_code = OK;
		}
	}

	if (OK != ret_code)	{
		// TODO:
		std::wcout << L"STOP" << std::endl;
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
