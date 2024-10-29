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

