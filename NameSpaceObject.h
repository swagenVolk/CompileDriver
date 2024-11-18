/*
 * NameSpaceObject.h
 *
 *  Created on: Jun 6, 2024
 *      Author: Mike Volk
 *
 *	TODO: Not used anywhere yet
 */

#ifndef NAMESPACEOBJECT_H_
#define NAMESPACEOBJECT_H_

#include <string>
#include <list>
#include "ExprTreeNode.h"

class NameSpaceObject {
public:
	NameSpaceObject();
	virtual ~NameSpaceObject();

	NameSpaceObject * myParent;
	// Children can be: IF, ELSE IF and ELSE blocks; WHILE, FOR, DO WHILE blocks;
	// Fxn call definitions, expressions, variable declarations
	std::list<NameSpaceObject *> myChildren;

	// IF, ELSE IF, ELSE, WHILE, FOR, DO WHILE blocks all have a conditional expression
	// Fxn call definitions may have [0-MANY] parameters (conditionals?) passed
	std::list<std::shared_ptr<ExprTreeNode>> myConditions;
};

#endif /* NAMESPACEOBJECT_H_ */
