
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
/* ****************************************************************************
 *
 * ***************************************************************************/
Token::Token (tkn_type_enum found_type, std::wstring tokenized_str, int line_num, int col_pos) {

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
  this->is_rvalue = false;
}

Token::~Token()	{

	if (this->_string.length() > 0)	{
		// TODO: std::wcout << "TODO: ~Token " << this->_string << std::endl;
		this->_string.erase(0, this->_string.length());
	}
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
		case UINT16_TKN								:
			ret_string = L"UINT16_TKN";
			break;
		case UINT32_TKN								:
			ret_string = L"UINT32_TKN";
			break;
		case UINT64_TKN								:
			ret_string = L"UINT64_TKN";
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
    case OPR8R_TKN               :
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

	desc.append (this->_string);

	// Give STRINGs, DATETIMEs and SPR8Rs some context clues
	if (this->tkn_type == STRING_TKN || this->tkn_type == DATETIME_TKN)
		desc.append (L"\"");
	else if (this->tkn_type == SPR8R_TKN)
		desc.append (L"'");

	// TODO: Filename?
	desc.append (L" on line ");
	desc.append (std::to_wstring(this->line_number));
	desc.append (L" column ");
	desc.append (std::to_wstring(this->column_pos));

	return (desc);
}

void Token::dumpOut ()	{
	std::wcout << "Token type = " << this->get_type_str() << "; string = " << this->_string << "; _unsigned = " << this->_unsigned << "; _signed = " << this->_signed << "; _double = " << this->_unsigned << "; line_number = " << this->line_number << "; column_pos = " << this->column_pos << std::endl;
}

/* ****************************************************************************
 *
 * ***************************************************************************/
TokenCompareResult Token::compare (Token & otherTkn)	{
	TokenCompareResult compareRez;

	if ((this->tkn_type == UINT16_TKN || this->tkn_type == UINT32_TKN || this->tkn_type == UINT64_TKN
			|| this->tkn_type == INT16_TKN || this->tkn_type == INT32_TKN || this->tkn_type == INT64_TKN
			|| this->tkn_type == STRING_TKN || this->tkn_type == DATETIME_TKN || this->tkn_type == DOUBLE_TKN)
			&& (otherTkn.tkn_type == UINT16_TKN || otherTkn.tkn_type == UINT32_TKN || otherTkn.tkn_type == UINT64_TKN
					|| otherTkn.tkn_type == INT16_TKN || otherTkn.tkn_type == INT32_TKN || otherTkn.tkn_type == INT64_TKN
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
				case UINT16_TKN:
				case UINT32_TKN:
				case UINT64_TKN:
					this->_unsigned > otherTkn._unsigned ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
					this->_unsigned >= otherTkn._unsigned ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
					this->_unsigned < otherTkn._unsigned ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
					this->_unsigned <= otherTkn._unsigned ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
					this->_unsigned == otherTkn._unsigned ? compareRez.equals = isTrue : compareRez.equals = isFalse;
					break;

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

		} else if (this->tkn_type == UINT16_TKN || this->tkn_type == UINT32_TKN || this->tkn_type == UINT64_TKN)	{
			if (otherTkn.tkn_type == INT16_TKN || otherTkn.tkn_type == INT32_TKN || otherTkn.tkn_type == INT64_TKN) {
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

		} else if (this->tkn_type == INT16_TKN || this->tkn_type == INT32_TKN || this->tkn_type == INT64_TKN) {
			if (otherTkn.tkn_type == UINT16_TKN || otherTkn.tkn_type == UINT32_TKN || otherTkn.tkn_type == UINT64_TKN)	{
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
			if (otherTkn.tkn_type == UINT16_TKN || otherTkn.tkn_type == UINT32_TKN || otherTkn.tkn_type == UINT64_TKN)	{
				this->_double > otherTkn._unsigned ? compareRez.gr8rThan = isTrue : compareRez.gr8rThan = isFalse;
				this->_double >= otherTkn._unsigned ? compareRez.gr8rEquals = isTrue : compareRez.gr8rEquals = isFalse;
				this->_double < otherTkn._unsigned ? compareRez.lessThan = isTrue : compareRez.lessThan = isFalse;
				this->_double <= otherTkn._unsigned ? compareRez.lessEquals = isTrue : compareRez.lessEquals = isFalse;
				this->_double == otherTkn._unsigned ? compareRez.equals = isTrue : compareRez.equals = isFalse;

			} else if (otherTkn.tkn_type == INT16_TKN || otherTkn.tkn_type == INT32_TKN || otherTkn.tkn_type == INT64_TKN) {
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
			case UINT16_TKN :
			case UINT32_TKN :
			case UINT64_TKN :
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
