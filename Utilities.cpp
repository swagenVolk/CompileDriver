/*
 * Utilities.cpp
 *
 * Junk drawer catch-all class for useful utilities.
 *  Created on: Oct 1, 2024
 *      Author: mike
 */

#include "Utilities.h"
#include <stdint.h>

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
			case KEYWORD_TKN:
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
