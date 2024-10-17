/*
 * NameSpaceObject.cpp
 * TODO: This class not currently used
 *  Created on: Jun 6, 2024
 *      Author: Mike Volk
 */

#include "NameSpaceObject.h"

NameSpaceObject::NameSpaceObject() {
	// TODO Auto-generated constructor stub
	myParent = NULL;

}

NameSpaceObject::~NameSpaceObject() {
	// TODO Auto-generated destructor stub

	while (!myChildren.empty())	{
		delete (myChildren.front());
	}

	while (!myConditions.empty())	{
		delete (myConditions.front());
	}
}

