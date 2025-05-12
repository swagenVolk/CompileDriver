int16 signed_result;

// TODO: print_line doesn't handle \n in the string parameter. Potential future improvement to handle
// escape sequences. Will have to decide if it's worth the effort or not.
print_line ("");
print_line ("********** SIGNED INTEGER VARIABLE EXAMPLES USING print_line() and str() **********");

signed_result = (1 + 1) * 2;
print_line ("print_line output: signed_result = " + str(signed_result) + ";");

signed_result = 2 * 2 + 1;
print_line ("print_line output: signed_result = " + str(signed_result) + ";");

signed_result = 4 + 3;
print_line ("print_line output: signed_result = " + str(signed_result) + ";");

signed_result = 3 + 2;
print_line ("print_line output: signed_result = " + str(signed_result) + ";");

signed_result = 2 + 1;
print_line ("print_line output: signed_result = " + str(signed_result) + ";");

print_line ("");
print_line ("********** UNSIGNED INTEGER VARIABLE EXAMPLES USING print_line() and str() **********");

uint32 unsigned_result;
unsigned_result = (1 + 1) * 2;
print_line ("print_line output: unsigned_result = " + str(unsigned_result) + ";");

unsigned_result = 2 * 2 + 1;
print_line ("print_line output: unsigned_result = " + str(unsigned_result) + ";");

unsigned_result = 4 + 3;
print_line ("print_line output: unsigned_result = " + str(unsigned_result) + ";");

unsigned_result = 3 + 2;
print_line ("print_line output: unsigned_result = " + str(unsigned_result) + ";");

unsigned_result = 2 + 1;
print_line ("print_line output: unsigned_result = " + str(unsigned_result) + ";");


print_line ("");
print_line ("********** SIGNED INTEGER EXPRESSION EXAMPLES USING print_line() and str() **********");

print_line ("Expression passed to str: (1 + 1) * 2;       Resolved to: " + str((1 + 1) * 2) + ";");
print_line ("Expression passed to str: 2 * 2 + 1;         Resolved to: " + str(2 * 2 + 1) + ";");
print_line ("Expression passed to str: 4 + 3;             Resolved to: " + str(4 + 3) + ";");
print_line ("Expression passed to str: 3 + 2;             Resolved to: " + str(3 + 2) + ";");
print_line ("Expression passed to str: 2 + 1;             Resolved to: " + str(2 + 1) + ";");

print_line ("");
print_line ("********** UNSIGNED INTEGER EXPRESSION EXAMPLES USING print_line() and str() **********");

print_line ("Expression passed to str: (0x1 + 0x1) * 0x2; Resolved to: " + str((0x1 + 0x1) * 0x2) + ";");
print_line ("Expression passed to str: 0x2 * 0x2 + 0x1;   Resolved to: " + str(0x2 * 0x2 + 0x1) + ";");
print_line ("Expression passed to str: 0x4 + 0x3;         Resolved to: " + str(0x4 + 0x3) + ";");
print_line ("Expression passed to str: 0x3 + 0x2;         Resolved to: " + str(0x3 + 0x2) + ";");
print_line ("Expression passed to str: 0x2 + 0x1;         Resolved to: " + str(0x2 + 0x1) + ";");
