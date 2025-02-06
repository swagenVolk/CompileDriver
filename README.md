# CompileDriver
Abridged C compiler and interpreter.  The compiler produces byte codes from user source and the intepreter consumes and executes them. The intent of this project is to practice C++ while searching for my next job.  I have written a similar compiler and interpreter previously that enabled customers to extend my tools without knowing the underlying mechanics and also keep their extensions local to their projects.  This implementation adds support for the prefix and postfix operators [++,--] as well as the ternary operators [?,:] which were not supported in the previous instantiation.

The current status as of 2025-02-03 is that expressions are functional, but other blocks such as [if,else if,else,for,while] are not *currently* supported. I worked on expressions first, because those blocks require expression evaluation, with the exception of the [else] block. I plan on adding support for user defined functions, and perhaps multiple files.  I don't intend to add support for the [switch] statement or structs. My focus is on code practice on something that is an open ended challenge that is interesting to me. Please keep that in mind when you see in the code that I also wrote my own file parser. I understand that if this were any kind of commercial project or something beyond a learning exercise, it would be much better to use what's already available and not "re-invent the wheel".

