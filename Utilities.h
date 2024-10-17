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

class Utilities {
public:
	Utilities();
	virtual ~Utilities();
	std::wstring stringToWstring (std::string skinnyStr);
	std::wstring getLastSegment (std::wstring pluralSegments, std::wstring delimiter);
};

#endif /* UTILITIES_H_ */
