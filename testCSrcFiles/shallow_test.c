// SIMPLE DECLARATIONS
uint8 one, two, three, four, five, six, seven;

seven = three + four;
// Should get messages about using unitialized variables in an expression
// Uninitialized variable used - USER_WORD_TKN(U)->[four]
// Uninitialized variable used - USER_WORD_TKN(U)->[three]

// SIMPLE ASSIGNMENTS
one = 1;
two = 2;
three = 3;
four = 4;
five = 5;
six = 6;
seven = 7;

// INTITIALIZATION WITH SIMPLE MATH 
uint8 init1 = 1, init2 = 2;
uint16 twenty = four * five, twentySix = twenty + six, fortyTwo = six * seven, fiftySix = seven * seven + seven;
uint16 fifty = seven * seven + init1++;
uint16 fiftyTwo = seven * seven + ++init2;

// TERNARY
int8 count = 1;
string count1 = count == 1 ? "one" : count == 2 ? "two" : "MANY";
count = 2;
string count2 = count == 1 ? "one" : count == 2 ? "two" : "MANY";
count = 3;
string count3 = count == 1 ? "one" : count == 2 ? "two" : count == 3 ? "three" : "TOO MANY";
count = 5;
string countMany;
countMany = count == 1 ? "one" : count == 2 ? "two" : count == 3 ? "three" : count == 4 ? "four" : "MANY";
int8 rzlt_12345 = one >= two ? 1 : three <= two ? 2 : three == four ? 3 : six > seven ? 4 : six > (two << two) ? 5 : 12345; 

string countLots, countHordes, countBunches = "BUNCHES", countLegions;
count = 1723;
countLots = count == 1 ? "one" : count == 2 ? "two" : count == 3 ? "three" : count == 4 ? "four" : count == 5 ? "five" : countHordes = "HORDES";

int16 altCount = 5;
altCount == 1 ? "one" : altCount == 2 ? "two" : altCount == 3 ? "three" : altCount == 4 ? "four" : altCount == 5 ? countLegions = "Legions" : countHordes = "HORDES";

// BOOLEANS
bool isTrue1 = one <= two ? true : false;
bool isTrue2 = false;
isTrue2 = (fiftyTwo > 50 || count >= 6) ? true : false;

bool isFalse1 = fiftySix <= fiftyTwo ? true : false;
bool isFalse2 = true;
isFalse2 = (count <= 6 && fiftyTwo > 50) ? true : false;

// STRINGS
string MikeWasHere = "Mike was HERE!!!!";
MikeWasHere += " But he went skipping to the Louvre";

// MATH & ORDERING
int16 expect128 = 3 * 4 / 3 + 4 << 4;
int16 expect256 = 3 * 4 / 3 + 4 << 4 + 1;

// [:] [;] - Already covered
// [!] [++] [--] [~] 
bool isTrue3 = !false;
bool isFalse3 = !true;
int16 expect1 = ~0xFFFE;
int16 expect2 = ~(0xFFFD);

int8 decExpect6 = 10;

int8 decExpect10 = decExpect6--;
int8 decExpect8 = --decExpect6;
int8 decExpect14 = (--decExpect6 + decExpect6--);
int8 decExpect12 = decExpect6-- + ++decExpect6;

// [%] [&] [&&] [*] [+] [-] [/] [?] [^] [|] [||]
// [==] [!=] [<] [<=] [>] [>=] 
// [=] [%=] [&=] [*=] [+=] [-=] [/=] [<<=] [>>=] [^=] [|=] 
int64 mathMod8 = 2056;
mathMod8 %= 64;

uint32 mathBitAnd0xDooDFa8e = 0xDEADFACE;
mathBitAnd0xDooDFa8e &= 0xF00FFabF;

int64 mathX111 = 37;
mathX111 *= 3;

int32 mathPlus3579 = 1234;
mathPlus3579 += 2345;

int16 mathSub2222 = 5678;
mathSub2222 -= 3456;

int16 mathDiv512 = 4096;
mathDiv512 /= 8;

uint32 mathLshft90210 = 0x9021; 
mathLshft90210 <<= 4;

uint32 mathRshftCAFE = 0xCAFEFACE;
mathRshftCAFE >>= 16;

uint64 mathXorFFs = 0xFEEDCAFE;
mathXorFFs ^= 0x01123501;

uint32 mathBitOr0xDEADFBDE = 0xDEADFACE;
mathBitOr0xDEADFBDE |= 0xB1E;


// [<<] [>>] 
int8 eight = 8;
int16 expect512 = two << eight;
int16 expect1024 = two << eight + 1;
int16 expect513 = (two << eight) + 1;
int16 expect32 = eight << two;
int16 expect64 = eight == expect32 >> two ? expect512 >> 3 : eight * two;


// [||] short circuit
int16 shortCircuitOr456 = 456;
bool isShortCircuitOrTrue1;

// Proper: Inline assignment enclosed in parentheses; [;] is the only OPR8R with lower 
// precedence than any assignment OPR8R [=] [+=] [-=] [*=] [/=] [%=] [<<=] [>>=] [&=] [^=] [|=] 
isShortCircuitOrTrue1 = (one * two * three + four >= seven) || (shortCircuitOr456 = 789);

// IMPROPER: 
// isShortCircuitOrTrue1 = (one * two * three + four >= seven) || shortCircuitOr456 = 789;
// USER ERROR MESSAGES: Unique messages = 1; Total messages = 1;
// Left operand of an assignment operator must be a named variable: EXEC_OPR8R_TKN(U)->[=] Assignment operation may need to be enclosed in parentheses.
// more_test.cpp:120:82


// [&&] short circuit
int16 shortCircuitAnd987 = 987;
bool isShortCircuitAndFalse1 = (one * two >= three || two * three > six || three * four < seven || four / two < one) && (three % two > 1 || (shortCircuitAnd987 = 654));
//                              ^ Evaluates FALSE; right side of [&&] OPR8R short-circuits                           ^ and bogus assignment shouldn't happen

// [?] short circuit

int8 whichIfBlock = 0;

if (shortCircuitAnd987 == 987)	{
  whichIfBlock = 1;
	
} else if (shortCircuitAnd987 == 654)	{
	whichIfBlock = 2;
	
} else {
	whichIfBlock = 3;
}

int8 witchBlock = 0;

if (shortCircuitAnd987 == 987)	{
  witchBlock = 1;
	
} else if (shortCircuitAnd987 == 654)	{
	witchBlock = 2;
	
} else {
	witchBlock = 3;
}



/* *************** <COMPILATION STAGE> ***************   
compileRetCode = 0
INTERNAL ERROR MESSAGES: Unique messages = 0; Total messages = 0;

USER ERROR MESSAGES: Unique messages = 0; Total messages = 0;

USER WARNING MESSAGES: Unique messages = 2; Total messages = 2;
Uninitialized variable used - USER_WORD_TKN(U)->[four]
more_test.cpp:4:17
Uninitialized variable used - USER_WORD_TKN(U)->[three]
more_test.cpp:4:9

USER INFO MESSAGES: Unique messages = 0; Total messages = 0;

// ********** <SCOPE LEVEL 0> ********** 
MikeWasHere             = "Mike was HERE!!!! But he went skipping to the Louvre";
count                   = 1723;
count1                  = "one";
count2                  = "two";
count3                  = "three";
countBunches            = "BUNCHES";
countHordes             = "HORDES";
countLots               = "HORDES";
countMany               = "MANY";
decExpect10             = 10;
decExpect12             = 12;
decExpect14             = 14;
decExpect6              = 6;
decExpect8              = 8;
eight                   = 8;
expect1                 = 1;
expect1024              = 1024;
expect128               = 128;
expect2                 = 2;
expect256               = 256;
expect32                = 32;
expect512               = 512;
expect513               = 513;
expect64                = 64;
fifty                   = 0x32;
fiftySix                = 0x38;
fiftyTwo                = 0x34;
five                    = 0x5;
fortyTwo                = 0x2a;
four                    = 0x4;
init1                   = 0x2;
init2                   = 0x3;
isFalse1                = false;
isFalse2                = false;
isFalse3                = false;
isShortCircuitAndFalse1 = false;
isShortCircuitOrTrue1   = true;
isTrue1                 = true;
isTrue2                 = true;
isTrue3                 = true;
mathBitAnd0xDooDFa8e    = 0xd00dfa8e;
mathBitOr0xDEADFBDE     = 0xdeadfbde;
mathDiv512              = 512;
mathLshft90210          = 0x90210;
mathMod8                = 8;
mathPlus3579            = 3579;
mathRshftCAFE           = 0xcafe;
mathSub2222             = 2222;
mathX111                = 111;
mathXorFFs              = 0xffffffff;
one                     = 0x1;
rzlt_12345              = 12345;
seven                   = 0x7;
shortCircuitAnd987      = 987;
shortCircuitOr456       = 456;
six                     = 0x6;
three                   = 0x3;
twenty                  = 0x14;
twentySix               = 0x1a;
two                     = 0x2;
// ********** </SCOPE LEVEL 0> ********** 
 *************** </COMPILATION STAGE> *************** */
/* *************** <EXECUTION STAGE> ***************   
ret_code = 0
// ********** <SCOPE LEVEL 0> ********** 
MikeWasHere             = "Mike was HERE!!!! But he went skipping to the Louvre";
count                   = 1723;
count1                  = "one";
count2                  = "two";
count3                  = "three";
countBunches            = "BUNCHES";
countHordes             = "HORDES";
countLots               = "HORDES";
countMany               = "MANY";
decExpect10             = 10;
decExpect12             = 12;
decExpect14             = 14;
decExpect6              = 6;
decExpect8              = 8;
eight                   = 8;
expect1                 = 1;
expect1024              = 1024;
expect128               = 128;
expect2                 = 2;
expect256               = 256;
expect32                = 32;
expect512               = 512;
expect513               = 513;
expect64                = 64;
fifty                   = 0x32;
fiftySix                = 0x38;
fiftyTwo                = 0x34;
five                    = 0x5;
fortyTwo                = 0x2a;
four                    = 0x4;
init1                   = 0x2;
init2                   = 0x3;
isFalse1                = false;
isFalse2                = false;
isFalse3                = false;
isShortCircuitAndFalse1 = false;
isShortCircuitOrTrue1   = true;
isTrue1                 = true;
isTrue2                 = true;
isTrue3                 = true;
mathBitAnd0xDooDFa8e    = 0xd00dfa8e;
mathBitOr0xDEADFBDE     = 0xdeadfbde;
mathDiv512              = 512;
mathLshft90210          = 0x90210;
mathMod8                = 8;
mathPlus3579            = 3579;
mathRshftCAFE           = 0xcafe;
mathSub2222             = 2222;
mathX111                = 111;
mathXorFFs              = 0xffffffff;
one                     = 0x1;
rzlt_12345              = 12345;
seven                   = 0x7;
shortCircuitAnd987      = 987;
shortCircuitOr456       = 456;
six                     = 0x6;
three                   = 0x3;
twenty                  = 0x14;
twentySix               = 0x1a;
two                     = 0x2;
// ********** </SCOPE LEVEL 0> ********** 
 *************** </EXECUTION STAGE> *************** */
