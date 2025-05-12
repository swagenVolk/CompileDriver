/*
 * CompileExecTerms.h
 *
 *  Created on: Jan 4, 2024
 *      Author: Mike Volk
 */

#ifndef COMPILEEXECTERMS_H_
#define COMPILEEXECTERMS_H_

#include "BaseLanguageTerms.h"

// Clarified OPR8Rs for Generated/interpreted code
#define PRE_INCR_OPR8R			L"+1"
#define POST_INCR_OPR8R			L"1+"
#define PRE_DECR_OPR8R			L"-1"
#define POST_DECR_OPR8R			L"1-"
#define UNARY_PLUS_OPR8R 		L"+U"
#define UNARY_MINUS_OPR8R		L"-U"
#define BINARY_PLUS_OPR8R 	L"B+"
#define BINARY_MINUS_OPR8R	L"B-"

#define SYS_CALL_STR        L"str"
#define SYS_CALL_PRINT_LINE L"print_line"

class CompileExecTerms: public BaseLanguageTerms {
public:
	CompileExecTerms();
	virtual ~CompileExecTerms();
};

#endif /* COMPILEEXECTERMS_H_ */
