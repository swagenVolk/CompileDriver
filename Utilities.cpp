/*
 * Utilities.cpp
 *
 * Junk drawer catch-all class for useful utilities.
 *  Created on: Oct 1, 2024
 *      Author: Mike Volk
 */

#include "Utilities.h"
#include "Token.h"
#include "common.h"
#include "locale_strings.h"
#include <cctype>
#include <cwctype>
#include <iostream>
#include <stdint.h>
#include <string>

Utilities::Utilities() {

}

Utilities::~Utilities() {
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
 * TODO: Should various dumpTokenList fxns get moved into Token class?
 * ***************************************************************************/
void Utilities::dumpTokenList (std::vector<Token> & tokenStream, CompileExecTerms execTerms, std::wstring callersSrcFile, int lineNum)	{
	std::wstring tknStrmStr = L"";

	dumpTokenList (tokenStream, 0, execTerms, callersSrcFile, lineNum);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
void Utilities::dumpTokenList (std::vector<Token> & tokenStream, int startIdx, CompileExecTerms execTerms, std::wstring callersSrcFile, int lineNum)	{
	std::wstring tknStrmStr = L"";


	std::wcout << L"********** dumpTokenList called from " << callersSrcFile << L":" << lineNum << L" **********" << std::endl;
	if (startIdx >= 0 && startIdx < tokenStream.size())	{
		for (int idx = startIdx; idx < tokenStream.size(); idx++)	{
			std::wcout << tokenStream[idx].getBracketedValueStr();
		}
	
	} else {
		std::wcout << L"tokenStream size = " << tokenStream.size() << L" but passed in startIdx = " << startIdx;
	}

	std::wcout << std::endl;

}

/* ****************************************************************************
 *
 * ***************************************************************************/
void Utilities::dumpTokenList (TokenPtrVector & tknPtrVector, CompileExecTerms execTerms
	, std::wstring callersSrcFile, int lineNum, bool isShowDetail)	{
	std::wstring tknStrmStr = L"";

	dumpTokenList (tknPtrVector, 0, execTerms, callersSrcFile, lineNum, isShowDetail);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
void Utilities::dumpTokenList (TokenPtrVector & tknPtrVector, int startIdx, CompileExecTerms execTerms
	, std::wstring callersSrcFile, int lineNum, bool isShowDetail)	{
	std::wstring tknStrmStr = L"";


	std::wcout << L"********** dumpTokenList called from " << callersSrcFile << L":" << lineNum << L" **********" << std::endl;
	if (startIdx >= 0 && startIdx < tknPtrVector.size())	{
		for (int idx = startIdx; idx < tknPtrVector.size(); idx++)	{
			Token currTkn = *tknPtrVector[idx];
			if (!isShowDetail)
				std::wcout << currTkn.getBracketedValueStr();
			else
			 	std::wcout << currTkn.descr_line_num_col() << std::endl;
		}
	
	} else {
		std::wcout << L"tokenStream size = " << tknPtrVector.size() << L" but passed in startIdx = " << startIdx;
	}

	std::wcout << std::endl;

}

/* ****************************************************************************
 *
 * ***************************************************************************/
std::wstring Utilities::getTokenListStr (std::vector<Token> & tokenStream, int caretTgtIdx, int & caretPos)	{
	std::wstring tknStrmStr = L"";
	if (caretTgtIdx < 0 || caretTgtIdx > tokenStream.size())
		caretPos = -1;
	
	for (int idx = 0; idx < tokenStream.size(); idx++)	{
		if (idx == caretTgtIdx)
			caretPos = tknStrmStr.length();
		tknStrmStr.append(tokenStream[idx].getBracketedValueStr());
	}

	return (tknStrmStr);
}