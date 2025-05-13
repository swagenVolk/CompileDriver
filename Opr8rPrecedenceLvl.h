/*
 * Opr8rPrecedenceLvl.h
 *
 *  Created on: Oct 4, 2024
 *      Author: Mike Volk
 */

#ifndef OPR8RPRECEDENCELVL_H_
#define OPR8RPRECEDENCELVL_H_

#include <list>
#include "Operator.h"

class Opr8rPrecedenceLvl {
public:
  Opr8rPrecedenceLvl();
  virtual ~Opr8rPrecedenceLvl();

  std::list<Operator> opr8rs;
};

#endif /* OPR8RPRECEDENCELVL_H_ */
