/*
 * Operator.h
 *
 *  Created on: Jun 14, 2024
 *      Author: Mike Volk
 */

#ifndef OPERATOR_H_
#define OPERATOR_H_

#include <string>
#include <cassert>
#include <cstdint>

// Used by type_mask
#define UNARY             0x1
#define BINARY            0x2
#define TERNARY_1ST       0x4
#define TERNARY_2ND       0x8
#define PREFIX            0x10
#define POSTFIX           0x20
#define STATEMENT_ENDER   0x40

// Used by valid_usage
#define USR_SRC           0x1
#define GNR8D_SRC         0x2

class Operator {
public:
  Operator();
  Operator(std::wstring in_symbol, uint8_t in_type_mask, uint8_t in_valid_usage, int in_num_src_operands, int in_num_exec_operands
    , uint8_t in_op_code, std::wstring inDescr);
  virtual ~Operator();
  std::wstring symbol;
  uint8_t type_mask;
  uint8_t valid_usage;
  int numReqSrcOperands;
  int numReqExecOperands;
  uint8_t op_code;
  std::wstring description;

  Operator& operator= (const Operator& opr8r);
};

#endif /* OPERATOR_H_ */
