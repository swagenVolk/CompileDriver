int16 one = 1, two = one + one, three = two + one, four = three + one, five = three + two, six = three * two, seven = four + three, eight = four * two, nine = three * three;

int32 result;

result = ((one + two) + three) * (four + six) + (six * three);
