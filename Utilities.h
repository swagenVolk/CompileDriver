/*
 * Utilities.h
 *
 *  Created on: Oct 1, 2024
 *      Author: mike
 */

#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <string>
#include <iterator>
#include <iostream>
#include "Token.h"
#include "CompileExecTerms.h"

class Utilities {
public:
	Utilities();
	virtual ~Utilities();
	std::wstring stringToWstring (std::string skinnyStr);
	std::wstring getLastSegment (std::wstring pluralSegments, std::wstring delimiter);
	std::wstring joinStrings (std::vector<std::wstring> & strVector, std::wstring spr8r, bool ignoreBlankEntries);
	void dumpTokenList (std::vector<Token> & tokenStream, CompileExecTerms execTerms, std::wstring callersSrcFile, int lineNum);

};

#endif /* UTILITIES_H_ */
