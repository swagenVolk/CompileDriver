/*
 * ExprTreeNode.h
 *
 *  Created on: Jun 5, 2024
 *      Author: Mike Volk
 */

#ifndef EXPRTREENODE_H_
#define EXPRTREENODE_H_

#include "Token.h"

class ExprTreeNode {
public:
	ExprTreeNode(Token * startTkn);
	virtual ~ExprTreeNode();

  Token * originalTkn;

	// TODO: Struggled to avoid using a pointer here
  Token * resultTkn;					// Storage place for bubbled-up result of resolving ExpressionTree.  Is this actually necessary for active evaluation?
  ExprTreeNode * myParent;
  ExprTreeNode * _1stChild;		// Left operand for a BINARY, POSTFIX or the conditional in a TERNARY
  ExprTreeNode * _2ndChild;		// Right operand for a BINARY, PREFIX or the true branch of a TERNARY
  ExprTreeNode * _3rdChild;		// Only used for the false branch of a TERNARY

  void showTree (ExprTreeNode * treeNode);

private:
  void buildTreeGraph (ExprTreeNode * treeNode, std::vector<std::wstring> & treeGraph, int scopeDepth, int lftRgt3rdChild);
};

typedef std::vector<ExprTreeNode*> ExprTreeNodePtrVector;

#endif /* EXPRTREENODE_H_ */
