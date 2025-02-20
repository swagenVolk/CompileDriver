# CompileDriver
Abridged C compiler and interpreter.  The compiler produces byte codes from user source and the intepreter consumes and executes them. This is a C++ practice project while I search for my next job. I previously wrote a similar compiler and interpreter that enabled customers to extend my tools without their needing to understand the tools' underlying mechanics and also kept their extensions local to their projects. This implementation adds support for the prefix and postfix operators [++,--] as well as the ternary operators [?,:].

The current status as of 2025-02-20 is that expressions, variable declarations and [if,else if,else] blocks are functional. Other blocks such as [for,while] are not *currently* supported. I worked on expressions first, because those blocks require expression evaluation, with the exception of the [else] block. I plan on adding support for user defined functions, and possibly multiple files.  I don't intend to add support for the [switch] statement or structs. My focus is code practice on an open ended challenge. Please keep that in mind when you see in the code that I also wrote my own file parser. I understand that if this were any kind of commercial project or something beyond a learning exercise, I would use what's already available and not "re-invent the wheel".

Please see the Wiki for more comprehensive and up-to-date info.

