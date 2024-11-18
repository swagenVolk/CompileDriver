
/* ****************************************************************************
 * The Token class will be used to store information on a sequence of charactors
 * parsed out by the file_parser class.  
 * Info can include:
 * Token type
 * Line number the Token started on
 * Column position in the line that the Token started on
 * ***************************************************************************/
#include "Token.h"
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
  line_number = 0;
  column_pos = 0;
  _unsigned = 0;
  _signed = 0;
  _double = 0.0;
  is_Rvalue = false;
  isInitialized = false;

}

/* ****************************************************************************
 *
 * ***************************************************************************/
Token::Token (tkn_type_enum found_type, std::wstring tokenized_str, int line_num, int col_pos) {

  // TODO: Token value not automatically filled in, even though tokenized_str should contain enough info

  this->tkn_type = found_type;
  this->_string = tokenized_str;
  this->line_number = line_num;
  this->column_pos = col_pos;
  this->_unsigned = 0;
  this->_signed = 0;
  this->_double = 0.0;
  // A post-fix operator will turn an object into an rvalue, and it is considered to be a "temporary" object
  // during the evaluation of the current expression, and that temporary object can no longer be
  // modified.
  // The source code
  // while (someVal++ < 37 && otherVal++ < 43) {...}
  // will generated underlying code like so:
  // while (someVal < 37 && otherVal < 43) {someVal += 1; otherVal +=1; ...}
  this->is_Rvalue = false;


}

Token::~Token()	{

	if (this->_string.length() > 0)	{
		this->_string.erase(0, this->_string.length());
	}
}

Token& Token::operator= (const Token & srcTkn)
{
	// self-assignment check
	if (this == &srcTkn)
		return (*this);

	// if data exists in the current string, delete it
	_string.clear();
	_string = srcTkn._string;
	tkn_type = srcTkn.tkn_type;
	_unsigned = srcTkn._unsigned;
	_signed = srcTkn._signed;
	_double = srcTkn._double;
	line_number = srcTkn.line_number;
	column_pos = srcTkn.column_pos;
	is_Rvalue = srcTkn.is_Rvalue;


	// TODO: I don't understand the comment below
	// return the existing object so we can chain this operator
	return (*this);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
std::wstring Token::get_type_str()  {
  wstring ret_string;

  switch (this->tkn_type)  {

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
    case KEYWORD_TKN             :
      ret_string = L"KEYWORD_TKN";
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
      ret_string = L"OPR8R_TKN";
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
std::wstring Token::description ()	{
	std::wstring desc = get_type_str();
	desc.append (L" ");

	// Give STRINGs, DATETIMEs and SPR8Rs some context clues
	if (this->tkn_type == STRING_TKN || this->tkn_type == DATETIME_TKN)
		desc.append (L"\"");
	else if (this->tkn_type == SPR8R_TKN)
		desc.append (L"'");

	if (!_string.empty())	{
		desc.append (this->_string);
	}

	// Give STRINGs, DATETIMEs and SPR8Rs some context clues
	if (this->tkn_type == STRING_TKN || this->tkn_type == DATETIME_TKN)
		desc.append (L"\"");
	else if (this->tkn_type == SPR8R_TKN)
		desc.append (L"'");

	if (_unsigned != 0)	{
		std::wstringstream wstream;
		wstream << L"0x" << std::hex << _unsigned;
		desc.append (wstream.str());
		desc.append (L";");
	}

	if (_signed != 0)	{
		desc.append (L" _signed = ");
		desc.append (std::to_wstring (_signed));
		desc.append (L";");

	}

	if (_double != 0.0)	{
		desc.append (L" _double = ");
		desc.append (std::to_wstring (_double));
		desc.append (L";");
	}

	// TODO: Filename?
	desc.append (L" on line ");
	desc.append (std::to_wstring(this->line_number));
	desc.append (L" column ");
	desc.append (std::to_wstring(this->column_pos));

	return (desc);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
TokenCompareResult Token::compare (Token & otherTkn)	{
	TokenCompareResult compareRez;

	if ((this->tkn_type == UINT8_TKN || this->tkn_type == UINT16_TKN || this->tkn_type == UINT32_TKN || this->tkn_type == UINT64_TKN
			|| this->tkn_type == INT8_TKN || this->tkn_type == INT16_TKN || this->tkn_type == INT32_TKN || this->tkn_type == INT64_TKN
			|| this->tkn_type == STRING_TKN || this->tkn_type == DATETIME_TKN || this->tkn_type == DOUBLE_TKN)
			&& (otherTkn.tkn_type == UINT8_TKN || otherTkn.tkn_type == UINT16_TKN || otherTkn.tkn_type == UINT32_TKN || otherTkn.tkn_type == UINT64_TKN
					|| otherTkn.tkn_type == INT8_TKN || otherTkn.tkn_type == INT16_TKN || otherTkn.tkn_type == INT32_TKN || otherTkn.tkn_type == INT64_TKN
					|| otherTkn.tkn_type == STRING_TKN || otherTkn.tkn_type == DATETIME_TKN || otherTkn.tkn_type == DOUBLE_TKN))	{
		// Both types are valid for comparison. Will check later if these 2 types can be compared against one another

		if (this->tkn_type == otherTkn.tkn_type)	{
			switch (this->tkn_type)	{
				case STRING_TKN:
					this->_string > otherTkn._string ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
					this->_string >= otherTkn._string ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
					this->_string < otherTkn._string ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
					this->_string <= otherTkn._string ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
					this->_string == otherTkn._string ? compareRez.equals = isTrue : compareRez.equals = isFalse;
					break;

				case DATETIME_TKN:
				case UINT8_TKN:
				case UINT16_TKN:
				case UINT32_TKN:
				case UINT64_TKN:
					this->_unsigned > otherTkn._unsigned ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
					this->_unsigned >= otherTkn._unsigned ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
					this->_unsigned < otherTkn._unsigned ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
					this->_unsigned <= otherTkn._unsigned ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
					this->_unsigned == otherTkn._unsigned ? compareRez.equals = isTrue : compareRez.equals = isFalse;
					break;

				case INT8_TKN:
				case INT16_TKN:
				case INT32_TKN:
				case INT64_TKN:
					this->_signed > otherTkn._signed ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
					this->_signed >= otherTkn._signed ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
					this->_signed < otherTkn._signed ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
					this->_signed <= otherTkn._signed ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
					this->_signed == otherTkn._signed ? compareRez.equals = isTrue : compareRez.equals = isFalse;
					break;

				case DOUBLE_TKN:
					this->_unsigned > otherTkn._double ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
					this->_unsigned >= otherTkn._double ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
					this->_unsigned < otherTkn._double ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
					this->_unsigned <= otherTkn._double ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
					this->_unsigned == otherTkn._double ? compareRez.equals = isTrue : compareRez.equals = isFalse;
					break;
				default:
					break;

			}

		} else if (this->tkn_type == UINT8_TKN || this->tkn_type == UINT16_TKN || this->tkn_type == UINT32_TKN || this->tkn_type == UINT64_TKN)	{
			if (otherTkn.tkn_type == INT8_TKN || otherTkn.tkn_type == INT16_TKN || otherTkn.tkn_type == INT32_TKN || otherTkn.tkn_type == INT64_TKN) {
				this->_unsigned > otherTkn._signed ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
				this->_unsigned >= otherTkn._signed ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
				this->_unsigned < otherTkn._signed ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
				this->_unsigned <= otherTkn._signed ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
				this->_unsigned == otherTkn._signed ? compareRez.equals = isTrue : compareRez.equals = isFalse;

			} else if (otherTkn.tkn_type == DOUBLE_TKN)	{
				this->_unsigned > otherTkn._double ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
				this->_unsigned >= otherTkn._double ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
				this->_unsigned < otherTkn._double ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
				this->_unsigned <= otherTkn._double ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
				this->_unsigned == otherTkn._double ? compareRez.equals = isTrue : compareRez.equals = isFalse;
			}

		} else if (this->tkn_type == INT8_TKN || this->tkn_type == INT16_TKN || this->tkn_type == INT32_TKN || this->tkn_type == INT64_TKN) {
			if (otherTkn.tkn_type == UINT8_TKN || otherTkn.tkn_type == UINT16_TKN || otherTkn.tkn_type == UINT32_TKN || otherTkn.tkn_type == UINT64_TKN)	{
				this->_signed > otherTkn._unsigned ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
				this->_signed >= otherTkn._unsigned ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
				this->_signed < otherTkn._unsigned ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
				this->_signed <= otherTkn._unsigned ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
				this->_signed == otherTkn._unsigned ? compareRez.equals = isTrue : compareRez.equals = isFalse;

			} else if (otherTkn.tkn_type == DOUBLE_TKN)	{
				this->_unsigned > otherTkn._double ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
				this->_unsigned >= otherTkn._double ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
				this->_unsigned < otherTkn._double ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
				this->_unsigned <= otherTkn._double ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
				this->_unsigned == otherTkn._double ? compareRez.equals = isTrue : compareRez.equals = isFalse;
			}

		} else if (this->tkn_type == DOUBLE_TKN)	{
			if (otherTkn.tkn_type == UINT8_TKN || otherTkn.tkn_type == UINT16_TKN || otherTkn.tkn_type == UINT16_TKN || otherTkn.tkn_type == UINT32_TKN || otherTkn.tkn_type == UINT64_TKN)	{
				this->_double > otherTkn._unsigned ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
				this->_double >= otherTkn._unsigned ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
				this->_double < otherTkn._unsigned ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
				this->_double <= otherTkn._unsigned ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
				this->_double == otherTkn._unsigned ? compareRez.equals = isTrue : compareRez.equals = isFalse;

			} else if (otherTkn.tkn_type == INT8_TKN || otherTkn.tkn_type == INT16_TKN || otherTkn.tkn_type == INT32_TKN || otherTkn.tkn_type == INT64_TKN) {
				this->_double > otherTkn._signed ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
				this->_double >= otherTkn._signed ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
				this->_double < otherTkn._signed ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
				this->_double <= otherTkn._signed ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
				this->_double == otherTkn._signed ? compareRez.equals = isTrue : compareRez.equals = isFalse;
			}
		}
	}

	return (compareRez);

}

/* ****************************************************************************
 *
 * ***************************************************************************/
bool Token::isOperand ()	{
	bool isRand = false;

		switch (this->tkn_type)	{
			case KEYWORD_TKN :
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

	// KEYWORD_TKN probably means a variable name.  If so, then Token hasn't been resolved yet
	// TODO: Not sure how to handle DATETIME_TKN yet
	assert (tkn_type != KEYWORD_TKN && tkn_type != DATETIME_TKN);

	switch (tkn_type)	{
		case STRING_TKN :
			if (_string.size() > 0)
				isTrue = true;
			break;
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

	_string = newValue;
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

	} else if (isUnsigned() && newValTkn.isUnsigned())	{
		// Both are UNSIGNED
		if (tkn_type >= newValTkn.tkn_type)
			// Keep the user declared larger data_type size
			_unsigned = newValTkn._unsigned;
		else
			this->resetToUnsigned (newValTkn._unsigned);
		ret_code = OK;

	}	else if (isSigned() && newValTkn.isSigned())	{
		// Both are SIGNED
		if (tkn_type >= newValTkn.tkn_type)
			// Keep the user declared larger data_type size
			_signed = newValTkn._signed;
		else
			this->resetToSigned (newValTkn._signed);
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



	return (ret_code);

}
