/* ****************************************************************************
 * NEEDS TESTING:
 * USER_ERROR should never cause an INTERNAL_ERROR. Maybe try using an unknown data type to re-create the issue.
 * Check that all operations are acting on initialized operands.
 * Left, center and right justified parentheses in expressions
 * Clarity of error messages
 *
 * TODO:
 * Make ILLUSTRATIVE a flag option
 * Illustrative - display a tree; make the formatting look nice
 * Illustrative mode - verbose output for instructional purposes
 * 	When reading in expression list from Interpreted file, could add file location to Tokens (maybe)
 *  Tie together expression start on both stages with interpreted file location
 * Generic ("Unique") scope objects?
 * Should ROOT scope include a checksum? If so, should others have the field also, but gets ignored?
 * Differentiate between expressions and statements?  ie include support for [,] OPR8R beyond var declaration?
 * Be clear & consistent about where type checking happens!
 * Maintain data type until assignment, then do data type conversion if necessary
 * Method to regression test lots of expressions and compare results against regular compiler
 * Use a static analyzer
 * How can I check for memory leaks|danglers?
 * Look for opportunities to clarify (ie #defines -> enums?) to make debugging easier
 * How many pointers can I replace with essentially a copy? (grep -HEn "\bnew\b" *.cpp -> [0])
 * Compile fxn calls and put into NameSpace
 *
 * FUTURE:
 * Disassembler
 * Array support
 * Directory/index for fxn calls so Interpreter doesn't have to search through the object
 * file for the location. It can do a quick(er) lookup
 *
 * RECENTLY DONE:
 * Illustrative - description of what each OPR8R does?  Allude to info early; show when OPR8R's neighbors get moved.  Add to Operator class
 * Illustrative mode - verbose output for instructional purposes
 *  Show that [(] and [?] open a new expression scope level
 *  Show when an expr scope level is closed, that the opening [(] or [?] is overwritten
 *	Print out top scope of expression after (or end of) moveNeighborsIntoTree.  
 *	Could indicate tree w/ additional chars e.g. [{*}] [/*\] [_*_] [`*`] or /*\
 *	Dump out compilation token list at selected stages. Include indication of shrubification
 *	During interpretation, dump out token list on each operation
 * Have a defined ROOT scope in the interpreted file
 * User warning on encapsulated [=] since it has lowest priority
 * isFailed -> failedOnSrcLine at RunTimeInterpreter class level - Display via destructor if set
 * Test short circuit for [||],[&&] and [?] OPR8Rs
 * Short circuiting - Changed 12O -> O12
 * string variables - support was mostly already in place
 * bool - added;
 * FileParser recognize data types and reserved words.
 * Interpreter act on [PRE|POST]FIX OPR8Rs when doing an expression
 * Type check operand of [PRE|POST]FIX OPR8Rs - this should already happen via exprParser
 * Write out [PRE|POST]FIX part of expression
 * Figure out how to keep progressing through compilation as much as reasonable on user errors
 * 	isFailed vs. isFailedStop.....
 * 	How many user errors before we quit outright?
 * 	Fxn to look for end of currently busticated expression
 * 	Post INFO message so that user knows when we went speculative
 * Figure out what changes to make to errorInfo.set
 * 	Error logging
 * 	Might need to separate out user line:col to make it easier to filter out - Single message, multiple instances
 * Create UserMessages construct
 * Add assignment OPR8R mechanics
 * Break scopeStack out into its own class for sharing between GeneralParser and ExpressionParser?
 * Update explanation of how an expression is flattened and written out to interpreted file.
 * Declutter ExpressionParser, GeneralParser, RunTimeInterpreter
 * Propagate use of InfoWarnError, and bubble info up to the code responsible for displaying info to user
 * Compile variable declarations and put into NameSpace
 * 	Initialization expressions are allowed!
 * 	Will need an isInitialized var of some kind
 * SMRT pointers
 * Tighten up type conversions
 * Tested unary OPR8R mechanics
 * Dividing 2 SIGNED Tokens that should return a DOUBLE still returns a SIGNED
 * Should I re-visit the 1<->2 swapping of :'s operands? Probably not; at least for now
 * Add unary OPR8R mechanics
 * Test current BINARY OPR8Rs
 * Add Interpreter handling of TERNARY conditions - FALSE & TRUE paths coded and tested
 * EXEC_OPR8R_TKN
 * Evaluate an expression with Interpreter
 * Mechanics of writing expression out to interpreted file
 * ***************************************************************************/

#include "RunTimeInterpreter.h"
#include "common.h"
#include "CompileExecTerms.h"
#include "FileParser.h"
#include "Token.h"
#include "Utilities.h"
#include "GeneralParser.h"
#include <iostream>
#include <string>

using namespace std;
logLvlEnum logLevel = SILENT;

/* ****************************************************************************
 *
 * ***************************************************************************/
bool isCmdLineArgsOK (int argc, const char * argv[])	{
	bool isGood = false;
	bool isFailed = false;
	int idx = 2;
	std::string nextArg, nextValue;
	int completedArgCnt = 0;

	while (!isFailed && idx < argc)	{
		if (idx % 2 == 0)	{
			// Pick up the flag
			nextArg = argv[idx];
		} else {
			// Grab the value
			nextValue = argv[idx];
			if (nextArg.compare("-l") == 0 || nextArg.compare("--log_level") == 0)	{

				if (nextValue.compare ("SILENT") == 0)	{
					logLevel = SILENT;
					completedArgCnt++;

				} else if (nextValue.compare ("ILLUSTRATIVE") == 0)	{
					logLevel = ILLUSTRATIVE;
					completedArgCnt++;
					
				} else if (nextValue.compare ("VERBOSE") == 0)	{
					logLevel = VERBOSE;
					completedArgCnt++;
					
				} else if (nextValue.compare ("EFFUSIVE") == 0)	{
					logLevel = EFFUSIVE;
					completedArgCnt++;
					
				} else if (nextValue.compare ("DEBUG") == 0)	{
					logLevel = DEBUG;
					completedArgCnt++;
					
				} else	{
					isFailed = true;
				}
			
			 } else {
				isFailed = true;
			}
			nextArg.clear();
		}

		idx++;
	}
	
	if (!nextArg.empty())
		isFailed = true;

	if (!isFailed && completedArgCnt > 0)
		isGood = true;

	return (isGood);
}
/* ****************************************************************************
 *
 * ***************************************************************************/
int main(int argc, const char * argv[])
{
  int ret_code = GENERAL_FAILURE;
  int num_errors = 0;
  Utilities util;
	bool isArgsOK = false;

  if (argc >= 2)  {
    std::string input_file_path = argv[1];

    std::wstring userSrcFileName;

    // Prep our source code input file
    // TODO: This is hacky, but #include <codecvt> (for std::wstring_convert) can't be found
    // TODO: Might not be necessary until Unicode is supported for file names in the future
    userSrcFileName = util.getLastSegment(util.stringToWstring(input_file_path), L"/");

		if (argc > 2)
			isArgsOK = isCmdLineArgsOK(argc, argv);
		else 
			isArgsOK = true;

		if (isArgsOK)	{
			TokenPtrVector tokenStream;
			CompileExecTerms srcExecTerms;
			FileParser fileParser (srcExecTerms, userSrcFileName);
			if (OK == fileParser.gnr8_token_stream(input_file_path, tokenStream))	{

				// TODO: For early debug
				// util.dumpTokenList(tokenStream, 0, srcExecTerms, L"main", __LINE__, true);
				std::string output_file_name = "interpreted_file.o";
				std::wstring wide_output_file_name = util.stringToWstring(output_file_name);

				std::shared_ptr<StackOfScopes> rootScope = std::make_shared <StackOfScopes> ();
				// TODO: Previously passing &, but it appeared to be behaving like a copy: UserMessages userMessages;
				std::shared_ptr<UserMessages> userMessages = std::make_shared <UserMessages> ();
				GeneralParser generalParser (tokenStream, userSrcFileName, srcExecTerms, userMessages, output_file_name, rootScope, logLevel);

				std::wcout << std::endl << L"/* *************** <COMPILATION STAGE> **************** */" << std::endl;
				int compileRetCode = generalParser.compileRootScope();
				std::wcout << std::endl << L"Compiler ret_code = " << compileRetCode << std::endl;
				userMessages->showMessagesByInsertOrder(true);
				std::wcout << std::endl << L"/* *************** </COMPILATION STAGE> *************** */" << std::endl;

				int numUnqUserErrors, numTotalUserErrors;
				userMessages->getUserErrorCnt(numUnqUserErrors, numTotalUserErrors);
				// TODO: I expected to be able to re-use userMessages, but having problems
				userMessages.reset();
				rootScope.reset();

				if (compileRetCode == OK && numUnqUserErrors == 0)	{
					// TODO: Open input file here; I don't know how to make an fstream member variable (might not be possible)
					std::string interpretedFileName = output_file_name;
					std::shared_ptr<UserMessages> execMessages = std::make_shared <UserMessages> ();
					std::shared_ptr<StackOfScopes> execVarScope = std::make_shared <StackOfScopes> ();
		
					RunTimeInterpreter interpreter (interpretedFileName, userSrcFileName, execVarScope, execMessages, logLevel);

					std::wcout << std::endl;
					std::wcout << L"/* *************** <INTERPRETER STAGE> **************** */" << std::endl;
					ret_code = interpreter.execRootScope();
					std::wcout << L"Interpreter ret_code = " << ret_code << std::endl << std::endl;
					// execMessages->showMessagesByInsertOrder(true);
					execMessages->showMessagesByGroup();
					execVarScope->displayVariables();
					std::wcout << L"/* *************** </INTERPRETER STAGE> *************** */" << std::endl;
					execMessages.reset();
					execVarScope.reset();
				}
			}
		}
	}

	if (!isArgsOK)	{
		std::wcout << "Wrong # of arguments!" << std::endl;
	}

	return (ret_code);
}
