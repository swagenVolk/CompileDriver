/*
 * FileLineCol.cpp
 *
 * Simple class to hold file, line and column info for reporting errors from both user source code
 * and internal errors.
 *  Created on: Nov 21, 2024
 *      Author: Mike Volk
 */

#include "FileLineCol.h"
FileLineCol::FileLineCol()	{
	lineNumber = 0;
	columnPos = 0;
}
FileLineCol::FileLineCol(std::wstring in_fileName, int in_lineNumber, int in_colPos) {
	// Auto-generated constructor stub
	fileName = in_fileName;
	lineNumber = in_lineNumber;
	columnPos = in_colPos;

}

FileLineCol::~FileLineCol() {
	// Auto-generated destructor stub
	if (fileName.length() > 0)	{
		fileName.erase(0, fileName.length());
	}

}

FileLineCol& FileLineCol::operator= (const FileLineCol & src)
{
	// self-assignment check
	if (this == &src)
		return (*this);

	// if data exists in the current string, delete it
	if (!fileName.empty())
		fileName.clear();

	if (!src.fileName.empty())
		fileName = src.fileName;
	lineNumber = src.lineNumber;
	columnPos = src.columnPos;

	// TODO: I don't understand the comment below
	// return the existing object so we can chain this operator
	return (*this);
}


