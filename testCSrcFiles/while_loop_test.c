// Borrowed code from for_loop_test.c

int8 sum_66 = 0;

int8 idx = 0; 
while (idx < 12)
  sum_66 += idx++;
  
int16 break_sum_155 = 0;
int8 odx = 11;

while (odx < 22)  {
  break_sum_155 += odx;
  
  if (odx == 20) 
    break;
  
  odx++;
  
}

/* The sum of sequential (or consecutive) numbers can be calculated using the formula: 
 * Sum = n/2 ×(first number+last number)
 * , where n is the total count of numbers. 
 * For example, to find the sum of numbers from 1 to 10, you would use n=10
 * , first number = 1, and last number = 10, resulting in a sum of 55.
 */
 
int16 first = 21, last = 34;

int32 seq_sum_plan = (last - first + 1)/2 * (first + last);
int32 seq_sum_real = 0;

int16 nextNum = first;

while (nextNum <= last) {
  seq_sum_real += nextNum;
  nextNum++;
}
/*
while (1 + 2 + 3 == 3 + 2 + 1)  {
  nextNum++;
}

Compiler ret_code = 0
USER ERROR: [while] loop control conditional expression is static, which leads to infinite looping: RESERVED_WORD_TKN(U)->[while] while_loop_test.c:41:1
*/

