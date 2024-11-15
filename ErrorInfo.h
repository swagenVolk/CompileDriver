/*
 * ErrorInfo.h
 *
 *  Created on: Nov 14, 2024
 *      Author: Mike Volk
 */

#ifndef ERRORINFO_H_
#define ERRORINFO_H_

#include "common.h"
#include <string>


enum error_type_enum	{
	UNKNOWN_ERROR
	,USER_ERROR
	,INTERNAL_ERROR

};

typedef error_type_enum errorType;

class ErrorInfo {
public:
	ErrorInfo ();
	virtual ~ErrorInfo();
	bool isEmpty();
	void set (errorType type, std::wstring srcFileName, int srcLineNum, std::wstring msgForUser);
	std::wstring getErrorTypeStr();
	std::wstring getSrcFileName();
	int getSrcLineNum();
	std::wstring getUserMsgFld();
	std::wstring getFormattedMsg();

private:
	errorType typeOfError;
	std::wstring ourSrcFileName;
	int	ourSrcLineNum;
	std::wstring userMsg;
};

#endif /* ERRORINFO_H_ */
