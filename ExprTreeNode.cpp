/*
 * ExprTreeNode.cpp
 * This class holds a Token that originally came from the user's source file.  It has multiple child pointers that allow for linking
 * multiple ExprTreeNode instances together in hierarchical tree structure.
 *
 *  Created on: Jun 5, 2024
 *      Author: Mike Volk
 */

#include "ExprTreeNode.h"
#include <memory>

ExprTreeNode::ExprTreeNode(std::shared_ptr<Token> startOpr8rTkn) {
	// Attach this shared_ptr to an already existing shared_ptr of type Token
	originalTkn = startOpr8rTkn;
  scopenedBy = NULL;
  _1stChild = NULL;
  _2ndChild = NULL;
	treeParent = NULL;
}

ExprTreeNode::~ExprTreeNode() {
	// Clean up our allocated memory
	originalTkn.reset();
	_1stChild.reset();
	_2ndChild.reset();
}
