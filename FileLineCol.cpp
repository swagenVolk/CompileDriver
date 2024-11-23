/*
 * FileLineCol.cpp
 *
 *  Created on: Nov 21, 2024
 *      Author: Mike Volk
 */

#include "FileLineCol.h"
FileLineCol::FileLineCol()	{
	lineNumber = 0;
	columnPos = 0;
}
FileLineCol::FileLineCol(std::wstring in_fileName, int in_lineNumber, int in_colPos) {
	// TODO Auto-generated constructor stub
	fileName = in_fileName;
	lineNumber = in_lineNumber;
	columnPos = in_colPos;

}

FileLineCol::~FileLineCol() {
	// TODO Auto-generated destructor stub
}

FileLineCol& FileLineCol::operator= (const FileLineCol & src)
{
	// self-assignment check
	if (this == &src)
		return (*this);

	// if data exists in the current string, delete it
	fileName.clear();
	fileName = src.fileName;
	lineNumber = src.lineNumber;
	columnPos = src.columnPos;

	// TODO: I don't understand the comment below
	// return the existing object so we can chain this operator
	return (*this);
}


