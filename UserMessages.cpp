/*
 * UserMessages.cpp
 *
 *  Created on: Nov 21, 2024
 *      Author: Mike Volk
 *
 *  Class to hold messages for user and to provide duplicate squashing and ordered presentation of 
 *  info messages, warnings, user errors and internal errors.
 */

#include "UserMessages.h"
#include "FileLineCol.h"
#include "InfoWarnError.h"
#include <memory>

UserMessages::UserMessages() {
	absoluteInsertPos = 0;

}

UserMessages::~UserMessages() {
	reset();
}

void UserMessages::reset()	{
	absoluteInsertPos = 0;

	while (!infoMessages.empty())	{
		auto itr8r = infoMessages.begin();
		itr8r->second.reset();
		infoMessages.erase(itr8r);
	}

	while (!warningMessages.empty())	{
		auto itr8r = warningMessages.begin();
		itr8r->second.reset();
		warningMessages.erase(itr8r);
	}

	while (!userErrorMessages.empty())	{
		auto itr8r = userErrorMessages.begin();
		itr8r->second.reset();
		userErrorMessages.erase(itr8r);
	}

	while (!internalErrorMessages.empty())	{
		auto itr8r = internalErrorMessages.begin();
		itr8r->second.reset();
		internalErrorMessages.erase(itr8r);
	}
}

/* ****************************************************************************
 *
 * ***************************************************************************/
void UserMessages::insertNoDupes (std::map<std::wstring, std::shared_ptr<std::vector<FileLineCol>>> & messagesHolder
	, InfoWarnError & msg, FileLineCol newFileLineCol)	{

	auto search = messagesHolder.find(msg.getUserMsgFld());
	if (search == messagesHolder.end())	{
		std::shared_ptr<std::vector<FileLineCol>> emptyMsgList = std::make_shared <std::vector<FileLineCol>> ();
		newFileLineCol.insertPos = absoluteInsertPos++;
		emptyMsgList->push_back (newFileLineCol);
		messagesHolder.insert ( std::pair {msg.getUserMsgFld(), emptyMsgList} );

	} else	{
		std::shared_ptr<std::vector<FileLineCol>> msgList = search->second;
		bool isFlcExists = false;

		for (auto itr8r = msgList->begin(); itr8r != msgList->end() && !isFlcExists; itr8r++)	{
			// Make sure we don't insert duplicates
			FileLineCol currFlc = *itr8r;
			if (currFlc.fileName == newFileLineCol.fileName && currFlc.lineNumber == newFileLineCol.lineNumber && currFlc.columnPos == newFileLineCol.columnPos)
				isFlcExists = true;
		}
		if (!isFlcExists)	{
			newFileLineCol.insertPos = absoluteInsertPos++;
			msgList->push_back (newFileLineCol);
		}
	}
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int UserMessages::logMsg (InfoWarnError & msg)	{

	int ret_code = GENERAL_FAILURE;

	info_warn_error_type msgType	= msg.getTypeOfError();
	if (INFO == msgType)	{
		FileLineCol newInfoFlc(msg.getUserSrcFileName(), msg.getUserSrcLineNum(), msg.getUserSrcColPos());
		insertNoDupes(infoMessages, msg, newInfoFlc);
		ret_code = OK;

	} else if (WARNING == msgType)	{
		FileLineCol newWarnFlc(msg.getUserSrcFileName(), msg.getUserSrcLineNum(), msg.getUserSrcColPos());
		insertNoDupes(warningMessages, msg, newWarnFlc);
		ret_code = OK;

	} else if (USER_ERROR == msgType)	{
		FileLineCol newErrorFlc(msg.getUserSrcFileName(), msg.getUserSrcLineNum(), msg.getUserSrcColPos());
		insertNoDupes(userErrorMessages, msg, newErrorFlc);
		ret_code = OK;

	} else if (INTERNAL_ERROR == msgType)	{
		FileLineCol newInternalFlc(msg.getOurSrcFileName(), msg.getOurSrcLineNum(), 0);
		insertNoDupes(internalErrorMessages, msg, newInternalFlc);
		ret_code = OK;

	}

	return (ret_code);

}

/* ****************************************************************************
 *
 * ***************************************************************************/
int UserMessages::logMsg (info_warn_error_type msgType, std::wstring userMsg, std::wstring srcFileName, int lineNumber, int columnPos)	{
	int ret_code = GENERAL_FAILURE;
	InfoWarnError infoWarnError;

	if (msgType == INTERNAL_ERROR)
		infoWarnError.setInternalError (srcFileName, lineNumber, userMsg);
	else
		infoWarnError.setUserMsg (msgType, srcFileName, lineNumber, columnPos, userMsg);

	ret_code = logMsg (infoWarnError);

	return (ret_code);

}
/* ****************************************************************************
 *
 * ***************************************************************************/
std::pair <int, int> UserMessages::getUniqueTotalMsgCnt (std::map<std::wstring, std::shared_ptr<std::vector<FileLineCol>>> & messagesHolder)	{

	std::pair <int, int> unique_total {0,0};

	for (auto itr8r = messagesHolder.begin(); itr8r != messagesHolder.end(); itr8r++)	{
		std::shared_ptr<std::vector<FileLineCol>> msgInstances = itr8r->second;
		unique_total.first++;
		unique_total.second += msgInstances->size();
	}

	return unique_total;
}

/* ****************************************************************************
 *
 * ***************************************************************************/
void UserMessages::displayMessagesInHolder (std::map<std::wstring, std::shared_ptr<std::vector<FileLineCol>>> & messagesHolder)	{

	for (auto uniqR8r = messagesHolder.begin(); uniqR8r != messagesHolder.end(); uniqR8r++)	{
		// Print out the message on the 1st line
		std::wcout << uniqR8r->first << std::endl;

		std::shared_ptr<std::vector<FileLineCol>> msgInstances = uniqR8r->second;

		for (int idx = 0; idx < msgInstances->size(); idx++)	{
			std::wcout << msgInstances->at(idx).fileName << L":" << msgInstances->at(idx).lineNumber << L":" << msgInstances->at(idx).columnPos << std::endl;
		}
	}

	// Make a spacer line prior to the next entry getting written out
	std::wcout << std::endl;
}

/* ****************************************************************************
 *
 * ***************************************************************************/
void UserMessages::getUserErrorCnt (int & numUnique, int & numTotal)	{

	std::pair <int, int> uniq_total_pair = getUniqueTotalMsgCnt (userErrorMessages);
	numUnique = uniq_total_pair.first;
	numTotal = uniq_total_pair.second;

}

/* ****************************************************************************
 *
 * ***************************************************************************/
void UserMessages::getInternalErrorCnt (int & numUnique, int & numTotal)	{

	std::pair <int, int> uniq_total_pair = getUniqueTotalMsgCnt (internalErrorMessages);
	numUnique = uniq_total_pair.first;
	numTotal = uniq_total_pair.second;

}

/* ****************************************************************************
 *
 * ***************************************************************************/
void UserMessages::showMessagesByGroup ()	{

	std::pair <int, int> uniq_total_pair;

	uniq_total_pair = getUniqueTotalMsgCnt (internalErrorMessages);
	std::wcout << L"INTERNAL ERROR MESSAGES: Unique messages = " << uniq_total_pair.first << L"; Total messages = " << uniq_total_pair.second << ";" << std::endl;
	displayMessagesInHolder (internalErrorMessages);

	uniq_total_pair = getUniqueTotalMsgCnt (userErrorMessages);
	std::wcout << L"USER ERROR MESSAGES: Unique messages = " << uniq_total_pair.first << L"; Total messages = " << uniq_total_pair.second << ";" << std::endl;
	displayMessagesInHolder (userErrorMessages);

	uniq_total_pair = getUniqueTotalMsgCnt (warningMessages);
	std::wcout << L"USER WARNING MESSAGES: Unique messages = " << uniq_total_pair.first << L"; Total messages = " << uniq_total_pair.second << ";" << std::endl;
	displayMessagesInHolder (warningMessages);

	uniq_total_pair = getUniqueTotalMsgCnt (infoMessages);
	std::wcout << L"USER INFO MESSAGES: Unique messages = " << uniq_total_pair.first << L"; Total messages = " << uniq_total_pair.second << ";" << std::endl;
	displayMessagesInHolder (infoMessages);

}

/* ****************************************************************************
 *
 * ***************************************************************************/
void UserMessages::putHolderMsgsInOrder (std::map<std::wstring, std::shared_ptr<std::vector<FileLineCol>>> & messagesHolder
		, std::vector<std::pair <std::wstring, FileLineCol>> & orderedMsgs
		, std::vector<std::wstring> & orderedMsgTypes
		, std::wstring msgTypeStr)	{

	assert (orderedMsgs.size() == absoluteInsertPos);

	for (auto unqMsgR8r = messagesHolder.begin(); unqMsgR8r != messagesHolder.end(); unqMsgR8r++)	{
		std::wstring unqMsg = unqMsgR8r->first;
		std::shared_ptr<std::vector<FileLineCol>> msgInstances = unqMsgR8r->second;

		for (auto instr8r = msgInstances->begin(); instr8r != msgInstances->end(); instr8r++)	{
			FileLineCol fileLineCol = *instr8r;
			orderedMsgs[fileLineCol.insertPos].first = unqMsg;
			orderedMsgs[fileLineCol.insertPos].second = fileLineCol;
			orderedMsgTypes[fileLineCol.insertPos] = msgTypeStr;
		}

	}
}

/* ****************************************************************************
 * 
 * ***************************************************************************/
void UserMessages::showMessagesByInsertOrder (bool isOrderAscending)	{


	if (absoluteInsertPos > 0)	{
		std::vector<std::pair <std::wstring, FileLineCol>> orderedMsgs;
		orderedMsgs.reserve(absoluteInsertPos);
		int idx;
		FileLineCol emptyFlc;

		for (idx = 0; idx < absoluteInsertPos; idx++)	{
			// .reserve only allocated space, but did nothing to the count, so add some blanks
			orderedMsgs.push_back(std::pair {L"", emptyFlc});
		}

		std::vector<std::wstring> orderedMsgTypes;
		orderedMsgTypes.reserve(absoluteInsertPos);
		for (idx = 0; idx < absoluteInsertPos; idx++)	{
			// .reserve only allocated space, but did nothing to the count, so add some blanks
			orderedMsgTypes.push_back(L"");
		}

		putHolderMsgsInOrder (infoMessages, orderedMsgs, orderedMsgTypes, L"INFO");
		putHolderMsgsInOrder (warningMessages, orderedMsgs, orderedMsgTypes, L"WARNING");
		putHolderMsgsInOrder (userErrorMessages, orderedMsgs, orderedMsgTypes, L"USER ERROR");
		putHolderMsgsInOrder (internalErrorMessages, orderedMsgs, orderedMsgTypes, L"INTERNAL ERROR");

		for (isOrderAscending ? idx = 0 : idx = absoluteInsertPos;
				isOrderAscending ? idx < absoluteInsertPos : idx >= 0;
				isOrderAscending ? idx++ : idx--)	{
			// Traverse the array in ASC or DESC order based on callers preference
			std::wcout << orderedMsgTypes[idx] << L": " << orderedMsgs[idx].first << L" " << orderedMsgs[idx].second.fileName << L":"
					<< orderedMsgs[idx].second.lineNumber << L":" << orderedMsgs[idx].second.columnPos << std::endl;
		}
	}

}

/* ****************************************************************************
 * Helper fxn used when a class tracks failure by internal source line number
 * but didn't use UserMessages to store a detailed message. Useful for some cases
 * where the error condition was never really expected, and adding in a lot of
 * verbiage via a logMsg call seems overblown. The class destructor can do a check
 * via this call, and if there is no corresponding message it can dump out a message
 * indicating the source file and line number of the failure. This will hopefully
 * speed up debug efforts.
 * ***************************************************************************/
bool UserMessages::isExistsInternalError (std::wstring fileName, int lineNum)	{
	bool isExists = false;

	for (auto outr8r = internalErrorMessages.begin(); outr8r != internalErrorMessages.end() && !isExists; outr8r++)	{
		std::wstring unqMsg = outr8r->first;
		std::shared_ptr<std::vector<FileLineCol>> msgInstances = outr8r->second;

		for (auto innr8r = msgInstances->begin(); innr8r != msgInstances->end() && !isExists; innr8r++)	{
			if (innr8r->fileName == fileName && innr8r->lineNumber == lineNum)
				isExists = true;

		}
	}

	return (isExists);
}