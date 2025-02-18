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
 * Breakdown of expressions
 * Single term:	[Variable|Literal]
 *
 * LEADING OPERATORS
 * Prefix op:		[++|--][Variable] -> result converts to R-value
 *	HOW IT ENDS: 	Variable
 *
 * UNARY:				[~|!|-][EXPR]			-> result converts to R-value
 *	HOW IT ENDS: 	[EXPR] will either be a single term or an expression contained in parentheses
 *
 * TRAILING OPERATORS
 * Postfix op:	[Variable][++|--]	-> result converts to R-value ???
 *	HOW IT ENDS: 	++ or -- OPR8R
 *
 * MEZZO OPERATORS
 * BINARY:			[EXPR][OPR8R][EXPR]
 *	HOW IT ENDS:
 *
 * TERTIARY:		[EXPR][?][EXPR][:][EXPR]
 *	HOW IT ENDS: 	1st expression is either a single term, encapsulated in (), or the ? operator ends it
 *								2nd expression ends with : operator
 *								3rd expression is either a single term, encapsulated in (), or we run out of tokens
 */

#include "ExpressionParser.h"

#include <bits/stdint-uintn.h>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

#include "CompileExecTerms.h"
#include "ExprTreeNode.h"
#include "InfoWarnError.h"
#include "OpCodes.h"
#include "Operator.h"
#include "Token.h"
#include "UserMessages.h"
#include "StackOfScopes.h"
#include "common.h"

ExpressionParser::ExpressionParser(CompileExecTerms & inUsrSrcTerms, std::shared_ptr<StackOfScopes> inVarScopeStack
		, std::wstring userSrcFileName, std::shared_ptr<UserMessages> userMessages, logLvlEnum logLvl) {
	// TODO Auto-generated constructor stub
	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");
	usrSrcTerms = inUsrSrcTerms;
	scopedNameSpace = inVarScopeStack;
	this->userSrcFileName = userSrcFileName;
	this->userMessages = userMessages;
	logLevel = logLvl;
	isExprVarDeclaration = false;
	isExprClosed = false;
}


ExpressionParser::~ExpressionParser() {
	// TODO Auto-generated destructor stub
	cleanScopeStack();
}

/* ****************************************************************************
 * Parse through the current expression and if it's well formed, commit it to
 * the interpreted stream. If it's not well formed, generate a clear error
 * message to the user.
 * TODO: isEndedByComma -> isEnclosedByParens? isParenBound?
 * ***************************************************************************/
int ExpressionParser::makeExprTree (TokenPtrVector & tknStream, std::shared_ptr<ExprTreeNode> & expressionTree
		, Token & enderTkn, bool isEndedByComma, bool & isCallerExprClosed, bool isInVarDec)  {
  int ret_code = GENERAL_FAILURE;
	isExprClosed = false;
	int exprCloseLine = 0;
	isExprVarDeclaration = isInVarDec;
	
  if (expressionTree == NULL)	{
  	userMessages->logMsg (INTERNAL_ERROR, L"Passed parameter expressionTree is NULL!", thisSrcFile, __LINE__, 0);

  } else	{

		std::shared_ptr<NestedScopeExpr> rootScope = std::make_shared<NestedScopeExpr>();
		exprScopeStack.push_back (rootScope);

		bool isStopFail = false;
		bool is1stTkn = true;
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
			while (!isStopFail && !isExprClosed)	{
				// Consume flat stream of Tokens in current expression; attach each to an ExprTreeNode for tree transformation
				if (!tknStream.empty())	{
					std::shared_ptr<Token> currTkn = tknStream.front();

					if (is1stTkn)	{
						// Token that starts expression will determine how we end the expression
						if (OK != getExpectedEndToken(currTkn, curr_legal_tkn_types, expectedEndTkn, isEndedByComma))	{
							isStopFail = true;
							break;
						
						} else if ((logLevel == ILLUSTRATIVE && !isExprVarDeclaration) || logLevel > ILLUSTRATIVE)		{
							std::wcout << L"// Starting compilation of expression that begins on " << userSrcFileName;
							std::wcout << L":" << std::to_wstring(currTkn->get_line_number()) << L":" << std::to_wstring(currTkn->get_column_pos());
							std::wcout << std::endl << std::endl;
						}
						is1stTkn = false;
					}

					if ((currTkn->tkn_type != SRC_OPR8R_TKN && expectedEndTkn.tkn_type == currTkn->tkn_type && currTkn->_string == expectedEndTkn._string)
							|| (currTkn->tkn_type == SRC_OPR8R_TKN && currTkn->_string == usrSrcTerms.get_statement_ender()))	{
						// Expression ended by a SPR8R - e.g. [,] or ;
						tknStream.erase(tknStream.begin());

						if (0 == (curr_legal_tkn_types & (VAR_NAME_NXT_OK|LITERAL_NXT_OK|OPEN_PAREN_NXT_OK)))	{
							// Check if expression is in a closeable state
							isExprClosed = true;
							exprCloseLine = __LINE__;
							enderTkn = *currTkn;

							top = exprScopeStack.size() - 1;
							if (exprScopeStack[top]->scopedKids.size() == 0)	{
								userMessages->logMsg (INTERNAL_ERROR, L"Expression ender " + currTkn->descr_sans_line_num_col() + L" closed an empty express!", thisSrcFile, __LINE__, 0);
								isStopFail = true;

							} else if (top > 0 || exprScopeStack[top]->scopedKids.size() > 1)	{
								if (OK != closeNestedScopes())	{
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

					} else if (!isExpectedTknType (curr_legal_tkn_types, next_legal_tkn_types, currTkn))	{
						userMessages->logMsg (USER_ERROR
								, L"Unexpected token type! Expected " + makeExpectedTknTypesStr(curr_legal_tkn_types) + L" but got " + currTkn->descr_sans_line_num_col()
								, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
						isStopFail = true;

					} else if ((currTkn->tkn_type == SRC_OPR8R_TKN && (TERNARY_2ND & usrSrcTerms.get_type_mask(currTkn->_string)))
							&& !isTernaryOpen())	{
						// Unexpected TERNARY_2ND
						userMessages->logMsg (USER_ERROR
								,L"Got middle ternary operand " + currTkn->descr_sans_line_num_col() + L" without required preceding starting ternary operator " + usrSrcTerms.get_ternary_1st()
								,  userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
						isStopFail = true;

					} else if ((currTkn->tkn_type == SRC_OPR8R_TKN && (TERNARY_2ND & usrSrcTerms.get_type_mask(currTkn->_string)))
							&& get2ndTernaryCnt() > 0)	{
						// Unexpected *EXTRA* TERNARY_2ND
						userMessages->logMsg (USER_ERROR, L"Got unexpected additional middle ternary operand " + currTkn->descr_sans_line_num_col()
								, userSrcFileName, currTkn->get_line_number(), currTkn->get_column_pos());
						isStopFail = true;

					} else if ((currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L"(")
							|| (currTkn->tkn_type == SRC_OPR8R_TKN && (TERNARY_1ST & usrSrcTerms.get_type_mask(currTkn->_string))))	{
						// Open parenthesis or 1st ternary
						if (OK != openSubExprScope(tknStream))	{
							isStopFail = true;
						}

					} else if (currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L")")	{
						// Close parenthesis - syntactic sugar that just melts away
						// Remove Token from stream without destroying - move to flat expression in current scope
						tknStream.erase(tknStream.begin());

						if (OK != closeNestedScopes ())	{
							isStopFail = true;

						} else if (expectedEndTkn.tkn_type == SPR8R_TKN && expectedEndTkn._string == currTkn->_string 
							&& exprScopeStack.size() == 1 && exprScopeStack[0]->scopedKids.size() == 1)	{
							// TODO: Failure when expecting to close by a [;]
							isExprClosed = true;
							enderTkn = *currTkn;
							exprCloseLine = __LINE__;
						}

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
			}

			if (isExprClosed && !isStopFail)	{
				int numTknsLeftInExpr = exprScopeStack[0]->scopedKids.size();
				if (exprScopeStack.size() == 1 && 1 == numTknsLeftInExpr)	{
						// TODO: Double check tree health before calling it a day?
						ret_code = OK;
						expressionTree = exprScopeStack[0]->scopedKids[0];

						if ((logLevel == ILLUSTRATIVE && !isExprVarDeclaration) || logLevel > ILLUSTRATIVE)	{
							expressionTree->showTree(thisSrcFile, __LINE__);
						}
					} else	{
					std::wstring devMsg = L"Expression closed but tree conversion failed! Remaining Token count at stack top = ";
					devMsg.append ( std::to_wstring(numTknsLeftInExpr));
					devMsg.append (L"; # scope levels = ");
					devMsg.append (std::to_wstring(exprScopeStack.size()));
					devMsg.append (L"; exprCloseLine = ");
					devMsg.append (std::to_wstring(exprCloseLine));
					devMsg.append(L";");
					userMessages->logMsg (INTERNAL_ERROR, devMsg, thisSrcFile, __LINE__, 0);
					showDebugInfo (thisSrcFile, __LINE__);
				}
			}
		}

		cleanScopeStack();
  }

	isCallerExprClosed = isExprClosed;
	// Init for next call into proc
	isExprClosed = false;

  return (ret_code);
}

/* ****************************************************************************
 * Next Token is a opening parenthesis or an opening ternary (?)
 * Increase the current scope level of the expression
 * Deeper|greater scope corresponds to higher precedence, or deeper levels of
 * parentheses.
 * ***************************************************************************/
int ExpressionParser::openSubExprScope (TokenPtrVector & tknStream)  {
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
 int ExpressionParser::closeNestedScopes()	{
	int ret_code = GENERAL_FAILURE;

	bool isParentLinked = false;
	bool isRootScope = false;
	bool isStopFail = false;

	if (OK == makeTreeAndLinkParent(isParentLinked))	{
		int  prevStackSize = exprScopeStack.size(), currStackSize;
	
		while (!isParentLinked && !isRootScope && !isStopFail)	{
			if (OK != makeTreeAndLinkParent(isParentLinked))	{
				isStopFail = true;
				break;
	
			} else	{
				currStackSize = exprScopeStack.size();
				
				if (exprScopeStack.size() == 1)	{
					// Only the root scope remains. Check for correctness
					isRootScope = true;
					int numRootKids = exprScopeStack[0]->scopedKids.size();
					// TODO: Should this call be necessary?
					if (numRootKids > 1 && OK != turnClosedScopeIntoTree (exprScopeStack[exprScopeStack.size() - 1]->scopedKids))	{
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
int ExpressionParser::makeTreeAndLinkParent (bool & isParentFndYet)  {
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
			ret_code = turnClosedScopeIntoTree (exprScopeStack[exprScopeStack.size() - 1]->scopedKids);

	} else {
		int stackSzB4 = exprScopeStack.size();

		if (!isStopFail && isParentFndYet != isOpenedByTernary)	{
			// Call turnClosedScopeIntoTree to collapse the current flat list of ExprTreeNodes into
			// a hierarchical tree based on OPR8R precedence - a root ExprTreeNode
			if (OK == turnClosedScopeIntoTree (exprScopeStack[exprScopeStack.size() - 1]->scopedKids, isOpenedByTernary))	{
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
								subExpr->myParentScope = scopener;
								isExprAttached = true;
								ret_code = OK;

							} else	{
									subExpr->showTree(thisSrcFile, __LINE__);
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
								ret_code = turnClosedScopeIntoTree (exprScopeStack[exprScopeStack.size() - 1]->scopedKids);
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
							printScopeStack(thisSrcFile, __LINE__);
							isStopFail = true;

						} else	{
							userMessages->logMsg (INTERNAL_ERROR, L"Dazed and Confubalated!", thisSrcFile, __LINE__, 0);
							isStopFail = true;
							if (subExpr != NULL)	{
								subExpr->showTree(thisSrcFile, __LINE__);
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
			printScopeStack(thisSrcFile, __LINE__);
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
int ExpressionParser::moveNeighborsIntoTree (Operator & opr8r, ExprTreeNodePtrVector & currScope
	, int opr8rIdx, opr8rReadyState opr8rState, bool isMoveLeftNbr, bool isMoveRightNbr)	{
	int ret_code = GENERAL_FAILURE;
	bool isFailed = false;

	if (opr8rIdx >= 0 && opr8rIdx < currScope.size())	{

		std::shared_ptr<ExprTreeNode> opr8rNode = currScope.at(opr8rIdx);
		std::shared_ptr<ExprTreeNode> leftNbr = NULL;
		std::shared_ptr<ExprTreeNode> rightNbr = NULL;
		Token tmpTkn;
		std::wstring lookUpMsg;

		if (isMoveRightNbr)	{
			if ((currScope.size() - 1) < (opr8rIdx + 1))	{
				isFailed = true;

			} else {
				rightNbr = currScope.at(opr8rIdx + 1);
				if (rightNbr == NULL)	{
					isFailed = true;
				
				} else {
					if ((opr8r.type_mask & PREFIX) && (rightNbr->originalTkn->tkn_type != USER_WORD_TKN 
							|| !usrSrcTerms.isViableVarName(rightNbr->originalTkn->_string)
							|| OK != scopedNameSpace->findVar(rightNbr->originalTkn->_string, 0, tmpTkn, READ_ONLY, lookUpMsg)))	{
						// Make sure our right neighbor is a variable name before moving
						isFailed = true;
					
					} else	{
						currScope.erase(currScope.begin() + (opr8rIdx + 1));
						if (opr8rNode != NULL)	{
							if (opr8rState == ATTACH_1ST && opr8rNode->_1stChild == NULL)
								opr8rNode->_1stChild = rightNbr;
							else if ((opr8rState == ATTACH_2ND || opr8rState == ATTACH_BOTH) && opr8rNode->_2ndChild == NULL)
								opr8rNode->_2ndChild = rightNbr;
							else
							 	isFailed = true;
						} else	{
							isFailed = true;
						}
					}
				}
			}
		}

		if (isMoveLeftNbr)	{
			if (opr8rIdx - 1 < 0)	{
				isFailed = true;

			} else {
				leftNbr = currScope.at(opr8rIdx - 1);
				if (leftNbr == NULL)	{
					isFailed = true;
				
				} else {
					if ((opr8r.type_mask & POSTFIX) && (leftNbr->originalTkn->tkn_type != USER_WORD_TKN 
							|| !usrSrcTerms.isViableVarName(leftNbr->originalTkn->_string)
							|| OK != scopedNameSpace->findVar(leftNbr->originalTkn->_string, 0, tmpTkn, READ_ONLY, lookUpMsg)))	{
						// Make sure our left neighbor is a variable name before moving
						isFailed = true;
					
					} else	{
						currScope.erase(currScope.begin() + (opr8rIdx - 1));
						if (opr8rNode != NULL)	{
							if ((opr8rState == ATTACH_1ST || opr8rState == ATTACH_BOTH) && opr8rNode->_1stChild == NULL)
								opr8rNode->_1stChild = leftNbr;
							else if (opr8rState == ATTACH_2ND && opr8rNode->_2ndChild == NULL)
								opr8rNode->_2ndChild = leftNbr;
							else
							 	isFailed = true;
						
						} else {
							isFailed = true;
						}
					}
				}
			}
		}

		if (!isFailed && (isMoveLeftNbr || isMoveRightNbr))	{
			// We tried to do something and didn't fail at it!
			ret_code = OK;
			if ((logLevel == ILLUSTRATIVE && !isExprVarDeclaration) || logLevel > ILLUSTRATIVE)	{
				std::wstring bannerMsg;
				if (opr8rState == ATTACH_1ST)
					bannerMsg.append(L"Left operand");
				else if (opr8rState == ATTACH_1ST)
					bannerMsg.append(L"Right operand");
				else if (opr8rState == ATTACH_BOTH)
					bannerMsg.append(L"Left & right operands");

				bannerMsg.append (L" moved under (like tree branches)");
				bannerMsg.append (L" [" + opr8r.symbol + L"] operator");
				if (!opr8r.description.empty())	{
					bannerMsg.append (L" (");
					bannerMsg.append (opr8r.description);
					bannerMsg.append (L")");
				}
				printSingleScope (bannerMsg , exprScopeStack.size() - 1);
			}
		}
	
	}

	return (ret_code);
}

/* ****************************************************************************
 * ***************************************************************************/
int ExpressionParser::turnClosedScopeIntoTree (ExprTreeNodePtrVector & currScope)  {
	return turnClosedScopeIntoTree(currScope, false);
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
int ExpressionParser::turnClosedScopeIntoTree (ExprTreeNodePtrVector & currScope, bool isOpenedByTernary)  {
  int ret_code = GENERAL_FAILURE;
  bool isStopFail = false;
  bool isReachedEOL = false;
  bool isNowTree = false;

  ExprTreeNodePtrVector::iterator currNodeR8r;
  std::shared_ptr<ExprTreeNode> currNode = NULL;
  std::list<Opr8rPrecedenceLvl>::iterator outr8r;
  std::list<Operator>::iterator innr8r;
  Opr8rPrecedenceLvl precedenceLvl;
	Operator tern2ndOpr8r;
  std::wstring unary1stOpr8r = usrSrcTerms.get_ternary_1st();
  std::wstring unary2ndOpr8r = usrSrcTerms.get_ternary_2nd();

	// [?]._1stChild is the conditional and should be the resolved expression directly to the left
	// [?]_2ndChild is the [:] OPR8R; [:]._1stChild is the TRUE path; [:]_2ndChild is the FALSE path
	if (((logLevel == ILLUSTRATIVE && !isExprVarDeclaration) || logLevel > ILLUSTRATIVE)
		&& (exprScopeStack.size() > 1 || (exprScopeStack.size() == 1 && exprScopeStack[0]->scopedKids.size() > 1)))	{
		std::wstring bannerMsg = L"Current highest precedence sub-expression closed; compiler has";
		if (!isExprClosed) 
			bannerMsg.append (L" NOT");
	 	bannerMsg.append (L" read in all Tokens for this expression.");
		printScopeStack (bannerMsg, false);
	}
	
  // TODO: Code inspect, comment, simplify if you can
	for (outr8r = usrSrcTerms.grouped_opr8rs.begin(); outr8r != usrSrcTerms.grouped_opr8rs.end() && !isStopFail && !isNowTree; outr8r++){
		// Move through each precedence level of OPR8Rs. Note that some precedence levels will have multiple OPR8Rs and they must be
		// treated as having the same precedence, and therefore we can't rely on an ABSOUTE ordering of OPR8R precedence
		precedenceLvl = *outr8r;
		for (innr8r = precedenceLvl.opr8rs.begin(); innr8r != precedenceLvl.opr8rs.end() && !isStopFail && !isNowTree; ++innr8r){
			Operator currOpr8r = *innr8r;

			if (currOpr8r.valid_usage & GNR8D_SRC)	{
				// Filter out OPR8Rs only valid for USER_SRC ([pre|post]-fix ++,--; unary +|- ) and match on unique GNR8D_SRC equivalent
				// OPR8R match and it hasn't had any child nodes attached to it yet
				bool isOpr8rExhausted = false;

				if (isOpenedByTernary && currOpr8r.op_code == TERNARY_2ND_OPR8R_OPCODE)	{
					// We need to special case the [:] OPR8R and make its precedence LOWER than anything else in the current scope
					// to ensure any *contained* assignment OPR8Rs get treated as higher priority and are pushed deeper into the tree,
					// even though the C OPR8R precedence declares [:] precedence > precedence of [=] [+=] [-=] [*=] [/=] [%=] [<<=] [>>=] [&==] [|==] [^==]
					isOpr8rExhausted = true;
					tern2ndOpr8r = currOpr8r;
				}

				while (!isOpr8rExhausted && !isStopFail && !isNowTree)	{
					opr8rReadyState opr8rState = OPR8R_NOT_READY;
					int startSize = currScope.size();
					bool isMoveLeftNbr = false;
					bool isMoveRightNbr = false;
					int listIdx = 0;

					for (currNodeR8r = currScope.begin(); currNodeR8r != currScope.end() && !isStopFail && !isNowTree && !opr8rState; currNodeR8r++)	{
						// Move left-to-right through the expression. Do some work on match with current OPR8R 
						// When objects are moved around, the iterator will be invalidated and need to restart.

						std::shared_ptr<ExprTreeNode> currNode = *currNodeR8r;
						// Make an alias variable for code readability
						std::shared_ptr <Token> currTkn = currNode->originalTkn;

						if (currNode == NULL)	{
							userMessages->logMsg (INTERNAL_ERROR, L"ExprTreeNode pointer is NULL! ", thisSrcFile, __LINE__, 0);
							isStopFail = true;

						} else if (currTkn->tkn_type == SRC_OPR8R_TKN && currTkn->_string == currOpr8r.symbol)	{
							// IFF OPR8R at current precedence level is encountered at this expression scope, we can move some|all neighbors under this OPR8R
							if (currOpr8r.symbol == usrSrcTerms.get_ternary_1st())	{
								if (currNode->_1stChild == NULL && currNode->_2ndChild != NULL)	{
									// TERNARY_1ST [TRUE|FALSE] paths resolved; conditional to left should be resolved and ready to be our _1stChild
									opr8rState = ATTACH_1ST;
									isMoveLeftNbr = true;
								}
							} else	{
								// For this OPR8R, determine which neighbors get removed from list and attached to OPR8R 
								if ((currOpr8r.type_mask & UNARY) && currNode->_1stChild == NULL)	{
									opr8rState = ATTACH_1ST;
									isMoveRightNbr = true;
								
								} else if ((currOpr8r.type_mask & PREFIX) && currNode->_1stChild == NULL)	{
									opr8rState = ATTACH_1ST;
									isMoveRightNbr = true;

								} else if ((currOpr8r.type_mask & POSTFIX) && currNode->_1stChild == NULL)	{
									opr8rState = ATTACH_1ST;
									isMoveLeftNbr = true;

								} else if ((currOpr8r.type_mask & BINARY) && currNode->_1stChild == NULL && currNode->_2ndChild == NULL)	{
									opr8rState = ATTACH_BOTH;
									isMoveLeftNbr = true;
									isMoveRightNbr = true;
								}
							}
						}
						if (!opr8rState)	
							listIdx++;
					}

					if (opr8rState != OPR8R_NOT_READY)	{
						if (OK != moveNeighborsIntoTree (currOpr8r, currScope, listIdx, opr8rState, isMoveLeftNbr, isMoveRightNbr))
							isStopFail = true;

						else if (currScope.size() >= startSize)	{
							isStopFail = true;
							std::wstring devMsg = L"Source expression not reduced in size while handling OPR8R " + currOpr8r.symbol;
							userMessages->logMsg(INTERNAL_ERROR, devMsg, thisSrcFile, __LINE__, 0);
						}
					} else {
						isOpr8rExhausted = true;
					}
				}
			}
		}
	}

	if (!isStopFail && isOpenedByTernary)	{
		// Time to take care of the delayed [:] OPR8R
		for (int idx = 0; idx < currScope.size(); idx++)	{
			if (currScope[idx]->originalTkn->tkn_type == SRC_OPR8R_TKN && currScope[idx]->originalTkn->_string == usrSrcTerms.get_ternary_2nd())	{
				if (OK != moveNeighborsIntoTree (tern2ndOpr8r, currScope, idx, ATTACH_BOTH, true, true))	{
					isStopFail = true;
					userMessages->logMsg (INTERNAL_ERROR, L"Failed to attach branches to OPR8R [" + usrSrcTerms.get_ternary_2nd() + L"]"
						, thisSrcFile, __LINE__, 0);
				} 
				break;
			}
		}
	}


	if (!isStopFail) {
		// TODO: Check for only OPR8Rs left @ currScope? Check if they're treed up?
		if (isNowTree)
			ret_code = OK;
		else if (currScope.size() == 1)
			ret_code = OK;
		else	{
			userMessages->logMsg (INTERNAL_ERROR, L"Could not make tree", thisSrcFile, __LINE__, 0);
			isStopFail = true;
			showDebugInfo (thisSrcFile, __LINE__);
		}
	}

	if (ret_code != OK && !isStopFail)	{
		userMessages->logMsg (INTERNAL_ERROR, L"Unhandled error", thisSrcFile, __LINE__, 0);
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

	if (FXN_CALL_NXT_OK & expected_tkn_types)  {
		if (expected.length() > 0)
			expected.append(L"|");
		expected.append (L"FXN_CALL");
	}

	expected.insert (0, L"[");
	expected.append(L"]");

	return expected;
}

/* ****************************************************************************
 * Check if the TERNARY_1st OPR8R was encountered previously at the currently
 * opened scope.
 * ***************************************************************************/
bool ExpressionParser::isTernaryOpen ()  {
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
 * Check if the TERNARY_1st OPR8R was encountered previously at the currently
 * opened scope.
 * ***************************************************************************/
std::wstring ExpressionParser::getMyParentSymbol ()  {
  std::wstring symbol;

	int stackSize = exprScopeStack.size();
	if (stackSize >= 1)	{
		std::shared_ptr<NestedScopeExpr> currScope = exprScopeStack[stackSize - 1];

		if (currScope->myParentScopener != 0)	{
			std::shared_ptr<ExprTreeNode> scopener = (std::shared_ptr<ExprTreeNode>)currScope->myParentScopener;

			if (scopener->originalTkn != NULL 
				&& (scopener->originalTkn->tkn_type == SRC_OPR8R_TKN || scopener->originalTkn->tkn_type == EXEC_OPR8R_TKN))	{
				symbol = scopener->originalTkn->_string;
			}
		}
	}

	return (symbol);
}

/* ****************************************************************************
 * Return the count of TERNARY_2ND OPR8Rs encountered at the current scope.
 * There should be only 1 per scope
 * ***************************************************************************/
int ExpressionParser::get2ndTernaryCnt ()  {
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
 * ***************************************************************************/
bool ExpressionParser::isExpectedTknType (uint32_t allowed_tkn_types, uint32_t & next_legal_tkn_types, std::shared_ptr<Token> curr_tkn)  {
  bool isTknTypeOK = false;

  if (curr_tkn != NULL)	{
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
  		if (isTernaryOpen())
  			next_legal_tkn_types |= TERNARY_OPR8R_2ND_NXT_OK;

  	} else if ((allowed_tkn_types & LITERAL_NXT_OK) && curr_tkn->tkn_type == STRING_TKN || curr_tkn->tkn_type == DATETIME_TKN
  			|| curr_tkn->tkn_type == UINT8_TKN || curr_tkn->tkn_type == UINT16_TKN
				|| curr_tkn->tkn_type == UINT32_TKN || curr_tkn->tkn_type == UINT64_TKN
  			|| curr_tkn->tkn_type == INT8_TKN || curr_tkn->tkn_type == INT16_TKN
				|| curr_tkn->tkn_type == INT32_TKN || curr_tkn->tkn_type == INT64_TKN
				|| curr_tkn->tkn_type == DOUBLE_TKN || curr_tkn->tkn_type == BOOL_TKN)	{
    	// LITERAL
  		isTknTypeOK = true;
  		next_legal_tkn_types = (BINARY_OPR8R_NXT_OK|TERNARY_OPR8R_1ST_NXT_OK|CLOSE_PAREN_NXT_OK);
  		if (isTernaryOpen())
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
			next_legal_tkn_types = (VAR_NAME_NXT_OK|LITERAL_NXT_OK|OPEN_PAREN_NXT_OK);

  	} else if ((allowed_tkn_types & POSTFIX_OPR8R_NXT_OK) && curr_tkn->tkn_type == SRC_OPR8R_TKN && !execPostfixOpr8rStr.empty())	{
			// Make user source OPR8R unambiguous for internal use
			curr_tkn->_string = execPostfixOpr8rStr;
			isTknTypeOK = true;
			next_legal_tkn_types = (BINARY_OPR8R_NXT_OK|TERNARY_OPR8R_1ST_NXT_OK|CLOSE_PAREN_NXT_OK);
			if (isTernaryOpen())
				next_legal_tkn_types |= TERNARY_OPR8R_2ND_NXT_OK;

  	} else if ((allowed_tkn_types & TERNARY_OPR8R_2ND_NXT_OK) && curr_tkn->tkn_type == SRC_OPR8R_TKN
  			&& (TERNARY_2ND & usrSrcTerms.get_type_mask(curr_tkn->_string)))	{
  		isTknTypeOK = true;
  		// TODO: Is PREFIX_OPR8R legit here?
  		next_legal_tkn_types = (VAR_NAME_NXT_OK|LITERAL_NXT_OK|PREFIX_OPR8R_NXT_OK|UNARY_OPR8R_NXT_OK|OPEN_PAREN_NXT_OK);

  	} else if ((allowed_tkn_types & TERNARY_OPR8R_1ST_NXT_OK) && curr_tkn->tkn_type == SRC_OPR8R_TKN
  			&& (TERNARY_1ST & usrSrcTerms.get_type_mask(curr_tkn->_string)))	{
    	// TERNARY_OPR8R
  		isTknTypeOK = true;
  		next_legal_tkn_types = (VAR_NAME_NXT_OK|LITERAL_NXT_OK|PREFIX_OPR8R_NXT_OK|UNARY_OPR8R_NXT_OK|OPEN_PAREN_NXT_OK);

  	} else if ((allowed_tkn_types & BINARY_OPR8R_NXT_OK) && curr_tkn->tkn_type == SRC_OPR8R_TKN && !execBinaryOpr8rStr.empty())	{
    	// BINARY_OPR8R
 			curr_tkn->_string = execBinaryOpr8rStr;
			next_legal_tkn_types = (VAR_NAME_NXT_OK|LITERAL_NXT_OK|PREFIX_OPR8R_NXT_OK|UNARY_OPR8R_NXT_OK|OPEN_PAREN_NXT_OK);
			isTknTypeOK = true;

  	} else if ((allowed_tkn_types & OPEN_PAREN_NXT_OK) && SPR8R_TKN == curr_tkn->tkn_type && 0 == curr_tkn->_string.compare(L"("))	{
    	// OPEN_PAREN
  		isTknTypeOK = true;
  		next_legal_tkn_types = (VAR_NAME_NXT_OK|LITERAL_NXT_OK|PREFIX_OPR8R_NXT_OK|UNARY_OPR8R_NXT_OK|OPEN_PAREN_NXT_OK);

  	} else if ((allowed_tkn_types & CLOSE_PAREN_NXT_OK) && SPR8R_TKN == curr_tkn->tkn_type && 0 == curr_tkn->_string.compare(L")"))	{
    	// CLOSE_PAREN
  		isTknTypeOK = true;
  		next_legal_tkn_types = (BINARY_OPR8R_NXT_OK|TERNARY_1ST|CLOSE_PAREN_NXT_OK);

  		if (isTernaryOpen())
  			next_legal_tkn_types |= TERNARY_OPR8R_2ND_NXT_OK;

  	} else if ((allowed_tkn_types & DCLR_VAR_OR_FXN_NXT_OK) && USER_WORD_TKN == curr_tkn->tkn_type && usrSrcTerms.is_valid_datatype(curr_tkn->_string))	{
  		// DCLR_VAR_OR_FXN
  		// TODO: isTknTypeOK = true;
  		// TODO: This should be handled by GeneralParser
  		next_legal_tkn_types = (0x0);
  	}

  	// TODO: FXN_CALL
  }

  return (isTknTypeOK);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
void ExpressionParser::cleanScopeStack()	{
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
int ExpressionParser::getExpectedEndToken (std::shared_ptr<Token> startTkn, uint32_t & _1stTknType, Token & expectedEndTkn, bool isEndedByComma)	{
	int ret_code = GENERAL_FAILURE;
	bool isStopFail = false;

	if (startTkn != NULL)	{
		expectedEndTkn.resetToken();
	
		if (startTkn->tkn_type == SPR8R_TKN && startTkn->_string == L"(")	{
			// TODO: What about uint32 someVar = (3 + 2 *5), <-- We could miss this in the caller
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

		} else	{
			isStopFail = true;
  		userMessages->logMsg (INTERNAL_ERROR, L"Could not determine _1stTknType for " + startTkn->descr_sans_line_num_col(), thisSrcFile, __LINE__, 0);
		}

		if (isEndedByComma)	{
			expectedEndTkn.resetToken();
			expectedEndTkn.tkn_type = SPR8R_TKN;
			expectedEndTkn._string = L",";
		}

		if (!isStopFail && expectedEndTkn.tkn_type == START_UNDEF_TKN)	{
			std::wstring statementEnder = usrSrcTerms.get_statement_ender();
			if (statementEnder.length() > 0)	{
				expectedEndTkn.tkn_type = SRC_OPR8R_TKN;
				expectedEndTkn._string = statementEnder;

			} else	{
				isStopFail = true;
	  		userMessages->logMsg (INTERNAL_ERROR, L"Could not find STATEMENT_ENDER operator! ", thisSrcFile, __LINE__, 0);
			}
		}

	  if (!isStopFail)
	  	ret_code = OK;
	}

	return (ret_code);
}

/* ****************************************************************************
 * Used an aid in debugging and instruction
 * ***************************************************************************/
 void ExpressionParser::printSingleScope (std::wstring headerMsg, int scopeLvl)	{
	if (scopeLvl >=0 && scopeLvl < exprScopeStack.size())	{
		std::wstringstream outLine;

		if (!headerMsg.empty())
			std::wcout << headerMsg << std::endl;

		outLine << L"Scope Level " << scopeLvl << L": ";
		
		std::shared_ptr<NestedScopeExpr> chosenScope = exprScopeStack[scopeLvl];
		std::vector<std::shared_ptr<ExprTreeNode>>::iterator kidR8r;

		for (kidR8r = chosenScope->scopedKids.begin(); kidR8r != chosenScope->scopedKids.end(); kidR8r++)	{
			std::shared_ptr<ExprTreeNode> currKid = *kidR8r;
			if (currKid->_1stChild != NULL)
				// Give the user a visual hint that it's a tree
				outLine << L"/";
			else
				outLine << L"[";
			outLine << currKid->originalTkn->_string;
			if (currKid->_2ndChild != NULL)
				// Give the user a visual hint that it's a tree
				outLine << L"\\";
			else
				outLine << L"]";
		}

		std::wcout << outLine.str() << std::endl;

		if (!headerMsg.empty())
			std::wcout << std::endl;

	}
}

/* ****************************************************************************
 * Used an aid in debugging and instruction
 * ***************************************************************************/
void ExpressionParser::printScopeStack (std::wstring fileName, int lineNumber)	{

	std::wstringstream banner;
	banner << L"********** ExpressionParser::printScopeStack called from " << fileName << L":" << lineNumber << L" **********";
	printScopeStack (banner.str(), false);

}

/* ****************************************************************************
 * Used an aid in debugging and instruction
 * ***************************************************************************/
 void ExpressionParser::printScopeStack (std::wstring bannerMsg, bool isUseDefault)	{
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
		printSingleScope (L"", scopeLvl);
		scopeLvl--;
	}

 }

/* ****************************************************************************
 * Used an aid in debugging.
 * ***************************************************************************/
void ExpressionParser::showDebugInfo (std::wstring srcFileName, int lineNum)	{
	printScopeStack(srcFileName, lineNum);
	if (exprScopeStack.size() > 0)	{
		ExprTreeNodePtrVector::iterator nodeR8r;

		ExprTreeNodePtrVector & childList = exprScopeStack[exprScopeStack.size() - 1]->scopedKids;

		for (nodeR8r = childList.begin(); nodeR8r != childList.end(); nodeR8r++)	{
			std::shared_ptr<ExprTreeNode> currNode = *nodeR8r;
			currNode->showTree(srcFileName, lineNum);
		}
	}
}
