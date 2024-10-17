/*
 * TokenCompareResult.cpp
 * Holds equivalence info [<,<=,==,>,>=] when 2 Tokens are compared to one another
 *  Created on: May 6, 2024
 *      Author: Mike Volk
 */

#include "TokenCompareResult.h"

TokenCompareResult::TokenCompareResult() {
	// TODO Auto-generated constructor stub
	this->gr8rThan = compareFailed;
	this->gr8rEquals = compareFailed;
	this->lessThan = compareFailed;
	this->lessEquals = compareFailed;
	this->equals = compareFailed;

}

TokenCompareResult::~TokenCompareResult() {
	// TODO Auto-generated destructor stub
}

