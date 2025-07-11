/*
 * InterpretedFileWriter.h
 *
 *  Created on: Oct 14, 2024
 *      Author: Mike Volk
 */

#ifndef INTERPRETEDFILEWRITER_H_
#define INTERPRETEDFILEWRITER_H_

#include <string>
#include <fstream>
#include <memory>
#include "ExprTreeNode.h"
#include "Utilities.h"
#include "CompileExecTerms.h"
#include "UserMessages.h"

class InterpretedFileWriter {
public:
  InterpretedFileWriter(std::string output_file_name, CompileExecTerms & inExecTerms, std::shared_ptr<UserMessages> userMessages);
  virtual ~InterpretedFileWriter();
  int writeFlatExprToFile(std::vector<Token> & flatExprTknLists, bool isIllustrative);

  // TODO: Is making these "public" legit?
  int writeFlexLenOpCode (uint8_t op_code);
  int writeObjectLen (uint32_t objStartPos);
  int writeRawUnsigned (uint64_t  payload, int payloadBitSize);
  int writeString (uint8_t op_code, std::wstring tokenStr);
  int write_user_var (std::wstring tokenStr, bool is_array);
  uint32_t getWriteFilePos ();

private:
  std::wstring thisSrcFile;
  std::wstring outFileName;
  Utilities util;
  CompileExecTerms * execTerms;
  std::ofstream outputStream;
  std::shared_ptr<UserMessages> userMessages;

  int writeAtomicOpCode (uint8_t op_code);
  int write8BitOpCode (uint8_t op_code, uint8_t payload);
  int write16BitOpCode (uint8_t op_code, uint16_t payload);
  int write32BitOpCode (uint8_t op_code, uint32_t payload);
  int write64BitOpCode (uint8_t op_code, uint64_t payload);
  int writeRawString (std::wstring tokenStr);
  int writeToken (Token token);
  
};

#endif /* INTERPRETEDFILEWRITER_H_ */
