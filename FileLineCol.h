/*
 * FileLineCol.h
 *
 *  Created on: Nov 21, 2024
 *      Author: Mike Volk
 */

#ifndef FILELINECOL_H_
#define FILELINECOL_H_

#include <string>

class FileLineCol {
public:
	FileLineCol();
	FileLineCol(std::wstring in_fileName, int in_lineNumber, int in_colPos);
	virtual ~FileLineCol();
	FileLineCol& operator= (const FileLineCol & src);

	std::wstring fileName;
	int	lineNumber;
	int columnPos;
	int insertPos;
};

#endif /* FILELINECOL_H_ */
