/*
 * TokenCompareResult.cpp
 * Holds equivalence info [<,<=,==,>,>=] when 2 Tokens are compared to one another
 *  Created on: May 6, 2024
 *      Author: Mike Volk
 */

#include "TokenCompareResult.h"

TokenCompareResult::TokenCompareResult() {
  gr8rThan = compareFailed;
  gr8rEquals = compareFailed;
  lessThan = compareFailed;
  lessEquals = compareFailed;
  equals = compareFailed;

}

TokenCompareResult::~TokenCompareResult() {
}

