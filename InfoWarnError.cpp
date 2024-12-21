/*
 * InfoWarnError.cpp
 *
 *  Created on: Nov 14, 2024
 *      Author: Mike Volk
 */

#include "InfoWarnError.h"

InfoWarnError::InfoWarnError() {
	typeOfError = UNKNOWN_ERROR;
	ourSrcLineNum = 0;
	userSrcLineNum = 0;
	userSrcColumnPos = 0;
}

InfoWarnError::~InfoWarnError() {
	ourSrcFileName.clear();
	userSrcFileName.clear();
	userMsg.clear();
}

void InfoWarnError::set1stInSrcStack(std::wstring srcFileName) {
	if (srcFileStack.empty())
		srcFileStack = srcFileName;
}


InfoWarnError& InfoWarnError::operator= (const InfoWarnError & srcErrInfo)
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

	userSrcFileName = srcErrInfo.userSrcFileName;
	userSrcLineNum = srcErrInfo.userSrcLineNum;
	userSrcColumnPos = srcErrInfo.userSrcColumnPos;
	userMsg.clear();
	userMsg = srcErrInfo.userMsg;


	// TODO: I don't understand the comment below
	// return the existing object so we can chain this operator
	return (*this);
}

void InfoWarnError::setInternalError(std::wstring srcFileName, int srcLineNum, std::wstring msgForUser) {
	typeOfError = INTERNAL_ERROR;
	ourSrcFileName = srcFileName;
	ourSrcLineNum = srcLineNum;
	userMsg = msgForUser;
}

void InfoWarnError::setUserMsg(info_warn_error_type type, std::wstring userSrcFileName, int userSrcLineNum, int userSrcColPos, std::wstring msgForUser) {
	typeOfError = type;
	this->userSrcFileName = userSrcFileName;
	this->userSrcLineNum = userSrcLineNum;
	userSrcColumnPos = userSrcColPos;
	userMsg = msgForUser;
}

void InfoWarnError::set(info_warn_error_type type, std::wstring userSrcFileName, int userSrcLineNum, int userSrcColPos, std::wstring srcFileName, int srcLineNum, std::wstring msgForUser) {
	typeOfError = type;
	this->userSrcFileName = userSrcFileName;
	this->userSrcLineNum = userSrcLineNum;
	this->userSrcColumnPos = userSrcColPos;
	ourSrcFileName = srcFileName;
	ourSrcLineNum = srcLineNum;
	userMsg = msgForUser;
}


bool InfoWarnError::isEmpty()	{
	bool isAvailable = false;

	if (typeOfError == UNKNOWN_ERROR && userMsg.empty())
		isAvailable = true;

	return isAvailable;
}

std::wstring InfoWarnError::getFormattedMsg ()	{
	std::wstring msg;

	switch (typeOfError)	{
		case WARNING:
			msg = L"WARNING: ";
			break;
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

std::wstring InfoWarnError::getErrorTypeStr()	{
	std::wstring typeStr;

	switch (typeOfError)	{
		case INFO:
			typeStr = L"INFO";
		break;
		case WARNING:
			typeStr = L"WARNING";
			break;
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

/* ****************************************************************************
 *
 * ***************************************************************************/
std::wstring InfoWarnError::getOurSrcFileName()	{
	return (ourSrcFileName);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
info_warn_error_type InfoWarnError::getTypeOfError()	{
	return (typeOfError);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InfoWarnError::getOurSrcLineNum()	{
	return (ourSrcLineNum);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
std::wstring InfoWarnError::getUserMsgFld()	{
	return (userMsg);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
std::wstring InfoWarnError::getUserSrcFileName()	{
	return (userSrcFileName);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InfoWarnError::getUserSrcLineNum()	{
	return (userSrcLineNum);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InfoWarnError::getUserSrcColPos()	{
	return (userSrcColumnPos);
}

/* ****************************************************************************
 * Get the chronological order an instance was inserted into some kind of
 * outside container.
 * ***************************************************************************/
int InfoWarnError::get_insertedPos ()	{
	return (insertedPos);
}

/* ****************************************************************************
 * Used by outside container object to note the chronological order each instance
 * is inserted into the outside container. Will allow UI code to order messages
 * [INFO|WARN|USER_ERROR|INTERNAL_ERROR] in multiple ways.
 * ***************************************************************************/
void InfoWarnError::set_insertedPos (int orderPos)	{
	insertedPos = orderPos;
}



