/*
 * ErrorInfo.cpp
 *
 *  Created on: Nov 14, 2024
 *      Author: Mike Volk
 */

#include "ErrorInfo.h"

ErrorInfo::ErrorInfo() {
	typeOfError = UNKNOWN_ERROR;
	ourSrcLineNum = 0;
}



ErrorInfo::~ErrorInfo() {
	ourSrcFileName.clear();
	userMsg.clear();
}

void ErrorInfo::set1stInSrcStack(std::wstring srcFileName) {
	if (srcFileStack.empty())
		srcFileStack = srcFileName;
}


ErrorInfo& ErrorInfo::operator= (const ErrorInfo & srcErrInfo)
{
	// self-assignment check
	if (this == &srcErrInfo)
		return (*this);

	// if data exists in the current string, delete it
	typeOfError = srcErrInfo.typeOfError;

	if (std::string::npos == srcFileStack.find (srcErrInfo.ourSrcFileName))	{
		// Check for existence 1st...really don't want cycles and unbound growth!
		srcFileStack.append (STACK_SPR8R);
		srcFileStack.append (srcErrInfo.ourSrcFileName);
	}

	ourSrcFileName = srcErrInfo.ourSrcFileName;
	ourSrcLineNum = srcErrInfo.ourSrcLineNum;
	userMsg.clear();
	userMsg = srcErrInfo.userMsg;


	// TODO: I don't understand the comment below
	// return the existing object so we can chain this operator
	return (*this);
}


void ErrorInfo::set(errorType type, std::wstring srcFileName, int srcLineNum, std::wstring msgForUser) {
	typeOfError = type;
	ourSrcFileName = srcFileName;
	ourSrcLineNum = srcLineNum;
	userMsg = msgForUser;
}

bool ErrorInfo::isEmpty()	{
	bool isAvailable = false;

	if (typeOfError == UNKNOWN_ERROR && userMsg.empty())
		isAvailable = true;

	return isAvailable;
}

std::wstring ErrorInfo::getFormattedMsg ()	{
	std::wstring msg;

	switch (typeOfError)	{
		case USER_ERROR:
			msg = L"USER_ERROR: ";
			msg.append (userMsg);
			break;
		case INTERNAL_ERROR:
			msg = L"INTERNAL_ERROR: Encountered on ";
			msg.append (ourSrcFileName);
			msg.append (L":");
			msg.append (std::to_wstring(ourSrcLineNum));
			msg.append (L". ");
			msg.append (userMsg);
			msg.append (L"\n");
			msg.append (L"Source stack (-cycles): ");
			msg.append (srcFileStack);

			break;
		default:
			msg = L"UNKNOWN_ERROR: ";
			msg.append (!ourSrcFileName.empty() ? ourSrcFileName : L"UNKNOWN_FILE");
			msg.append (L":");
			msg.append (ourSrcLineNum > 0 ? std::to_wstring(ourSrcLineNum) : L"???");
			msg.append (L". ");
			msg.append (!userMsg.empty() ? userMsg : L"NO USER MESSAGE.");
	}

	return msg;
}

std::wstring ErrorInfo::getErrorTypeStr()	{
	std::wstring typeStr;

	switch (typeOfError)	{
		case USER_ERROR:
			typeStr = L"USER_ERROR";
			break;
		case INTERNAL_ERROR:
			typeStr = L"INTERNAL_ERROR";
			break;
		default:
			typeStr = L"UNKNOWN_ERROR";
	}

	return (typeStr);
}

std::wstring ErrorInfo::getSrcFileName()	{
	return (ourSrcFileName);
}

int ErrorInfo::getSrcLineNum()	{
	return (ourSrcLineNum);
}

std::wstring ErrorInfo::getUserMsgFld()	{
	return (userMsg);
}


