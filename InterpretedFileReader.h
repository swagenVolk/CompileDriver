/*
 * InterpretedFileReader.h
 *
 *  Created on: Oct 24, 2024
 *      Author: Mike Volk
 */

#ifndef INTERPRETEDFILEREADER_H_
#define INTERPRETEDFILEREADER_H_

#include <string>
#include <istream>
#include <iostream>
#include <fstream>
#include <memory>
#include "common.h"
#include "ExprTreeNode.h"
#include "Utilities.h"
#include "OpCodes.h"
#include "Operator.h"
#include "CompileExecTerms.h"
#include "Token.h"


class InterpretedFileReader {
public:
  InterpretedFileReader ();
  InterpretedFileReader(std::string input_file_name, CompileExecTerms & inExecTerms);
  virtual ~InterpretedFileReader();
  int readExprIntoList (std::vector<Token> & exprTknStream);
  int readNextByte (uint8_t & nextByte);
  int peekNextByte (uint8_t & nextByte);
  int readNextDword (uint32_t & nextDword);
  uint32_t getPos ();
  int setPos (uint32_t newFilePos);
  bool isEOF ();

  // TODO: Would the fxns below be more generic if exprTknStream was excluded?  Probably.....
  int resolveOpr8r (uint8_t op_code, Token & nxtTkn);
  int readFixedRange (uint8_t op_code, Token & nxtTkn);
  int readString (uint8_t op_code, Token & nxtTkn);
  int readUserVar (Token & nxtTkn);


protected:
  int readNextWord (uint16_t & nextWord);
  int readNextQword (uint64_t & nextQword);
  int readRawUnsigned (uint64_t & payload, int payloadByteSize);

private:
  std::wstring thisSrcFile;
  std::wstring inFileName;
  std::ifstream inputStream;
  Utilities util;
  CompileExecTerms * execTerms;

};

#endif /* INTERPRETEDFILEREADER_H_ */
