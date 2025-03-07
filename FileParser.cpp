/* ****************************************************************************
 * The file_parser class will read through an input file and generate a stream
 * of corresponding tokens.
 * NOTE: The stream of tokens will be stored BACKWARDS in a vector to make
 * it easier to remove/manipulate using pop_back since I don't see anything like
 * pop_front.
 *
 * TODO:
 * Col pos was off. Test using # characters since previous EOL
 * Unicode!
 * Find hex #'s without leading 0x? - Problematic; 1234 decimal != 1234 hex
 * Do I need to worry about goofy dates, like 10/17/2022?  10:32:46 8/13/2023?
 * Test with tons of different files
 * Problem immediately after Ye Olde Skule comment block that started on line 37 and was 10 lines long
 *   SPR8R_TKN '(' on line 37 column 1 <-- *should* be line 47!
 *   USER_WORD_TKN count on line 47 column 2
 *
 * DONE:
 * ***************************************************************************/
#include "Token.h"
using namespace std;
#include "FileParser.h"

/* ****************************************************************************
 * file_parser constructor
 * ***************************************************************************/
FileParser::FileParser (BaseLanguageTerms & inCompilerTerms, std::wstring fileName) {
  curr_file_pos = 0;
	line_num = 1;
  curr_line_start_pos = 0;
  curr_tkn_starts_on_line_num = 1;
  curr_tkn_starts_on_col_pos = 1;
  num_chars_chomped_this_line = 0;
  num_lines_parsed = 0;
  compilerTerms = inCompilerTerms;
  this->fileName = fileName;
}

/* ****************************************************************************
 * Get the next character from the input file to possibly start a new Token or
 * append to a Token that is already under construction.
 * ***************************************************************************/
int FileParser::get_next_char(std::fstream & input_stream, wchar_t & next_char, bool is_peek)  {
  int ret_code = CATASTROPHIC_FAILURE;

  uint8_t byte_0, byte_1, byte_2, byte_3, remaining2[2], remaining3[3];
  uint16_t  scratch_pad;

  if (input_stream.tellg() < 0)
    ret_code = END_OF_FILE;
  else  {
    input_stream.read(reinterpret_cast<char*>(&byte_0), sizeof byte_0); // binary input

    if ((byte_0 & UTF8_SNGL_BYTE_CLR_BITS) == 0) {
      // single byte UTF-8 char 0x0xxxxxxx
      next_char = byte_0;
      ret_code = OK;

    } else if ((byte_0 & UTF8_DBL_BYTE_SET_BITS) == UTF8_DBL_BYTE_SET_BITS && (byte_0 & UTF8_DBL_BYTE_CLR_BITS) == 0) {
      // double byte UTF-8 char 0x110xxxxx 0x10xxxxxx
      input_stream.read(reinterpret_cast<char*>(&byte_1), sizeof byte_1); // binary input
      if ((byte_1 & BIT_7) == BIT_7 && (byte_1 & BIT_6) == 0) {
        // Twiddle some bits -> https://en.wikipedia.org/wiki/UTF-8
        next_char = (byte_0 & 0x1F);
        next_char <<= 6;
        scratch_pad = byte_1 & ~(BIT_7);
        next_char |= scratch_pad;
        ret_code = OK;
      }

    } else if ((byte_0 & UTF8_TRPL_BYTE_SET_BITS) == UTF8_TRPL_BYTE_SET_BITS && (byte_0 & UTF8_TRPL_BYTE_CLR_BITS) == 0) {
      // triple byte UTF-8 char 0x1110xxxx 0x10xxxxxx 0x10xxxxxx
      input_stream.read(reinterpret_cast<char*>(&remaining2), sizeof remaining2); // binary input
      byte_1 = remaining2[0];
      byte_2 = remaining2[1];

      if ((byte_1 & BIT_7) == BIT_7 && (byte_1 & BIT_6) ==  0 && (byte_2 & BIT_7) == BIT_7 && (byte_2 & BIT_6) == 0) {
        // Twiddle some bits -> https://en.wikipedia.org/wiki/UTF-8
        next_char = (byte_0 & 0xF);
        next_char <<= 12;
        scratch_pad = (byte_1 & 0x1F);
        scratch_pad <<= 6;
        next_char |= scratch_pad;
        scratch_pad = (byte_2 & 0x1F);
        next_char |= scratch_pad;
        ret_code = OK;
      }

    } else if ((byte_0 & UTF8_QUAD_BYTE_SET_BITS) == UTF8_QUAD_BYTE_SET_BITS && (byte_0 & UTF8_QUAD_BYTE_CLR_BITS) == 0) {
      // triple byte UTF-8 char 0x11110xxx 0x10xxxxxx 0x10xxxxxx 0x10xxxxxx
      input_stream.read(reinterpret_cast<char*>(&remaining3), sizeof remaining3); // binary input
      byte_1 = remaining3[0];
      byte_2 = remaining3[1];
      byte_2 = remaining3[1];
      if ((byte_1 & BIT_7) == BIT_7 && (byte_1 & BIT_6) == 0 && (byte_2 & BIT_7) == BIT_7 && (byte_2 & BIT_6) == 0) {
        // Twiddle some bits -> https://en.wikipedia.org/wiki/UTF-8
        next_char = (byte_0 & 0x3);
        next_char <<= 18;
        scratch_pad = (byte_1 & 0x1F);
        scratch_pad <<= 12;
        next_char |= scratch_pad;
        scratch_pad = (byte_2 & 0x1F);
        scratch_pad <<= 6;
        next_char |= scratch_pad;
        scratch_pad = (byte_3 & 0x1F);
        next_char |= scratch_pad;
        ret_code = OK;
      }
    }
  }

  if (ret_code == OK && !is_peek) {
    num_chars_chomped_this_line++;
  }

  return (ret_code);

}

/* ****************************************************************************
 * Override of previous get_next_char that allowed for "peeking" without consuming
 * ***************************************************************************/
int FileParser::get_next_char(std::fstream & input_stream, wchar_t & next_char)  {
  return (get_next_char (input_stream, next_char, false));
}

/* ****************************************************************************
 * Peek at the next wchar_t in the file stream without consuming it
 * ***************************************************************************/
int FileParser::peek_next_char (std::fstream & input_stream, wchar_t & peeked_char) {
  int ret_code = GENERAL_FAILURE;

  long restore_pos = input_stream.tellg();
  // Need to get and then reset file pos at end
  int wrkng_ret_code = get_next_char (input_stream, peeked_char, true);

  if (wrkng_ret_code == OK) {
    input_stream.seekg(restore_pos);
    // TODO: Failure check
    ret_code = OK;
  }

  return (ret_code);
}

/* ****************************************************************************
 * Convert this string Token into a datetime, if possible.
 * ***************************************************************************/
void FileParser::cnvrt_tkn_if_datetime (std::shared_ptr<Token> pssbl_datetime_tkn) {

  if (pssbl_datetime_tkn != NULL) {
    std::wstring date_time_str = pssbl_datetime_tkn->_string;
    std::wstring curr_str;
    std::wstring next_seg;
    std::deque<wstring> date_time_parts;
    int space_pos, spr8r_pos, failing_line_num = 0, idx, curr_val, multiplier;
    wchar_t spr8r;
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0, millisecond = 0;
    std::wstring::size_type sz;   // alias of size_t

    // Break the wstring up into components under the assumption that spaces separate the components
    while (!date_time_str.empty())  {
      space_pos = date_time_str.find(' ');

      if (space_pos == std::wstring::npos) {
        date_time_parts.push_back (date_time_str);
        date_time_str.clear();

      } else if (space_pos > 0)  {
        date_time_parts.push_back (date_time_str.substr(0, space_pos));
        date_time_str.erase (0, space_pos);

      } else {
        // Beginning of wstring has an unexpected space wchar_t
        date_time_str.erase (0, 1);
      }
    }
    /*
     *  2022-10-14 11:19:56.987
     *  2022-10-14 11:19:56.98
     *  2022-10-14 11:19:56.9
     *  2022-10-14 11:19:56
     *  2022-10-14 11:19
     *  2022-10-14
    */

    if (date_time_parts.size() > 2)
      failing_line_num = __LINE__;

    if (failing_line_num == 0 && date_time_parts.size() >= 1)  {
      //  We might have at least the DATE part
      std::wstring YYYY_MM_DD_str = date_time_parts[0];
      std::deque<wstring> YYYY_MM_DD_parts;

      if (YYYY_MM_DD_str.length() > 4 /* year 2022 */)  {
        spr8r = YYYY_MM_DD_str[4];
        if (!iswpunct(spr8r) && spr8r != '-' && spr8r != '/' && spr8r != '\\' && spr8r != '.')
          failing_line_num = __LINE__;

      } else  {
        failing_line_num = __LINE__;
      }

      // Break the wstring up into components under the assumption that spaces separate the components
      while (!YYYY_MM_DD_str.empty() && failing_line_num == 0)  {
        // TODO: 2017/12/23 could also be valid
        spr8r_pos = YYYY_MM_DD_str.find(spr8r);

        if (spr8r_pos == std::wstring::npos) {
          YYYY_MM_DD_parts.push_back (YYYY_MM_DD_str);
          YYYY_MM_DD_str.clear();

        } else if (spr8r_pos > 0)  {
          next_seg = YYYY_MM_DD_str.substr(0, spr8r_pos);
          YYYY_MM_DD_parts.push_back (next_seg);
          YYYY_MM_DD_str.erase (0, spr8r_pos + 1);

        } else {
          // Beginning of wstring has an unexpected dash wchar_t
          failing_line_num = __LINE__;
        }
      }

      if (failing_line_num == 0 && YYYY_MM_DD_parts.size() != 3) {
        failing_line_num = __LINE__;

      } else if (failing_line_num == 0)  {
        // Right # of components for date YYYY-MM-DD
        bool is_leap_year = false;
        for (idx = 0; idx < 3 && failing_line_num == 0; idx++) {
          curr_str = YYYY_MM_DD_parts[idx];
          try {
            curr_val = std::stoi(curr_str, &sz);
            switch (idx)  {
              case 0:
                //  Year
                if (curr_val >= 1970 && curr_val <= 2100) {
                  year = curr_val;
                  if (year % 4 == 0 && year % 100 == 0 && year % 400 == 0)
                    is_leap_year = true;
                  else if (year % 4 && year % 100 > 0)
                    is_leap_year = true;

                } else  {
                  failing_line_num = __LINE__;
                }
                break;
              case 1:
                // Month
                if (curr_val >= 1 && curr_val <= 12)
                  month = curr_val;
                else
                  failing_line_num = __LINE__;
                break;
              case 2:
                // Day
                if (curr_val < 1 || curr_val > 31)  {
                  failing_line_num = __LINE__;

                } else  {
                  // Months with 31 days: January, March, May, July, August, October, and December.
                  day = curr_val;
                  if (month == 2) {
                    if ((is_leap_year && day > 29) || (!is_leap_year && day > 28)) {
                      failing_line_num = __LINE__;
                    } else if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30) {
                      //  April June September November
                      failing_line_num = __LINE__;
                    }
                  }
                }
                break;
              default:
                failing_line_num = __LINE__;
                break;
            }

          }
          catch(std::invalid_argument& e){
            // if no conversion could be performed
            failing_line_num = __LINE__;
          }
          catch(std::out_of_range& e){
            // if the converted value would fall out of the range of the result type
            // or if the underlying function (std::strtol or std::strtoull) sets errno
            // to ERANGE.
            failing_line_num = __LINE__;
          }
          catch(...) {
            // everything else
            failing_line_num = __LINE__;
          }
        }
      }
    }

    if (failing_line_num == 0 && date_time_parts.size() >= 2)  {
      //  We might have the HH:MM:SS part
      std::list<wstring> time_parts;
      std::wstring HHMMSS_str = date_time_parts[1];
      std::deque<wstring> HHMMSS_parts;
      // Break the wstring up into components under the assumption that spaces separate the components
      idx = 0;
      /*
      *  11:19:56.987
      *  11:19:56.98
      *  11:19:56.9
      *  11:19:56
      *  11:19
      */

      while (!HHMMSS_str.empty() && failing_line_num == 0)  {
        //  11:19:56.987
        //    ^ end of idx 0
        //       ^ idx 1
        //          ^ idx 2
        //                ^ idx 3
        if (idx < 2)
          spr8r = ':';
        else if (idx >= 2)
          spr8r = '.';
        else  {
          failing_line_num = __LINE__;
        }

        spr8r_pos = HHMMSS_str.find(spr8r);

        if (spr8r_pos == std::wstring::npos) {
          HHMMSS_parts.push_back (HHMMSS_str);
          HHMMSS_str.clear();

        } else if (spr8r_pos > 0)  {
          HHMMSS_parts.push_back (HHMMSS_str.substr(0, spr8r_pos));
          HHMMSS_str.erase (0, spr8r_pos + 1);

        } else {
          // Beginning of wstring has an unexpected dash wchar_t
          failing_line_num = __LINE__;
        }
        idx++;
      }

      if (HHMMSS_parts.size() >= 1 && HHMMSS_parts.size() <= 4) {
        // Right # of components for date HHMMSS.uuu
        bool is_leap_year = false;
        for (idx = 0; idx < HHMMSS_parts.size() && failing_line_num == 0; idx++) {
          curr_str = HHMMSS_parts[idx];
          try {
            curr_val = std::stoi(curr_str, &sz);
            switch (idx)  {
              case 0:
                //  HOUR
                if (curr_val >= 0 && curr_val <= 23) {
                  hour = curr_val;
                } else  {
                  failing_line_num = __LINE__;
                }
                break;
              case 1:
              case 2:
                // MINUTE
                if (curr_val >= 0 && curr_val <= 59)  {
                  if (idx == 1)
                    minute = curr_val;
                  else if (idx == 2)
                    second = curr_val;

                } else  {
                  failing_line_num = __LINE__;
                }
                break;
              case 3:
                // MILLISECOND
                if (curr_val >= 0 && curr_val <= 999)  {
                  if (curr_str.length() == 1)
                    // 11:19:56.9
                    multiplier = 100;
                  else if (curr_str.length() == 2)
                    // 11:19:56.98
                    multiplier = 10;
                  else
                    // 11:19:56.987
                    multiplier = 1;
                  millisecond = curr_val * multiplier;

                } else  {
                  failing_line_num = __LINE__;
                }
                break;
              default:
                failing_line_num = __LINE__;
                break;
            }
          }
          catch(std::invalid_argument& e){
            // if no conversion could be performed
            failing_line_num = __LINE__;
          }
          catch(std::out_of_range& e){
            // if the converted value would fall out of the range of the result type
            // or if the underlying function (std::strtol or std::strtoull) sets errno
            // to ERANGE.
            failing_line_num = __LINE__;
          }
          catch(...) {
            // everything else
            failing_line_num = __LINE__;
          }
        }
      } else  {
        failing_line_num = __LINE__;
      }
    }

    if (failing_line_num == 0 && year > 0 && month > 0 && day > 0)  {
      pssbl_datetime_tkn->tkn_type = DATETIME_TKN;
      // Put milliseconds since epoch -> _unsigned for later comparisons
      std::tm tm{};  // zero initialise
      tm.tm_year = year-1900;
      tm.tm_mon = month-1;
      tm.tm_mday = day;
      tm.tm_hour = hour;
      tm.tm_min = minute;
      tm.tm_sec = second;
      // TODO: Not daylight saving
      tm.tm_isdst = 0;
      std::time_t seconds_since_epoch = std::mktime(&tm);
      pssbl_datetime_tkn->_unsigned =  (seconds_since_epoch * 1000) + millisecond;
    }
  }
}

/* ****************************************************************************
 * Now that the Token wstring has ended, ensure that it's of the right type
 * ***************************************************************************/
void FileParser::resolve_final_tkn_type (std::shared_ptr<Token>  tkn_of_ambiguity)  {

  std::wstring::iterator str_r8r;
  int idx;
  int64_t cnvrtd_signed;
  uint64_t cnvrtd_unsigned;
  std::wstring::size_type sz;   // alias of size_t

  if (tkn_of_ambiguity != NULL) {
    switch (tkn_of_ambiguity->tkn_type)  {
      case STRING_TKN:
        cnvrt_tkn_if_datetime (tkn_of_ambiguity);
        tkn_of_ambiguity->isInitialized = true;
        break;
      case UINT8_TKN:
      case UINT16_TKN:
      case UINT32_TKN:
      case UINT64_TKN:
        // Check to make sure it's not junk
        if (tkn_of_ambiguity->_string.length() < 3 || tkn_of_ambiguity->_string.length() > 18)
          // 0x won't cut it & anything longer than -> 0x0123456789ABCDEF is too big
          tkn_of_ambiguity->tkn_type = JUNK_TKN;
        else  {
          idx = 0;
          for(str_r8r = tkn_of_ambiguity->_string.begin(); str_r8r != tkn_of_ambiguity->_string.cend() && tkn_of_ambiguity->tkn_type != JUNK_TKN; ++str_r8r) {
            if (idx >= 2) {
              if (!((*str_r8r >= int('0') && *str_r8r <= int('9')) || (*str_r8r >= int('a') && *str_r8r <= int('f')) || (*str_r8r >= int('A') && *str_r8r <= int('F'))))
                tkn_of_ambiguity->tkn_type = JUNK_TKN;
            }
            idx++;
          }
        }

        if (tkn_of_ambiguity->tkn_type != JUNK_TKN) {
          try {
          	cnvrtd_unsigned = std::stoul(tkn_of_ambiguity->_string, &sz, 16);
            tkn_of_ambiguity->_unsigned = cnvrtd_unsigned;

            // Store this unsigned number (literal?) in the smallest possible size
            if (cnvrtd_unsigned < ceil(exp2(8)))  {
            	tkn_of_ambiguity->tkn_type = UINT8_TKN;
              tkn_of_ambiguity->isInitialized = true;
            } else if (cnvrtd_unsigned < ceil(exp2(16)))  {
            	tkn_of_ambiguity->tkn_type = UINT16_TKN;
              tkn_of_ambiguity->isInitialized = true;
            } else if (cnvrtd_unsigned < ceil(exp2(32)))  {
            	tkn_of_ambiguity->tkn_type = UINT32_TKN;
              tkn_of_ambiguity->isInitialized = true;
            } else if (cnvrtd_unsigned < ceil(exp2(64)))  {
            	tkn_of_ambiguity->tkn_type = UINT64_TKN;
            } else if (cnvrtd_unsigned < ceil(exp2(64)))  {
            } else  {
              tkn_of_ambiguity->tkn_type = JUNK_TKN;
            }
          }
          catch(std::invalid_argument& e){
            // if no conversion could be performed
            tkn_of_ambiguity->tkn_type = JUNK_TKN;
          }
          catch(std::out_of_range& e){
            // if the converted value would fall out of the range of the result type
            // or if the underlying function (std::strtol or std::strtoull) sets errno
            // to ERANGE.
            tkn_of_ambiguity->tkn_type = JUNK_TKN;
          }
          catch(...) {
            // everything else
            tkn_of_ambiguity->tkn_type = JUNK_TKN;
          }

          if (tkn_of_ambiguity->tkn_type != JUNK_TKN)
            tkn_of_ambiguity->isInitialized = true;
        }
        break;
      case INT8_TKN:
      case INT16_TKN:
      case INT32_TKN:
      case INT64_TKN:
        // Check to make sure it's not junk
        idx = 0;
        for(str_r8r = tkn_of_ambiguity->_string.begin(); str_r8r != tkn_of_ambiguity->_string.cend() && tkn_of_ambiguity->tkn_type != JUNK_TKN; ++str_r8r) {
          if (!(*str_r8r >= int('0') && *str_r8r <= int('9')))
            tkn_of_ambiguity->tkn_type = JUNK_TKN;
          idx++;
        }

        if (tkn_of_ambiguity->tkn_type != JUNK_TKN) {
          try {
            cnvrtd_signed = std::stol(tkn_of_ambiguity->_string, &sz);
            tkn_of_ambiguity->_signed = cnvrtd_signed;

            // Store this signed number (literal?) in the smallest possible size
            // Use 2's complement to determine range for each signed data type
            if (cnvrtd_signed >= (0 - ceil(exp2(7))) && cnvrtd_signed < ceil(exp2(7)))  
            	tkn_of_ambiguity->tkn_type = INT8_TKN;
            else if (cnvrtd_signed >= (0 - ceil(exp2(15))) && cnvrtd_signed < ceil(exp2(15))) 
            	tkn_of_ambiguity->tkn_type = INT16_TKN;
            else if (cnvrtd_signed >= (0 - ceil(exp2(31))) && cnvrtd_signed < ceil(exp2(31))) 
            	tkn_of_ambiguity->tkn_type = INT32_TKN;
            else if (cnvrtd_signed >= (0 - ceil(exp2(63))) && cnvrtd_signed < ceil(exp2(63))) 
            	tkn_of_ambiguity->tkn_type = INT64_TKN;
            else  
              tkn_of_ambiguity->tkn_type = JUNK_TKN;

          } catch(std::invalid_argument& e){
            // if no conversion could be performed
            tkn_of_ambiguity->tkn_type = JUNK_TKN;
          
          } catch(std::out_of_range& e){
            // if the converted value would fall out of the range of the result type
            // or if the underlying function (std::strtol or std::strtoull) sets errno
            // to ERANGE.
            tkn_of_ambiguity->tkn_type = JUNK_TKN;
          
          } catch(...) {
            // everything else
            tkn_of_ambiguity->tkn_type = JUNK_TKN;
          }
          
          if (tkn_of_ambiguity->tkn_type != JUNK_TKN)
            tkn_of_ambiguity->isInitialized = true;
        }
        break;
      case SRC_OPR8R_TKN:
        // Check to make sure it's not junk
      	if (!compilerTerms.is_valid_opr8r(tkn_of_ambiguity->_string, USR_SRC))
          // Not a valid OPR8R
          tkn_of_ambiguity->tkn_type = JUNK_TKN;
        break;

      // Nothing to do for these types
      case END_OF_STREAM_TKN:
      case SPR8R_TKN:
      case JUNK_TKN:                 // e.g. 200KbarKnives
        break;
      case USER_WORD_TKN:
        if (compilerTerms.isDataType(tkn_of_ambiguity->_string))
          tkn_of_ambiguity->tkn_type = DATA_TYPE_TKN;
        else if (compilerTerms.isReservedWord (tkn_of_ambiguity->_string))  {
          if (tkn_of_ambiguity->_string == FALSE_RESERVED_WORD) {
            tkn_of_ambiguity->tkn_type = BOOL_TKN;
            tkn_of_ambiguity->_unsigned = 0;
            tkn_of_ambiguity->isInitialized = true;

          } else if (tkn_of_ambiguity->_string == TRUE_RESERVED_WORD) {
            tkn_of_ambiguity->tkn_type = BOOL_TKN;
            tkn_of_ambiguity->_unsigned = 1;
            tkn_of_ambiguity->isInitialized = true;

          } else  {
            tkn_of_ambiguity->tkn_type = RESERVED_WORD_TKN;
          }
        }
        break;

      // Illegal for a *committed* Token(s) or syntactic sugar
      case BRKN_TKN:
      case START_UNDEF_TKN:
      case WHITE_SPACE_TKN:
      case OLD_SCHOOL_CMMNT_TKN:
      case TIL_EOL_CMMNT_TKN:
        // Will need a Token to indicate EOS since the compiler
        // could empty the list while the file_parser is still producing,
        // but this kind of Token should never need
      case DATETIME_TKN:
        // TODO:
      default:
        tkn_of_ambiguity->tkn_type = BRKN_TKN;
        break;
    }
  }
}

/* ****************************************************************************
* Based on the 1st character of this next Token, make a guess as to what type it
* is.  Note that the final Token type will be determined when the Token is
* "committed" to the list.
* ***************************************************************************/
tkn_type_enum FileParser::start_new_tkn_get_type (std::fstream & input_stream, TokenPtrVector & token_stream, wchar_t curr_char, std::wstring & tkn_str)  {
  tkn_type_enum tkn_type = BRKN_TKN;

  // TODO: Handle negative #'s
  wchar_t peeked;
  tkn_str.clear();

  if (iswspace(curr_char)) {
    tkn_type = WHITE_SPACE_TKN;

  } else if (iswalpha (curr_char) || curr_char == '_')	{
    tkn_type = USER_WORD_TKN;
  }

  else if (iswpunct(curr_char))  {
    std::wstring sngl_char_symbol;
    sngl_char_symbol += curr_char;
    if (compilerTerms.is_sngl_char_spr8r(curr_char))  {
      // Consume this Token right away by adding it to the Token stream
      curr_file_pos = input_stream.tellg();
      std::shared_ptr<Token> tkn = std::make_shared<Token> (SPR8R_TKN, sngl_char_symbol, fileName, line_num, num_chars_chomped_this_line);
      token_stream.push_back(tkn);
      tkn_type = START_UNDEF_TKN;

    } else if (compilerTerms.is_atomic_opr8r(curr_char)) {
      // Consume this Token right away by adding it to the Token stream
      curr_file_pos = input_stream.tellg();
      std::shared_ptr<Token> tkn = std::make_shared<Token> (SRC_OPR8R_TKN, sngl_char_symbol, fileName, line_num, num_chars_chomped_this_line);
      token_stream.push_back(tkn);
      tkn_type = START_UNDEF_TKN;

    } else if (curr_char == '"') {
      tkn_type = STRING_TKN;

    } else if (curr_char == '/')  {
      if (OK == peek_next_char(input_stream, peeked)) {
        if (peeked == '*')
          tkn_type = OLD_SCHOOL_CMMNT_TKN;
        else if (peeked == '/')
          tkn_type = TIL_EOL_CMMNT_TKN;
        else {
          tkn_type = SRC_OPR8R_TKN;
        }
      }
    } else  {
      tkn_type = SRC_OPR8R_TKN;
    }
  } else if (iswdigit(curr_char))  {
  	// These 64-bit data types will be used as place holders. When the Token is finalized, we can
  	// drop the data type size of this literal down to the minimum size that can accommodate the data.
  	if (curr_char == '0' && OK == peek_next_char(input_stream, peeked) && (peeked == 'X' || peeked == 'x'))  {
      tkn_type = UINT64_TKN;
    
    } else  {
      tkn_type = INT64_TKN;
    }
  }
  
  if (tkn_type != BRKN_TKN && tkn_type != WHITE_SPACE_TKN && !compilerTerms.is_atomic_opr8r(curr_char) && !compilerTerms.is_sngl_char_spr8r(curr_char))  {
    // Start accumulating the characters for this Token IFF it hasn't already been committed to the Token stream
    if (tkn_type != STRING_TKN && curr_char != '"')
      // No need to add the opening " for a STRING_TKN
      tkn_str = curr_char;
  }

  curr_tkn_starts_on_line_num = line_num;
  curr_tkn_starts_on_col_pos = num_chars_chomped_this_line;

  return (tkn_type);
}

/* ****************************************************************************
 * Detect the end of an Olde schule comment (like this one)
 * ***************************************************************************/
int FileParser::is_end_olde_skul_cmmnt (std::fstream & input_stream, wchar_t curr_char, std::wstring & tkn_str, int & is_end)  {
  int ret_code = OK;
  is_end = 0;
  
  if (curr_char == '*') {
    wchar_t peeked;
    if (OK == peek_next_char (input_stream, peeked) && peeked == '/')  {
      tkn_str += curr_char;
      tkn_str += peeked;
      is_end = 1;

      // Need to consume this character since we only peeked @ it above
      prev_char = curr_char;
      if (OK != get_next_char (input_stream, peeked)) {
        // TODO: Check for valid END_OF_FILE
        ret_code = CATASTROPHIC_FAILURE;
      }
    }
  }

  return (ret_code);
}

/* ****************************************************************************
 * Have we hit EOL?  Not that Unix and Windows differ, and Mac might also
 * "Think Different"
 * ***************************************************************************/
int FileParser::is_end_of_line (std::fstream & input_stream, wchar_t curr_char, int & is_EOL)  {
  int ret_code = OK;
  is_EOL = 0;


  int ascii_val = int(curr_char);

  if (ascii_val == CARRIAGE_RETURN)  {
    is_EOL = 1;
    wchar_t peeked;
    ret_code = peek_next_char (input_stream, peeked);
    if (OK == ret_code && int(peeked) == LINE_FEED)  {
      // EOL in Windows files is CR/LF
      // Need to consume this character since we only peeked @ it above
      if (OK != get_next_char (input_stream, peeked)) {
        // TODO: Check for valid END_OF_FILE
        ret_code = CATASTROPHIC_FAILURE;
      }
    }
  } else if (ascii_val == LINE_FEED) {
    is_EOL = 1;
  }

  if (is_EOL) {
    line_num++;
    curr_line_start_pos = input_stream.tellg();
    num_chars_chomped_this_line = 0;
  }

  return (ret_code);
}

/* ****************************************************************************
 * Given a file name, parse through it and generate a stream of tokens from the
 * file.
 * ***************************************************************************/
int FileParser::gnr8_token_stream(std::string file_name, TokenPtrVector & token_stream) {
  int ret_code = GENERAL_FAILURE, nxt_ret;
  int num_chars_read = 0;
  line_num = 1;
  int failed_on_src_line_num = 0;
  token_stream.clear();

  // TODO: Right place?
  std::setlocale(LC_ALL, "en_US.utf8");

  std::fstream input_stream(file_name, input_stream.binary | input_stream.in);
  if (!input_stream.is_open()) {
    std::cout << "ERROR: Failed to open " << file_name << std::endl;
    failed_on_src_line_num = __LINE__;

  } else {
    // TODO: This is hacky, but #include <codecvt> (for std::wstring_convert) can't be found
    // TODO: Might not be necessary until we decide to support Unicode file names in the future
    std::string::iterator str_ir8r;
    std::wstring wide_file_name;
    wchar_t next_wide;
    char next_char;
    for (str_ir8r = file_name.begin(); str_ir8r != file_name.end(); str_ir8r++) {
      next_char = *str_ir8r;
      next_wide =  next_char;
      wide_file_name += next_wide;
    }

    int is_EOL, is_end;
    bool is_EOF = false;

    bool is_done = false;
    wchar_t peeked, curr_char;
    wstring curr_str;
    curr_str.clear();

    curr_file_pos = input_stream.tellg();
    // Prime the pump with 1st character from file and make no assumptions about what it is
    nxt_ret = get_next_char (input_stream, curr_char);
    if (nxt_ret != OK)  {
      ret_code = nxt_ret;
      failed_on_src_line_num = __LINE__;
    }
    tkn_type_enum curr_tkn_type = START_UNDEF_TKN;
    
    while (failed_on_src_line_num == 0 && !is_EOF) {
      num_chars_read++;

      // TODO: Do I check for EOL here? And if it is EOL, commit in-flight Token
      if (CATASTROPHIC_FAILURE == is_end_of_line(input_stream, curr_char, is_EOL)) {
        ret_code = CATASTROPHIC_FAILURE;
        failed_on_src_line_num = __LINE__;

      } else if (is_EOL && curr_tkn_type != OLD_SCHOOL_CMMNT_TKN)  {
        std::shared_ptr<Token> tkn = std::make_shared <Token> (curr_tkn_type, curr_str, fileName, curr_tkn_starts_on_line_num, curr_tkn_starts_on_col_pos);
        resolve_final_tkn_type (tkn);
        if (tkn->tkn_type != BRKN_TKN)
          token_stream.push_back(tkn);
        curr_str.clear();
        curr_tkn_type = START_UNDEF_TKN;

      } else  {
        switch (curr_tkn_type)  {
          case START_UNDEF_TKN:
            curr_tkn_type = start_new_tkn_get_type (input_stream, token_stream, curr_char, curr_str);
            break;

          case WHITE_SPACE_TKN         :
            if (!iswspace(curr_char)) 
              // No longer chomping on white space, so we're starting a new Token
              curr_tkn_type = start_new_tkn_get_type (input_stream, token_stream, curr_char, curr_str);
            break;
          case USER_WORD_TKN             :
            if (iswspace(curr_char) || compilerTerms.is_sngl_char_spr8r(curr_char) || (iswpunct(curr_char) && curr_char != '_') )  {
              // Space, spr8r or punctuation (except _) ends a USER_WORD
              std::shared_ptr<Token> tkn = std::make_shared<Token>(curr_tkn_type, curr_str, fileName, curr_tkn_starts_on_line_num, curr_tkn_starts_on_col_pos);
              resolve_final_tkn_type (tkn);
              token_stream.push_back(tkn);
              curr_str.clear();

              if (iswspace(curr_char))
                  curr_tkn_type = START_UNDEF_TKN;
              else 
                curr_tkn_type = start_new_tkn_get_type (input_stream, token_stream, curr_char, curr_str);

            } else  {
              curr_str += curr_char;
            }

            break;
          case STRING_TKN              :
            // TODO: Is it reasonable to expect date times to be contained within quotes?
            /*
            *  2022-10-14 11:19:56.987
            *  2022-10-14 11:19:56.98
            *  2022-10-14 11:19:56.9
            *  2022-10-14 11:19:56
            *  2022-10-14 11:19
            *  2022-10-14
            */
          case DATETIME_TKN            :
            // TODO: Can strings cross multiple lines? ???
            if (curr_char == '"' && prev_char != '\\') {
              // We got our closing quote and it was *NOT* escaped
              std::shared_ptr<Token> tkn = std::make_shared <Token> (curr_tkn_type, curr_str, fileName, curr_tkn_starts_on_line_num, curr_tkn_starts_on_col_pos);
              resolve_final_tkn_type (tkn);
              if (tkn->tkn_type != BRKN_TKN)
                token_stream.push_back(tkn);
              curr_str.clear();
              curr_tkn_type = START_UNDEF_TKN;
            } else  {
              curr_str += curr_char;
            }
            break;
          case OLD_SCHOOL_CMMNT_TKN     :
            if (CATASTROPHIC_FAILURE == is_end_olde_skul_cmmnt (input_stream, curr_char, curr_str, is_end))  {
              ret_code = CATASTROPHIC_FAILURE;
              failed_on_src_line_num = __LINE__;

            } else if (is_end)  {
              // Comments are syntactic sugar that just melt away and do *NOT* get added to the Token stream
              curr_str.clear();
              curr_tkn_type = START_UNDEF_TKN;

            } else  {
              curr_str += curr_char;
            }
            break;
          case TIL_EOL_CMMNT_TKN   :
            curr_str += curr_char;
            break;
          case UINT8_TKN					 :
          case UINT16_TKN          :
          case UINT32_TKN          :
          case UINT64_TKN          :
          case INT8_TKN            :
          case INT16_TKN		       :
          case INT32_TKN		       :
          case INT64_TKN		       :
            if (iswpunct(curr_char) || iswspace (curr_char) || compilerTerms.is_sngl_char_spr8r(curr_char))  {
              std::shared_ptr<Token> tkn = std::make_shared <Token> (curr_tkn_type, curr_str, fileName, curr_tkn_starts_on_line_num, curr_tkn_starts_on_col_pos);
              resolve_final_tkn_type (tkn);
              if (tkn->tkn_type != BRKN_TKN)
                token_stream.push_back(tkn);
              curr_str.clear();

              if (iswspace(curr_char)) {
                curr_tkn_type = START_UNDEF_TKN;
              } else
              curr_tkn_type = start_new_tkn_get_type (input_stream, token_stream, curr_char, curr_str);
            } else  {
              curr_str += curr_char;
            }
            break;
          case SRC_OPR8R_TKN               :
            // OPR8Rs are made up of punctuation characters, *except* for the 1-char separators
            // A single character OPR8R (e.g. ;) will end the currently accumulating OPR8R, and
            // both will be added to the Token stream in order
            if (!iswpunct(curr_char) || compilerTerms.is_sngl_char_spr8r(curr_char) || compilerTerms.is_atomic_opr8r(curr_char) || curr_char == '"')  {
              std::shared_ptr<Token> opr8r = std::make_shared <Token> (curr_tkn_type, curr_str, fileName, curr_tkn_starts_on_line_num, curr_tkn_starts_on_col_pos);
              resolve_final_tkn_type (opr8r);
              if (opr8r->tkn_type != BRKN_TKN)
                token_stream.push_back(opr8r);
              curr_str.clear();
              curr_tkn_type = start_new_tkn_get_type (input_stream, token_stream, curr_char, curr_str);
            } else  {
              curr_str += curr_char;
            }
            break;

          case SPR8R_TKN               :
            // SPR8R's are only 1 charactor long, and should have already been taken care of
            failed_on_src_line_num = __LINE__;
            break;
          default:
            failed_on_src_line_num = __LINE__;
            std::wcout << "failed_on_src_line_num = " << failed_on_src_line_num << "; prev_char = " << std::hex << prev_char << "; curr_char = " << curr_char << ";" << std::endl << std::dec;
            break;
        }
      }

      prev_char = curr_char;
      nxt_ret = get_next_char(input_stream, curr_char);

      if (nxt_ret == END_OF_FILE)
        is_EOF = true;
      else if (OK != nxt_ret)
        // TODO: Will need to check for END_OF_FILE condition
        failed_on_src_line_num = __LINE__;
    }

    if (!curr_str.empty() && failed_on_src_line_num == 0) {
      std::shared_ptr<Token> tkn = std::make_shared <Token> (curr_tkn_type, curr_str, fileName, curr_tkn_starts_on_line_num, curr_tkn_starts_on_col_pos);
      resolve_final_tkn_type (tkn);
      if (tkn->tkn_type != BRKN_TKN)
        token_stream.push_back(tkn);
      curr_str.clear();
    }

    input_stream.close();

    std::shared_ptr<Token> eos_tkn = std::make_shared <Token> (END_OF_STREAM_TKN, END_OF_STREAM_STR);
    token_stream.push_back (eos_tkn);

    if (failed_on_src_line_num == 0)
      ret_code = OK;
    else  {
      ret_code = failed_on_src_line_num;
      std::wcout << "curr_str = [" << curr_str << "] on line # " << curr_tkn_starts_on_line_num << " at column " << curr_tkn_starts_on_col_pos << std::endl;
    }
  }

  return (ret_code);
}


