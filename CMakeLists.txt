cmake_minimum_required (VERSION 3.0...4.0)

project (CompileDriver)

add_compile_options(-ggdb)

add_executable(
	CompileDriver 
	Utilities.h Utilities.cpp
	common.h ErrorInfo.h ErrorInfo.cpp Operator.h Operator.cpp TokenCompareResult.h TokenCompareResult.cpp Token.h Token.cpp
	OpCodes.h Opr8rPrecedenceLvl.h Opr8rPrecedenceLvl.cpp
	BaseLanguageTerms.h BaseLanguageTerms.cpp CompileExecTerms.h CompileExecTerms.cpp RunTimeInterpreter.h RunTimeInterpreter.cpp
	ExprTreeNode.h ExprTreeNode.cpp
	NameSpaceObject.h NameSpaceObject.cpp
	FileParser.h FileParser.cpp
	NestedScopeExpr.h NestedScopeExpr.cpp
	InterpretedFileWriter.h InterpretedFileWriter.cpp InterpretedFileReader.h InterpretedFileReader.cpp
	ExpressionParser.h ExpressionParser.cpp 
	ScopeFrame.h ScopeFrame.cpp
	GeneralParser.h GeneralParser.cpp
	CompileDriver.cpp
)

