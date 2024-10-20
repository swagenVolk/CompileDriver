/*
 * ExpressionParser.cpp
 *
 *  Created on: Jun 11, 2024
 *      Author: Mike Volk
 *  Class to parse and check a user source level expression.
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
#include <iostream>

#include "CompileExecTerms.h"
#include "Operator.h"

ExpressionParser::ExpressionParser(TokenPtrVector & inTknStream, CompileExecTerms & inUsrSrcTerms) {
	// TODO Auto-generated constructor stub
	this->tknStream = inTknStream;
	this->usrSrcTerms = inUsrSrcTerms;
	errOnOurSrcLineNum = 0;
	thisSrcFile = util.getLastSegment(util.stringToWstring(__FILE__), L"/");

}

ExpressionParser::~ExpressionParser() {
	// TODO Auto-generated destructor stub
	cleanScopeStack();
}

/* ****************************************************************************
 * Parse through the current expression and if it's well formed, commit it to
 * the interpreted stream. If it's not well formed, generated a clear error
 * message to the user.
 * ***************************************************************************/
int ExpressionParser::parseExpression (InterpretedFileWriter & intrprtrWriter)  {
  int ret_code = GENERAL_FAILURE;

  NestedScopeExpr * rootScope = new NestedScopeExpr ((ExprTreeNode *)NULL);
  exprScopeStack.push_back (rootScope);

	bool isWholeExprClosed = false;
  bool isFailed = false;
  bool is1stTkn = true;
  uint32_t curr_legal_tkn_types;
  uint32_t next_legal_tkn_types;
  uint32_t prev_tkn_type = 0;
  uint32_t curr_tkn_type;
  Token expectedEndTkn(START_UNDEF_TKN, L"", 0, 0);

  if (tknStream.empty())	{
		errOnOurSrcLineNum = __LINE__;
		errorMsg = L"Token stream is unexpectedly empty!";

  } else	{

  	// Consume flat stream of Tokens in current expression; attach each to an ExprTreeNode for tree transformation
		while (!isFailed && !isWholeExprClosed)	{
			if (!tknStream.empty())	{
				Token *currTkn = tknStream.front();

				if (is1stTkn)	{
					// Token that starts expression will determine how we end the expression
					// TODO: Are there valid expressions that have a ";" in the middle AND at the end?
					if (OK != getExpectedEndToken(currTkn, curr_legal_tkn_types, expectedEndTkn))	{
						isFailed = true;
						break;
					}
					is1stTkn = false;
				}

				if (!isExpectedTknType (curr_legal_tkn_types, next_legal_tkn_types, currTkn))	{
					isFailed = true;
					errOnOurSrcLineNum = __LINE__;
					errorMsg = L"Unexpected token type! Expected ";
					errorMsg.append(makeExpectedTknTypesStr(curr_legal_tkn_types));
					errorMsg.append (L" but got ");
					errorMsg.append (currTkn->description());

				} else if ((currTkn->tkn_type == OPR8R_TKN && (TERNARY_2ND & usrSrcTerms.get_type_mask(currTkn->_string)))
						&& !isTernaryOpen())	{
					isFailed = true;
					errOnOurSrcLineNum = __LINE__;
					errorMsg = L"Got middle ternary operand ";
					errorMsg.append (currTkn->description());
					errorMsg.append (L" without required preceding starting ternary operator ");
					errorMsg.append (usrSrcTerms.get_ternary_1st());

				} else if ((currTkn->tkn_type == OPR8R_TKN && (TERNARY_2ND & usrSrcTerms.get_type_mask(currTkn->_string)))
						&& get2ndTernaryCnt() > 0)	{
					isFailed = true;
					errOnOurSrcLineNum = __LINE__;
					errorMsg = L"Got unexpected additional middle ternary operand ";
					errorMsg.append (currTkn->description());

				} else if ((currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L"(")
						|| (currTkn->tkn_type == OPR8R_TKN && (TERNARY_1ST & usrSrcTerms.get_type_mask(currTkn->_string))))	{
					// Open parenthesis or 1st ternary
					if (OK != openSubExprScope())	{
						isFailed = true;
					}

				} else if (currTkn->tkn_type == SPR8R_TKN && currTkn->_string == L")")	{
					// Close parenthesis - syntactic sugar that just melts away
					tknStream.erase(tknStream.begin());
					delete currTkn;
					currTkn = NULL;
					bool isOpenParenFndYet = false;

					if (OK != closeParenClosesScope(isOpenParenFndYet))	{
						isFailed = true;
					} else if (!isOpenParenFndYet)	{
						// Nested TERNARY OPR8Rs. Need to try closing scopes until we
						// hit the originating "(" that we opened with
						int stackHtB4, stackHtAfter;

						while (!isOpenParenFndYet && !isFailed)	{
							stackHtB4 = exprScopeStack.size();
							if (OK != closeParenClosesScope(isOpenParenFndYet))	{
								isFailed = true;
								break;

							} else	{
								stackHtAfter = exprScopeStack.size();
								if (stackHtAfter >= stackHtB4)	{
									isFailed = true;
									errOnOurSrcLineNum = __LINE__;
									errorMsg = L"closeSubExprScope did not reduce expression scope depth. Before call:  ";
									errorMsg.append (std::__cxx11::to_wstring(stackHtB4));
									errorMsg.append (L"; After call: ");
									errorMsg.append (std::__cxx11::to_wstring(stackHtAfter));
									errorMsg.append (L";");
								}
							}

							if (!isFailed)	{
								// Check if the expression has been collapsed down to a single ExprTreeNode
								if (exprScopeStack.size() == 1 && exprScopeStack[0]->scopedKids.size() == 1)	{
									// TODO: Double check tree health before calling it a day?
									isWholeExprClosed = true;
								}
							}
						}
					} else	{
						// Check if this also completes our expression
						if (exprScopeStack.size() == 1)	{
							// isExprScope TODO
							if (1 == exprScopeStack[0]->scopedKids.size())
								// TODO: Double check tree health before calling it a day?
								isWholeExprClosed = true;
						}
					}
				} else if (currTkn->tkn_type == END_OF_STREAM_TKN) {
					isFailed = true;
					errOnOurSrcLineNum = __LINE__;
					errorMsg = L"parseExpression should never hit END_OF_STREAM_TKN!";

				} else	{
					// Remove Token from stream without destroying - move to flat expression in current scope
					tknStream.erase(tknStream.begin());
					ExprTreeNode * treeNode = new ExprTreeNode (currTkn);

					if (expectedEndTkn.tkn_type == OPR8R_TKN && currTkn->tkn_type == OPR8R_TKN && currTkn->_string == expectedEndTkn._string)	{
						// TODO: Recursively wrap up the expression scope stack (Put ; at the root and what should be a single ExprTreeNode as the 1st child
						isWholeExprClosed = true;
					} else	{
						exprScopeStack[exprScopeStack.size() - 1]->scopedKids.push_back (treeNode);

						if (currTkn->tkn_type == OPR8R_TKN && (TERNARY_2ND & usrSrcTerms.get_type_mask(currTkn->_string)))
							// Keep track of secondary ternary operators; expecting only 1 paired with 1st ternary, which
							// opened a new scope inside the expression
							exprScopeStack[exprScopeStack.size() - 1]->ternary2ndCnt++;
					}
				}

				curr_legal_tkn_types = next_legal_tkn_types;
				// Only reason to look back at previous Token is for a postfix opr8r; must be a VAR_NAME to be valid.  Can't have been converted to an lvalue
				prev_tkn_type = curr_tkn_type;
			}
		}

		if (isWholeExprClosed && !isFailed)	{
			ret_code = OK;
			exprScopeStack[0]->scopedKids[0]->showTree(exprScopeStack[0]->scopedKids[0], thisSrcFile, __LINE__);
			intrprtrWriter.writeExpressionToFile(exprScopeStack[0]->scopedKids[0]);
			// TODO: Write expression out to the the interpretable file

		}
  }


  if (ret_code != OK) {
  	if (errorMsg.length() == 0)	{
  		std::wcout << " ERROR: Failure occurred but errorMsg was not set!" << std::endl;

		} else	{
			std::wcout << "ERROR: " << errorMsg << std::endl;
	  	std::wcout << "Error encountered on internal source " << thisSrcFile << ":" << errOnOurSrcLineNum << std::endl;
		}
  }

  cleanScopeStack();

  return (ret_code);
}

/* ****************************************************************************
 * Next Token is either a closing parenthesis or a statement ending ";"
 * Consume the closing ')'. It is syntactic sugar that doesn't need to be kept.
 * For a STATEMENT_ENDER (eg ";"), this will be kept as the root of the
 * expression tree and exercised last when the expression is executed either in
 * compile mode or interpreter mode.
 * Attempt to resolve the expression in this scope to
 * a single codeToken by working through the precedence of the operators.
 * Decrease the current scope level of the expression
 *
 * TODO: When the closed scope gets collapsed into a tree and linked to its
 * parent scope, we need to see if the parent scope can be collapsed also &
 * recursively.  It's turtles all the way down!
 * ***************************************************************************/
int ExpressionParser::closeParenClosesScope (bool & isOpenParenFndYet)  {
  int ret_code = GENERAL_FAILURE;
	bool isScopenerFound = false;
	bool isExprAttached = false;
	bool isScopeTopTernary = false;
	bool isFailed = false;

	// Start off expecting we won't find the matching opening '(' at this scope
	isOpenParenFndYet = false;

	// Get pointer to ExprTreeNode that contains the '(' or [?] that opened the top scope
	// This fxn was called when we encountered a ')', but the corresponding '(' could be
	// several scope levels deep if we've got nested TERNARY OPR8Rs
	ExprTreeNode * scopener = NULL;
	std::wstring scopenerDesc;

	if (exprScopeStack.size() >= 2)	{
		scopener = (ExprTreeNode *)(exprScopeStack[exprScopeStack.size() - 1]->myParentScopener);

	}
	if (scopener == NULL)	{
		errorMsg = L"ExprTreeNode that opens current scope was not set earlier";
		errOnOurSrcLineNum = __LINE__;

	} else {
		scopenerDesc = scopener->originalTkn->description();
		if (scopener->originalTkn->tkn_type == SPR8R_TKN && 0 == scopener->originalTkn->_string.compare(L"("))	{
			isOpenParenFndYet = true;
			// TODO: Might be hurting myself here.....
			// exprScopeStack[exprScopeStack.size() - 1]->myParentScopener = NULL;

		} else if (scopener->originalTkn->tkn_type == OPR8R_TKN && scopener->originalTkn->_string == usrSrcTerms.get_ternary_1st())	{
			isScopeTopTernary = true;

		} else	{
			isFailed = true;
			errorMsg = L"Current scope NOT opened by either '(' or ";
			errorMsg.append (usrSrcTerms.get_ternary_1st());
			errorMsg.append(L" but with ");
			errorMsg.append(scopenerDesc);
			errOnOurSrcLineNum = __LINE__;
		}

		if (!isFailed && isOpenParenFndYet != isScopeTopTernary)	{
			// Call turnClosedScopeIntoTree to collapse the current flat list of ExprTreeNodes into
			// a hierarchical tree based on OPR8R precedence - a root ExprTreeNode
			if (OK == turnClosedScopeIntoTree (exprScopeStack[exprScopeStack.size() - 1]->scopedKids))	{
				// Update the placeholder "(" ExprTreeNode so that it now points to the parent ExprTreeNode
				// TODO: After expression is completed, then remove the intermediary "(" objects ???????
				NestedScopeExpr * stackTop = exprScopeStack[exprScopeStack.size() - 1];

				ExprTreeNode * subExpr = stackTop->scopedKids[0];
				exprScopeStack.erase(exprScopeStack.end());
				delete stackTop;

				int stackSize = exprScopeStack.size();

				if (stackSize >= 1)	{
					ExprTreeNodePtrVector::iterator nodeR8r;
					int currStackIdx = stackSize - 1;
					// Make an alias variable for code readability
					ExprTreeNodePtrVector & childList = exprScopeStack[currStackIdx]->scopedKids;

					for (nodeR8r = childList.begin(); nodeR8r != childList.end() && !isScopenerFound && !isFailed; nodeR8r++)	{
						ExprTreeNode * currNode = *nodeR8r;

						if ((void *)currNode == (void *)scopener)	{
							// Found the branch that opened the previous scope. Now attach subExpr to it
							isScopenerFound = true;

							if (isOpenParenFndYet)	{
								// Don't make our sub-expression a child to the "(" that opened our scope
								// Replace it with our sub-expression since the "(" is vestigial at this point
								// List clean-up pending after this loop
								// TODO: myParent?
								childList.insert (nodeR8r, subExpr);
								isExprAttached = true;

							} else if (isScopeTopTernary && subExpr->originalTkn->_string == usrSrcTerms.get_ternary_2nd())	{
								// TODO: Double check
								scopener->_2ndChild = subExpr;
								subExpr->myParent = scopener;
								isExprAttached = true;
								ret_code = OK;

							} else if (isScopeTopTernary && subExpr->originalTkn->_string != usrSrcTerms.get_ternary_2nd())	{
								// Scope was opened by TERNARY_1ST, and root of subExpr is NOT direct TERNARY_2ND
								// The TERNARY_2ND *may* be buried deeper in the tree; probably below an assignment OPR8R
								// that has lower priority. Tree precedence is inverted; deeper|lower in tree means higher precedence
								// ********** EXPRESSION TREE **********
								// ROOT[=]
								// [desc, :]
								//       (2nd)[Third, ?]
								//                (2nd)[:, ==]
								//        (1st)[four, MANY](2nd)[count, 4]
								scopener->_2ndChild = subExpr;
								subExpr->myParent = scopener;
								isExprAttached = true;
								ret_code = OK;

								ExprTreeNode * child1 = subExpr->_1stChild;
								ExprTreeNode * child2 = subExpr->_2ndChild;
								if (child2 != NULL && child2->originalTkn != NULL && child2->originalTkn->_string == usrSrcTerms.get_ternary_1st())	{
									// TODO: Is there any reason to check child1?
									scopener->_2ndChild = subExpr;
									subExpr->myParent = scopener;
									isExprAttached = true;
									ret_code = OK;

								} else	{
									errOnOurSrcLineNum = __LINE__;
									isFailed = true;
									errorMsg = L"Current scope opened by ";
									errorMsg.append (usrSrcTerms.get_ternary_1st());
									errorMsg.append(L" but could not find paired ");
									errorMsg.append(usrSrcTerms.get_ternary_2nd());
								}
							}
						}
					}

					if (isOpenParenFndYet && isExprAttached)	{
						// Need to remove that vestigial "(" from the list
						bool isVestigeDeleted = false;
						ExprTreeNodePtrVector::iterator delR8r;

						for (delR8r = childList.begin(); delR8r != childList.end() && !isVestigeDeleted; delR8r++)	{
							ExprTreeNode * currBranch = (ExprTreeNode *)*delR8r;
							if (currBranch == scopener)	{
								delR8r = childList.erase(delR8r);
								delete ((ExprTreeNode *)scopener);
								scopener = NULL;
								isVestigeDeleted = true;
								ret_code = OK;
							}
						}
						if (!isVestigeDeleted)	{
							errOnOurSrcLineNum = __LINE__;
							isFailed = true;
							errorMsg = L"INTERNAL ERROR: Failed deleting vestigial ";
							errorMsg.append (scopenerDesc);
						}
					}

					if (ret_code != OK)	{
						if (!isScopenerFound || !isExprAttached)	{
							errOnOurSrcLineNum = __LINE__;
							if (!isScopenerFound)
								errorMsg = L"Could not match with scope opened by ";
							else
								errorMsg = L"Failed to attach resolved sub-expression with scope opened by ";
							errorMsg.append (scopenerDesc);
							errorMsg.append (L" with current scope stack level = ");
							// TODO: Why __cxx11:: ? Seems weird......and very version specific!
							errorMsg.append (std::__cxx11::to_wstring(exprScopeStack.size() - 1));
							printScopeStack(thisSrcFile, __LINE__);

						} else	{
							errorMsg = L"Me no SASA!!!";
							errOnOurSrcLineNum = __LINE__;
							if (subExpr != NULL)	{
								subExpr->showTree(subExpr, thisSrcFile, __LINE__);
							}
						}
					}
				}
			}
			// After exiting, we'll continue on with Tokens 1 scope up.  Could be another ")" that closes
			// the now current scope, or other kinds of Tokens that continue the current sub-expression.
		}
	}

	if (ret_code != OK && errorMsg.length() == 0)	{
		errorMsg = L"Unhandled INTERNAL ERROR";
		errOnOurSrcLineNum = __LINE__;
	}

	return (ret_code);
}

/* ****************************************************************************
 * For this sub-expression contained in 1 scope level (ie inside 1 set of parentheses)
 * work through the precedence ordered list of OPR8Rs make a shrubbery
 * Should wind up as a single OPR8R at the top of a shrub/tree
 * TODO: This might be where L2R and R2L associativity comes in
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
int ExpressionParser::turnClosedScopeIntoTree (ExprTreeNodePtrVector & currScope)  {
  int ret_code = GENERAL_FAILURE;
  bool isFailed = false;
  bool isReachedEOL = false;
  bool isNowTree = false;

  std::vector<Operator>::iterator opr8rItr8r;
  std::wstring tgtOpr8r;
  ExprTreeNodePtrVector::iterator prevNodeR8r;
  ExprTreeNodePtrVector::iterator currNodeR8r;
  ExprTreeNodePtrVector::iterator nextNodeR8r;
  ExprTreeNode * prevNode = NULL;
  ExprTreeNode * currNode = NULL;
  ExprTreeNode * nextNode = NULL;

  std::list<Opr8rPrecedenceLvl>::iterator outr8r;
  std::list<Operator>::iterator innr8r;
  Opr8rPrecedenceLvl precedenceLvl;

  std::wstring unary1stOpr8r = usrSrcTerms.get_ternary_1st();
  std::wstring unary2ndOpr8r = usrSrcTerms.get_ternary_2nd();

  // TODO: Code inspect, comment, simplify if you can
	for (outr8r = usrSrcTerms.grouped_opr8rs.begin(); outr8r != usrSrcTerms.grouped_opr8rs.end() && !isFailed && !isNowTree; outr8r++){
		// Move through each precedence level of OPR8Rs. Note that some precedence levels will have multiple OPR8Rs and they must be
		// treated as having the same precedence, and therefore we can't rely on an ABSOUTE ordering of OPR8R precedence
		precedenceLvl = *outr8r;

		isReachedEOL = false;

		while (!isReachedEOL && !isFailed && !isNowTree)	{
			bool isOpr8rReady = false;

			for (currNodeR8r = currScope.begin(); currNodeR8r != currScope.end() && !isFailed && !isNowTree && !isOpr8rReady; currNodeR8r++)	{
				// Move left-to-right through the expression. When an OPR8R is encountered, check to see if it matches any of the OPR8Rs
				// within the current precedence level
				// When objects are moved around, the iterator will be invalidated and need to restart.

				currNode = *currNodeR8r;
				// Make an alias variable for code readability
				Token * currTkn = currNode->originalTkn;

				if (currNode == NULL)	{
					errorMsg = L"ExprTreeNode pointer is NULL! ";
					errOnOurSrcLineNum = __LINE__;
					isFailed = true;

				} else if (currTkn->tkn_type == OPR8R_TKN && ((currTkn->_string != unary1stOpr8r && currNode->_1stChild == NULL)
						|| (currTkn->_string == unary1stOpr8r && currNode->_2ndChild == NULL)))	{
					// IFF OPR8R at current precedence level is encountered at this expression scope, we can move some|all neighbors under this OPR8R
					Operator tgtOpr8r(L"",0,0,0);

					// TODO: Could improve on this loop by re-starting it AFTER the last OPR8R completed on the previous loop
					for (innr8r = precedenceLvl.opr8rs.begin(); innr8r != precedenceLvl.opr8rs.end() && !isFailed && !isOpr8rReady; ++innr8r){
						Operator pssblOpr8r = *innr8r;

						if (pssblOpr8r.valid_for_mask & GNR8D_SRC && currNode->originalTkn->_string == pssblOpr8r.symbol)	{
							// Filter out OPR8Rs only valid for USER_SRC ([pre|post]-fix ++,--; unary +|- ) and match on unique GNR8D_SRC equivalent
							// OPR8R match and it hasn't had any child nodes attached to it yet
							isOpr8rReady = true;
							// TODO: Could use an "=" operator override for Operator class
							tgtOpr8r.symbol = pssblOpr8r.symbol;
							tgtOpr8r.numOperands = pssblOpr8r.numOperands;
							tgtOpr8r.type_mask = pssblOpr8r.type_mask;
							tgtOpr8r.valid_for_mask = pssblOpr8r.valid_for_mask;
						}

						if (isOpr8rReady)	{
							nextNode = NULL;
							prevNode = NULL;

							if ((tgtOpr8r.type_mask & POSTFIX) || (tgtOpr8r.type_mask & BINARY) || (tgtOpr8r.type_mask & TERNARY_2ND)
									|| (tgtOpr8r.type_mask & TERNARY_1ST /* TODO: && currNode->_2ndChild != NULL*/))	{
								// Remove previous operand node in list and attach to _1stChild
								// TERNARY_2ND (:) treated similarly to BINARY at this juncture

								// For TERNARY_1ST:
								// Both the TRUE and the FALSE paths of the TERNARY statement have been resolved at deeper scopes
								// The conditional that precedes the ? OPR8R should also have been resolved (operator precedence)

								if (currNodeR8r != currScope.begin())	{
									prevNodeR8r = currNodeR8r;
									prevNodeR8r--;
									prevNode = *prevNodeR8r;

									if (!(tgtOpr8r.type_mask & POSTFIX) || (prevNode->originalTkn->tkn_type == KEYWORD_TKN)) {
										// POSTFIX OPR8R ([++|--] only works on a variable. Note that BINARY OPR8Rs can work
										// with OPR8Rs to the left and right if they're at the top of a sub-expression tree
										if (tgtOpr8r.type_mask & TERNARY_1ST)
											// TODO: Reversing expected order on TERNARY
											currNode->_2ndChild = prevNode;
										else
											currNode->_1stChild = prevNode;
										prevNode->myParent = currNode;
									} else	{
										isFailed = true;
										errorMsg = L"Expected an variable name before ";
										errorMsg.append (currNode->originalTkn->description());
										errorMsg.append (L" but instead got: ");
										errorMsg.append (prevNode->originalTkn->description());
										errOnOurSrcLineNum = __LINE__;
									}

								} else	{
									isFailed = true;
									errorMsg = L"Expected a Token before ";
									errorMsg.append (currNode->originalTkn->description());
									errorMsg.append (L" but nothing precedes it at current scope of expression.");
									errOnOurSrcLineNum = __LINE__;
								}
							}

							if ((tgtOpr8r.type_mask & PREFIX) || (tgtOpr8r.type_mask & UNARY)
									|| ((tgtOpr8r.type_mask & BINARY) && TERNARY_1ST != (tgtOpr8r.type_mask & TERNARY_1ST)))	{
								// Remove next operand node in list and attach to _1stChild or _2ndChild, depending on OPR8R type
								// TERNARY_2ND (:) treated similarly to BINARY at this juncture
								nextNodeR8r = currNodeR8r;
								nextNodeR8r++;
								if (nextNodeR8r != currScope.end())	{
									nextNode = *nextNodeR8r;

									if (!(tgtOpr8r.type_mask & PREFIX) || (nextNode->originalTkn->tkn_type == KEYWORD_TKN)) {
										// PREFIX OPR8Rs can only work on variables

										if ((tgtOpr8r.type_mask & PREFIX) || (tgtOpr8r.type_mask & UNARY))
											currNode->_1stChild = nextNode;
										else
											currNode->_2ndChild = nextNode;

										nextNode->myParent = currNode;

									} else {
										isFailed = true;
										errorMsg = L"Expected a keyword after ";
										errorMsg.append (currNode->originalTkn->description());
										errorMsg.append (L" but instead got: ");
										errorMsg.append (nextNode->originalTkn->description());
										errOnOurSrcLineNum = __LINE__;
									}
								} else	{
									isFailed = true;
									errorMsg = L"Expected a Token after ";
									errorMsg.append (currNode->originalTkn->description());
									errorMsg.append (L" but nothing follows it at current scope of expression.");
									errOnOurSrcLineNum = __LINE__;
								}
							}

							if (prevNode != NULL || nextNode != NULL)	{
								// Need to do some housekeeping on our list
								ExprTreeNodePtrVector::iterator delR8r;
								for (delR8r = currScope.begin(); delR8r != currScope.end();)	{
									// For moved nodes, remove from list in reverse order
									if (nextNode != NULL && (void *)*delR8r == (void *)nextNode)	{
										currScope.erase (delR8r);
									}
									else if (prevNode != NULL && (void *)*delR8r == (void *)prevNode)	{
										currScope.erase (delR8r);
									} else {
										delR8r++;
									}
								}

								if (currScope.size() == 1 && !isFailed)	{
									// Flat list of multiple ExprTreeNodes has been TREED down to 1
									isNowTree = true;

								} else if (currScope.size() == 2 && !isFailed)	{
									// Special case check for TERNARY that has HIGHER PRECEDENCE and has already the 2nd child
									// resolved but 1st child is still NULL (because TERNARYs swap Op1<->Op2 position
									// ROOT[=]							            ROOT[?]
									// [desc, :]						            [NULL,:]
									//       (2nd)[Third, ==]			            (2nd)[four, MANY]
									//                  (2nd)[count, 4]
									// and move the other ExprTreeNode underneath ? _1stChild
									Token * branch0Tkn = currScope[0]->originalTkn;
									Token * branch1Tkn = currScope[1]->originalTkn;
									if (branch1Tkn->_string == usrSrcTerms.get_ternary_1st() && currScope[1]->_1stChild == NULL)	{

										delR8r = currScope.begin();
										ExprTreeNode * kidToMove = NULL;
										kidToMove = *delR8r;
										currScope.erase(delR8r);
										currScope[0]->_1stChild = kidToMove;
										isNowTree = true;
									}
								}
							}
						}
					}
				}
			}

			if (currNodeR8r == currScope.end())
				isReachedEOL = true;
		}
	}

	if (!isFailed) {
		// TODO: Check for only OPR8Rs left @ currScope? Check if they're treed up?
		if (isNowTree)
			ret_code = OK;
		else if (currScope.size() == 1)
			ret_code = OK;
		else	{
			errorMsg = L"Unhandled INTERNAL ERROR - Could not make tree!";
			errOnOurSrcLineNum = __LINE__;
			printScopeStack(thisSrcFile, __LINE__);
			if (exprScopeStack.size() > 0)	{
				ExprTreeNodePtrVector::iterator nodeR8r;

				ExprTreeNodePtrVector & childList = exprScopeStack[exprScopeStack.size() - 1]->scopedKids;

				for (nodeR8r = childList.begin(); nodeR8r != childList.end(); nodeR8r++)	{
					ExprTreeNode * currNode = *nodeR8r;
					currNode->showTree(currNode, thisSrcFile, __LINE__);
				}
			}
		}
	}

	if (ret_code != OK && errorMsg.length() == 0)	{
		errorMsg = L"Unhandled INTERNAL ERROR";
		errOnOurSrcLineNum = __LINE__;
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
		expected.append(L"KEYWORD (variable name)");
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
		NestedScopeExpr * currScope = exprScopeStack[stackSize - 1];
		if (currScope->myParentScopener != NULL)	{
			ExprTreeNode * scopener = (ExprTreeNode *)currScope->myParentScopener;

			if (scopener->originalTkn->tkn_type == OPR8R_TKN
					&& (TERNARY_1ST & usrSrcTerms.get_type_mask(scopener->originalTkn->_string)))
				isT3rnOpen = true;
		}
	}

	return isT3rnOpen;
}

/* ****************************************************************************
 * Return the count of TERNARY_2ND OPR8Rs encountered at the current scope.
 * There should be only 1 per scope
 * ***************************************************************************/
int ExpressionParser::get2ndTernaryCnt ()  {
  int count = 0;

	int stackSize = exprScopeStack.size();
	if (stackSize > 0)	{
		NestedScopeExpr * currScope = exprScopeStack[exprScopeStack.size() - 1];
		count = currScope->ternary2ndCnt;
	}

	return count;
}

/* ****************************************************************************
 * Centralized logic determines if the current Token is legal given the allowed
 * Token types. If legal, sets the type(s) the legal types for the next Token.
 * ***************************************************************************/
bool ExpressionParser::isExpectedTknType (uint32_t allowed_tkn_types, uint32_t & next_legal_tkn_types, Token * curr_tkn)  {
  bool isTknTypeOK = false;

  if (curr_tkn != NULL)	{
  	if ((allowed_tkn_types & VAR_NAME_NXT_OK) && curr_tkn->tkn_type == KEYWORD_TKN
 	  	// VAR_NAME
  		// TODO: && found in NameSpace
  		)	{
  		isTknTypeOK = true;
  		next_legal_tkn_types = (POSTFIX_OPR8R_NXT_OK|BINARY_OPR8R_NXT_OK|TERNARY_OPR8R_1ST_NXT_OK|CLOSE_PAREN_NXT_OK);
  		if (isTernaryOpen())
  			next_legal_tkn_types |= TERNARY_OPR8R_2ND_NXT_OK;
  	}

  	// LITERAL
  	if ((allowed_tkn_types & LITERAL_NXT_OK) && curr_tkn->tkn_type == STRING_TKN || curr_tkn->tkn_type == DATETIME_TKN
  			|| curr_tkn->tkn_type == UINT8_TKN || curr_tkn->tkn_type == UINT16_TKN
				|| curr_tkn->tkn_type == UINT32_TKN || curr_tkn->tkn_type == UINT64_TKN
  			|| curr_tkn->tkn_type == INT8_TKN || curr_tkn->tkn_type == INT16_TKN
				|| curr_tkn->tkn_type == INT32_TKN || curr_tkn->tkn_type == INT64_TKN
				|| curr_tkn->tkn_type == DOUBLE_TKN)	{
  		isTknTypeOK = true;
  		next_legal_tkn_types = (BINARY_OPR8R_NXT_OK|TERNARY_OPR8R_1ST_NXT_OK|CLOSE_PAREN_NXT_OK);
  		if (isTernaryOpen())
  			next_legal_tkn_types |= TERNARY_OPR8R_2ND_NXT_OK;
  	}

  	// PREFIX_OPR8R
  	if ((allowed_tkn_types & PREFIX_OPR8R_NXT_OK) && curr_tkn->tkn_type == OPR8R_TKN
			&& (PREFIX & usrSrcTerms.get_type_mask(curr_tkn->_string)))	{
  		isTknTypeOK = true;
  		// Make user source OPR8R unambiguous for internal use
  		// TODO: Could have a separate Token member variable for this and keep user OPR8R around
  		// TODO: Do I mark this as an R-value now, or is this implied somehow via the OPR8R?
  		curr_tkn->_string = usrSrcTerms.getUniqPrefixOpr8r(curr_tkn->_string);
  		next_legal_tkn_types = (VAR_NAME_NXT_OK);
  	}

  	// UNARY_OPR8R
  	if ((allowed_tkn_types & UNARY_OPR8R_NXT_OK) && curr_tkn->tkn_type == OPR8R_TKN
			&& (UNARY & usrSrcTerms.get_type_mask(curr_tkn->_string)))	{
  		isTknTypeOK = true;
  		// Make user source OPR8R unambiguous for internal use
  		// TODO: Could have a separate Token member variable for this and keep user OPR8R around
  		// TODO: Do I mark this as an R-value now, or is this implied somehow via the OPR8R?
  		curr_tkn->_string = usrSrcTerms.getUniqUnaryOpr8r(curr_tkn->_string);
  		next_legal_tkn_types = (VAR_NAME_NXT_OK|LITERAL_NXT_OK|CLOSE_PAREN_NXT_OK);
  	}

  	// POSTFIX_OPR8R
  	if ((allowed_tkn_types & POSTFIX_OPR8R_NXT_OK) && curr_tkn->tkn_type == OPR8R_TKN
			&& (POSTFIX & usrSrcTerms.get_type_mask(curr_tkn->_string)))	{
  		isTknTypeOK = true;
  		// Make user source OPR8R unambiguous for internal use
  		// TODO: Could have a separate Token member variable for this and keep user OPR8R around
  		// TODO: Do I mark this as an R-value now, or is this implied somehow via the OPR8R?
  		curr_tkn->_string = usrSrcTerms.getUniqPostfixOpr8r(curr_tkn->_string);
  		// TODO: Double check TERNARY
  		next_legal_tkn_types = (BINARY_OPR8R_NXT_OK|TERNARY_OPR8R_1ST_NXT_OK|CLOSE_PAREN_NXT_OK);
  		if (isTernaryOpen())
  			next_legal_tkn_types |= TERNARY_OPR8R_2ND_NXT_OK;
  	}

  	// BINARY_OPR8R
  	if ((allowed_tkn_types & BINARY_OPR8R_NXT_OK) && curr_tkn->tkn_type == OPR8R_TKN
			&& (BINARY & usrSrcTerms.get_type_mask(curr_tkn->_string)))	{
  		isTknTypeOK = true;
  		next_legal_tkn_types = (VAR_NAME_NXT_OK|LITERAL_NXT_OK|PREFIX_OPR8R_NXT_OK|UNARY_OPR8R_NXT_OK|OPEN_PAREN_NXT_OK);
  	}

  	// TERNARY_OPR8R
  	if ((allowed_tkn_types & TERNARY_OPR8R_1ST_NXT_OK) && curr_tkn->tkn_type == OPR8R_TKN
  			&& (TERNARY_1ST & usrSrcTerms.get_type_mask(curr_tkn->_string)))	{
  		isTknTypeOK = true;
  		// TODO: Is PREFIX_OPR8R legit here?
  		next_legal_tkn_types = (VAR_NAME_NXT_OK|LITERAL_NXT_OK|UNARY_OPR8R_NXT_OK|OPEN_PAREN_NXT_OK);
  	}

  	if ((allowed_tkn_types & TERNARY_OPR8R_2ND_NXT_OK) && curr_tkn->tkn_type == OPR8R_TKN
  			&& (TERNARY_2ND & usrSrcTerms.get_type_mask(curr_tkn->_string)))	{
  		isTknTypeOK = true;
  		// TODO: Is PREFIX_OPR8R legit here?
  		next_legal_tkn_types = (VAR_NAME_NXT_OK|LITERAL_NXT_OK|UNARY_OPR8R_NXT_OK|OPEN_PAREN_NXT_OK);
  	}

  	if ((allowed_tkn_types & OPEN_PAREN_NXT_OK) && SPR8R_TKN == curr_tkn->tkn_type && 0 == curr_tkn->_string.compare(L"("))	{
    	// OPEN_PAREN
  		isTknTypeOK = true;
  		next_legal_tkn_types = (VAR_NAME_NXT_OK|LITERAL_NXT_OK|PREFIX_OPR8R_NXT_OK|UNARY_OPR8R_NXT_OK|OPEN_PAREN_NXT_OK);
  	}

  	if ((allowed_tkn_types & CLOSE_PAREN_NXT_OK) && SPR8R_TKN == curr_tkn->tkn_type && 0 == curr_tkn->_string.compare(L")"))	{
    	// CLOSE_PAREN
  		isTknTypeOK = true;
  		next_legal_tkn_types = (BINARY_OPR8R_NXT_OK|TERNARY_1ST|CLOSE_PAREN_NXT_OK);

  		if (isTernaryOpen())
  			next_legal_tkn_types |= TERNARY_OPR8R_2ND_NXT_OK;

  	}

  	if ((allowed_tkn_types & DCLR_VAR_OR_FXN_NXT_OK) && KEYWORD_TKN == curr_tkn->tkn_type && usrSrcTerms.is_valid_datatype(curr_tkn->_string))	{
  		// DCLR_VAR_OR_FXN
  		// TODO: isTknTypeOK = true;
  		next_legal_tkn_types = (0x0);
  	}

  	// FXN_CALL

  }

  return (isTknTypeOK);
}

/* ****************************************************************************
 *
 * ***************************************************************************/
void ExpressionParser::cleanScopeStack()	{
	std::vector<NestedScopeExpr *>::reverse_iterator scopeR8r;
	std::vector<ExprTreeNode *>::reverse_iterator kidR8r;
	int lastKidIdx;

	// Clean up our allocated memory
	for (scopeR8r = exprScopeStack.rbegin(); scopeR8r != exprScopeStack.rend(); scopeR8r++)	{
		NestedScopeExpr * currScope = *scopeR8r;

		while (!currScope->scopedKids.empty())	{
			lastKidIdx = currScope->scopedKids.size() - 1;
			ExprTreeNode * lastKid = currScope->scopedKids[lastKidIdx];
			currScope->scopedKids.pop_back();
			delete (lastKid);
		}

		exprScopeStack.pop_back();
		delete (currScope);
	}
}

/* ****************************************************************************
 * Next Token is a opening parenthesis or an opening ternary (?)
 * Increase the current scope level of the expression
 * ***************************************************************************/
int ExpressionParser::openSubExprScope ()  {
  int ret_code = GENERAL_FAILURE;

	Token *currTkn = tknStream.front();
	// Remove the Token from the stream without destroying it
	tknStream.erase(tknStream.begin());
	ExprTreeNode * branchNode = new ExprTreeNode (currTkn);
	exprScopeStack[exprScopeStack.size() - 1]->scopedKids.push_back (branchNode);

	// Create enclosed scope with pointer back to originating ExprTreeNode/Token (probably an open paren)
	NestedScopeExpr * nextScope = new NestedScopeExpr (branchNode);
	// We need to point back to the node that opened this new scope
	nextScope->myParentScopener = branchNode;
	exprScopeStack.push_back (nextScope);
	ret_code = OK;

	return (ret_code);
}

/* ****************************************************************************
 * What indicates the ending legal Token that closes out the expression?
 * Depends on what opened it up
 * OPEN_PAREN - Expression ends with a ")" of the same scope
 * VAR_NAME - Expression ends with a ";"
 * LITERAL - Expression ends with a ";"
 * FXN_CALL - Begin Token is a KEYWORD representing a previously defined fxn call.  Expression ends with a ";"
 * PREFIX_OPR8R - e.g. ++idx;  Expression ends with a ";"
 * 	TODO:  ++(idx); ++((idx)); ++(((idx))) + jdx++; etc. are legal statements
 * UNARY_OPR8R - A bit non-sensical, but is still legal (e.g. ~jdx;).  Expression ends with a ";"
 *
 * DCLR_VAR_OR_FXN - Begin Token is a KEYWORD representing a datatype.  Expression ends with a ";"
 * 	TODO: Handle this via a different fxn to make life easier.
 *
 * ***************************************************************************/
int ExpressionParser::getExpectedEndToken (Token * startTkn, uint32_t & _1stTknType, Token & expectedEndTkn)	{
	int ret_code = GENERAL_FAILURE;

	if (startTkn != NULL)	{
		if (startTkn->tkn_type == SPR8R_TKN && startTkn->_string == L"(")	{
			expectedEndTkn.tkn_type = SPR8R_TKN;
			expectedEndTkn._string = L")";
			_1stTknType = OPEN_PAREN_NXT_OK;
			ret_code = OK;

		} else if (startTkn->tkn_type == KEYWORD_TKN)	{
			// Must be [VAR_NAME|FXN_CALL] that currently exists in the NameSpace

		} else if ((startTkn->tkn_type == OPR8R_TKN && ((PREFIX|UNARY) & usrSrcTerms.get_type_mask(startTkn->_string)))
			|| startTkn->tkn_type == STRING_TKN
			|| startTkn->tkn_type == DATETIME_TKN
			|| startTkn->tkn_type == UINT16_TKN
			|| startTkn->tkn_type == UINT32_TKN
			|| startTkn->tkn_type == UINT64_TKN
			|| startTkn->tkn_type == INT16_TKN
			|| startTkn->tkn_type == INT32_TKN
			|| startTkn->tkn_type == INT64_TKN
			|| startTkn->tkn_type == DOUBLE_TKN)	{
			expectedEndTkn.tkn_type = OPR8R_TKN;

			std::wstring statementEnder = usrSrcTerms.get_statement_ender();
			if (statementEnder.length() > 0)	{
				expectedEndTkn._string = statementEnder;
		  	ret_code = OK;
		  }

		  if (ret_code != OK)	{
	  		errOnOurSrcLineNum = __LINE__;
	  		// TODO: This is an internal error. How should this be marked and expressed?
	  		errorMsg = L"Could not find STATEMENT_ENDER operator! ";
		  }
		}
	}

	return ret_code;
}

/* ****************************************************************************
 * Used an aid in debugging.
 * ***************************************************************************/
void ExpressionParser::printScopeStack (std::wstring fileName, int lineNumber)	{

	std::vector<NestedScopeExpr *>::reverse_iterator scopeR8r;
	std::vector<ExprTreeNode *>::iterator kidR8r;
	int scopeLvl = exprScopeStack.size() - 1;

	std::wcout << L"********** ExpressionParser::printScopeStack called from " << fileName << L":" << lineNumber << L" **********" << std::endl;
	for (scopeR8r = exprScopeStack.rbegin(); scopeR8r != exprScopeStack.rend(); scopeR8r++)	{
		std::wcout << L"Scope Level " << scopeLvl << L":";
		NestedScopeExpr * currScope = *scopeR8r;
		for (kidR8r = currScope->scopedKids.begin(); kidR8r != currScope->scopedKids.end(); kidR8r++)	{
			ExprTreeNode * currKid = *kidR8r;
			std::wcout << L" " << currKid->originalTkn->_string;
		}

		// Done with this scope level
		std::wcout << std::endl;
		scopeLvl--;
	}
}
