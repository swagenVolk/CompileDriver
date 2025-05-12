/*
 * Utilities.h
 *
 *  Created on: Oct 1, 2024
 *      Author: Mike Volk
 */

#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <string>
#include "Token.h"

class Utilities {
public:
	Utilities();
	virtual ~Utilities();
	std::wstring stringToWstring (std::string skinnyStr);
	std::wstring getLastSegment (std::wstring pluralSegments, std::wstring delimiter);
	std::wstring joinStrings (std::vector<std::wstring> & strVector, std::wstring spr8r, bool ignoreBlankEntries);
	std::wstring trim (std::wstring inStr);
	void splitString (std::wstring inStr, std::wstring spr8r, std::vector<std::wstring> & strVector);
	std::wstring getTokenListStr (std::vector<Token> & tokenStream, int caretTgtIdx, int & caretPos);
};

#endif /* UTILITIES_H_ */
