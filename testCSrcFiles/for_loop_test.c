int8 sum_66 = 0;

for (int8 idx = 0; idx < 12; idx++)
  sum_66 += idx;

int16 break_sum_155 = 0;

for (int8 odx = 11; odx < 22; odx++)  {
  break_sum_155 += odx;
  
  if (odx == 20) 
    break;
}

/* The sum of sequential (or consecutive) numbers can be calculated using the formula: 
 * Sum = n/2 ×(first number+last number)
 * , where n is the total count of numbers. 
 * For example, to find the sum of numbers from 1 to 10, you would use n=10
 * , first number = 1, and last number = 10, resulting in a sum of 55.
 */
 
int16 first = 21, last = 34;
/*
double seq_sum_plan = (last - first + 1)/2 * (first + last);
INTERNAL ERROR: Failed to convert variable [seq_sum_plan] of type DOUBLE_TKN to INT16_TKN(I)->[385] in for_loop_test.c on|near SRC_OPR8R_TKN(U)->[=] on line 17 column 23 GeneralParser.cpp:687:0

*/

int32 seq_sum_plan = (last - first + 1)/2 * (first + last);
int32 seq_sum_real = 0;

for (int16 nextNum = first; nextNum <= last; nextNum++) {
  seq_sum_real += nextNum;
}
/*
int16 sum = 0;
for (int16 start = 0; (2 * 3 + 4) == (3 * 2 + 4); 57 >= 57) {
  sum++;
}
USER ERROR: [for] loop control conditional expression is static and will lead to infinite looping: RESERVED_WORD_TKN(U)->[for] for_loop_test.c:37:1
USER ERROR: [for] loop control last expression is static and will lead to infinite looping: RESERVED_WORD_TKN(U)->[for] for_loop_test.c:37:1
*/
