int8 one = 1, two = 2, three = 3, four = 4, five = 5, six = 6, seven = 7, eight = 8, nine = 9, ten = 10;

int8 expectBlock10 = -1;

// [if][else if][else] blocks with fairly simple comparison expressions

if (one >= two)	{
	expectBlock10 = 1;
	
} else if (two >= three) 
	// No curly bracket on purpose; poor coding style. Weird but legal
	expectBlock10 = 2;

else if (three >= four)	
	expectBlock10 = 3;

else if (four >= five)	{
	expectBlock10 = 4;

}	else if (five >= six)
	expectBlock10 = 5;

else if (six >= seven)	{
	expectBlock10 = 6;
	
} else if (seven >= eight)	{
	expectBlock10 = 7;
	
} else if (eight >= nine)	{
	expectBlock10 = 8;
	
} else if (nine >= ten)	{
	expectBlock10 = 9;

} else if (three * four > ten + one)	{
	expectBlock10 = 10;

} else {
	expectBlock10 = 11;
}

int16 eleven = 11, twelve = 12, thirteen = 13, fourteen = 14, fifteen = 15, sixteen = 16, seventeen = 17, eighteen = 18, nineteen = 19, twenty = 20;

int16 expectBlock2 = -1;

if (one + two + three * four > 15)	{
	expectBlock2 = 1;

} else if (two * three + four * five == twenty + six)	{
	expectBlock2 = 2;

}


int16 expectBlock1 = -1;

if (one + two + three * four == 15)	{
	expectBlock1 = 1;

} else if (two * three + four * five == twenty + six)	{
	expectBlock1 = 2;

} else {
	expectBlock1 = 3;
}
