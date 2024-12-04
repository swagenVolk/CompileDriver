/*
 * UserMessages.cpp
 *
 *  Created on: Nov 21, 2024
 *      Author: Mike Volk
 */

#include "UserMessages.h"
#include "FileLineCol.h"
#include "InfoWarnError.h"

UserMessages::UserMessages() {
	// TODO Auto-generated constructor stub
	absoluteInsertPos = 0;

}

UserMessages::~UserMessages() {
	// TODO Auto-generated destructor stub
	for (auto itr8r = infoMessages.begin(); itr8r != infoMessages.end();)	{
		itr8r->second.reset();
		infoMessages.erase(itr8r++);
	}

	for (auto itr8r = warningMessages.begin(); itr8r != warningMessages.end();)	{
		itr8r->second.reset();
		warningMessages.erase(itr8r++);
	}

	for (auto itr8r = userErrorMessages.begin(); itr8r != userErrorMessages.end();)	{
		itr8r->second.reset();
		userErrorMessages.erase(itr8r++);
	}

	for (auto itr8r = internalErrorMessages.begin(); itr8r != internalErrorMessages.end();)	{
		itr8r->second.reset();
		internalErrorMessages.erase(itr8r++);
	}

}

/* ****************************************************************************
 *
 * ***************************************************************************/
int UserMessages::logMsg (InfoWarnError & msg)	{

	int ret_code = GENERAL_FAILURE;

	info_warn_error_type msgType	= msg.getTypeOfError();
	if (INFO == msgType)	{
		FileLineCol fileLineCol(msg.getUserSrcFileName(), msg.getUserSrcLineNum(), msg.getUserSrcColPos());
		fileLineCol.insertPos = absoluteInsertPos++;

		auto search = infoMessages.find(msg.getUserMsgFld());
		if (search == infoMessages.end())	{
			std::shared_ptr<std::vector<FileLineCol>> flcList = std::make_shared <std::vector<FileLineCol>> ();
			flcList->push_back (fileLineCol);
			infoMessages.insert ( std::pair {msg.getUserMsgFld(), flcList} );

		} else	{
			std::shared_ptr<std::vector<FileLineCol>> infoMsgList = search->second;
			infoMsgList->push_back (fileLineCol);
		}
		ret_code = OK;

	} else if (WARNING == msgType)	{
		FileLineCol fileLineCol(msg.getUserSrcFileName(), msg.getUserSrcLineNum(), msg.getUserSrcColPos());
		fileLineCol.insertPos = absoluteInsertPos++;

		auto search = warningMessages.find(msg.getUserMsgFld());
		if (search == warningMessages.end())	{
			std::shared_ptr<std::vector<FileLineCol>> flcList = std::make_shared <std::vector<FileLineCol>> ();
			flcList->push_back (fileLineCol);
			warningMessages.insert ( std::pair {msg.getUserMsgFld(), flcList} );

		} else	{
			std::shared_ptr<std::vector<FileLineCol>> warnMsgList = search->second;
			warnMsgList->push_back (fileLineCol);
		}
		ret_code = OK;


	} else if (USER_ERROR == msgType)	{
		FileLineCol fileLineCol(msg.getUserSrcFileName(), msg.getUserSrcLineNum(), msg.getUserSrcColPos());
		fileLineCol.insertPos = absoluteInsertPos++;

		auto search = userErrorMessages.find(msg.getUserMsgFld());
		if (search == userErrorMessages.end())	{
			std::shared_ptr<std::vector<FileLineCol>> flcList = std::make_shared <std::vector<FileLineCol>> ();
			flcList->push_back (fileLineCol);
			userErrorMessages.insert ( std::pair {msg.getUserMsgFld(), flcList} );

		} else	{
			std::shared_ptr<std::vector<FileLineCol>> userErrMsgList = search->second;
			userErrMsgList->push_back (fileLineCol);
		}
		ret_code = OK;


	} else if (INTERNAL_ERROR == msgType)	{
		FileLineCol fileLineCol(msg.getUserSrcFileName(), msg.getUserSrcLineNum(), msg.getUserSrcColPos());
		fileLineCol.insertPos = absoluteInsertPos++;

		auto search = internalErrorMessages.find(msg.getUserMsgFld());
		if (search == internalErrorMessages.end())	{
			std::shared_ptr<std::vector<FileLineCol>> flcList = std::make_shared <std::vector<FileLineCol>> ();
			flcList->push_back (fileLineCol);
			internalErrorMessages.insert ( std::pair {msg.getUserMsgFld(), flcList} );

		} else	{
			std::shared_ptr<std::vector<FileLineCol>> internalErrList = search->second;
			internalErrList->push_back (fileLineCol);
		}
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
