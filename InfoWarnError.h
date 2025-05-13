/*
 * InfoWarnError.h
 *
 *  Created on: Nov 14, 2024
 *      Author: Mike Volk
 */

#ifndef INFOWARNERROR_H_
#define INFOWARNERROR_H_

#include "common.h"
#include <string>

#define STACK_SPR8R   L"->"


enum info_warn_error_enum {
  UNKNOWN_ERROR
  ,INFO
  ,WARNING
  ,USER_ERROR
  ,INTERNAL_ERROR

};

typedef info_warn_error_enum info_warn_error_type;

class InfoWarnError {
public:
  InfoWarnError();
  virtual ~InfoWarnError();
  void set1stInSrcStack(std::wstring srcFileName);
  bool isEmpty();
  InfoWarnError& operator= (const InfoWarnError & srcErrorInfo);

  void setUserMsg(info_warn_error_type type, std::wstring userSrcFileName, int userSrcLineNum, int userSrcColPos, std::wstring msgForUser);
  void setInternalError(std::wstring srcFileName, int srcLineNum, std::wstring msgForUser);
  void set(info_warn_error_type type, std::wstring userSrcFileName, int userSrcLineNum, int userSrcColPos, std::wstring srcFileName, int srcLineNum, std::wstring msgForUser);
  std::wstring getErrorTypeStr();
  info_warn_error_type getTypeOfError();

  std::wstring getOurSrcFileName();
  int getOurSrcLineNum();

  std::wstring getUserSrcFileName();
  int getUserSrcLineNum();
  int getUserSrcColPos();

  std::wstring getUserMsgFld();
  std::wstring getFormattedMsg();
  int get_insertedPos ();
  void set_insertedPos (int orderPos);

private:
  info_warn_error_type typeOfError;
  std::wstring ourSrcFileName;
  std::wstring userSrcFileName;
  std::wstring srcFileStack;
  int ourSrcLineNum;
  int userSrcLineNum;
  int userSrcColumnPos;
  std::wstring userMsg;
  int insertedPos;
};

#endif /* INFOWARNERROR_H_ */
