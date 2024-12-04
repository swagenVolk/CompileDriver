/* ****************************************************************************
 * NEEDS TESTING:
 *
 * TODO:
 * Type check operand of [PRE|POST]FIX OPR8Rs - this should already happen via exprParser
 * Be clear & consistent about where type checking happens!
 * Write out [PRE|POST]FIX part of expression
 * Interpreter act on [PRE|POST]FIX OPR8Rs when doing an expression
 * Figure out how to keep progressing through compilation as much as reasonable on user errors
 * 	isFailed vs. isFailedStop.....
 * 	How many user errors before we quit outright?
 * 	Fxn to look for end of currently busticated expression
 * 	Post INFO message so that user knows when we went speculative
 * Look for opportunities to clarify (ie #defines -> enums?) to make debugging easier
 * Add unseen mechanics for PREFIX and POSTFIX OPR8Rs
 * How many pointers can I replace with essentially a copy? (grep -HEn "\bnew\b" *.cpp -> [0])
 * How can I check for memory leaks|danglers?
 * Method to regression test lots of expressions and compare results against regular compiler
 * Compile fxn calls and put into NameSpace
 *
 * Directory/index for fxn calls so Interpreter doesn't have to search through the object
 * file for the location. It can do a quick(er) lookup
 *
 * RECENTLY DONE:
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

#include "common.h"
#include "CompileExecTerms.h"
#include "FileParser.h"
#include "Token.h"
#include "Utilities.h"
#include "GeneralParser.h"
#include <iostream>

using namespace std;

/* ****************************************************************************
 *
 *
 * ***************************************************************************/
int main(int argc, const char * argv[])
{
  int ret_code = GENERAL_FAILURE;
  int num_errors = 0;
  Utilities util;

  if (argc == 2)  {
    std::string input_file = argv[1];

    std::wstring userSrcFileName;

    // Prep our source code input file
    // TODO: This is hacky, but #include <codecvt> (for std::wstring_convert) can't be found
    // TODO: Might not be necessary until Unicode is supported for file names in the future
    userSrcFileName = util.getLastSegment(util.stringToWstring(input_file), L"/");

    TokenPtrVector tokenStream;
    CompileExecTerms srcExecTerms;
    FileParser fileParser (srcExecTerms, userSrcFileName);
    if (OK == fileParser.gnr8_token_stream(input_file, tokenStream))	{

    	std::string output_file_name = "interpreted_file.o";
			std::wstring wide_output_file_name = util.stringToWstring(output_file_name);

			std::shared_ptr<VariablesScope> varScope = std::make_shared <VariablesScope> ();
			// TODO: Previously passing &, but it appeared to be behaving like a copy: UserMessages userMessages;
			std::shared_ptr<UserMessages> userMessages = std::make_shared <UserMessages> ();
			GeneralParser generalParser (tokenStream, userSrcFileName, srcExecTerms, userMessages, output_file_name, varScope);
			int compileRetCode = generalParser.findKeyWordObjects();

			userMessages->showMessagesByInsertOrder(true);

			if (compileRetCode == OK)	{
				// TODO: Open input file here; I don't know how to make an fstream member variable (might not be possible)
				std::string interpretedFileName = output_file_name;
				std::ifstream executableStream (interpretedFileName, executableStream.binary | executableStream.in);
				if (!executableStream.is_open()) {
					std::cout << "ERROR: Failed to open input file " << output_file_name << std::endl;

				} else	{
					// TODO: Will need to change this up since we'll be executing more than just 1 expression
					ret_code = OK;
#if 0
						InterpretedFileReader interpretedReader (executableStream, srcExecTerms);
						std::vector<Token> exprTknList;
						interpretedReader.readExprIntoList (exprTknList);
						executableStream.close();

						RunTimeInterpreter interpreter(srcExecTerms);
						// TODO: Right now, there's just variable declarations and I'm expecting an expression!
						// Could just check for an op_code
						if (OK != interpreter.resolveFlattenedExpr (exprTknList, ))	{
							// TODO:
							std::wcout << ??? << std::endl;
						}
#endif
				}
			}
		} else  {
			std::wcout << "Wrong # of arguments!" << std::endl;
		}
	}


  // TODO testStreamInterpreter();

  int count = 1;
  std::wcout << L"Final answer for count = " << count << L": " << (1 + 2 * (3 + 4 * (5 + 6 * 7 * (count == 1 ? 10 : count == 2 ? 11 : count == 3 ? 12 : count == 4 ? 13 : 33)))) << std::endl;

  count = 2;
  std::wcout << L"Final answer for count = " << count << L": " << (1 + 2 * (3 + 4 * (5 + 6 * 7 * (count == 1 ? 10 : count == 2 ? 11 : count == 3 ? 12 : count == 4 ? 13 : 33)))) << std::endl;

  count = 5;
  std::wcout << L"Final answer for count = " << count << L": " << (1 + 2 * (3 + 4 * (5 + 6 * 7 * (count == 1 ? 10 : count == 2 ? 11 : count == 3 ? 12 : count == 4 ? 13 : 33)))) << std::endl;

  std::wcout << std::endl << "********** END OF MAIN **********" << std::endl;
  return (ret_code);
}
