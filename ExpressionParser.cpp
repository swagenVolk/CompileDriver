/*
 * ExpressionParser.cpp
 *
 *  Created on: Jun 11, 2024
 *      Author: Mike Volk
 *  Class to parse and check a user source level expression. The FileParser has turned the user's source file into a stream of Tokens.
 *  When an expression has been encountered in the Token stream, this class will handle compilation of that expression; it will check
 *  for correctness and build a parse tree that includes the precedence of operations, which is dictated by the user's placement of
 *  parentheses and defined precedence for operators contained within a specific expression scope level (contained in the same set of
 *  parentheses).
 *
 *	Example source level expression:
 *	one + two * (three + (four * five))
 *              ^        ^ Open parentheses -> new scope level
 *
 *  Winds up looking like this:
 *	Scope level 2:	[four][*][five]
 *	Scope level 1:	[three][+][(]
 *	Scope level 0:	[one][+][two][*][(]
 *
 *	When a [(] is encountered, open new scope level and work on that level. Leave [(] Token in list of previous scope
 *  as a marker to link the newly opened scope back to when it gets resolved into a single Token tree, with an OPR8R 
 *  at the root.
 *
 *  Scope level 2 is resolved to a tree, shown below:
 *				[*]
 *	[four]	[five]
 *	Scope level 2's list consists of only [*].  Operands [four] and [five] have been moved & "hidden" inside the [*] object,
 *  which will replace the [(] object in Scope level 1 that was responsible for opening Scope level 2.
 *	Scope level 2 is removed, and now we've got this:
 *
 *	Scope level 1:	[three][+][*]
 *	Scope level 0:	[one][+][two][*][(]
 *
 *	Now we go back to working on Scope level 1, and this gets turned into a tree also, shown below:
 *					[+]
 *	[three]			[*]
 *
 *	So now we're going to close Scope level 1 and replace the [(] in Scope level 0 that opened up level 1:
 *	Scope level 0:	[one][+][two][*][+]
 *
 *  According to OPR8R precedence, [*] is highest, so we remove the neighbors [two] and [+] from the list
 *	and attach them under [*], like so:
 *				[*]
 *	[two]			[+]
 *
 *	and now we've got:
 *	Scope level 0:	[one][+][*]
 *  
 *	OPR8R [*] was worked on, so that leaves [+] next, and that gets turned into a tree:
 *				[+]
 *	[one]			[*]
 *
 *  The expression 
 *	one + two * (three + (four * five))
 *	gets turned into a parse tree looks like this:
 *				[+]
 *	[one]				[*]
 *					[two]					[+]
 *								[three]				 [*]
 *													[four]	[five]
 *
 *	Deepest levels resolved first.  Assume variable names contain corresponding numeric value.
 *  [four][*][five] 	-> 20
 *	[three][+][20] 		-> 23
 *  [two][*][23]			-> 46
 *	[one][+][46]			-> 47
 *	[47] is our final result.
 *  
 *  TODO:
 *  Internal vs. user error messaging
 *  Check for proper handling of memory - no danglers
 *
 */

#include "ExpressionParser.h"

#include <bits/stdint-uintn.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "CompileExecTerms.h"
#include "ExprTreeNode.h"
#include "InfoWarnError.h"
#include "InterpretedFileWriter.h"
#include "OpCodes.h"
#include "Operator.h"
#include "Opr8rPrecedenceLvl.h"
#include "RunTimeInterpreter.h"
#include "Token.h"
#include "UserMessages.h"
#include "StackOfScopes.h"
#include "common.h"

ExpressionParser::ExpressionParser(CompileExecTerms & inUsrSrcTerms, std::shared_ptr<StackOfScopes> inVarScopeStack
		, std::wstring userSrcFileName, std::shared_ptr<UserMessages> userMessages, logLvlEnum logLvl) {

	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");
	usrSrcTerms = inUsrSrcTerms;
	scopedNameSpace = inVarScopeStack;
	this->userSrcFileName = userSrcFileName;
	this->userMessages = userMessages;
	logLevel = logLvl;
	isExprVarDeclaration = false;
  failed_on_src_line = 0;
  num_var_leaf_nodes = 0;
}


ExpressionParser::~ExpressionParser() {
	if (failed_on_src_line > 0 && !userMessages->isExistsInternalError(thisSrcFile, failed_on_src_line))	{
		// Dump out a debugging hint
		std::wcout << L"FAILURE on " << thisSrcFile << L":" << failed_on_src_line << std::endl;
	}
}

/* ****************************************************************************
 * Parse through the current expression and if it's not well formed, generate 
 * a clear error message to the user.
 * ***************************************************************************/
int ExpressionParser::makeExprTree (TokenPtrVector & tknStream, std::shared_ptr<ExprTreeNode> & expressionTree
		, Token & enderTkn, expr_ender_type ended_by, bool & isCallerExprClosed, bool isInVarDec, bool & is_expr_static)  {
  int ret_code = GENERAL_FAILURE;
	int expr_closed__LINE = 0;
	isExprVarDeclaration = isInVarDec;
  num_var_leaf_nodes = 0;
  // Start off pessimistic
  is_expr_static = true;

  std::vector<std::shared_ptr<NestedScopeExpr>> exprScopeStack;
	
  if (expressionTree == NULL)	{
  	userMessages->logMsg (INTERNAL_ERROR, L"Passed parameter expressionTree is NULL!", thisSrcFile, __LINE__, 0);

  } else	{
		std::shared_ptr<NestedScopeExpr> rootScope = std::make_shared<NestedScopeExpr>();
		exprScopeStack.push_back (rootScope);

		bool isStopFail = false;
		int num_tkns_done = 0;
		int top;
		uint32_t curr_legal_tkn_types;
		uint32_t next_legal_tkn_types;
		uint32_t prev_tkn_type;
		uint32_t curr_tkn_type;
		Token expectedEndTkn(START_UNDEF_TKN, L"");

		if (tknStream.empty())	{
			userMessages->logMsg (INTERNAL_ERROR, L"Token stream is unexpectedly empty!", thisSrcFile, __LINE__, 0);
			isStopFail = true;

		} else	{
			while (!isStopFail && 0 == expr_closed__LINE)	{
				// Consume flat stream of Tokens in current expression; attach each to an ExprTreeNode for tree transformation
				if (!tknStream.empty())	{
					std::shared_ptr<Token> currTkn = tknStream.front();

					if (0 == num_tkns_done)	{
						// Token that starts expression will determine how we end the expression
						if (OK != getExpectedEndToken(currTkn, curr_legal_tkn_types, expectedEndTkn, ended_by))	{
							isStopFail = true;
							break;
						
						} else if ((logLevel == ILLUSTRATIVE && !isExprVarDeclaration) || logLevel > ILLUSTRATIVE)		{
							std::wcout << L"// ILLUSTRATIVE MODE: Starting compilation of expression that begins on " << userSrcFileName;
							std::wcout << L":" << std::to_wstring(currTkn->get_line_number()) << L":" << std::to_wstring(currTkn->get_column_pos());
							std::wcout << std::endl << std::endl;
						}
					}

          if ((currTkn->tkn_type == expectedEndTkn.tkn_type && currTkn->_string == expectedEndTkn._string)
							|| (currTkn->tkn_type == SRC_OPR8R_TKN && currTkn->_string == usrSrcTerms.get_statement_ender()))	{
						// Expression ended by a SPR8R - e.g. [,] or ;
						tknStream.erase(tknStream.begin());

						if (0 == (curr_legal_tkn_types & (VAR_NAME_NXT_OK|LITERAL_NXT_OK|SYSTEM_CALL_NXT_OK|OPEN_PAREN_NXT_OK)))	{
							// Check if expression is in a closeable state
							expr_closed__LINE = __LINE__;
							enderTkn = *currTkn;

							top = exprScopeStack.size() - 1;
							if (exprScopeStack[top]->scopedKids.size() == 0)	{
								userMessages->logMsg (INTERNAL_ERROR, L"Expression ender " + currTkn->descr_sans_line_num_col() + L" closed an empty expression!", thisSrcFile, __LINE__, 0);
								isStopFail = true;

							} else if (top > 0 || exprScopeStack[top]->scopedKids.size() >= 1)	{
								if (OK != closeNestedScopes(expr_closed__LINE > 0, exprScopeStack))	{
									// TODO:
									isStopFail = true;
								}
							}

						} else	{
							userMessages->logMsg (USER_ERROR, L"Expression ender " + currTkn->descr_sans_line_num_col() + L" not used validly."
									,  userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
							// TODO: Possibly an opportunity to have the caller continue on. Might be prudent to change the error code accordingly
							isStopFail = true;
						}

					} else if (!isExpectedTknType (curr_legal_tkn_types, next_legal_tkn_types, currTkn, exprScopeStack))	{
						userMessages->logMsg (USER_ERROR
								, L"Unexpected token type! Expected " + makeExpectedTknTypesStr(curr_legal_tkn_types) + L" but got " + currTkn->descr_sans_line_num_col()
								, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
						isStopFail = true;

					} else if ((currTkn->tkn_type == SRC_OPR8R_TKN && (TERNARY_2ND & usrSrcTerms.get_type_mask(currTkn->_string)))
							&& !isTernaryOpen(exprScopeStack))	{
						// Unexpected TERNARY_2ND
						userMessages->logMsg (USER_ERROR
								,L"Got middle ternary operand " + currTkn->descr_sans_line_num_col() + L" without required preceding starting ternary operator " + usrSrcTerms.get_ternary_1st()
								,  userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
						isStopFail = true;

					} else if ((currTkn->tkn_type == SRC_OPR8R_TKN && (TERNARY_2ND & usrSrcTerms.get_type_mask(currTkn->_string)))
							&& get2ndTernaryCnt(exprScopeStack) > 0)	{
						// Unexpected *EXTRA* TERNARY_2ND
						userMessages->logMsg (USER_ERROR, L"Got unexpected additional middle ternary operand " + currTkn->descr_sans_line_num_col()
								, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
						isStopFail = true;

          } else if ((currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L"(")
							|| (currTkn->tkn_type == SRC_OPR8R_TKN && (TERNARY_1ST & usrSrcTerms.get_type_mask(currTkn->_string))))	{
						// Open parenthesis or 1st ternary
						if (OK != openSubExprScope(tknStream, exprScopeStack))	{
							isStopFail = true;
						}

					} else if (currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L")")	{
						// Close parenthesis - syntactic sugar that just melts away
						// Remove Token from stream without destroying - move to flat expression in current scope
						tknStream.erase(tknStream.begin());

						if (OK != closeNestedScopes (expr_closed__LINE > 0, exprScopeStack))	{
							isStopFail = true;

						} else if (expectedEndTkn.tkn_type == SPR8R_TKN && expectedEndTkn._string == currTkn->_string 
							&& exprScopeStack.size() == 1 && exprScopeStack[0]->scopedKids.size() == 1)	{
							// TODO: Failure when expecting to close by a [;]
							enderTkn = *currTkn;
							expr_closed__LINE = __LINE__;
						}

          } else if (currTkn->tkn_type == SYSTEM_CALL_TKN)  {
						tknStream.erase(tknStream.begin());

            // The system call and its parameters will get processed and encapsulated into sys_call_node
            std::shared_ptr<ExprTreeNode> sys_call_node = std::make_shared<ExprTreeNode> (currTkn);

            if (OK != compile_system_call(tknStream, sys_call_node))
              // TODO: Opportunity to check for continued progress?
              isStopFail = true;
            else if (!exprScopeStack.empty())
              exprScopeStack[exprScopeStack.size() - 1]->scopedKids.push_back(sys_call_node);
            else
              isStopFail = true;

          } else if (currTkn->tkn_type == END_OF_STREAM_TKN) {
						userMessages->logMsg (INTERNAL_ERROR, L"parseExpression should never hit END_OF_STREAM_TKN!", thisSrcFile, __LINE__, 0);
						isStopFail = true;
						tknStream.erase(tknStream.begin());

          } else	{
						// Remove Token from stream without destroying - move to flat expression in current scope
						tknStream.erase(tknStream.begin());
						std::shared_ptr<ExprTreeNode> treeNode = std::make_shared<ExprTreeNode> (currTkn);
						top = exprScopeStack.size() - 1;
						std::shared_ptr<NestedScopeExpr> topScope = exprScopeStack[top];
						topScope->scopedKids.push_back (treeNode);

						if (currTkn->tkn_type == SRC_OPR8R_TKN && (TERNARY_2ND & usrSrcTerms.get_type_mask(currTkn->_string)))
							// Keep track of secondary ternary operators; expecting only 1 paired with 1st ternary, which
							// opened a new scope inside the expression
							exprScopeStack[top]->ternary2ndCnt++;
						// TODO: Don't accumulate junk. We've moved|shared what was in this pointer by now
						treeNode.reset();
					}

					curr_legal_tkn_types = next_legal_tkn_types;
					// Only reason to look back at previous Token is for a postfix opr8r; must be a VAR_NAME to be valid.  Can't have been converted to an lvalue
					prev_tkn_type = curr_tkn_type;

					// Don't accumulate junk. We've moved|shared what was in this pointer by now
					currTkn.reset();
				}
        num_tkns_done++;
      }

			if (expr_closed__LINE && !isStopFail)	{
				int numTknsLeftInExpr = exprScopeStack[0]->scopedKids.size();
				if (exprScopeStack.size() == 1 && 1 == numTknsLeftInExpr)	{
					ret_code = OK;
					expressionTree = exprScopeStack[0]->scopedKids[0];

          is_expr_static = (num_var_leaf_nodes == 0);

					if ((logLevel == ILLUSTRATIVE && !isExprVarDeclaration) || logLevel > ILLUSTRATIVE)	{
            std::wcout << L"Compiler's Parse Tree for Complete Expression" << std::endl;
            displayParseTree(expressionTree, 0);
 					}
				} else	{
					std::wstring devMsg = L"Expression closed but tree conversion failed! Remaining Token count at stack top = ";
					devMsg.append ( std::to_wstring(numTknsLeftInExpr));
					devMsg.append (L"; # scope levels = ");
					devMsg.append (std::to_wstring(exprScopeStack.size()));
					devMsg.append (L"; exprCloseLine = ");
					devMsg.append (std::to_wstring(expr_closed__LINE));
					devMsg.append(L";");
					userMessages->logMsg (INTERNAL_ERROR, devMsg, thisSrcFile, __LINE__, 0);
					showDebugInfo (thisSrcFile, __LINE__, exprScopeStack);
				}
			}
		}

		cleanScopeStack(exprScopeStack);
  }

	isCallerExprClosed = expr_closed__LINE > 0;

  return (ret_code);
}

/* ****************************************************************************
 * General utility fxn to replace often replicated code
 * ***************************************************************************/
 int ExpressionParser::check_for_expected_token (TokenPtrVector & tknStream, Token & prev_tkn, std::wstring pattern_str, bool is_consume_tkn) {
  int ret_code = GENERAL_FAILURE;

  if (tknStream.empty())	{
  	userMessages->logMsg (USER_ERROR, L"Expected Token after " + prev_tkn._string + L" but stream is empty!", userSrcFileName
      , prev_tkn.get_line_number(), prev_tkn.get_column_pos());
    SET_FAILED_ON_SRC_LINE;

  } else {
    std::shared_ptr <Token> curr_tkn = tknStream.front();

    if (curr_tkn->_string != pattern_str) {
      userMessages->logMsg (USER_ERROR, L"Expected \"" + pattern_str + L"\"  after " + prev_tkn._string + L" but instead got: " + curr_tkn->descr_sans_line_num_col()
        , userSrcFileName, curr_tkn->get_line_number(), curr_tkn->get_column_pos());
      SET_FAILED_ON_SRC_LINE;
    
    } else {
      if (is_consume_tkn)
        tknStream.erase (tknStream.begin());
      ret_code = OK;
    }  
  }
  
  return ret_code;
}

/* ****************************************************************************
 * Next Token is a opening parenthesis or an opening ternary (?)
 * Increase the current scope level of the expression
 * Deeper|greater scope corresponds to higher precedence, or deeper levels of
 * parentheses.
 * ***************************************************************************/
int ExpressionParser::openSubExprScope (TokenPtrVector & tknStream, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack)  {
  int ret_code = GENERAL_FAILURE;

	std::shared_ptr <Token>currTkn = tknStream.front();
	// Remove the Token from the stream without destroying it
	tknStream.erase(tknStream.begin());
	std::shared_ptr<ExprTreeNode> branchNode = std::make_shared<ExprTreeNode> (currTkn);
	exprScopeStack[exprScopeStack.size() - 1]->scopedKids.push_back (branchNode);

	// Create enclosed scope with pointer back to originating ExprTreeNode/Token (probably an open paren)
	// Attach this shared_ptr to an already existing shared_ptr of type NestedScopeExpr
	std::shared_ptr<NestedScopeExpr> nextScope (std::make_shared<NestedScopeExpr> (branchNode));
	// We need to point back to the node that opened this new scope
	exprScopeStack.push_back (nextScope);
	ret_code = OK;

	return (ret_code);
}

/* ****************************************************************************
 * Current expression scope (nested parentheses level) is closed and needs to
 * get converted from a stream of Tokens to a parse tree, and also linked to its
 * parent scope.  Check if the parent scope can be collapsed also &
 * recursively.  It's turtles all the way down!
* ***************************************************************************/
 int ExpressionParser::closeNestedScopes(bool isExprClosed, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack)	{
	int ret_code = GENERAL_FAILURE;

	bool isParentLinked = false;
	bool isRootScope = false;
	bool isStopFail = false;

	if (OK == makeTreeAndLinkParent(isParentLinked, isExprClosed, exprScopeStack))	{
		int  prevStackSize = exprScopeStack.size(), currStackSize;
	
		while (!isParentLinked && !isRootScope && !isStopFail)	{
			if (OK != makeTreeAndLinkParent(isParentLinked, isExprClosed, exprScopeStack))	{
				isStopFail = true;
				break;
	
			} else	{
				currStackSize = exprScopeStack.size();
				
				if (exprScopeStack.size() == 1)	{
					// Only the root scope remains. Check for correctness
					isRootScope = true;
					int numRootKids = exprScopeStack[0]->scopedKids.size();
					if (numRootKids > 1 && OK != turnClosedScopeIntoTree (exprScopeStack[exprScopeStack.size() - 1]->scopedKids, isExprClosed, exprScopeStack))	{
						isStopFail = true;
					}
				
				} else if (prevStackSize <= currStackSize)	{
					isStopFail = true;
					userMessages->logMsg(INTERNAL_ERROR, L"Failed to reduce stack size while making tree", thisSrcFile, __LINE__, 0);
				}
			}
		}

		if (!isStopFail)
			ret_code = OK;
	}

	return (ret_code);

}

/* ****************************************************************************
 * Next Token is either a closing parenthesis or a statement ending ";"
 * Both of which are syntactic sugar that doesn't need to be kept.
 * Attempt to resolve the expression in this scope to
 * a single codeToken by working through the precedence of the operators.
 * Decrease the current scope level of the expression
 * ***************************************************************************/
int ExpressionParser::makeTreeAndLinkParent (bool & isParentFndYet, bool isExprClosed, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack)  {
  int ret_code = GENERAL_FAILURE;
	bool isScopenerFound = false;
	bool isRootScope = false;
	bool isExprAttached = false;
	bool isOpenedByTernary = false;
	bool isStopFail = false;

	// Start off expecting we won't find the matching opening '(' or [?] at this scope
	isParentFndYet = false;

	// Get pointer to ExprTreeNode that contains the '(' or [?] that opened the top scope
	// This fxn was called when we encountered a ')', but the corresponding '(' could be
	// several scope levels deep if we've got nested TERNARY OPR8Rs
	std::shared_ptr<ExprTreeNode> scopener = NULL;
	std::wstring scopenerDesc;

	int top = exprScopeStack.size() - 1;
	if (exprScopeStack.size() == 1)	{
		isRootScope = true;

	} else if (exprScopeStack.size() >= 2)	{
		scopener = (std::shared_ptr<ExprTreeNode>)(exprScopeStack[top]->myParentScopener);
		scopenerDesc = scopener->originalTkn->descr_sans_line_num_col();

		if (scopener->originalTkn->tkn_type == SPR8R_TKN && 0 == scopener->originalTkn->_string.compare(L"("))	{
			isParentFndYet = true;

		} else if (scopener->originalTkn->tkn_type == SRC_OPR8R_TKN && scopener->originalTkn->_string == usrSrcTerms.get_ternary_1st())	{
			isOpenedByTernary = true;

		} else	{
			userMessages->logMsg (USER_ERROR, L"Current scope NOT opened by either '(' or " + usrSrcTerms.get_ternary_1st() + L" but with " + scopenerDesc
					,  userSrcFileName, scopener->originalTkn->get_line_number(), scopener->originalTkn->get_column_pos());
			isStopFail = true;
		}
	}
	
	if (!isRootScope && scopener == NULL)	{
		userMessages->logMsg (INTERNAL_ERROR, L"ExprTreeNode that opens current scope was not set earlier", thisSrcFile, __LINE__, 0);
		isStopFail = true;

	} else if (isRootScope) {
			ret_code = turnClosedScopeIntoTree (exprScopeStack[exprScopeStack.size() - 1]->scopedKids, isExprClosed, exprScopeStack);

	} else {
		int stackSzB4 = exprScopeStack.size();

		if (!isStopFail && isParentFndYet != isOpenedByTernary)	{
			// Call turnClosedScopeIntoTree to collapse the current flat list of ExprTreeNodes into
			// a hierarchical tree based on OPR8R precedence - a root ExprTreeNode
			if (OK == turnClosedScopeIntoTree (exprScopeStack[exprScopeStack.size() - 1]->scopedKids, isExprClosed, isOpenedByTernary, exprScopeStack))	{
				// Update the placeholder "(" ExprTreeNode so that it now points to the parent ExprTreeNode
				// TODO: After expression is completed, then remove the intermediary "(" objects ???????
				std::shared_ptr<NestedScopeExpr> stackTop = exprScopeStack[exprScopeStack.size() - 1];

				std::shared_ptr<ExprTreeNode> subExpr = stackTop->scopedKids[0];
				exprScopeStack.erase(exprScopeStack.end());
				stackTop.reset();

				int stackSize = exprScopeStack.size();

				if (stackSize >= 1)	{
					ExprTreeNodePtrVector::iterator nodeR8r;
					int currStackIdx = stackSize - 1;
					// Make an alias variable for code readability
					ExprTreeNodePtrVector & childList = exprScopeStack[currStackIdx]->scopedKids;

					for (nodeR8r = childList.begin(); nodeR8r != childList.end() && !isScopenerFound && !isStopFail; nodeR8r++)	{
						std::shared_ptr<ExprTreeNode> currNode = *nodeR8r;

						if (currNode.get() == scopener.get())	{
							// Found the branch that opened the previous scope. Now attach subExpr to it
							isScopenerFound = true;

							if (isParentFndYet)	{
								// Don't make our sub-expression a child to the "(" that opened our scope
								// Replace it with our sub-expression since the "(" is vestigial at this point
								// List clean-up pending after this loop
								// TODO: myParent?
								childList.insert (nodeR8r, subExpr);
								isExprAttached = true;

							} else if (isOpenedByTernary && subExpr->originalTkn->_string == usrSrcTerms.get_ternary_2nd() && scopener->_2ndChild == NULL)	{
								// The subexpression root is the [:] OPR8R
								scopener->_2ndChild = subExpr;
								subExpr->scopenedBy = scopener;
								// TODO: Is line below relevant?
								subExpr->treeParent = scopener;
								isExprAttached = true;
								ret_code = OK;

							} else	{
                  displayParseTree(subExpr, 0);
									std::wstring userMsg = L"Current scope opened by [";
									userMsg.append(usrSrcTerms.get_ternary_1st());
									userMsg.append (L"] but could not find paired [");
									userMsg.append (usrSrcTerms.get_ternary_2nd());
									userMsg.append (L"]");
									userMessages->logMsg (USER_ERROR, userMsg,  userSrcFileName, scopener->originalTkn->get_line_number(), scopener->originalTkn->get_column_pos());
									isStopFail = true;
							}
						}
					}

					if (isParentFndYet && isExprAttached)	{
						// Need to remove that vestigial "(" from the list
						bool isVestigeDeleted = false;
						ExprTreeNodePtrVector::iterator delR8r;

						for (delR8r = childList.begin(); delR8r != childList.end() && !isVestigeDeleted; delR8r++)	{
							std::shared_ptr<ExprTreeNode> currBranch = (std::shared_ptr<ExprTreeNode>)*delR8r;
							if (currBranch == scopener)	{
								delR8r = childList.erase(delR8r);
								scopener.reset();
								scopener = NULL;
								isVestigeDeleted = true;
								ret_code = OK;
							}
						}
						if (!isVestigeDeleted)	{
							userMessages->logMsg (INTERNAL_ERROR, L"Failed deleting vestigial " + scopenerDesc, thisSrcFile, __LINE__, 0);
							isStopFail = true;
						}
					}

					if (ret_code != OK)	{
						if (isScopenerFound && !isExprAttached && exprScopeStack.size() == 1)	{
							// TODO: Figure out boolz
							if (exprScopeStack[0]->scopedKids.size() > 1)	{
								ret_code = turnClosedScopeIntoTree (exprScopeStack[exprScopeStack.size() - 1]->scopedKids, isExprClosed, exprScopeStack);
							}	else	{
							 	ret_code = OK;
							}

						} else if (!isScopenerFound || !isExprAttached)	{
							std::wstring scopeErrMsg;
							if (!isScopenerFound)
								scopeErrMsg = L"Could not match with scope opened by ";
							else
								scopeErrMsg = L"Failed to attach resolved sub-expression with scope opened by ";

							userMessages->logMsg (USER_ERROR, scopeErrMsg + scopenerDesc + L" with current scope stack level = " + std::__cxx11::to_wstring(exprScopeStack.size() - 1)
									, userSrcFileName, scopener->originalTkn->get_line_number(), scopener->originalTkn->get_column_pos());
							printScopeStack(thisSrcFile, __LINE__, exprScopeStack);
							isStopFail = true;

						} else	{
							userMessages->logMsg (INTERNAL_ERROR, L"Dazed and Confubalated!", thisSrcFile, __LINE__, 0);
							isStopFail = true;
							if (subExpr != NULL)	{
								displayParseTree(subExpr, thisSrcFile, __LINE__);
							}
						}
					}
				}
			}
			// After exiting, we'll continue on with Tokens 1 scope up.  Could be another ")" that closes
			// the now current scope, or other kinds of Tokens that continue the current sub-expression.
		}

		int stackSzAfter = exprScopeStack.size();
		if (stackSzAfter >= stackSzB4)	{
			userMessages->logMsg (INTERNAL_ERROR
					, L"Failed to reduce expression scope depth. Before call: " + std::__cxx11::to_wstring(stackSzB4)
					+ L"; After call: " + std::__cxx11::to_wstring(stackSzAfter) + L";"
					, thisSrcFile, __LINE__, 0);
			printScopeStack(thisSrcFile, __LINE__, exprScopeStack);
			isStopFail = true;
		}
	}

	if (ret_code != OK && !isStopFail)	{
		userMessages->logMsg (INTERNAL_ERROR, L"Error not handled!", thisSrcFile, __LINE__, 0);
		isStopFail = true;
	}

	return (ret_code);
}

/* ****************************************************************************
 * Proc to hide details of taking neighbors of an OPR8R contained inside a flat list
 * e.g. 3 + 4
 * and removing them from the list to become children of the OPR8R, more like a tree
 * 								[+]
 * _1stChild->[3]			_2ndChild->[4]
 *
 * The caller has the context to know how many operands this OPR8R requires and
 * whether the left neighbor, right neighbor or both need to be removed from the
 * list and attached to the OPR8R.
 * ***************************************************************************/
int ExpressionParser::moveNeighborsIntoTree (ExprTreeNodePtrVector & currScope
	, int opr8rIdx, opr8rReadyState opr8rState, bool isMoveLeftNbr, bool isMoveRightNbr
  , std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack)	{
	int ret_code = GENERAL_FAILURE;

	if (opr8rIdx >= 0 && opr8rIdx < currScope.size())	{
    Operator opr8r;
    usrSrcTerms.getExecOpr8rDetails(usrSrcTerms.getOpCodeFor(currScope[opr8rIdx]->originalTkn->_string), opr8r);

		std::shared_ptr<ExprTreeNode> opr8rNode = currScope.at(opr8rIdx);
		std::shared_ptr<ExprTreeNode> leftNbr = NULL;
		std::shared_ptr<ExprTreeNode> rightNbr = NULL;
		Token tmpTkn;
		std::wstring lookUpMsg;

		if (isMoveRightNbr)	{
			if ((currScope.size() - 1) < (opr8rIdx + 1))	{
				SET_FAILED_ON_SRC_LINE;

			} else {
				rightNbr = currScope.at(opr8rIdx + 1);
				if (rightNbr == NULL)	{
					SET_FAILED_ON_SRC_LINE;
				
				} else {
          if (usrSrcTerms.is_viable_var_name(rightNbr->originalTkn->_string))
            // > 0 variable leaf nodes means that this expression isn't fixed|static
            num_var_leaf_nodes++;

          if ((opr8r.type_mask & PREFIX) && (rightNbr->originalTkn->tkn_type != USER_WORD_TKN 
							|| !usrSrcTerms.is_viable_var_name(rightNbr->originalTkn->_string)
							|| OK != scopedNameSpace->findVar(rightNbr->originalTkn->_string, 0, tmpTkn, READ_ONLY, lookUpMsg)))	{
						// Make sure our right neighbor is a variable name before moving
						SET_FAILED_ON_SRC_LINE;
					
					} else	{
						currScope.erase(currScope.begin() + (opr8rIdx + 1));
						if (opr8rNode != NULL)	{
							if (opr8rState == ATTACH_1ST && opr8rNode->_1stChild == NULL)	{
								opr8rNode->_1stChild = rightNbr;
								rightNbr->treeParent = opr8rNode;
							
							}	else if ((opr8rState == ATTACH_2ND || opr8rState == ATTACH_BOTH) && opr8rNode->_2ndChild == NULL)	{
								opr8rNode->_2ndChild = rightNbr;
								rightNbr->treeParent = opr8rNode;
							
							}	else	{
                SET_FAILED_ON_SRC_LINE;
							}
						} else	{
							SET_FAILED_ON_SRC_LINE;
						}
					}
				}
			}
		}

		if (isMoveLeftNbr)	{
			if (opr8rIdx - 1 < 0)	{
				SET_FAILED_ON_SRC_LINE;

			} else {
				leftNbr = currScope.at(opr8rIdx - 1);
				if (leftNbr == NULL)	{
					SET_FAILED_ON_SRC_LINE;
				
				} else {
          if (usrSrcTerms.is_viable_var_name(leftNbr->originalTkn->_string))
            // > 0 variable leaf nodes means that this expression isn't fixed|static
            num_var_leaf_nodes++;

          if ((opr8r.type_mask & POSTFIX) && (leftNbr->originalTkn->tkn_type != USER_WORD_TKN 
							|| !usrSrcTerms.is_viable_var_name(leftNbr->originalTkn->_string)
							|| OK != scopedNameSpace->findVar(leftNbr->originalTkn->_string, 0, tmpTkn, READ_ONLY, lookUpMsg)))	{
						// Make sure our left neighbor is a variable name before moving
						SET_FAILED_ON_SRC_LINE;
					
					} else	{
						currScope.erase(currScope.begin() + (opr8rIdx - 1));
						if (opr8rNode != NULL)	{
							if ((opr8rState == ATTACH_1ST || opr8rState == ATTACH_BOTH) && opr8rNode->_1stChild == NULL)	{
								opr8rNode->_1stChild = leftNbr;
								leftNbr->treeParent = opr8rNode;
						
							} else if (opr8rState == ATTACH_2ND && opr8rNode->_2ndChild == NULL)	{
								opr8rNode->_2ndChild = leftNbr;
								leftNbr->treeParent = opr8rNode;
							
							} else	{
                SET_FAILED_ON_SRC_LINE;
							}
						
						} else {
							SET_FAILED_ON_SRC_LINE;
						}
					}
				}
			}
		}

		if (!failed_on_src_line && (isMoveLeftNbr || isMoveRightNbr))	{
			// We tried to do something and didn't fail at it!
			ret_code = OK;
			if ((logLevel == ILLUSTRATIVE && !isExprVarDeclaration) || logLevel > ILLUSTRATIVE)	{
				int updatedOprIdx = opr8rIdx;

				std::wstring bannerMsg;
				if (opr8rState == ATTACH_1ST)	{
					bannerMsg.append(L"Left operand");
					updatedOprIdx--;
				
				}	else if (opr8rState == ATTACH_1ST)	{
					bannerMsg.append(L"Right operand");
				
				} else if (opr8rState == ATTACH_BOTH)	{
					bannerMsg.append(L"Left & right operands");
					updatedOprIdx--;
				}

				bannerMsg.append (L" moved under (like tree branches)");
				bannerMsg.append (L" [" + opr8r.symbol + L"] operator");
				if (!opr8r.description.empty())	{
					bannerMsg.append (L" (");
					bannerMsg.append (opr8r.description);
					bannerMsg.append (L")");
				}
				if (exprScopeStack.size() > 1 || currScope.size() > 1)	{
					// Avoid printing out a completed parse tree, because that happens elsewhere
					int opr8rStartPos;

					printSingleScope (bannerMsg , exprScopeStack.size() - 1, updatedOprIdx, opr8rStartPos, exprScopeStack);
					
					if (updatedOprIdx >= 0 && updatedOprIdx < currScope.size())	{
						std::wcout << L"Parse tree of moved operator " << std::endl;
						displayParseTree (opr8rNode, opr8rStartPos);
 					}
					std::wcout << std::endl;
				}
			}
		}
	}

	return (ret_code);
}

/* ****************************************************************************
 * ***************************************************************************/
 int ExpressionParser::exec_delayed_ternary_2nd (ExprTreeNodePtrVector & currScope, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack) {
  int ret_code = GENERAL_FAILURE;

  for (int idx = 0; idx < currScope.size(); idx++)	{
    if (currScope[idx]->originalTkn->tkn_type == SRC_OPR8R_TKN && currScope[idx]->originalTkn->_string == usrSrcTerms.get_ternary_2nd())	{
      // Special case for [:] OPR8R - set treeParent to [?] 2nd child that at a shallower scope level,
      // NOT attached to one or both of its direct neighbors like a nORmaL OPR8R
      int topIdx = exprScopeStack.size() - 1;
      if (topIdx > 0 && exprScopeStack[topIdx]->myParentScopener != NULL && exprScopeStack[topIdx]->myParentScopener->_2ndChild != NULL)
        currScope[idx]->treeParent = exprScopeStack[topIdx]->myParentScopener->_2ndChild;

      if (OK != moveNeighborsIntoTree (currScope, idx, ATTACH_BOTH, true, true, exprScopeStack))	{
        SET_FAILED_ON_SRC_LINE;
        userMessages->logMsg (INTERNAL_ERROR, L"Failed to attach branches to OPR8R [" + usrSrcTerms.get_ternary_2nd() + L"]"
          , thisSrcFile, __LINE__, 0);
      
      } else {
        ret_code = OK;
      }
      break;
    }
  }

  return ret_code;

}

/* ****************************************************************************
 * ***************************************************************************/
 bool ExpressionParser::is_delay_tern2nd (ExprTreeNodePtrVector & currScope)  {
  bool is_skip = false;

  ExprTreeNodePtrVector::iterator currNodeR8r;
  
  for (currNodeR8r = currScope.begin(); currNodeR8r != currScope.end() && !is_skip; currNodeR8r++)  {
    std::shared_ptr<ExprTreeNode> curr_node = *currNodeR8r;

    if (curr_node->originalTkn->tkn_type == SRC_OPR8R_TKN && curr_node->originalTkn->_string != usrSrcTerms.get_ternary_2nd() 
      && curr_node->_1stChild == NULL && curr_node->_2ndChild == NULL)  {
      // curr_node is an operator, but it isn't the [:] operator, and it has no attachments, meaning it hasn't been 
      // resolved yet.
      is_skip = true;
    }
  }

  return is_skip;
}

/* ****************************************************************************
 * ***************************************************************************/
 int ExpressionParser::exec_prec_lvl_opr8rs (ExprTreeNodePtrVector & currScope,  Opr8rPrecedenceLvl & precedenceLvl
  , bool & is_skip_tern2nd, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack) {


  // TODO: std::wcout << L"**************** BEGIN exec_prec_lvl_opr8rs **************** " << std::endl;

  int ret_code = GENERAL_FAILURE;
  is_skip_tern2nd = false;
  bool isPrecLvlExhausted = false;
  ExprTreeNodePtrVector::iterator currNodeR8r;
  bool isOpenedByTernary = false;

  int topIdx = exprScopeStack.size() - 1;
  if (topIdx >= 0 && exprScopeStack[topIdx]->myParentScopener != NULL)  {
    auto myParent = exprScopeStack[topIdx]->myParentScopener;
    if (myParent->originalTkn->tkn_type == SRC_OPR8R_TKN && myParent->originalTkn->_string == usrSrcTerms.get_ternary_1st())
      isOpenedByTernary = true;
  }

  while (!isPrecLvlExhausted && !failed_on_src_line)	{
    // Not done w/ precedence level until we check each operator node in the current list for a match
    opr8rReadyState opr8rState = OPR8R_NOT_READY;
    int startSize = currScope.size();
    bool isMoveLeftNbr = false;
    bool isMoveRightNbr = false;
    int nodeListIdx = 0;
    bool isOpr8rExecd = false;
    bool isEndOfNodeList = false;

    currNodeR8r = currScope.begin();

    while (!failed_on_src_line && !isEndOfNodeList && !failed_on_src_line && !isOpr8rExecd) {
      // Move left-to-right through the expression. Do some work if there's a match to an OPR8R at the current precedence level
      // When objects are moved around, the iterator will be invalidated and need to restart.
      if (currNodeR8r == currScope.end()) {
        isEndOfNodeList = true;            

      } else {
        std::shared_ptr<ExprTreeNode> currNode = *currNodeR8r;
        // Make an alias variable for code readability
        std::shared_ptr <Token> currTkn = currNode->originalTkn;
  
        if (currTkn->tkn_type == SRC_OPR8R_TKN) {
          // Check if this operator is contained in our current precedence level, and needs to get executed
          bool isPrecLvlSrchDone = false;
          std::list<Operator>::iterator precItr8r = precedenceLvl.opr8rs.begin(); 

          Operator node_opr8r;
          usrSrcTerms.getExecOpr8rDetails(usrSrcTerms.getOpCodeFor(currTkn->_string), node_opr8r);
          
          while (!failed_on_src_line && !isPrecLvlSrchDone && !isOpr8rExecd)  {
            if (precItr8r == precedenceLvl.opr8rs.end())  {
              // Check if we're done
              isPrecLvlSrchDone = true;
              break;
            }

            if ((precItr8r->valid_usage & GNR8D_SRC) && precItr8r->op_code == node_opr8r.op_code)  {
              // Filter out OPR8Rs only valid for USER_SRC ([pre|post]-fix ++,--; unary +|- ) and match on unique GNR8D_SRC equivalent
  
              if (isOpenedByTernary && node_opr8r.op_code == TERNARY_2ND_OPR8R_OPCODE)	{
                // We need to special case SKIP the [:] OPR8R in this proc and make its precedence LOWER than anything else in the current scope
                // to ensure any *contained* assignment OPR8Rs get treated as higher priority and are pushed deeper into the tree,
                // even though the C OPR8R precedence declares [:] precedence > precedence of [=] [+=] [-=] [*=] [/=] [%=] [<<=] [>>=] [&=] [|=] [^=]
                is_skip_tern2nd = is_delay_tern2nd (currScope);
              
              } 
              
              if (!is_skip_tern2nd)  {
                // IFF OPR8R at current precedence level is encountered at this expression scope, we can move some|all neighbors under this OPR8R
                // TODO: std::wcout << L"Next precedence OPR8R: " << node_opr8r.symbol << std::endl;
                if (node_opr8r.op_code == TERNARY_1ST_OPR8R_OPCODE)	{
                  if (currNode->_1stChild == NULL && currNode->_2ndChild != NULL)	{
                    // TERNARY_1ST [TRUE|FALSE] paths resolved; conditional to left should be resolved and ready to be our _1stChild
                    opr8rState = ATTACH_1ST;
                    isMoveLeftNbr = true;
                  }
                } else	{
                  // For this OPR8R, determine which neighbors get removed from list and attached to OPR8R 
                  if ((node_opr8r.type_mask & UNARY) && currNode->_1stChild == NULL)	{
                    opr8rState = ATTACH_1ST;
                    isMoveRightNbr = true;
                  
                  } else if ((node_opr8r.type_mask & PREFIX) && currNode->_1stChild == NULL)	{
                    opr8rState = ATTACH_1ST;
                    isMoveRightNbr = true;
  
                  } else if ((node_opr8r.type_mask & POSTFIX) && currNode->_1stChild == NULL)	{
                    opr8rState = ATTACH_1ST;
                    isMoveLeftNbr = true;
  
                  } else if ((node_opr8r.type_mask & BINARY) && currNode->_1stChild == NULL && currNode->_2ndChild == NULL)	{
                    opr8rState = ATTACH_BOTH;
                    isMoveLeftNbr = true;
                    isMoveRightNbr = true;
                  }
                }

                if (opr8rState != OPR8R_NOT_READY)  {
                  // Ready to execute this operator
                  if (OK != moveNeighborsIntoTree (currScope, nodeListIdx, opr8rState, isMoveLeftNbr, isMoveRightNbr, exprScopeStack)) {
                    SET_FAILED_ON_SRC_LINE;
                  
                  } else {
                    isOpr8rExecd = true;
                    opr8rState = OPR8R_NOT_READY;
                  }
                }
              }
            }
            
            precItr8r++;
          }
        }
      }
      if (opr8rState == OPR8R_NOT_READY)  {
        currNodeR8r++;
        nodeListIdx++;
      } 
    }

    if (isEndOfNodeList && opr8rState == OPR8R_NOT_READY) {
      isPrecLvlExhausted = true;
    }
  }

	if (!failed_on_src_line) {
    ret_code = OK;
	}

  return ret_code;
}

/* ****************************************************************************
 * ***************************************************************************/
int ExpressionParser::turnClosedScopeIntoTree (ExprTreeNodePtrVector & currScope, bool isExprClosed, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack)  {
	return turnClosedScopeIntoTree(currScope, false, isExprClosed, exprScopeStack);
}

/* ****************************************************************************
 * For this sub-expression contained in 1 scope level (ie inside 1 set of parentheses)
 * work through the precedence ordered list of OPR8Rs and make a shrubbery! (tree)
 * Should wind up as a single OPR8R at the top of a shrub/tree
 *
 * eg 1 + 3 * 2
 * * has precedence, so it's handled 1st
 * 1 + *
 *     3,2
 * Now handle + OPR8R
 *   +
 * 1   *
 *    3,2
 * Higher precedence operations get pushed further down the tree
 * ***************************************************************************/
int ExpressionParser::turnClosedScopeIntoTree (ExprTreeNodePtrVector & currScope, bool isOpenedByTernary
  , bool isExprClosed, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack)  {
  int ret_code = GENERAL_FAILURE;
  bool isStopFail = false;
  bool isReachedEOL = false;

  ExprTreeNodePtrVector::iterator currNodeR8r;
  std::shared_ptr<ExprTreeNode> currNode = NULL;
  std::list<Opr8rPrecedenceLvl>::iterator outr8r;
  std::list<Operator>::iterator innr8r;
  Opr8rPrecedenceLvl precedenceLvl;
	Operator tern2ndOpr8r;

	// [?]._1stChild is the conditional and should be the resolved expression directly to the left
	// [?]_2ndChild is the [:] OPR8R; [:]._1stChild is the TRUE path; [:]_2ndChild is the FALSE path
	if (((logLevel == ILLUSTRATIVE && !isExprVarDeclaration) || logLevel > ILLUSTRATIVE)
		&& (exprScopeStack.size() > 1 || (exprScopeStack.size() == 1 && exprScopeStack[0]->scopedKids.size() > 1)))	{
		std::wstring bannerMsg = L"Current highest precedence sub-expression closed; compiler has";
		if (!isExprClosed) {
			bannerMsg.append (L" NOT");
    }
	 	bannerMsg.append (L" read in all Tokens for this expression.");
		printScopeStack (bannerMsg, false, exprScopeStack);
	}

  bool is_tern2nd_pending = false, is_tern2nd_skipped = false;
  // TODO: Code inspect, comment, simplify if you can
	for (outr8r = usrSrcTerms.grouped_opr8rs.begin(); outr8r != usrSrcTerms.grouped_opr8rs.end() && !isStopFail && ret_code != OK; outr8r++){
		// Move through each precedence level of OPR8Rs. Note that some precedence levels will have multiple OPR8Rs and they must be
		// treated as having the same precedence, and therefore we can't rely on an ABSOUTE ordering of OPR8R precedence
    precedenceLvl = *outr8r;

    if (OK != exec_prec_lvl_opr8rs (currScope, precedenceLvl, is_tern2nd_skipped, exprScopeStack))  {
      isStopFail = true;

    } else if (currScope.size() == 1)	{
			ret_code = OK;
		
    } else if (is_tern2nd_skipped) {
      is_tern2nd_pending = true;
    }
	}

  if (is_tern2nd_pending)
    ret_code = exec_delayed_ternary_2nd(currScope, exprScopeStack);

  if (ret_code != OK) {
    if (!isStopFail)
      userMessages->logMsg (INTERNAL_ERROR, L"Unhandled error", thisSrcFile, __LINE__, 0);
    else
      userMessages->logMsg (INTERNAL_ERROR, L"Could not make tree", thisSrcFile, __LINE__, 0);
		showDebugInfo (thisSrcFile, __LINE__, exprScopeStack);
	}


	return (ret_code);
}

/* ****************************************************************************
 * When an unexpected Token type is encountered in an expression, this proc will
 * create a user friendly string that indicates what type of Token(s) would have
 * been legal at the current point inside an expression.
 * ***************************************************************************/
std::wstring ExpressionParser::makeExpectedTknTypesStr (uint32_t expected_tkn_types)	{
	std::wstring expected;

	if (VAR_NAME_NXT_OK & expected_tkn_types)  {
		if (expected.length() > 0)
			expected.append(L"|");
		expected.append(L"USER_WORD (variable name)");
	}

	if (LITERAL_NXT_OK & expected_tkn_types)  {
		if (expected.length() > 0)
			expected.append(L"|");
		expected.append(L"LITERAL");
	}

	if (PREFIX_OPR8R_NXT_OK & expected_tkn_types)  {
		if (expected.length() > 0)
			expected.append(L"|");
		expected.append(L"PREFIX_OPR8R");
	}

	if (UNARY_OPR8R_NXT_OK & expected_tkn_types)  {
		if (expected.length() > 0)
			expected.append(L"|");
		expected.append(L"UNARY_OPR8R");
	}

	if (POSTFIX_OPR8R_NXT_OK & expected_tkn_types)  {
		if (expected.length() > 0)
			expected.append(L"|");
		expected.append(L"POSTFIX_OPR8R");
	}

	if (BINARY_OPR8R_NXT_OK & expected_tkn_types)  {
		if (expected.length() > 0)
			expected.append(L"|");
		expected.append(L"BINARY_OPR8R");
	}

	if (TERNARY_OPR8R_1ST_NXT_OK & expected_tkn_types)  {
		if (expected.length() > 0)
			expected.append(L"|");
		expected.append(usrSrcTerms.get_ternary_1st());
	}

	if (TERNARY_OPR8R_2ND_NXT_OK & expected_tkn_types)  {
		if (expected.length() > 0)
			expected.append(L"|");
		expected.append(usrSrcTerms.get_ternary_2nd());
	}

	if (OPEN_PAREN_NXT_OK & expected_tkn_types)  {
		if (expected.length() > 0)
			expected.append(L"|");
		expected.append(L"(");
	}

	if (CLOSE_PAREN_NXT_OK & expected_tkn_types)  {
		if (expected.length() > 0)
			expected.append(L"|");
		expected.append(L")");
	}

	if (DCLR_VAR_OR_FXN_NXT_OK & expected_tkn_types)  {
		if (expected.length() > 0)
			expected.append(L"|");
		expected.append(L"Variable or fxn declaration");
	}

	if (SYSTEM_CALL_NXT_OK & expected_tkn_types)  {
		if (expected.length() > 0)
			expected.append(L"|");
		expected.append (L"SYSTEM_CALL");
	}

	if (USER_FXN_CALL_NXT_OK & expected_tkn_types)  {
		if (expected.length() > 0)
			expected.append(L"|");
		expected.append (L"USER_FXN_CALL");
	}

	expected.insert (0, L"[");
	expected.append(L"]");

	return expected;
}

/* ****************************************************************************
 * Check if the TERNARY_1st OPR8R was encountered previously at the currently
 * opened scope.
 * ***************************************************************************/
bool ExpressionParser::isTernaryOpen (std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack)  {
  bool isT3rnOpen = false;

	int stackSize = exprScopeStack.size();
	if (stackSize >= 1)	{
		std::shared_ptr<NestedScopeExpr> currScope = exprScopeStack[stackSize - 1];

		if (currScope->myParentScopener != 0)	{
			std::shared_ptr<ExprTreeNode> scopener = (std::shared_ptr<ExprTreeNode>)currScope->myParentScopener;

			if (scopener->originalTkn->tkn_type == SRC_OPR8R_TKN
					&& (TERNARY_1ST & usrSrcTerms.get_type_mask(scopener->originalTkn->_string)))
				isT3rnOpen = true;
		}
	}

	return (isT3rnOpen);
}

/* ****************************************************************************
 * Return the count of TERNARY_2ND OPR8Rs encountered at the current scope.
 * There should be only 1 per scope
 * ***************************************************************************/
int ExpressionParser::get2ndTernaryCnt (std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack)  {
  int count = 0;

	int stackSize = exprScopeStack.size();
	if (stackSize > 0)	{
		std::shared_ptr<NestedScopeExpr> currScope = exprScopeStack[exprScopeStack.size() - 1];
		count = currScope->ternary2ndCnt;
	}

	return count;
}

/* ****************************************************************************
 * Centralized logic determines if the current Token is legal given the allowed
 * Token types. If legal, sets the type(s) the legal types for the next Token.
 * Think of this fxn as the guts of a state machine that determines if the next
 * state can be reached by the current state.  If not, the expression is not
 * well formed.
 * TODO: Handle void() system calls and user fxn calls?
 * ***************************************************************************/
bool ExpressionParser::isExpectedTknType (uint32_t allowed_tkn_types, uint32_t & next_legal_tkn_types, std::shared_ptr<Token> curr_tkn
  , std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack)  {
  bool isTknTypeOK = false;

  if (curr_tkn != NULL)	{
    next_legal_tkn_types = 0x0;
  	// Grab any disambiguated EXEC_OPR8R strings early to make life easier
  	std::wstring execPrefixOpr8rStr = usrSrcTerms.getUniqExecOpr8rStr(curr_tkn->_string, PREFIX);
  	std::wstring execUnaryOpr8rStr = usrSrcTerms.getUniqExecOpr8rStr(curr_tkn->_string, UNARY);
  	std::wstring execPostfixOpr8rStr = usrSrcTerms.getUniqExecOpr8rStr(curr_tkn->_string, POSTFIX);
  	std::wstring execBinaryOpr8rStr = usrSrcTerms.getUniqExecOpr8rStr(curr_tkn->_string, BINARY);

  	if ((allowed_tkn_types & VAR_NAME_NXT_OK) && curr_tkn->tkn_type == USER_WORD_TKN)	{
 	  	// VAR_NAME
  		// TODO: && found in NameSpace.  If it's not in the NameSpace, we can accumulate this error but still keep
  		// compiling and looking for additional user errors.  Same same for compile time interpretation.
			std::wstring lookUpMsg;
  		if (OK != scopedNameSpace->findVar(curr_tkn->_string, 0, scratchTkn, READ_ONLY, lookUpMsg))	{
					userMessages->logMsg (USER_ERROR, L"Variable " + curr_tkn->_string + L" was not declared"
						, userSrcFileName, curr_tkn->get_line_number(), curr_tkn->get_column_pos());
  		}

  		isTknTypeOK = true;
  		next_legal_tkn_types = (POSTFIX_OPR8R_NXT_OK|BINARY_OPR8R_NXT_OK|TERNARY_OPR8R_1ST_NXT_OK|CLOSE_PAREN_NXT_OK);
  		if (isTernaryOpen(exprScopeStack))
  			next_legal_tkn_types |= TERNARY_OPR8R_2ND_NXT_OK;

    } else if ((allowed_tkn_types & SYSTEM_CALL_NXT_OK) && curr_tkn->tkn_type == SYSTEM_CALL_TKN) {
      // TODO: What about sys calls that return void?
  		isTknTypeOK = true;
  		next_legal_tkn_types = (BINARY_OPR8R_NXT_OK|TERNARY_OPR8R_1ST_NXT_OK|CLOSE_PAREN_NXT_OK);

  	} else if ((allowed_tkn_types & LITERAL_NXT_OK) && curr_tkn->tkn_type == STRING_TKN || curr_tkn->tkn_type == DATETIME_TKN
  			|| curr_tkn->tkn_type == UINT8_TKN || curr_tkn->tkn_type == UINT16_TKN
				|| curr_tkn->tkn_type == UINT32_TKN || curr_tkn->tkn_type == UINT64_TKN
  			|| curr_tkn->tkn_type == INT8_TKN || curr_tkn->tkn_type == INT16_TKN
				|| curr_tkn->tkn_type == INT32_TKN || curr_tkn->tkn_type == INT64_TKN
				|| curr_tkn->tkn_type == DOUBLE_TKN || curr_tkn->tkn_type == BOOL_TKN)	{
    	// LITERAL
  		isTknTypeOK = true;
  		next_legal_tkn_types = (BINARY_OPR8R_NXT_OK|TERNARY_OPR8R_1ST_NXT_OK|CLOSE_PAREN_NXT_OK);
  		if (isTernaryOpen(exprScopeStack))
  			next_legal_tkn_types |= TERNARY_OPR8R_2ND_NXT_OK;

  	} else if ((allowed_tkn_types & PREFIX_OPR8R_NXT_OK) && curr_tkn->tkn_type == SRC_OPR8R_TKN && !execPrefixOpr8rStr.empty())	{
    	// PREFIX_OPR8R
			// Make user source OPR8R unambiguous for internal use
			curr_tkn->_string = execPrefixOpr8rStr;
			isTknTypeOK = true;
			next_legal_tkn_types = (VAR_NAME_NXT_OK);

  	} else if ((allowed_tkn_types & UNARY_OPR8R_NXT_OK) && curr_tkn->tkn_type == SRC_OPR8R_TKN && !execUnaryOpr8rStr.empty())	{
  		// Make user source OPR8R unambiguous for internal use
 			curr_tkn->_string = execUnaryOpr8rStr;
			isTknTypeOK = true;
			next_legal_tkn_types = (VAR_NAME_NXT_OK|LITERAL_NXT_OK|SYSTEM_CALL_NXT_OK|OPEN_PAREN_NXT_OK);

  	} else if ((allowed_tkn_types & POSTFIX_OPR8R_NXT_OK) && curr_tkn->tkn_type == SRC_OPR8R_TKN && !execPostfixOpr8rStr.empty())	{
			// Make user source OPR8R unambiguous for internal use
			curr_tkn->_string = execPostfixOpr8rStr;
			isTknTypeOK = true;
			next_legal_tkn_types = (BINARY_OPR8R_NXT_OK|TERNARY_OPR8R_1ST_NXT_OK|CLOSE_PAREN_NXT_OK);
			if (isTernaryOpen(exprScopeStack))
				next_legal_tkn_types |= TERNARY_OPR8R_2ND_NXT_OK;

  	} else if ((allowed_tkn_types & TERNARY_OPR8R_2ND_NXT_OK) && curr_tkn->tkn_type == SRC_OPR8R_TKN
  			&& (TERNARY_2ND & usrSrcTerms.get_type_mask(curr_tkn->_string)))	{
  		isTknTypeOK = true;
  		// TODO: Is PREFIX_OPR8R legit here?
  		next_legal_tkn_types = (VAR_NAME_NXT_OK|LITERAL_NXT_OK|SYSTEM_CALL_NXT_OK|PREFIX_OPR8R_NXT_OK|UNARY_OPR8R_NXT_OK|OPEN_PAREN_NXT_OK);

  	} else if ((allowed_tkn_types & TERNARY_OPR8R_1ST_NXT_OK) && curr_tkn->tkn_type == SRC_OPR8R_TKN
  			&& (TERNARY_1ST & usrSrcTerms.get_type_mask(curr_tkn->_string)))	{
    	// TERNARY_OPR8R
  		isTknTypeOK = true;
  		next_legal_tkn_types = (VAR_NAME_NXT_OK|LITERAL_NXT_OK|SYSTEM_CALL_NXT_OK|PREFIX_OPR8R_NXT_OK|UNARY_OPR8R_NXT_OK|OPEN_PAREN_NXT_OK);

  	} else if ((allowed_tkn_types & BINARY_OPR8R_NXT_OK) && curr_tkn->tkn_type == SRC_OPR8R_TKN && !execBinaryOpr8rStr.empty())	{
    	// BINARY_OPR8R
 			curr_tkn->_string = execBinaryOpr8rStr;
			next_legal_tkn_types = (VAR_NAME_NXT_OK|LITERAL_NXT_OK|SYSTEM_CALL_NXT_OK|PREFIX_OPR8R_NXT_OK|UNARY_OPR8R_NXT_OK|OPEN_PAREN_NXT_OK);
			isTknTypeOK = true;

  	} else if ((allowed_tkn_types & OPEN_PAREN_NXT_OK) && SPR8R_TKN == curr_tkn->tkn_type && 0 == curr_tkn->_string.compare(L"("))	{
    	// OPEN_PAREN
  		isTknTypeOK = true;
  		next_legal_tkn_types = (VAR_NAME_NXT_OK|LITERAL_NXT_OK|SYSTEM_CALL_NXT_OK|PREFIX_OPR8R_NXT_OK|UNARY_OPR8R_NXT_OK|OPEN_PAREN_NXT_OK);

  	} else if ((allowed_tkn_types & CLOSE_PAREN_NXT_OK) && SPR8R_TKN == curr_tkn->tkn_type && 0 == curr_tkn->_string.compare(L")"))	{
    	// CLOSE_PAREN
  		isTknTypeOK = true;
  		next_legal_tkn_types = (BINARY_OPR8R_NXT_OK|TERNARY_1ST|CLOSE_PAREN_NXT_OK);

  		if (isTernaryOpen(exprScopeStack))
  			next_legal_tkn_types |= TERNARY_OPR8R_2ND_NXT_OK;

  	} else if ((allowed_tkn_types & DCLR_VAR_OR_FXN_NXT_OK) && USER_WORD_TKN == curr_tkn->tkn_type && usrSrcTerms.is_valid_datatype(curr_tkn->_string))	{
  		// DCLR_VAR_OR_FXN
  		// TODO: isTknTypeOK = true;
  		// TODO: This should be handled by GeneralParser
  		next_legal_tkn_types = (0x0);
  	}

  	// TODO: FXN_CALL
  }

  return (isTknTypeOK && 0 != next_legal_tkn_types);
}

/* ****************************************************************************
 * Called after an expression has been compiled.  Releases memory and gets back
 * to an initial state.
 * ***************************************************************************/
void ExpressionParser::cleanScopeStack(std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack)	{
	std::vector<std::shared_ptr<NestedScopeExpr>>::reverse_iterator scopeR8r;
	std::vector<std::shared_ptr<ExprTreeNode>>::reverse_iterator kidR8r;
	int lastKidIdx;

	// Clean up our allocated memory
	for (scopeR8r = exprScopeStack.rbegin(); scopeR8r != exprScopeStack.rend(); scopeR8r++)	{
		std::shared_ptr<NestedScopeExpr> currScope = *scopeR8r;

		while (!currScope->scopedKids.empty())	{
			lastKidIdx = currScope->scopedKids.size() - 1;
			std::shared_ptr<ExprTreeNode> lastKid = currScope->scopedKids[lastKidIdx];
			currScope->scopedKids.pop_back();
			lastKid.reset();
		}

		exprScopeStack.pop_back();
		currScope.reset();
	}
}

/* ****************************************************************************
 * What indicates the ending legal Token that closes out the expression?
 * Depends on what opened it up
 * OPEN_PAREN - Expression ends with a ")" of the same scope
 * VAR_NAME - Expression ends with a ";"
 * LITERAL - Expression ends with a ";"
 * FXN_CALL - Begin Token is a USER_WORD representing a previously defined fxn call.  Expression ends with a ";"
 * PREFIX_OPR8R - e.g. ++idx;  Expression ends with a ";"
 * 	TODO:  ++(idx); ++((idx)); ++(((idx))) + jdx++; etc. are legal statements
 * UNARY_OPR8R - A bit non-sensical, but is still legal (e.g. ~jdx;).  Expression ends with a ";"
 *
 * DCLR_VAR_OR_FXN - Begin Token is a USER_WORD representing a datatype.  Expression ends with a ";"
 * 	TODO: Handle this via a different fxn to make life easier.
 *
 * ***************************************************************************/
int ExpressionParser::getExpectedEndToken (std::shared_ptr<Token> startTkn, uint32_t & _1stTknType, Token & expectedEndTkn, expr_ender_type ended_by)	{
	int ret_code = GENERAL_FAILURE;
	bool isStopFail = false;
  _1stTknType = 0x0;

	if (startTkn != NULL)	{
		expectedEndTkn.resetToken();
	
		if (startTkn->tkn_type == SPR8R_TKN && startTkn->_string == L"(")	{
			expectedEndTkn.tkn_type = SPR8R_TKN;
			expectedEndTkn._string = L")";
			_1stTknType = OPEN_PAREN_NXT_OK;

		} else if (startTkn->tkn_type == USER_WORD_TKN)	{
			// TODO: Must be [VAR_NAME|FXN_CALL] that currently exists in the NameSpace
			_1stTknType = (VAR_NAME_NXT_OK);

		} else if (startTkn->tkn_type == SRC_OPR8R_TKN && (UNARY & usrSrcTerms.get_type_mask(startTkn->_string)))	{
			_1stTknType = UNARY_OPR8R_NXT_OK;

		} else if (startTkn->tkn_type == SRC_OPR8R_TKN && (startTkn->_string == usrSrcTerms.getSrcOpr8rStrFor (PRE_INCR_OPR8R_OPCODE)
					|| startTkn->_string == usrSrcTerms.getSrcOpr8rStrFor (PRE_DECR_OPR8R_OPCODE)))	{
			_1stTknType = PREFIX_OPR8R_NXT_OK;

		} else if (startTkn->tkn_type == STRING_TKN
			|| startTkn->tkn_type == DATETIME_TKN
			|| startTkn->tkn_type == BOOL_TKN
			|| startTkn->tkn_type == UINT8_TKN
			|| startTkn->tkn_type == UINT16_TKN
			|| startTkn->tkn_type == UINT32_TKN
			|| startTkn->tkn_type == UINT64_TKN
			|| startTkn->tkn_type == INT8_TKN
			|| startTkn->tkn_type == INT16_TKN
			|| startTkn->tkn_type == INT32_TKN
			|| startTkn->tkn_type == INT64_TKN
			|| startTkn->tkn_type == DOUBLE_TKN)	{
				// TODO: I don't understand why I put this here....
			// expectedEndTkn.tkn_type = SRC_OPR8R_TKN;
			_1stTknType = LITERAL_NXT_OK;

    } else if (startTkn->tkn_type == SYSTEM_CALL_TKN) {
      _1stTknType = SYSTEM_CALL_NXT_OK;

		} else	{
			isStopFail = true;
  		userMessages->logMsg (INTERNAL_ERROR, L"Could not determine _1stTknType for " + startTkn->descr_sans_line_num_col(), thisSrcFile, __LINE__, 0);
		}

		if (ended_by == ENDS_IN_PARENTHESES)  {
      if (_1stTknType != OPEN_PAREN_NXT_OK)  {
        isStopFail = true;
        userMessages->logMsg(USER_ERROR, L"Expected opening \"(\" but got " + startTkn->descr_sans_line_num_col(), userSrcFileName, startTkn->get_line_number()
        , startTkn->get_column_pos());
      
      } else {
        expectedEndTkn.resetToken();
        expectedEndTkn.tkn_type = SPR8R_TKN;
        expectedEndTkn._string = L")";
      }
    
    } else  if (ended_by == ENDS_IN_COMMA)	{
			expectedEndTkn.resetToken();
			expectedEndTkn.tkn_type = SPR8R_TKN;
			expectedEndTkn._string = L",";
		
    } else if (ended_by == ENDS_IN_STATEMENT_ENDER) {
			std::wstring statementEnder = usrSrcTerms.get_statement_ender();
			if (statementEnder.length() > 0)	{
				expectedEndTkn.tkn_type = SRC_OPR8R_TKN;
				expectedEndTkn._string = statementEnder;

			} else	{
				isStopFail = true;
	  		userMessages->logMsg (INTERNAL_ERROR, L"Could not find STATEMENT_ENDER operator! ", thisSrcFile, __LINE__, 0);
			}
		}

	  if (!isStopFail && 0 != _1stTknType)
	  	ret_code = OK;
	}

	return (ret_code);
}

/* ****************************************************************************
 * Used an aid in debugging and instruction
 * ***************************************************************************/
void ExpressionParser::printSingleScope (std::wstring headerMsg, int scopeLvl, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack)	{
	int tmpPos;

	printSingleScope(headerMsg, scopeLvl, -1, tmpPos, exprScopeStack);
}


/* ****************************************************************************
 * Used an aid in debugging and instruction
 * ***************************************************************************/
 void ExpressionParser::printSingleScope (std::wstring headerMsg, int scopeLvl
	, int tgtIdx, int & tgtStartPos, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack)	{
	
	tgtStartPos = 0;

	if (scopeLvl >=0 && scopeLvl < exprScopeStack.size())	{
		std::wstring outLine;

		if (!headerMsg.empty())
			std::wcout << headerMsg << std::endl;

		outLine.append(L"Scope Level ");
		outLine.append (std::to_wstring(scopeLvl));
		outLine.append (L": ");
		
		std::shared_ptr<NestedScopeExpr> chosenScope = exprScopeStack[scopeLvl];
		std::vector<std::shared_ptr<ExprTreeNode>>::iterator kidR8r;
		int currIdx = 0;

		for (kidR8r = chosenScope->scopedKids.begin(); kidR8r != chosenScope->scopedKids.end(); kidR8r++)	{
			if (currIdx == tgtIdx)
				tgtStartPos = outLine.length();

			std::shared_ptr<ExprTreeNode> currKid = *kidR8r;
			if (currKid->_1stChild != NULL)
				// Give the user a visual hint that it's a tree
				outLine.append (L"/");
			else
				outLine.append (L"[");
			outLine.append (currKid->originalTkn->getValueStr());
			if (currKid->_2ndChild != NULL)
				// Give the user a visual hint that it's a tree
				outLine.append (L"\\");
			else
				outLine.append (L"]");

			currIdx++;				
		}

		std::wcout << outLine << std::endl;

		if (!headerMsg.empty())
			std::wcout << std::endl;

	}
}

/* ****************************************************************************
 * Used an aid in debugging and instruction
 * ***************************************************************************/
void ExpressionParser::printScopeStack (std::wstring fileName, int lineNumber
  , std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack)	{

	std::wstringstream banner;
	banner << L"********** ExpressionParser::printScopeStack called from " << fileName << L":" << lineNumber << L" **********";
	printScopeStack (banner.str(), false, exprScopeStack);

}

/* ****************************************************************************
 * Used an aid in debugging and instruction
 * ***************************************************************************/
 void ExpressionParser::printScopeStack (std::wstring bannerMsg, bool isUseDefault
  , std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack)	{
	std::wstring defaultMsg = L"Compiler's expression; may be incomplete. Scope levels > 0 opened by [(] or [?] from previous level.";
	defaultMsg.append (L"\n[] contains a single OPR8R, e.g., [*]. /\\ contains a tree with left and|or right operands beneath it, e.g., /*\\");

	if (isUseDefault && !defaultMsg.empty())
		std::wcout << defaultMsg << std::endl;

	else if (!isUseDefault && !bannerMsg.empty())
		std::wcout << bannerMsg << std::endl;

	std::vector<std::shared_ptr<NestedScopeExpr>>::reverse_iterator scopeR8r;
	std::vector<std::shared_ptr<ExprTreeNode>>::iterator kidR8r;
	int scopeLvl = exprScopeStack.size() - 1;

	for (scopeR8r = exprScopeStack.rbegin(); scopeR8r != exprScopeStack.rend(); scopeR8r++)	{
		
		printSingleScope (L"", scopeLvl, exprScopeStack);
		scopeLvl--;
	}

 }

/* ****************************************************************************
 * Used an aid in debugging.
 * ***************************************************************************/
void ExpressionParser::showDebugInfo (std::wstring srcFileName, int lineNum, std::vector<std::shared_ptr<NestedScopeExpr>> & exprScopeStack)	{
	printScopeStack(srcFileName, lineNum, exprScopeStack);
	if (exprScopeStack.size() > 0)	{
		ExprTreeNodePtrVector::iterator nodeR8r;

		ExprTreeNodePtrVector & childList = exprScopeStack[exprScopeStack.size() - 1]->scopedKids;

		for (nodeR8r = childList.begin(); nodeR8r != childList.end(); nodeR8r++)	{
			std::shared_ptr<ExprTreeNode> currNode = *nodeR8r;
			displayParseTree(currNode, 0);
		}
	}
}

/* ****************************************************************************
 * Procedure used in displaying the compiler's parse tree for an expression
 * Common fxn to build a string that describes:
 * Leaf node 			-> [4]
 * Binary branch 	-> /+\
 * Single child		-> /~] or [?\
 * ***************************************************************************/
 std::wstring ExpressionParser::makeTreeNodeStr (std::shared_ptr<ExprTreeNode> treeNode)	{
	std::wstring nodeStr;
	
	if (treeNode->_1stChild != NULL)
		nodeStr.append (L"/");
	else
	 	nodeStr.append (L"[");

	nodeStr.append (treeNode->originalTkn->getValueStr());

	if (treeNode->_2ndChild != NULL)
		nodeStr.append (L"\\");
	else
	 	nodeStr.append (L"]");

	return (nodeStr);
}

 /* ****************************************************************************
 * Procedure used in displaying the compiler's parse tree for an expression
 * Get the longest "line" by grabbing the greatest displayEndPos
 * Used to determine how much space is needed for the left side of the whole tree
 * ***************************************************************************/
 void ExpressionParser::getMaxLineLen (std::vector<std::vector<std::shared_ptr<ExprTreeNode>>> & treeLvlNodeLists
    , bool isLefty, int & maxLineLen)	{

	int odx;
	int idx;
	int outerChildPos;
	maxLineLen = 0;

	for (odx = 0; odx < treeLvlNodeLists.size(); odx++)	{
		std::vector<std::shared_ptr<ExprTreeNode>> currLvlList = treeLvlNodeLists[odx];
		int currListSize = currLvlList.size();

		for (idx = 0; idx < currListSize; idx++)	{
			// Work through all the BNIs at this level
			std::shared_ptr<ExprTreeNode> currNode = currLvlList.at(idx);

			int lineOuterSidePos;
			std::shared_ptr<ExprTreeNode> lastEntry;
			if (isLefty && !currLvlList.empty())	{
				// Get max pos from the END of the list
				lastEntry = currLvlList.at(currListSize - 1);
        if (lastEntry->displayEndPos > maxLineLen)
					maxLineLen = lastEntry->displayEndPos;
					
			} else if (!currLvlList.empty()) {
				// Get max pos from the FRONT of the list
				lastEntry = currLvlList.at(0);
        if (lastEntry->displayEndPos > maxLineLen)
					maxLineLen = lastEntry->displayEndPos;
			}
		}
	}
}

/* ****************************************************************************
 * Procedure used in displaying the compiler's parse tree for an expression
 * Put the left half of the completed parse tree into displayLines, which is 
 * displayed for user consumption
 * ***************************************************************************/
 int ExpressionParser::fillDisplayLeft (std::vector<std::wstring> & displayLines, std::vector<std::vector<std::shared_ptr<ExprTreeNode>>> & treeLvlNodeLists
	, int maxLineLen)	{
	int ret_code = GENERAL_FAILURE;

	std::wstring nextLine;
	int odx = 0;
	int idx = 0;
	int numBlanks;

	if (displayLines.size() >= treeLvlNodeLists.size() + 1)	{

		for (odx = 0; odx < treeLvlNodeLists.size(); odx++)	{
			std::vector<std::shared_ptr<ExprTreeNode>> currLvlList = treeLvlNodeLists[odx];
			nextLine.clear();

			for (idx = 0; idx < currLvlList.size(); idx++)	{
				// Build line from current expression scope level
				// from the center leftwards, or from R2L
				std::shared_ptr<ExprTreeNode> currBranch = currLvlList.at(idx);
				numBlanks = currBranch->displayStartPos - nextLine.size();
				if (numBlanks > 0)
					nextLine.insert (nextLine.begin(), numBlanks, L' ');

				nextLine = makeTreeNodeStr(currBranch) + nextLine;
			}

			// Throw some left padding on the front before committing this line
			numBlanks = maxLineLen - nextLine.size();
			if (numBlanks > 0)
				nextLine.insert (nextLine.begin(), numBlanks, L' ');

			// Commit the left half
			displayLines[odx + 1] = nextLine;
		}

		for (int blankIdx = odx + 1; blankIdx < displayLines.size(); blankIdx++)	{
			// For any lines from rightList but missing from leftList, 
			// fill left half w/ blanks
			displayLines[blankIdx].insert (displayLines[blankIdx].begin(), maxLineLen, L' ');
		}

    ret_code = OK;

  }

	return (ret_code);

}

/* ****************************************************************************
 * Procedure used in displaying the compiler's parse tree for an expression
 * Put the right half of the completed parse tree into displayLines, which is 
 * displayed for user consumption
 * ***************************************************************************/
 int ExpressionParser::fillDisplayRight (std::vector<std::wstring> & displayLines, std::vector<std::vector<std::shared_ptr<ExprTreeNode>>> & treeLvlNodeLists
	, int centerGapSpaces)	{

	int ret_code = GENERAL_FAILURE;
	std::wstring nextLine;
	int odx = 0;
	int idx = 0;
	size_t numBlanks;

	if (displayLines.size() >= treeLvlNodeLists.size() + 1)	{

		for (odx = 0; odx < treeLvlNodeLists.size(); odx++)	{
			std::vector<std::shared_ptr<ExprTreeNode>> currLvlList = treeLvlNodeLists[odx];
			nextLine.clear();

			for (idx = 0; idx < currLvlList.size(); idx++)	{
				// Build line from current expression scope level
				// from the center rightwards, or from left-to-right
				std::shared_ptr<ExprTreeNode> currBranch = currLvlList.at(idx);
				if (currBranch->displayStartPos > 0 && currBranch->displayStartPos > nextLine.size())	{
					numBlanks = currBranch->displayStartPos - nextLine.size();
					if (numBlanks > 0)
						nextLine.insert ( nextLine.end(), numBlanks, L' ');
				}
				nextLine.append (makeTreeNodeStr(currBranch));
			}

			if (centerGapSpaces > 0)
        // Prepend the center gap now; don't mess up earlier blank stuffing 
        nextLine.insert ( nextLine.begin(), centerGapSpaces, L' ');

			displayLines[odx + 1].append(nextLine);

		}

    ret_code = OK;
	}

	return (ret_code);
 }

/* ****************************************************************************
 * Procedure used in displaying the compiler's parse tree for an expression
 * Determine if the current ExprTreeNode is towards the center of the display
 * tree in its node pair, or towards the outside
 * ***************************************************************************/
 int ExpressionParser::setIsCenterNode (bool isLeftTree, int halfTreeLevel, std::shared_ptr<ExprTreeNode> currBranch) {
  int ret_code = GENERAL_FAILURE;

  if (currBranch != NULL) {
    if (halfTreeLevel == 0 && isLeftTree)  {
      if (currBranch->treeParent != NULL && currBranch == currBranch->treeParent->_1stChild) {
        currBranch->nodePos = CENTER_NODE;
        ret_code = OK;
      }
    } else if (halfTreeLevel == 0 && !isLeftTree) {
      if (currBranch->treeParent != NULL && currBranch == currBranch->treeParent->_2ndChild)  {
        currBranch->nodePos = CENTER_NODE;
        ret_code = OK;
      }
    } else if (isLeftTree)  {
      if (currBranch->treeParent != NULL && currBranch == currBranch->treeParent->_2ndChild)  {
        currBranch->nodePos = CENTER_NODE;
        ret_code = OK;
      
      } else if (currBranch->treeParent != NULL && currBranch == currBranch->treeParent->_1stChild) {
        currBranch->nodePos = OUTER_NODE;
        ret_code = OK;
      }
    } else {
      if (currBranch->treeParent != NULL && currBranch == currBranch->treeParent->_1stChild)  {
        currBranch->nodePos = CENTER_NODE;
        ret_code = OK;
      
      } else if (currBranch->treeParent != NULL && currBranch == currBranch->treeParent->_2ndChild) {
        currBranch->nodePos = OUTER_NODE;
        ret_code = OK;
      }
    }
  }

  assert (ret_code == OK);

  return ret_code;
}


/* ****************************************************************************
 * Procedure used in displaying the compiler's parse tree for an expression
 * 
 * ***************************************************************************/
 int ExpressionParser::displayParseTree (std::shared_ptr<ExprTreeNode> startBranch, std::wstring callersSrcFile, int srcLineNum)	{
	std::wcout << L"********** displayParseTree called from " << callersSrcFile << L":" << srcLineNum << L" **********" << std::endl;

	return (displayParseTree(startBranch, 0));
}


/* ****************************************************************************
 * Procedure used in displaying the compiler's parse tree for an expression
 * Serves as an ILLUSTRATIVE aid
 * After an expression tree has been built, this proc will create a 
 * graphical tree representation for display when operating in ILLUSTRATIVE mode
 * 
 * User source: 
 * result = (one + two) * three * four / six;
 *
 * Parse tree:
 *                        /=\
 *                [result]   //\
 *                        /*\   [six]
 *                 /*\ [four]
 *        /B+\ [three]       
 *  [one][two]               
 * ***************************************************************************/
 int ExpressionParser::displayParseTree (std::shared_ptr<ExprTreeNode> startBranch, int adjustToRight)	{
	int ret_code = GENERAL_FAILURE;

  if (startBranch != NULL)  {
    std::vector<std::wstring> displayLines;
    int leftMaxLineLen;
    bool isTreeRightHeavy = false;
    int ldx;

    if (startBranch->originalTkn->_string == usrSrcTerms.getSrcOpr8rStrFor (ASSIGNMENT_OPR8R_OPCODE)         
      || startBranch->originalTkn->_string == usrSrcTerms.getSrcOpr8rStrFor (PLUS_ASSIGN_OPR8R_OPCODE)        
      || startBranch->originalTkn->_string == usrSrcTerms.getSrcOpr8rStrFor (MINUS_ASSIGN_OPR8R_OPCODE)       
      || startBranch->originalTkn->_string == usrSrcTerms.getSrcOpr8rStrFor (MULTIPLY_ASSIGN_OPR8R_OPCODE)    
      || startBranch->originalTkn->_string == usrSrcTerms.getSrcOpr8rStrFor (DIV_ASSIGN_OPR8R_OPCODE)         
      || startBranch->originalTkn->_string == usrSrcTerms.getSrcOpr8rStrFor (MOD_ASSIGN_OPR8R_OPCODE)         
      || startBranch->originalTkn->_string == usrSrcTerms.getSrcOpr8rStrFor (LEFT_SHIFT_ASSIGN_OPR8R_OPCODE)  
      || startBranch->originalTkn->_string == usrSrcTerms.getSrcOpr8rStrFor (RIGHT_SHIFT_ASSIGN_OPR8R_OPCODE) 
      || startBranch->originalTkn->_string == usrSrcTerms.getSrcOpr8rStrFor (BITWISE_AND_ASSIGN_OPR8R_OPCODE) 
      || startBranch->originalTkn->_string == usrSrcTerms.getSrcOpr8rStrFor (BITWISE_XOR_ASSIGN_OPR8R_OPCODE) 
      || startBranch->originalTkn->_string == usrSrcTerms.getSrcOpr8rStrFor (BITWISE_OR_ASSIGN_OPR8R_OPCODE) )  {

      if (startBranch->_1stChild != NULL && startBranch->_1stChild->_1stChild == NULL && startBranch->_1stChild->_2ndChild == NULL) {
        // Only 1 operand on the left side
        if (startBranch->_2ndChild != NULL && (startBranch->_2ndChild->_1stChild != NULL || startBranch->_2ndChild->_2ndChild != NULL)) {
          // This tree would be RIGHT side heavy if we didn't intervene
          isTreeRightHeavy = true;
        }
      }
    }

    if (isTreeRightHeavy)  {
      if (OK != setFullTreeDisplayPos(startBranch->_2ndChild, displayLines, leftMaxLineLen)) {
        SET_FAILED_ON_SRC_LINE;

      } else {
        // Insert startBranch root at the top of displayLines
        std::wstring realRootStr = makeTreeNodeStr(startBranch);
        int rootStrLen = realRootStr.length();
        std::wstring lvlOneLeftChild = makeTreeNodeStr(startBranch->_1stChild);
        int reqSpace = rootStrLen + lvlOneLeftChild.length();

        if (reqSpace > leftMaxLineLen)  {
          int padSize = (reqSpace - leftMaxLineLen);
          leftMaxLineLen += padSize;
          for (ldx = 0; ldx < displayLines.size(); ldx++) {
            displayLines[ldx].insert (displayLines[ldx].begin(), padSize, L' ');
          }
        }

        int numPrecedingBlanks = leftMaxLineLen - realRootStr.length();
        if (numPrecedingBlanks > 0)
          realRootStr.insert(realRootStr.begin(), numPrecedingBlanks, L' ');
        displayLines.insert(displayLines.begin(), realRootStr);

        // Replace targeted blanks with root's left child into already existing line
        size_t startPos = numPrecedingBlanks - lvlOneLeftChild.length();
        std::wstring level1Str = displayLines[1];
        level1Str.replace(startPos, lvlOneLeftChild.length(), lvlOneLeftChild);
        displayLines[1] = level1Str;
      }
  
    } else if (OK != setFullTreeDisplayPos(startBranch, displayLines, leftMaxLineLen)) {
      SET_FAILED_ON_SRC_LINE;
    }
    
    if (!failed_on_src_line)  {
      std::wstring rightAdjustStr;
  
      if (leftMaxLineLen < adjustToRight)
        rightAdjustStr.insert ( rightAdjustStr.begin(), adjustToRight - leftMaxLineLen, L' ');
  
      for (ldx = 0; ldx < displayLines.size(); ldx++)
        std::wcout << rightAdjustStr << displayLines[ldx] << std::endl;
  
      ret_code = OK;
    }
  }

  return ret_code;
}

/* ****************************************************************************
 * Procedure used in displaying the compiler's parse tree for an expression
 *
 *                              /=\
 *                      [result]   /B+\
 *                              /*\   /?\ #2: Go to my parent
 *                /B+\         /B+\   /<\                         /:\ <-- #1: Where do I position this?
 *        /B+\ [three]  [four][six]   /<<\         /*\            [three][four]
 *  [one][two]                        /*\  [five]  [eight][four]          #3: Find the largest resolved 
 *                                    [six][one]                              outer pos at /*\ and base 
 *                                                                            new start pos from that
 * How is the start position of /:\ determined? The recursion goes from the
 * center to the outside, so we need to go back to what's already been resolved.
 * See steps #1, #, #3 above
 * ***************************************************************************/
 int ExpressionParser::findMaxOuterNodeEndPos (bool isLeftTree, std::shared_ptr<ExprTreeNode> searchBranch, int & maxEndPos)  {
  int ret_code = GENERAL_FAILURE;
  maxEndPos = 0;
  bool isDone = false;

  if (searchBranch != NULL) {
    while (!isDone) {
      if (searchBranch == NULL) {
        isDone = true;
      } else  {
        if (searchBranch->displayEndPos > maxEndPos)
          maxEndPos = searchBranch->displayEndPos;

        searchBranch = isLeftTree ? searchBranch->_1stChild : searchBranch->_2ndChild;
      }
    }
  }

  ret_code = OK;
  return ret_code;
}


/* ****************************************************************************
 * Procedure used in displaying the compiler's parse tree for an expression
 * Set start and end display positions for a leaf node
 * Leaf node's parent is a CENTER_NODE:
 *
 * /B+\
 * /*\                            /?\
 * /B+\               /B+\        /<\                       /:\
 * /B+\       [three] [four][six] /<<\        /*\           [three][four]
 * [one][two]                     /*\ [five]  [eight][four]
 *                            --> [six][one] 
 * What's [six]'s pos? 
 * Immediate parent /*\ is CENTER_NODE
 * Go up the tree until we hit an OUTER_NODE --> /?\ 
 * Jump over to OUTER_NODE's ancestor; pos info probably not be resolved yet!
 * Go down the child branches until we find a resolved OUTER_NODE
 *
 * Leaf node's parent is an OUTER_NODE:
 * 
 *
 * /B+\
 * /*\                            /?\
 * /B+\               /B+\        /<\                       /:\
 * /B+\       [three] [four][six] /<<\        /*\           [three][four]
 * [one][two]         ^^^         /*\ [five]  [eight][four]
 *                                [six][one]
 * 
 * What's [four]'s pos?
 * Immediate parent /B+\ is an OUTER_NODE
 * Go up 1 level to grandparent and jump over to parent's sibling, the /B+\
 * center leaning node and find the biggest endPos underneath it
 * ***************************************************************************/
int ExpressionParser::setCtrStartByPrevBndry (bool isLeftTree, std::shared_ptr<ExprTreeNode> currBranch) {
  int ret_code = GENERAL_FAILURE;
  bool isOutAncestorFnd = false;
  std::shared_ptr<ExprTreeNode> ctrSearchCousin;
  std::shared_ptr<ExprTreeNode> myGrandParent;
  std::shared_ptr<ExprTreeNode> myParent;

  if (currBranch != NULL && currBranch->nodePos == CENTER_NODE) {
      myParent = currBranch->treeParent;

    if (myParent == NULL) {
      SET_FAILED_ON_SRC_LINE;

    } else if (myParent->nodePos == CENTER_NODE)  {
      myGrandParent = myParent->treeParent;
      if (isLeftTree && myGrandParent != NULL && myGrandParent->_1stChild != NULL)
        ctrSearchCousin = myGrandParent->_1stChild;
      else if (!isLeftTree && myGrandParent != NULL && myGrandParent->_2ndChild != NULL)
        ctrSearchCousin = myGrandParent->_2ndChild;

    } else if (myParent->nodePos == OUTER_NODE) {
      myGrandParent = myParent->treeParent;
      if (isLeftTree && myGrandParent != NULL && myGrandParent->_2ndChild != NULL)
        ctrSearchCousin = myGrandParent->_2ndChild;
      else if (!isLeftTree && myGrandParent != NULL && myGrandParent->_1stChild != NULL)
        ctrSearchCousin = myGrandParent->_1stChild;
    }
    

    if (ctrSearchCousin != NULL)  {
      // Go down the child branches until we find a resolved OUTER_NODE
      int maxEndPos = 0;
      if (OK != findMaxOuterNodeEndPos (isLeftTree, ctrSearchCousin, maxEndPos))
        SET_FAILED_ON_SRC_LINE;

      else if (maxEndPos <= 0)
        SET_FAILED_ON_SRC_LINE;

      else  {
        currBranch->displayStartPos = maxEndPos + 1;
        ret_code = OK;
      }
    } else {
      SET_FAILED_ON_SRC_LINE;
    }
  }

  assert(ret_code == OK);
  return ret_code;
 }

/* ****************************************************************************
 * Procedure used in displaying the compiler's parse tree for an expression
 * Set start and end display positions for an outer leaf node based off of
 * our center sibling's previously set displayEndPos
 * ***************************************************************************/
int ExpressionParser::setOuterLeafNodeDisplayPos (bool isLeftTree, int halfTreeLevel, std::shared_ptr<ExprTreeNode> currBranch) {
  int ret_code = GENERAL_FAILURE;

  if (currBranch->nodePos == OUTER_NODE) {
    // Sibling CENTER_NODE should already be resolved, so use that
    std::shared_ptr<ExprTreeNode> centerNode;
    if (isLeftTree && currBranch->treeParent != NULL && currBranch->treeParent->_2ndChild != NULL)  
      centerNode = currBranch->treeParent->_2ndChild;
    else if (!isLeftTree && currBranch->treeParent != NULL && currBranch->treeParent->_1stChild != NULL)
      centerNode = currBranch->treeParent->_1stChild;

    if (centerNode != NULL) {
      int centerStrLen = makeTreeNodeStr(centerNode).length();
      int numSiblingOpr8rs = 0;

      currBranch->originalTkn->tkn_type == SRC_OPR8R_TKN ? numSiblingOpr8rs++ : 0;
      centerNode->originalTkn->tkn_type == SRC_OPR8R_TKN ? numSiblingOpr8rs++ : 0;
      currBranch->displayStartPos = centerNode->displayEndPos + (numSiblingOpr8rs == 2 ? DISPLAY_GAP_SPACES : numSiblingOpr8rs == 1 ? 1 : 0);
      currBranch->displayEndPos = currBranch->displayStartPos + makeTreeNodeStr(currBranch).length() + 1;
      ret_code = OK;
    }
  }

  assert (ret_code == OK);
  return ret_code;

}

/* ****************************************************************************
 * Procedure used in displaying the compiler's parse tree for an expression
 * Set start and end display positions for a branch [OPR8R] node
 * ***************************************************************************/
 int ExpressionParser::setBranchNodeDisplayPos (bool isLeftTree, int halfTreeLevel, std::shared_ptr<ExprTreeNode> currBranch) {
  int ret_code = GENERAL_FAILURE;
  int maxStartPos = -1;

  if (currBranch != NULL && currBranch->nodePos == CENTER_NODE) {
    bool isCompressionOK = false;
    // /B+\        [three] <- if isCompressionOK, push [three] in towards center of RIGHT tree
    // [one][two]             

    if (currBranch->_1stChild != NULL && currBranch->_1stChild->originalTkn->tkn_type != SRC_OPR8R_TKN
      && currBranch->_2ndChild != NULL && currBranch->_2ndChild->originalTkn->tkn_type != SRC_OPR8R_TKN)  {

      std::shared_ptr<ExprTreeNode> mySiblingNode;
      if (isLeftTree && currBranch->treeParent != NULL && currBranch->treeParent->_1stChild != NULL)  {
        // We can compress if our children are operands and so is our sibling
        mySiblingNode = currBranch->treeParent->_1stChild;
        if (mySiblingNode->originalTkn->tkn_type != SRC_OPR8R_TKN)
          isCompressionOK = true;

      } else if (!isLeftTree && currBranch->treeParent != NULL && currBranch->treeParent->_2ndChild != NULL)  {
        // We can compress if our children are operands and so is our sibling
        mySiblingNode = currBranch->treeParent->_2ndChild;
        if (mySiblingNode->originalTkn->tkn_type != SRC_OPR8R_TKN)
          isCompressionOK = true;

      }
    }

    if (isCompressionOK)  {
      currBranch->displayEndPos = currBranch->displayStartPos + makeTreeNodeStr(currBranch).length() + 1;
      ret_code = OK;

    } else if (isLeftTree && currBranch->_1stChild != NULL)  {
      currBranch->displayEndPos = currBranch->_1stChild->displayEndPos;
      ret_code = OK;

    } else if (!isLeftTree && currBranch->_2ndChild != NULL)  {
      currBranch->displayEndPos = currBranch->_2ndChild->displayEndPos;
      ret_code = OK;
    
    } else {
      SET_FAILED_ON_SRC_LINE;      
    }

  } else if (currBranch != NULL && currBranch->nodePos == OUTER_NODE) {
    // OUTSIDE_NODE - Hop over to our sibling CENTER_NODE and use its info that was updated before this OUTER_NODE
    std::shared_ptr<ExprTreeNode> centerNode;
    if (isLeftTree && currBranch->treeParent != NULL && currBranch->treeParent->_2ndChild != NULL)  
      centerNode = currBranch->treeParent->_2ndChild;
    else if (!isLeftTree && currBranch->treeParent != NULL && currBranch->treeParent->_1stChild != NULL)
      centerNode = currBranch->treeParent->_1stChild;

    if (centerNode != NULL) { 
      int centerStrLen = makeTreeNodeStr(centerNode).length();
      int numSiblingOpr8rs = 0;

      currBranch->originalTkn->tkn_type == SRC_OPR8R_TKN ? numSiblingOpr8rs++ : 0;
      centerNode->originalTkn->tkn_type == SRC_OPR8R_TKN ? numSiblingOpr8rs++ : 0;
      // TODO: Keep it simple for now
      // currBranch->displayStartPos = centerNode->displayEndPos + (numSiblingOpr8rs == 2 ? DISPLAY_GAP_SPACES : numSiblingOpr8rs == 1 ? 1 : 0);
      currBranch->displayStartPos = centerNode->displayEndPos + 1;
      
      int calcEndPos = currBranch->displayStartPos + makeTreeNodeStr(currBranch).length() + 1;
      // We need to get our displayEndPos bubbled up from our outside child
      if (isLeftTree && currBranch->_1stChild != NULL)  {
        // TODO: Is it possible we're larger?
        if (calcEndPos > currBranch->_1stChild->displayEndPos)
          currBranch->displayEndPos = calcEndPos;
        else
          // TODO: Do we ever get here?
          currBranch->displayEndPos = currBranch->_1stChild->displayEndPos;
        ret_code = OK;
      
      } else if (!isLeftTree && currBranch->_2ndChild != NULL)  {
        // TODO: Is it possible we're larger?
        if (calcEndPos > currBranch->_2ndChild->displayEndPos)
          currBranch->displayEndPos = calcEndPos;
        else
          // TODO: Do we ever get here?
          currBranch->displayEndPos = currBranch->_2ndChild->displayEndPos;
        ret_code = OK;
      }
    }
  }

  assert (ret_code == OK);
  return ret_code;

}


/* ****************************************************************************
 * Procedure used in displaying the compiler's parse tree for an expression
 * The CENTER_NODE has been set at the top of this sub-tree, so set all the
 * other CENTER_NODEs directly below to the same displayStartPos so they line 
 * up.
 * ***************************************************************************/
 int ExpressionParser::setDownstreamCenters (bool isLeftTree, std::shared_ptr<ExprTreeNode> currBranch)  {
  int ret_code = GENERAL_FAILURE;
  bool isDone = false;

  if (currBranch != NULL) {
    int startPos = currBranch->displayStartPos;
    // Nodes further down are uninitialized with displayRow = -1
    int currLvl = currBranch->displayRow;

    while (!isDone) {
      if (isLeftTree) {
        if (currBranch->_2ndChild == NULL) {
          isDone = true;
        
        } else {
          currBranch = currBranch->_2ndChild;
          currLvl++;
          currBranch->displayStartPos = startPos;
        }
      } else {
        // RIGHT tree
        if (currBranch->_1stChild == NULL) {
          isDone = true;
        
        } else {
          currBranch = currBranch->_1stChild;
          currLvl++;
          currBranch->displayStartPos = startPos;
        }
      }
    }

    if (isDone)
      ret_code = OK;
  }

  return ret_code;
}

/* ****************************************************************************
 * Procedure used in displaying the compiler's parse tree for an expression
 * Recursive proc to determine node display positions within one left|right half
 * of a complete parse tree expression
 * ***************************************************************************/
int ExpressionParser::setHalfTreeDisplayPos (bool isLeftTree, int halfTreeLevel, std::shared_ptr<ExprTreeNode> currBranch
  , std::vector<std::vector<std::shared_ptr<ExprTreeNode>>> & halfDisplayLines)	{
	int ret_code = GENERAL_FAILURE;

	if (currBranch != NULL)	{
    bool isCtrNode;

    while (halfDisplayLines.size() < (halfTreeLevel + 1))	{
      // Create level for this list if it doesn't already exist
      halfDisplayLines.push_back(std::vector<std::shared_ptr<ExprTreeNode>> ());
    }
  
    // Make an alias variable for code readability
    std::vector<std::shared_ptr<ExprTreeNode>> & currLvlList = halfDisplayLines[halfTreeLevel];
    currLvlList.push_back(currBranch);

    if (OK != setIsCenterNode (isLeftTree, halfTreeLevel, currBranch))
      SET_FAILED_ON_SRC_LINE;

    if (!failed_on_src_line)	{
      currBranch->displayRow = halfTreeLevel;

      if (currBranch->nodePos == CENTER_NODE) {
        if (currBranch->displayStartPos < 0) {
          if (halfTreeLevel <= 1) {
            // Init all downstream CENTER_NODE's displayStartPos -> 0
            currBranch->displayStartPos = 0;

            if (currBranch->originalTkn->tkn_type == SRC_OPR8R_TKN && OK != setDownstreamCenters (isLeftTree, currBranch))
              SET_FAILED_ON_SRC_LINE;

          } else if (halfTreeLevel >= 2) {
            // New CENTER_NODE with uninitialized displayStartPos. Look back to previous branch closer to
            // center. Find already resolved OUTER_NODE with greatest displayEndPos; use to set our displayStartPos
            if (currBranch->displayStartPos < 0)  {
              if (OK != setCtrStartByPrevBndry (isLeftTree, currBranch)) 
                SET_FAILED_ON_SRC_LINE;
              else if (currBranch->originalTkn->tkn_type == SRC_OPR8R_TKN && OK != setDownstreamCenters (isLeftTree, currBranch))
                SET_FAILED_ON_SRC_LINE;
            } 
          }
        }

        if (currBranch->displayEndPos == 0)
          // Set the end position, even if it's a SRC_OPR8R_TKN/branch that will be udpated later.
          // Nodes below us will need this info to calculate their start ps
          currBranch->displayEndPos = currBranch->displayStartPos + makeTreeNodeStr(currBranch).length();

        if (currBranch->originalTkn->tkn_type != SRC_OPR8R_TKN)
          // We hit a leaf node, so no more recursing
          ret_code = OK;
      
      } else if (currBranch->nodePos == OUTER_NODE && currBranch->originalTkn->tkn_type != SRC_OPR8R_TKN) {
        // We hit a leaf node, so stop recursing
        ret_code = setOuterLeafNodeDisplayPos (isLeftTree, halfTreeLevel, currBranch);
      } 
      
      if (!failed_on_src_line && currBranch->originalTkn->tkn_type == SRC_OPR8R_TKN) {
        // Recursive case
        if ((isLeftTree && currBranch->_2ndChild != NULL) || (!isLeftTree && currBranch->_1stChild != NULL))  {
          // Resolve CENTER branch 1st
          if (OK != setHalfTreeDisplayPos (isLeftTree, halfTreeLevel + 1
              , isLeftTree ? currBranch->_2ndChild : currBranch->_1stChild, halfDisplayLines))
            SET_FAILED_ON_SRC_LINE;
        }
  
        if (!failed_on_src_line && ((isLeftTree && currBranch->_1stChild != NULL) || (!isLeftTree && currBranch->_2ndChild != NULL)))  {
          // Resolve OUTSIDE branch next 
          if (OK != setHalfTreeDisplayPos (isLeftTree, halfTreeLevel + 1
              , isLeftTree ? currBranch->_1stChild : currBranch->_2ndChild, halfDisplayLines))
            SET_FAILED_ON_SRC_LINE;
        }
  
        if (!failed_on_src_line)  {
          if (OK != setBranchNodeDisplayPos (isLeftTree, halfTreeLevel, currBranch))
            SET_FAILED_ON_SRC_LINE;
        }
      }
    }
 
    if (!failed_on_src_line)  {
      ret_code = OK;
    }
  }

	return (ret_code);

}

/* ****************************************************************************
 * Procedure used in displaying the compiler's parse tree for an expression
 * Serves as an ILLUSTRATIVE aid
 * After an expression tree has been built, this proc will create a 
 * graphical tree representation for display when operating in 
 * ILLUSTRATIVE mode
 * 
 * What I've got right now:
 *         /=\
 * [result]   /B+\
 *            /*\/?\
 *            /B+\/B+\/<\/:\
 *            /B+\[three][four][six]/<<\/*\[three][four]
 *            [one][two]/*\[five][eight][four]
 *            [six][one]
 *
 * Start from top level on L or R side
 * R side - figure out max width for _1stChild side recursively
 * Push _2ndChild over so there's no overlap
 * figure out max width for _2ndChild side recursively
 * 
 * Probably want it to look something like below:
 * 
 * /B+\
 * /*\                       /?\
 * /B+\         /B+\         /<\                     /:\
 * /B+\ [three] [four][six]  /<<\       /*\          [three][four]
 * [one][two]                /*\ [five] [eight][four]
 *                           [six][one]
 *
 * inside operand,outside operand - no space between
 * inside operand,outside opr8r   - 1 space between
 * inside opr8r, outside operand  - 1 space between
 * inside opr8r, outside opr8r    - resolve inside 1st to determine gap between the two
 * ***************************************************************************/
 int ExpressionParser::setFullTreeDisplayPos (std::shared_ptr<ExprTreeNode> startBranch, std::vector<std::wstring> & displayLines
  , int & maxLeftLineLen)	{
	int ret_code = GENERAL_FAILURE;

	int left_ret;
	int right_ret;
  std::vector<std::vector<std::shared_ptr<ExprTreeNode>>> leftLines;
  std::vector<std::vector<std::shared_ptr<ExprTreeNode>>> rightLines;

	if (startBranch->_1stChild == NULL)
		left_ret = OK;
	else
	 	left_ret = setHalfTreeDisplayPos (true, 0, startBranch->_1stChild, leftLines);

 	if (startBranch->_2ndChild == NULL)
		right_ret = OK;
	else
		right_ret = setHalfTreeDisplayPos (false, 0, startBranch->_2ndChild, rightLines);

	if (left_ret == OK && right_ret == OK)	{			
    int odx, idx;

    int numDisplayLines = leftLines.size();
    if (rightLines.size() > numDisplayLines)
      numDisplayLines = rightLines.size();
    
    // Need some space for the root node
		numDisplayLines++;
    std::wstring rootNodeStr = makeTreeNodeStr(startBranch);

    
		int ldx;

		for (ldx = 0; ldx < numDisplayLines; ldx++)
			displayLines.push_back(L"");

    int centerGapSpaces = DISPLAY_GAP_SPACES;
    if (rootNodeStr.length() - 2 > centerGapSpaces)
      // Take off 2 so root node ends line up with direct children ends
      centerGapSpaces = rootNodeStr.length() - 2;

    getMaxLineLen(leftLines,true, maxLeftLineLen);
		displayLines[0].insert (displayLines[0].begin(), maxLeftLineLen, L' ');
		displayLines[0].append(rootNodeStr);

    if (OK != fillDisplayLeft(displayLines, leftLines, maxLeftLineLen))
      SET_FAILED_ON_SRC_LINE;
    else if (OK != fillDisplayRight(displayLines, rightLines, centerGapSpaces))
      SET_FAILED_ON_SRC_LINE;

    if (!failed_on_src_line)
      ret_code = OK;
 	}

	// Free up memory and reset each node's display position settings
  while (!leftLines.empty())  {
    std::vector<std::shared_ptr<ExprTreeNode>> currLines = *leftLines.begin();

    while (!currLines.empty())  {
      auto delNode = *currLines.begin();
      delNode->initDisplaySettings();
      currLines.erase(currLines.begin());
    }

    leftLines.erase(leftLines.begin());
  }

  while (!rightLines.empty())  {
    std::vector<std::shared_ptr<ExprTreeNode>> currLines = *rightLines.begin();

    while (!currLines.empty())  {
      auto delNode = *currLines.begin();
      delNode->initDisplaySettings();
      currLines.erase(currLines.begin());
    }

    rightLines.erase(rightLines.begin());
  }

	return (ret_code);

}

/* ****************************************************************************
 * Encountered a system call. Check that it's a recognized system call and has
 * the expected number and type of arguments.
 * ***************************************************************************/
 int ExpressionParser::compile_system_call (TokenPtrVector & tknStream, std::shared_ptr<ExprTreeNode> sys_call_node) {
  int ret_code = GENERAL_FAILURE;

  if (sys_call_node != NULL) {

    TokenTypeEnum call_ret_type;
    std::vector <uint8_t> parameters;

    if (OK != usrSrcTerms.get_system_call_details (sys_call_node->originalTkn->_string, parameters, call_ret_type))  {
      SET_FAILED_ON_SRC_LINE;
    
    } else if (OK != check_for_expected_token(tknStream, *sys_call_node->originalTkn, L"(", false))  {
      SET_FAILED_ON_SRC_LINE;

    } else { 
      // TODO: Currently this *should* be set to SYSTEM_CALL_TKN, but when we do that we lose the return data type
      // Does the return data type need to be preserved, since the return data type of the system call can be 
      // looked up when it's evaluated?
      // Could have flags is_system_call and is_user_fxn to indicate
      // Still need to do better with VOID data type
      // sys_call_node->originalTkn->tkn_type = call_ret_type;
      sys_call_node->originalTkn->tkn_type = SYSTEM_CALL_TKN;

      Token expr_ended_by;
      int make_tree_ret;
      bool is_expr_closed, is_expr_static;
      int expected_ret_tkn_cnt;

      if (0 == parameters.size()) {
        ret_code = check_for_expected_token(tknStream, *sys_call_node->originalTkn, L")", true);

      } else {
        RunTimeInterpreter interpreter (usrSrcTerms, scopedNameSpace, userSrcFileName, userMessages, logLevel);
        std::vector<Token> flatExprTkns;
        int idx = 0;
        std::wstring type_conversion_msg;

        for (; idx < parameters.size() && !failed_on_src_line; idx++) {
          std::shared_ptr<Token> empty_tkn = std::make_shared<Token>();
          std::shared_ptr<ExprTreeNode> param_expr_tree = std::make_shared<ExprTreeNode> (empty_tkn);
    
          make_tree_ret = makeExprTree (tknStream, param_expr_tree, expr_ended_by
            , idx + 1 < parameters.size() ? ENDS_IN_COMMA : ENDS_IN_PARENTHESES
            , is_expr_closed, false, is_expr_static);
    
          if (OK != make_tree_ret)  {
            SET_FAILED_ON_SRC_LINE;

          } else if (OK != usrSrcTerms.flattenExprTree(param_expr_tree, flatExprTkns))	{
            // (3 + 4) -> [3][4][+]
            SET_FAILED_ON_SRC_LINE;
          
          } else if (OK != interpreter.resolveFlatExpr(flatExprTkns, expected_ret_tkn_cnt)
              || expected_ret_tkn_cnt != 1)	{
            // TODO: Using this interpreter means using an EMPTY namespace that doesn't have the variables stuffed into 
            // the original namespace that came with GeneralParser's interpreter.  Might need to share the GP interpreter
            // back with ExpressionParser
            // TODO: Pass it in through the constructor
            SET_FAILED_ON_SRC_LINE;
      
          } else if (flatExprTkns.size() != 1)	{
            // flattenedExpr should have 1 Token left - the result of the expression
            userMessages->logMsg(INTERNAL_ERROR, L"Failed to resolve expression starting with " + sys_call_node->originalTkn->descr_sans_line_num_col()
                  , thisSrcFile, __LINE__, 0);
            SET_FAILED_ON_SRC_LINE;
            
          } else if (parameters[idx] != INVALID_OPCODE && OK != usrSrcTerms.tkn_type_converts_to_opcode(parameters[idx], flatExprTkns[0], L"parameter " + std::to_wstring(idx), type_conversion_msg)) {
            // INVALID_OPCODE is a special case 
            userMessages->logMsg(USER_ERROR, type_conversion_msg + L" for system call " + sys_call_node->originalTkn->_string
                  , userSrcFileName, sys_call_node->originalTkn->get_line_number(), sys_call_node->originalTkn->get_column_pos());
            SET_FAILED_ON_SRC_LINE;

          } else {
            sys_call_node->parameter_list.push_back(param_expr_tree);
          }

          flatExprTkns.clear();
        }
    
        if (idx == parameters.size() && failed_on_src_line == 0)  {
          ret_code = OK;
        }
      }
    }
  }

  return ret_code;
}


