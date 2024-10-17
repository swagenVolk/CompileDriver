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

NestedScopeExpr::NestedScopeExpr(ExprTreeNode * openParenPtr) {
	// TODO Auto-generated constructor stub
	myParentScopener = openParenPtr;
	ternary2ndCnt = 0;

}

NestedScopeExpr::~NestedScopeExpr() {
	// TODO Auto-generated destructor stub
}
