/*
 * InterpretedFileWriter.cpp
 *
 *  Created on: Oct 14, 2024
 *      Author: Mike Volk
 *
 * Utility class used by the Compiler to write objects out to the Interpreted file.
 *
 */

#include "InterpretedFileWriter.h"
#include "InfoWarnError.h"
#include "OpCodes.h"
#include "Token.h"
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

InterpretedFileWriter::InterpretedFileWriter(std::string output_file_name, CompileExecTerms & inExecTerms
    , std::shared_ptr<UserMessages> userMessages)
  : outputStream (output_file_name, outputStream.binary | outputStream.out)
{
  execTerms = & inExecTerms;
  this->userMessages = userMessages;

  // TODO: Are these asserts even necessary when the & operator is used in parameter list?
  assert (execTerms != NULL);
  thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");

  if (!outputStream.is_open()) {
    std::cout << "ERROR: Failed to open output file " << output_file_name << std::endl;
  }

  assert (outputStream.is_open());

}

InterpretedFileWriter::~InterpretedFileWriter() {
  if (outputStream.is_open())
    outputStream.close();
}

/* ****************************************************************************
 * Tree that represents an expression has already been flattened.  This fxn
 * just needs to write the Token stream out to the interpreted file as a
 * flexible length object.
 * ***************************************************************************/
int InterpretedFileWriter::writeFlatExprToFile (std::vector<Token> & flatExprTknList, bool isIllustrative)  {
  int ret_code = GENERAL_FAILURE;
  int idx;
  bool isFailed = false;

  if (flatExprTknList.empty())  {
    userMessages->logMsg (INTERNAL_ERROR, L"Flattened expression list is EMPTY!", thisSrcFile, __LINE__, 0);

  } else  {
    // Make sure we're at END of our output file
    outputStream.seekp(0, std::fstream::end);

    uint32_t startFilePos = outputStream.tellp();

    uint32_t length_pos = writeFlexLenOpCode (EXPRESSION_OPCODE);
    if (0 != length_pos)  {
      // Save off the position where the expression's total length is stored and
      // write 0s to it. It will get filled in later when writing the entire expression out has
      // been completed.
      for (idx = 0; idx < flatExprTknList.size() && !isFailed; idx++) {
        if (OK != writeToken(flatExprTknList[idx])) {
          isFailed = true;
          userMessages->logMsg (INTERNAL_ERROR, L"Failure writing Token in flat expression list out to interpreted file"
            , thisSrcFile, __LINE__, 0);
        }
      }

      if (!isFailed)
        ret_code = writeObjectLen (startFilePos);

      if (OK == ret_code && isIllustrative) {
        int caretPos;
        std::wcout << L"\nParse tree flattened and written out to interpreted file" << std::endl;
        std::wcout << util.getTokenListStr(flatExprTknList, 0, caretPos) << std::endl;
      }

    }
  }

  return (ret_code);
}

/* ****************************************************************************
 * Called after an entire object has been written out and the length now needs
 * to be filled in.  This fxn assumes that the position for the output interpreted
 * file is correct and at then end of the current object.
 * ***************************************************************************/
int InterpretedFileWriter::writeObjectLen (uint32_t objStartPos)  {
  int ret_code = GENERAL_FAILURE;
  bool isFailed = false;


  // Need fill in now known length of this expression
  uint32_t currFilePos = outputStream.tellp();
  uint32_t exprLen = (currFilePos - objStartPos);

  assert (exprLen > 0);

  // Write the current object's length
  outputStream.seekp(objStartPos + OPCODE_NUM_BYTES, std::fstream::beg);
  if (OK != writeRawUnsigned(exprLen, NUM_BITS_IN_DWORD))
    isFailed = true;

  // Now reset file pointer to pos at fxn begin, after the object was entirely written out
  outputStream.seekp(0, std::fstream::end);

  if (currFilePos == outputStream.tellp() && !isFailed)
    ret_code = OK;

  return (ret_code);
}


/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::writeAtomicOpCode (uint8_t op_code)  {
  int ret_code = GENERAL_FAILURE;

  if (op_code >= ATOMIC_OPCODE_RANGE_BEGIN && op_code <= ATOMIC_OPCODE_RANGE_END) {
    if (op_code <= LAST_VALID_OPR8R_OPCODE) {
      ret_code = writeRawUnsigned (op_code, NUM_BITS_IN_BYTE);

    } else if (op_code >= FIRST_VALID_DATA_TYPE_OPCODE && op_code <= LAST_VALID_DATA_TYPE_OPCODE) {
      ret_code = writeRawUnsigned (op_code, NUM_BITS_IN_BYTE);
    }
  }

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::writeFlexLenOpCode (uint8_t op_code) {
  int lengthPos = 0;
  int tmpLenPos = 0;
  uint32_t tempLen = 0x0;

  if (op_code >= FIRST_VALID_FLEX_LEN_OPCODE && op_code <= LAST_VALID_FLEX_LEN_OPCODE)  {
    if (OK == writeRawUnsigned (op_code, NUM_BITS_IN_BYTE)) {
      tmpLenPos = outputStream.tellp();
      if (OK == writeRawUnsigned (tempLen, NUM_BITS_IN_DWORD))
      lengthPos = tmpLenPos;
    }
  }

  return (lengthPos);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::write8BitOpCode (uint8_t op_code, uint8_t  payload)  {
  int ret_code = GENERAL_FAILURE;

  if (op_code >= FIXED_OPCODE_RANGE_BEGIN && op_code <= FIXED_OPCODE_RANGE_END) {
    if (op_code == UINT8_OPCODE || op_code == INT8_OPCODE)  {
      // Write op_code followed by payload out to file
      if (OK == writeRawUnsigned (op_code, NUM_BITS_IN_BYTE)
          && OK == writeRawUnsigned (payload, NUM_BITS_IN_BYTE))
        ret_code = OK;
    }
  }

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::write16BitOpCode (uint8_t op_code, uint16_t  payload)  {
  int ret_code = GENERAL_FAILURE;

  if (op_code >= FIXED_OPCODE_RANGE_BEGIN && op_code <= FIXED_OPCODE_RANGE_END) {
    if (op_code == UINT16_OPCODE || op_code == INT16_OPCODE)  {
      // Write op_code followed by payload out to file
      if (OK == writeRawUnsigned (op_code, NUM_BITS_IN_BYTE)
          && OK == writeRawUnsigned (payload, NUM_BITS_IN_WORD))
        ret_code = OK;
    }
  }

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::write32BitOpCode (uint8_t op_code, uint32_t  payload)  {
  int ret_code = GENERAL_FAILURE;

  if (op_code >= FIXED_OPCODE_RANGE_BEGIN && op_code <= FIXED_OPCODE_RANGE_END) {
    if (op_code == UINT32_OPCODE || op_code == INT32_OPCODE)  {
      // Write op_code followed by payload out to file
      if (OK == writeRawUnsigned (op_code, NUM_BITS_IN_BYTE)
          && OK == writeRawUnsigned(payload, NUM_BITS_IN_DWORD))
        ret_code = OK;
    }
  }

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::write64BitOpCode (uint8_t op_code, uint64_t  payload)  {
  int ret_code = GENERAL_FAILURE;

  if (op_code >= FIXED_OPCODE_RANGE_BEGIN && op_code <= FIXED_OPCODE_RANGE_END) {
    if (op_code == UINT64_OPCODE || op_code == INT64_OPCODE)  {
      // Write op_code followed by payload out to file
      if (OK == writeRawUnsigned (op_code, NUM_BITS_IN_BYTE)
          && OK == writeRawUnsigned (payload, NUM_BITS_IN_QWORD))
        ret_code = OK;
    }
  }

  return (ret_code);
}

/* ****************************************************************************
 * TODO: Any kind of check for success?
 * ***************************************************************************/
int InterpretedFileWriter::writeRawUnsigned (uint64_t  payload, int payloadBitSize) {
  int ret_code = GENERAL_FAILURE;
  uint64_t shift_mask = 0xFF << (payloadBitSize - NUM_BITS_IN_BYTE);
  uint64_t maskedQword;
  uint8_t nextByte;

  assert (payloadBitSize == NUM_BITS_IN_BYTE || payloadBitSize == NUM_BITS_IN_WORD || payloadBitSize == NUM_BITS_IN_DWORD || payloadBitSize == NUM_BITS_IN_QWORD);

  // Account for endian-ness 
  for (int idx = payloadBitSize/NUM_BITS_IN_BYTE; idx > 0; idx--) {
    maskedQword = (payload & shift_mask);
    maskedQword >>= ((idx - 1) * NUM_BITS_IN_BYTE);
    nextByte = maskedQword;
    outputStream.write(reinterpret_cast<char*>(&nextByte), 1);
    shift_mask >>= NUM_BITS_IN_BYTE;
  }
  ret_code = OK;

  return (ret_code);
}

/* ****************************************************************************
 * TODO: Any kind of check for success?
 * ***************************************************************************/
int InterpretedFileWriter::writeString (uint8_t op_code, std::wstring tokenStr) {
  int ret_code = GENERAL_FAILURE;

  uint32_t startFilePos = outputStream.tellp();

  uint32_t length_pos = writeFlexLenOpCode (op_code);
  if (0 != length_pos)  {
    // Save off the position where the expression's total length is stored and
    // write 0s to it. It will get filled in later when writing the entire expression out has
    // been completed.

    if (OK == writeRawString (tokenStr))  {
      ret_code = writeObjectLen (startFilePos);
    }
  }

  return (ret_code);
}

/* ****************************************************************************
 * TODO: Any kind of check for success?
 * USER_VAR_OPCODE  0x61  [op_code][total_length][STRING_OPCODE string] for scalar variable
 *                        [op_code][total_length][STRING_OPCODE string] [string|integer]+ for array variable
 * ***************************************************************************/
 int InterpretedFileWriter::write_user_var (std::wstring tokenStr, bool is_array) {
  int ret_code = GENERAL_FAILURE;
  bool is_failed = false;

  uint32_t startFilePos = outputStream.tellp();

  uint32_t length_pos = writeFlexLenOpCode (USER_VAR_OPCODE);
  if (0 != length_pos)  {
    // Save off the position where the expression's total length is stored and
    // write 0s to it. It will get filled in later when writing the entire expression out has
    // been completed.
    if (OK == writeString(STRING_OPCODE, tokenStr))  {
      if (is_array) {

      }

      if (!is_failed) {
        ret_code = writeObjectLen (startFilePos);
      }
    }
  }

  return (ret_code);
}

/* ****************************************************************************
 * TODO: Any kind of check for success?
 * ***************************************************************************/
int InterpretedFileWriter::writeRawString (std::wstring tokenStr) {
  int ret_code = GENERAL_FAILURE;
  bool isFailed = false;

  if (!tokenStr.empty())  {
    int numBytes = tokenStr.size() * 2;
    const wchar_t * strBffr = tokenStr.data();
    uint16_t nxtWord;
    int idx;

    for (idx = 0; idx < tokenStr.size() && !isFailed; idx++)  {
      nxtWord = strBffr[idx];
      if (OK != writeRawUnsigned (strBffr[idx], NUM_BITS_IN_WORD))
        isFailed = true;
      // outputStream.write(reinterpret_cast<char*>(&nxtWord), NUM_BYTES_IN_WORD)
    }

    if (idx == tokenStr.size() && !isFailed)
      ret_code = OK;

  } else  {
    ret_code = OK;
  }

  return (ret_code);
}


/* ****************************************************************************
 *
 * ***************************************************************************/
int InterpretedFileWriter::writeToken (Token token) {
  int ret_code = GENERAL_FAILURE;

  uint8_t tkn8Val;

  if (token.tkn_type == SRC_OPR8R_TKN && execTerms->get_statement_ender() == token._string)
    ret_code = OK;

  else {
    switch(token.tkn_type)  {
    case USER_WORD_TKN :
      // TODO: Need to handle is_array == true case eventually
      ret_code = write_user_var(token._string, false);
      break;
    case STRING_TKN :
      ret_code = writeString (STRING_OPCODE, token._string);
      break;
    case DATETIME_TKN :
      if (OK == writeAtomicOpCode(DATETIME_OPCODE))
        ret_code = writeRawString(token._string);
      break;
    case BOOL_TKN :
      if (OK == writeRawUnsigned (BOOL_DATA_OPCODE, NUM_BITS_IN_BYTE))
        ret_code = writeRawUnsigned (token._unsigned > 0 ? 1 : 0, NUM_BITS_IN_BYTE);
      break;
    case UINT8_TKN :
      tkn8Val = token._unsigned;
      if (OK == writeRawUnsigned (UINT8_OPCODE, NUM_BITS_IN_BYTE))
        ret_code = writeRawUnsigned (tkn8Val, NUM_BITS_IN_BYTE);
      break;
    case UINT16_TKN :
      if (OK == writeRawUnsigned (UINT16_OPCODE, NUM_BITS_IN_BYTE))
        ret_code = writeRawUnsigned (token._unsigned, NUM_BITS_IN_WORD);
      break;
    case UINT32_TKN :
      if (OK == writeRawUnsigned (UINT32_OPCODE, NUM_BITS_IN_BYTE))
        ret_code = writeRawUnsigned (token._unsigned, NUM_BITS_IN_DWORD);
      break;
    case UINT64_TKN :
      if (OK == writeRawUnsigned (UINT64_OPCODE, NUM_BITS_IN_BYTE))
        ret_code = writeRawUnsigned (token._unsigned, NUM_BITS_IN_QWORD);
      break;
    case INT8_TKN :
      if (OK == writeRawUnsigned (INT8_OPCODE, NUM_BITS_IN_BYTE))
        ret_code = writeRawUnsigned (token._signed, NUM_BITS_IN_BYTE);
      break;
    case INT16_TKN :
      if (OK == writeRawUnsigned (INT16_OPCODE, NUM_BITS_IN_BYTE))
        ret_code = writeRawUnsigned (token._signed, NUM_BITS_IN_WORD);
      break;
    case INT32_TKN :
      if (OK == writeRawUnsigned (INT32_OPCODE, NUM_BITS_IN_BYTE))
        ret_code = writeRawUnsigned (token._signed, NUM_BITS_IN_DWORD);
      break;
    case INT64_TKN :
      if (OK == writeRawUnsigned (INT64_OPCODE, NUM_BITS_IN_BYTE))
        ret_code = writeRawUnsigned (token._signed, NUM_BITS_IN_QWORD);
      break;
    case DOUBLE_TKN :
        ret_code = writeString (DOUBLE_OPCODE, token._string);
      break;
    case EXEC_OPR8R_TKN :
      ret_code = writeRawUnsigned (token._unsigned, NUM_BITS_IN_BYTE);
      break;
    case SYSTEM_CALL_TKN :
      ret_code = writeString (SYSTEM_CALL_OPCODE, token._string);
      break;
      
    case SPR8R_TKN :
    case SRC_OPR8R_TKN :
    default:
      break;

    }
  }

  return (ret_code);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
uint32_t InterpretedFileWriter::getWriteFilePos ()  {

  uint32_t currFilePos = outputStream.tellp();
  return (currFilePos);

}

