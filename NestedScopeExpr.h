/*
 * NestedScopeExpr.h
 *
 *  Created on: Jun 11, 2024
 *      Author: Mike Volk
 */

#ifndef NESTEDSCOPEEXPR_H_
#define NESTEDSCOPEEXPR_H_

#include "ExprTreeNode.h"

class NestedScopeExpr {
public:
  NestedScopeExpr(std::shared_ptr<ExprTreeNode> openParenPtr);
  NestedScopeExpr();
  virtual ~NestedScopeExpr();
  ExprTreeNodePtrVector scopedKids;
  std::shared_ptr<ExprTreeNode> myParentScopener;
  int ternary2ndCnt;
};

#endif /* NESTEDSCOPEEXPR_H_ */
