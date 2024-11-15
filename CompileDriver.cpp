/* ****************************************************************************
 * NEEDS TESTING:
 * SMRT pointers?
 * Tighten up type conversions
 * Look for opportunities to clarify (ie #defines -> enums?) to make debugging easier
 *
 * TODO:
 * Add variable declaration logic
 * 	Initialization expressions are allowed!
 * 	Will need an isInitialized var of some kind
 * 	Should a datatype KEYWORD perhaps open up a scope?
 * Add assignment OPR8R mechanics
 * Update explanation of how an expression is flattened and written out to interpreted file.
 * How many pointers/new(s) can I replace with essentially a copy?
 * Method to regression test lots of expressions and compare results against regular compiler
 * Compile variable declarations and put into NameSpace
 * Compile assignment and conditional statements
 * Use ExprTreeNode tree to resolve expression and check for any incompatible data types along the way
 * Compile fxn calls and put into NameSpace
 *
 * Directory/index for fxn calls so Interpreter doesn't have to search through the object
 * file for the location. It can do a quick(er) lookup
 *
 * RECENTLY DONE:
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

#include <list>
#include "common.h"
#include "CompileExecTerms.h"
#include "ExpressionParser.h"
#include "ExprTreeNode.h"
#include "FileParser.h"
#include "Token.h"
#include "InterpretedFileWriter.h"
#include "InterpretedFileReader.h"
#include "RunTimeInterpreter.h"
#include "Utilities.h"
#include "GeneralParser.h"

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

    std::wstring file_to_parse;

    // Prep our source code input file
    // TODO: This is hacky, but #include <codecvt> (for std::wstring_convert) can't be found
    // TODO: Might not be necessary until we decide to support Unicode file names in the future
    file_to_parse = util.stringToWstring(input_file);

    TokenPtrVector tokenStream;
    CompileExecTerms srcExecTerms;
    FileParser fileParser (srcExecTerms);
    if (OK == fileParser.gnr8_token_stream(input_file, tokenStream))	{

    	std::string output_file_name = "interpreted_file.o";
			std::wstring wide_output_file_name = util.stringToWstring(output_file_name);

			// TODO: Open output file here; I don't know how to make an fstream member variable (might not be possible)
    	std::ofstream output_stream(output_file_name, output_stream.binary | output_stream.out);
			if (!output_stream.is_open()) {
				std::cout << "ERROR: Failed to open output file " << output_file_name << std::endl;

			} else {
				// Input & output files are ready to go
				GeneralParser generalParser (tokenStream, srcExecTerms, output_file_name);
				int compileRetCode = generalParser.findKeyWordObjects();
				output_stream.close();

				if (compileRetCode == OK)	{
					// TODO: Open input file here; I don't know how to make an fstream member variable (might not be possible)
					std::string interpretedFileName = output_file_name;
		    	std::ifstream executableStream (interpretedFileName, executableStream.binary | executableStream.in);
					if (!executableStream.is_open()) {
						std::cout << "ERROR: Failed to open input file " << output_file_name << std::endl;

					} else	{
						// TODO: Will need to change this up since we'll be executing more than just 1 expression
						InterpretedFileReader interpretedReader (executableStream, srcExecTerms);
						std::vector<Token> exprTknList;
						interpretedReader.readExprIntoList (exprTknList);
						executableStream.close();

						RunTimeInterpreter interpreter(srcExecTerms);
						if (OK != interpreter.resolveFlattenedExpr (exprTknList))	{
							// TODO:
							std::wcout << L"resolveExpression FAILED!" << std::endl;
						}
					}
				}
			}
    }

    std::wcout << std::endl << "********** END OF MAIN **********" << std::endl;
    // TODO testStreamInterpreter();

  } else  {
    std::wcout << "Wrong # of arguments!" << std::endl;
  }

  int count = 1;

  std::wcout << L"Final answer for count = " << count << L": " << (1 + 2 * (3 + 4 * (5 + 6 * 7 * (count == 1 ? 10 : count == 2 ? 11 : count == 3 ? 12 : count == 4 ? 13 : 33)))) << std::endl;

  count = 2;
  std::wcout << L"Final answer for count = " << count << L": " << (1 + 2 * (3 + 4 * (5 + 6 * 7 * (count == 1 ? 10 : count == 2 ? 11 : count == 3 ? 12 : count == 4 ? 13 : 33)))) << std::endl;

  return (ret_code);
}
