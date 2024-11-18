/*
 * ExprTreeNode.cpp
 * This class holds a Token that originally came from the user's source file.  It has multiple child pointers that allow for linking
 * multiple ExprTreeNode instances together in hierarchical tree structure.
 * TODO: Check that myParent is getting filled in properly.
 *
 *  Created on: Jun 5, 2024
 *      Author: Mike Volk
 */

#include "ExprTreeNode.h"

ExprTreeNode::ExprTreeNode(std::shared_ptr<Token> startOpr8rTkn) {
	// Attach this shared_ptr to an already existing shared_ptr of type Token
	originalTkn = { startOpr8rTkn };
  myParent = NULL;
  _1stChild = NULL;
  _2ndChild = NULL;
}

ExprTreeNode::~ExprTreeNode() {
	// Clean up our allocated memory
	originalTkn.reset();
	_1stChild.reset();
	_2ndChild.reset();
}

/* ****************************************************************************
 * Recursive proc to fill in Token descriptions at corresponding tree levels
 * ***************************************************************************/
void ExprTreeNode::buildTreeGraph (std::shared_ptr<ExprTreeNode> treeNode, std::vector<std::wstring> & treeGraph, int scopeDepth, int _1stOr2ndChild)	{

	if (treeNode != NULL)	{
		// If we don't have a string at this depth|level yet, add one
		while ((treeGraph.size()) < (scopeDepth + 1))
			treeGraph.push_back(L"");

		// TODO: Calling Token description created a boo-boo deep in __wprintf_buffer()
		if (scopeDepth == 0)	{
			treeGraph[scopeDepth].append(L"ROOT[");
			treeGraph[scopeDepth].append (treeNode->originalTkn->_string);
			treeGraph[scopeDepth].append(L"]");
		}

		int nxtLvlDown = scopeDepth + 1;
		while ((treeGraph.size()) < (nxtLvlDown + 2))
			treeGraph.push_back(L"");

		if (treeGraph[nxtLvlDown].length() > 0)
			treeGraph[nxtLvlDown].append (L"     ");

		if (treeNode->_1stChild != NULL)	{
			if (_1stOr2ndChild == 1)
				treeGraph[nxtLvlDown].append(L"(1st)");
			else if (_1stOr2ndChild == 2)
				treeGraph[nxtLvlDown].append(L"(2nd)");

			treeGraph[nxtLvlDown].append(L"[");
			treeGraph[nxtLvlDown].append(treeNode->_1stChild->originalTkn->_string);
		}

		if (treeNode->_2ndChild != NULL)	{
			treeGraph[nxtLvlDown].append(L", ");
			treeGraph[nxtLvlDown].append(treeNode->_2ndChild->originalTkn->_string);
		}

		if (treeNode->_1stChild != NULL)	{
			// Close it off
			treeGraph[nxtLvlDown].append(L"]");
		}

		// Now do the recursive calls
		buildTreeGraph (treeNode->_1stChild, treeGraph, scopeDepth + 1, 1);
		buildTreeGraph (treeNode->_2ndChild, treeGraph, scopeDepth + 1, 2);
	}
}

/* ****************************************************************************
 * Serves as an aid for debugging.
 * After an expression tree has been built, this proc will create a crude
 * graphical tree representation so I can see if the tree matches what I
 * expected to happen.
 * ***************************************************************************/
void ExprTreeNode::showTree (std::shared_ptr<ExprTreeNode> treeNode, std::wstring fileName, int lineNumber)	{

	std::vector<std::wstring> treeGraph;

	std::wcout << L"********** ExprTreeNode::showTree called from "<< fileName << L":" << lineNumber << L" **********" << std::endl;

	buildTreeGraph (treeNode, treeGraph, 0, 0);

	for (std::vector<std::wstring>::iterator graphR8r = treeGraph.begin(); graphR8r != treeGraph.end(); graphR8r++)	{
		std::wcout << *graphR8r << std::endl;
	}
}

