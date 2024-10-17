/* ****************************************************************************
 * TODO:
 * How to write an expression out for Interpreter
 * Compile variable declarations and put into NameSpace
 * Compile assignment and conditional statements
 * Add/clarify data types - Do I need the 16 & 32 bit [u]int sizes? What about 8 bit?
 * Use ExprTreeNode tree to resolve expression and check for any incompatible data types along the way
 * Compile fxn calls and put into NameSpace * When printing out an ExprTreeNode, indicate whether output is LFT or RGT side child to help with visualization.
 *
 * Directory/index for fxn calls so Interpreter doesn't have to search through the object
 * file for the location. It can do a quick(er) lookup
 * ***************************************************************************/

#include <list>
#include "common.h"
#include "CompileExecTerms.h"
#include "ExpressionParser.h"
#include "ExprTreeNode.h"
#include "FileParser.h"
#include "RunTimeExecutor.h"
#include "Token.h"
#include "InterpretedFileWriter.h"
#include "Utilities.h"

using namespace std;
#if 0

int testStreamInterpreter()	{
	int ret_code = GENERAL_FAILURE;
	int numShovels = 2;
	int numCClamps = 4;
	int numCars = 1;
	int numBikes = 4;
	int numSprings = 13;

//	ORIGINAL STACK
//  RND|R8R			Scope Depth
//	||					0
//	>						1
//	numSprings	2
//	5						2
//	&&					1
//	>=					2
//	numBikes		3
//	5						3
//	&&					2
//	>						3
//	numCars			4
//	14					4
//	&&					3
//	==					4
//	numShovels	5
//	2						5
//	>						4
//	numCClamps	5
//	3						5


	CInterpreterOpr8rs interpreter;
	TokenPtrVector unifiedStream;
  Token * rootTkn = new Token (OPR8R_TKN, L"||", 1, 1);
  unifiedStream.push_back(rootTkn);
  Token * springCompareTkn = new Token (OPR8R_TKN, L">", 2, 1);
  unifiedStream.push_back(springCompareTkn);
  Token * numSpringsTkn = new Token (KEYWORD_TKN, L"numSprings", 3, 1);
  numSpringsTkn->_signed = numSprings;
  unifiedStream.push_back(numSpringsTkn);
  Token * literal5 = new Token (BASE10_NUMBER_TKN, L"5", 4, 1);
  literal5->_signed = 5;
  unifiedStream.push_back(literal5);

  /* ************** And1 ************** */
  Token * And1 = new Token (OPR8R_TKN, L"&&", 5, 1);
  unifiedStream.push_back(And1);

  Token * gteTkn = new Token (OPR8R_TKN, L">=", 6, 1);
  unifiedStream.push_back(gteTkn);

  Token * bikesVar = new Token (KEYWORD_TKN, L"numBikes", 7, 1);
  bikesVar->_signed = numBikes;
  unifiedStream.push_back(bikesVar);

  Token * literal5_1 = new Token (BASE10_NUMBER_TKN, L"5", 8, 1);
  literal5_1->_signed = 5;
  unifiedStream.push_back(literal5_1);


  /* ************** And2 ************** */
  Token * And2 = new Token (OPR8R_TKN, L"&&", 9, 1);
  unifiedStream.push_back(And2);

  Token * gtTkn = new Token (OPR8R_TKN, L">", 10, 1);
  unifiedStream.push_back(gtTkn);

  Token * carsVar = new Token (KEYWORD_TKN, L"numCars", 11, 1);
  carsVar->_signed = numCars;
  unifiedStream.push_back(carsVar);

  Token * literal14 = new Token (BASE10_NUMBER_TKN, L"14", 12, 1);
  literal14->_signed = 14;
  unifiedStream.push_back(literal14);


  /* ************** And3 ************** */
  Token * And3 = new Token (OPR8R_TKN, L"&&", 13, 1);
  unifiedStream.push_back(And3);
  Token * equivalenceOp = new Token (OPR8R_TKN, L"==", 14, 1);
  unifiedStream.push_back(equivalenceOp);

  Token * shovelsVar = new Token (KEYWORD_TKN, L"numShovels", 15, 1);
  shovelsVar->_signed = numShovels;
  unifiedStream.push_back(shovelsVar);;

  Token * literal2 = new Token (BASE10_NUMBER_TKN, L"2", 16, 1);
  literal2->_signed = 2;
  unifiedStream.push_back(literal2);


  Token * deepestGt = new Token (OPR8R_TKN, L">", 17, 1);
  unifiedStream.push_back(deepestGt);

  Token * numCClampsVar = new Token (KEYWORD_TKN, L"numCClamps", 18, 1);
  numCClampsVar->_signed = numCClamps;
  unifiedStream.push_back(numCClampsVar);

  Token * literal3 = new Token (BASE10_NUMBER_TKN, L"3", 19, 1);
  literal3->_signed = 3;
  unifiedStream.push_back(literal3);

  std::wcout << "********** INITIAL TOKEN STREAM; Expected final result of 1 **********" << std::endl;
  interpreter.dumpTokenStream (unifiedStream);

  ret_code = interpreter.consumeUnifiedStream (unifiedStream, 0);

	return (ret_code);

}

#endif
int testStreamInterpreterMiddleShortCircuit() 	{
	int ret_code = GENERAL_FAILURE;

	//	numBikes = 4;
	//	numCars = 1;
	//	numShovels = 2;
	//	numRakes = 2;
	//	numBigBrooms = 1;
	//	numSmallWwClamps = 4;
	//	numBigWwClamps = 4;
	//	numSmallSpringClamps = 13;
	//	numCeeClamps = 4;
	//
	//	if (numSmallSpringClamps < 13 || numBigWwClamps == 4 || (numBikes >= 5 && numCars > 14 && numShovels == 2 && numCeeClamps > 3))		// TRUE - Middle short circuit

	return (ret_code);
}
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

    TokenPtrVector tkn_ptr_vctr;
    CompileExecTerms cSrcTerms;
    FileParser fileParser (cSrcTerms);
    if (OK == fileParser.gnr8_token_stream(input_file, tkn_ptr_vctr))	{

    	std::string output_file_name = "interpreted_file.o";
			std::wstring wide_output_file_name = util.stringToWstring(output_file_name);

			// Open output file here; I don't know how to make an fstream member variable (might not be possible)
    	std::fstream output_stream(output_file_name, output_stream.binary | output_stream.out);
			if (!output_stream.is_open()) {
				std::cout << "ERROR: Failed to open output file " << output_file_name << std::endl;

			} else {
				// Input & output files are ready to go
				InterpretedFileWriter interpretedWriter (output_stream, cSrcTerms);
				ExpressionParser exprParser (tkn_ptr_vctr, cSrcTerms);
				ret_code = exprParser.parseExpression(interpretedWriter);

				output_stream.close();
			}
    }

    std::wcout << std::endl << "********** END OF MAIN **********" << std::endl;
    // TODO testStreamInterpreter();

  } else  {
    std::wcout << "Wrong # of arguments!" << std::endl;
  }

  return (ret_code);
}





