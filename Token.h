#ifndef TOKEN_H
#define TOKEN_H

#include "common.h"
#include "TokenCompareResult.h"
#include "FileLineCol.h"
#include <string>
#include <stdint.h>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <iostream>
#include <sstream>
#include <memory>
#include <cmath>

#define END_OF_STREAM_STR L"END_OF_STREAM"

// TODO: Pros & cons of enums vs. #defines
// #define bit mask would allow for checking allowed token types
// Should I make a = operator override?

enum tkn_type_enum {
  BRKN_TKN                  // Illegal for a *committed* token
  ,JUNK_TKN                 // e.g. 200KbarKnives
  ,START_UNDEF_TKN          // Illegal for a *committed* token
  ,WHITE_SPACE_TKN          // Illegal for a *committed* token - syntactic sugar
  ,KEYWORD_TKN             
  ,STRING_TKN              
  ,DATETIME_TKN            
  ,OLD_SCHOOL_CMMNT_TKN     // Illegal for a *committed* token - syntactic sugar
  ,TIL_EOL_CMMNT_TKN        // Illegal for a *committed* token - syntactic sugar
	// TODO: Are 16 & 32 bit representations necessary? Useful?  Might be useful from a user's perspective....enforce size limits
  ,UINT8_TKN
	,UINT16_TKN
	,UINT32_TKN
	,UINT64_TKN
	,INT8_TKN
	,INT16_TKN
	,INT32_TKN
	,INT64_TKN
  ,DOUBLE_TKN   
  ,SRC_OPR8R_TKN
	,EXEC_OPR8R_TKN						// Run-time OPR8R Token stores op_code in _unsigned rather than a string representation
  ,SPR8R_TKN
  ,END_OF_STREAM_TKN        // Expect file_parser producer and compiler consumer threads
                            // Will need a token to indicate EOS since the compiler
                            // could empty the list while the file_parser is still producing
};

typedef tkn_type_enum TokenTypeEnum;

class Token {
  public:
		Token();
		Token(tkn_type_enum found_type, std::wstring tokenized_str);
    Token(tkn_type_enum found_type, std::wstring tokenized_str, std::wstring srcFileName, int line_num, int col_pos);
    ~Token();
    Token& operator= (const Token & srcTkn);


    int tkn_type;
    std::wstring _string;
    uint64_t    _unsigned;
    int64_t     _signed;
    double       _double;
    FileLineCol src;
    std::wstring get_type_str();
    bool is_Rvalue;
    bool isInitialized;

    std::wstring get_tkn_type_by_enum (tkn_type_enum tkn_type);
    std::wstring descr_sans_line_num_col ();
    std::wstring descr_line_num_col ();
    std::wstring getValueStr ();
    TokenCompareResult compare (Token & otherTkn);
    bool isOperand();
    bool evalResolvedTokenAsIf ();
    bool isUnsigned ();
    bool isSigned ();
    void resetToken ();
    void resetToUnsigned (uint64_t newValue);
    void resetToSigned (int64_t newValue);
    void resetToDouble (double newValue);
    void resetToString (std::wstring newValue);
    int convertTo (Token newValTkn);
    int get_line_number ();
    int get_column_pos ();

  private:


};

typedef std::vector<std::shared_ptr<Token>> TokenPtrVector;


#endif
