/*
 * TokenCompareResult.h
 *
 *  Created on: May 6, 2024
 *      Author: Mike Volk
 */

#ifndef TOKENCOMPARERESULT_H_
#define TOKENCOMPARERESULT_H_

enum comparisonResults { isTrue, isFalse, compareFailed };

class TokenCompareResult {
public:
	TokenCompareResult();
	virtual ~TokenCompareResult();

	int gr8rThan;
	int gr8rEquals;
	int lessThan;
	int lessEquals;
	int equals;
};

#endif /* TOKENCOMPARERESULT_H_ */
