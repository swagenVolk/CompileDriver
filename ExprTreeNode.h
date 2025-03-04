/*
 * ExprTreeNode.h
 *
 *  Created on: Jun 5, 2024
 *      Author: Mike Volk
 */

#ifndef EXPRTREENODE_H_
#define EXPRTREENODE_H_

#include "Token.h"
#include <memory>
#include <iostream>

class ExprTreeNode {
public:
	ExprTreeNode(std::shared_ptr<Token> startOpr8rTkn);
	virtual ~ExprTreeNode();

  std::shared_ptr<Token> originalTkn;
  std::shared_ptr<ExprTreeNode> scopenedBy;
  std::shared_ptr<ExprTreeNode> _1stChild;		// Left operand for a BINARY, POSTFIX or the TERNARY FALSE* branch
  std::shared_ptr<ExprTreeNode> _2ndChild;		// Right operand for a BINARY, PREFIX or the TERNARY TRUE* branch
  														                // Ordering is opposite initial expectations to account for tree recursively 
                                              // flattened in [Operand1][Operand2][OPR8R] order
  std::shared_ptr<ExprTreeNode> treeParent;                                              

};

typedef std::vector<std::shared_ptr<ExprTreeNode>> ExprTreeNodePtrVector;

#endif /* EXPRTREENODE_H_ */
