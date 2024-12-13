/*
 * Utilities.cpp
 *
 * Junk drawer catch-all class for useful utilities.
 *  Created on: Oct 1, 2024
 *      Author: mike
 */

#include "Utilities.h"
#include "common.h"
#include <cctype>
#include <cwctype>
#include <stdint.h>
#include <string>

Utilities::Utilities() {
	// TODO Auto-generated constructor stub

}

Utilities::~Utilities() {
	// TODO Auto-generated destructor stub
}

/* ****************************************************************************
 * Take a "skinny" std::string and return a "wide" std::wstring
 * NOTE: This is probably only useful for ASCII 8-bit characters
 * TODO: check out std::wstring_convert
 * ***************************************************************************/
std::wstring Utilities::stringToWstring (std::string skinnyStr)	{
	std::wstring wideStr;

	// Fatten up the skinnyStr and make it W_I_D_E
	for (std::string::iterator str8r = skinnyStr.begin(); str8r != skinnyStr.end(); str8r++)	{
		wideStr.append (1, *str8r);
	}

	return (wideStr);
}

/* ****************************************************************************
 * Get the last segment of a wstring, e.g.
 * cmake.debug.linux.x86_64
 * from
 * /home/mike/eclipse-workspace/CompileDriver/build/cmake.debug.linux.x86_64
 * ***************************************************************************/
std::wstring Utilities::getLastSegment (std::wstring pluralSegments, std::wstring delimiter)	{
	std::wstring lastSeg;

	std::string::size_type pos = pluralSegments.rfind(delimiter);
	if (pos == std::string::npos)
		lastSeg = pluralSegments;
	else
		lastSeg = pluralSegments.substr(pos + delimiter.length());

	return (lastSeg);
}

/* ****************************************************************************
 * Take the contents of a vector of strings and concatenate them
 * ***************************************************************************/
std::wstring Utilities::joinStrings (std::vector<std::wstring> & strVector, std::wstring spr8r, bool ignoreBlankEntries)	{
	std::wstring concatStr;
	std::wstring nextStr;

	for (int idx = 0; idx < strVector.size(); idx++)	{
		nextStr = strVector[idx];
		if (!ignoreBlankEntries)	{
			if (idx > 0)
				concatStr.append(spr8r);
			concatStr.append(nextStr);
	
		} else if (!nextStr.empty()) {
			if (!concatStr.empty())
				concatStr.append(spr8r);
			concatStr.append(nextStr);
		}
	}

	return (concatStr);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
std::wstring Utilities::trim (std::wstring inStr)	{
	std::wstring retStr = L"";
	int idx;
	int firstValidPos = -1;
	int lastValidPos = -1;

	for (idx = 0; idx < inStr.length(); idx++)	{
		wchar_t currChar = inStr[idx];
		if (!std::iswspace(currChar))	{
			firstValidPos = idx;
			break;
		}
	}

	for (idx = inStr.length() - 1; idx >= 0; idx--)	{
		wchar_t currChar = inStr[idx];
		if (!std::iswspace(currChar))	{
			lastValidPos = idx;
			break;
		}
	}

	if (firstValidPos >= 0 && firstValidPos <= lastValidPos && lastValidPos < inStr.length())	
		// 2nd parameter of substr is a COUNT of characters, not a position
		retStr = inStr.substr(firstValidPos, ((lastValidPos + 1) - firstValidPos));

	return (retStr);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
void Utilities::splitString (std::wstring inStr, std::wstring spr8r, std::vector<std::wstring> & strVector)	{
	int ret_code = GENERAL_FAILURE;
	std::wstring::size_type foundPos;

	while (!inStr.empty())	{
		foundPos = inStr.find (spr8r);

		if (foundPos == std::string::npos)	{
			strVector.push_back (inStr);
			inStr.clear();
		
		} else {
			std::wstring segment = trim (inStr.substr(0, foundPos));
			if (!segment.empty())
				strVector.push_back(segment);
			inStr.erase (0, foundPos + spr8r.length());
		}
	}
}

/* ****************************************************************************
 *
 * ***************************************************************************/
void Utilities::dumpTokenList (std::vector<Token> & tokenStream, CompileExecTerms execTerms, std::wstring callersSrcFile, int lineNum)	{
	std::wstring tknStrmStr = L"";

	std::wcout << L"********** dumpTokenList called from " << callersSrcFile << L":" << lineNum << L" **********" << std::endl;
	int idx;
	for (idx = 0; idx < tokenStream.size(); idx++)	{
		Token listTkn = tokenStream[idx];
		std::wcout << L"[" ;

		switch (listTkn.tkn_type)	{
			case EXEC_OPR8R_TKN:
			if (listTkn._unsigned == PRE_INCR_OPR8R_OPCODE)	
				std::wcout << L"++(" << listTkn._string << L")";
			else if (listTkn._unsigned == PRE_DECR_OPR8R_OPCODE)
				std::wcout << L"--(" << listTkn._string << L")";
			else if (listTkn._unsigned == POST_INCR_OPR8R_OPCODE)
				std::wcout << L"(" << listTkn._string << L")++";
			else if (listTkn._unsigned == POST_DECR_OPR8R_OPCODE)
				std::wcout << L"(" << listTkn._string << L")--";
			else
				std::wcout << execTerms.getSrcOpr8rStrFor(listTkn._unsigned);
			break;
			case UINT8_TKN:
			case UINT16_TKN:
			case UINT32_TKN:
			case UINT64_TKN:
				std::wcout << listTkn._unsigned;
				break;
			case INT8_TKN:
			case INT16_TKN:
			case INT32_TKN:
			case INT64_TKN:
				std::wcout << listTkn._signed;
				break;
			case DOUBLE_TKN:
				std::wcout << listTkn._double;
				break;
			case STRING_TKN:
			case DATETIME_TKN:
			case USER_WORD_TKN:
			case SRC_OPR8R_TKN:
				std::wcout << listTkn._string;
				break;
			default:
				std::wcout << L"???";
				break;
		}
		std::wcout << L"] ";
	}

	std::wcout << std::endl;

}

