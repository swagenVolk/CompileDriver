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

enum branch_tuple_type_enum {
  UNKNOWN_NODE_TYPE
  ,CENTER_NODE
  ,OUTER_NODE
};

typedef branch_tuple_type_enum branchNodeDisplayType;

class ExprTreeNode {
public:
	ExprTreeNode(std::shared_ptr<Token> startOpr8rTkn);
	virtual ~ExprTreeNode();

  void initDisplaySettings();

  std::shared_ptr<Token> originalTkn;
  std::shared_ptr<ExprTreeNode> scopenedBy;
  std::shared_ptr<ExprTreeNode> _1stChild;		// Left operand for a BINARY, POSTFIX or the TERNARY FALSE* branch
  std::shared_ptr<ExprTreeNode> _2ndChild;		// Right operand for a BINARY, PREFIX or the TERNARY TRUE* branch
  														                // Ordering is opposite initial expectations to account for tree recursively 
                                              // flattened in [Operand1][Operand2][OPR8R] order
  std::shared_ptr<ExprTreeNode> treeParent;     
  
  int displayStartPos;
  int displayEndPos;
  int displayRow;
  int displayCol;
  branchNodeDisplayType nodePos;

  // TODO: Make this private?
  // For system_call or user defined fxn call, parameter_list will encapsulate the call's parameters at compile time
  std::vector <std::shared_ptr<ExprTreeNode>> parameter_list;


};

typedef std::vector<std::shared_ptr<ExprTreeNode>> ExprTreeNodePtrVector;

#endif /* EXPRTREENODE_H_ */
