int8 sum = 0;

for (int8 idx = 0; idx < 10; idx++)
  sum += idx;



/* The sum of sequential (or consecutive) numbers can be calculated using the formula: 
 * Sum = n/2 ×(first number+last number)
 * , where n is the total count of numbers. 
 * For example, to find the sum of numbers from 1 to 10, you would use n=10
 * , first number = 1, and last number = 10, resulting in a sum of 55.
 */
 
int16 first = 21, last = 34;
/*
double expectedSeqSum = (last - first + 1)/2 * (first + last);
INTERNAL ERROR: Failed to convert variable [expectedSeqSum] of type DOUBLE_TKN to INT16_TKN(I)->[385] in for_loop_test.c on|near SRC_OPR8R_TKN(U)->[=] on line 17 column 23 GeneralParser.cpp:687:0

*/

int32 expectedSeqSum = (last - first + 1)/2 * (first + last);
int32 actualSum = 0;

for (int16 nextNum = first; nextNum <= last; nextNum++) {
  actualSum += nextNum;
}


