// To keep the level of illustrative output reasonable, expressions inside variable declarations (ie on line below) are not logged
int16 one = 1, two = one + one, three = two + one, four = three + one, five = three + two, six = three * two;

int32 expect_5;

// The expression below will have illustrative logging.
expect_5 = one + two * three * four / six;
