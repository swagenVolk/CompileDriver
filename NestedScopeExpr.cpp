/*
 * NestedScopeExpr.cpp
 *
 * Used to hold an encapsulated sub-expression (e.g. inside a pair of parentheses)
 * Instances of this class will be held on a stack that represents the different
 * scope levels inside an expression as nested parentheses are discoverd.
 *
 *  Created on: Jun 11, 2024
 *      Author: Mike Volk
 */

#include "NestedScopeExpr.h"

NestedScopeExpr::NestedScopeExpr() {
  // Attach this shared_ptr to an already existing shared_ptr of type ExprTreeNode
  myParentScopener = NULL;
  ternary2ndCnt = 0;
  scopedKids.clear();

}


NestedScopeExpr::NestedScopeExpr(std::shared_ptr<ExprTreeNode> openParenPtr) {
  // Attach this shared_ptr to an already existing shared_ptr of type ExprTreeNode
  myParentScopener = openParenPtr;
  ternary2ndCnt = 0;
  scopedKids.clear();

}

NestedScopeExpr::~NestedScopeExpr() {
}
