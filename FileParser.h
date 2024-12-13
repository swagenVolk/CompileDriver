#ifndef FILE_PARSER_H
#define FILE_PARSER_H

#include <list>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <queue>
#include <ctime>
#include <cmath>
#include <stdio.h>
#include "BaseLanguageTerms.h"
#include "common.h"
#include "Token.h"
#include "Operator.h"

#define CARRIAGE_RETURN             13
#define LINE_FEED                   10

#define UTF8_SNGL_BYTE_CLR_BITS     0x80
#define UTF8_DBL_BYTE_SET_BITS      0xC0
#define UTF8_DBL_BYTE_CLR_BITS      0x20
#define UTF8_TRPL_BYTE_SET_BITS     0xE0
#define UTF8_TRPL_BYTE_CLR_BITS     0x10
#define UTF8_QUAD_BYTE_SET_BITS     0xF0
#define UTF8_QUAD_BYTE_CLR_BITS     0x08

#define END_OF_FILE                 0xE9D0FEED

class FileParser {
  public:
  	FileParser(BaseLanguageTerms & inCompilerTerms, std::wstring fileName);
    int gnr8_token_stream(std::string file_name, TokenPtrVector & token_stream);
  
  private:
    void cnvrt_tkn_if_datetime (std::shared_ptr<Token> pssbl_datetime_tkn);
    void resolve_final_tkn_type (std::shared_ptr<Token> tkn_of_ambiguity);
    std::wstring fileName;
    int line_num;
    int curr_tkn_starts_on_line_num;
    int curr_tkn_starts_on_col_pos;
    int num_chars_chomped_this_line;
    int num_lines_parsed;
    wchar_t prev_char;
    long curr_file_pos;
    long curr_line_start_pos;
    BaseLanguageTerms compilerTerms;


    /* ****************************************************************************
     *
     * ***************************************************************************/
    int get_next_char(std::fstream & input_stream, wchar_t & next_char, bool is_peek);

    /* ****************************************************************************
     *
     * ***************************************************************************/
    int get_next_char(std::fstream & input_stream, wchar_t & next_char);

    /* ****************************************************************************
     *
     * ***************************************************************************/
    int peek_next_char (std::fstream & input_stream, wchar_t & peeked_char);


    /* ****************************************************************************
     *
     * ***************************************************************************/
    void resolve_final_tkn_type (Token & rsvld_tkn);

    /* ****************************************************************************
     *
     * ***************************************************************************/
    tkn_type_enum start_new_tkn_get_type (std::fstream & input_file, TokenPtrVector & token_stream
        , wchar_t curr_char, std::wstring & tkn_str);
    /* ****************************************************************************
     *
     * ***************************************************************************/
    int is_end_olde_skul_cmmnt (std::fstream & input_file, wchar_t curr_char, std::wstring & tkn_str, int & is_end);
  
    /* ****************************************************************************
     *
     * ***************************************************************************/
    int is_end_of_line (std::fstream & input_file, wchar_t curr_char, int & is_EOL);
};

#endif


