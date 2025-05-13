/*
 * UserMessages.h
 *
 *  Created on: Nov 21, 2024
 *      Author: Mike Volk
 */

#ifndef USERMESSAGES_H_
#define USERMESSAGES_H_

#include <map>
#include <memory>
#include <iostream>
#include <string>
#include <vector>
#include "FileLineCol.h"
#include "InfoWarnError.h"
#include <cassert>

class UserMessages {
public:
  UserMessages();
  virtual ~UserMessages();
  void reset();

  int logMsg (InfoWarnError & msg);
  int logMsg (info_warn_error_type msgType, std::wstring userMsg, std::wstring srcFileName, int lineNumber, int columnPos);
  void getUserErrorCnt (int & numUnique, int & numTotal);
  void getInternalErrorCnt (int & numUnique, int & numTotal);
  void showMessagesByGroup ();
  void showMessagesByInsertOrder (bool isOrderAscending);
  bool isExistsInternalError (std::wstring fileName, int lineNum);

private:
  std::map<std::wstring, std::shared_ptr<std::vector<FileLineCol>>> infoMessages;
  std::map<std::wstring, std::shared_ptr<std::vector<FileLineCol>>> warningMessages;
  std::map<std::wstring, std::shared_ptr<std::vector<FileLineCol>>> userErrorMessages;
  std::map<std::wstring, std::shared_ptr<std::vector<FileLineCol>>> internalErrorMessages;

  void insertNoDupes (std::map<std::wstring, std::shared_ptr<std::vector<FileLineCol>>> & messagesHolder
  , InfoWarnError & msg, FileLineCol newFileLineCol);

  std::pair <int, int> getUniqueTotalMsgCnt (std::map<std::wstring, std::shared_ptr<std::vector<FileLineCol>>> & messagesHolder);
  void displayMessagesInHolder (std::map<std::wstring, std::shared_ptr<std::vector<FileLineCol>>> & messagesHolder);
  void putHolderMsgsInOrder (std::map<std::wstring, std::shared_ptr<std::vector<FileLineCol>>> & messagesHolder
      , std::vector<std::pair <std::wstring, FileLineCol>> & orderedMsgs
      , std::vector<std::wstring> & orderedMsgTypes
      , std::wstring msgTypeStr);

  int absoluteInsertPos;
};

#endif /* USERMESSAGES_H_ */
