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

ExprTreeNode::ExprTreeNode(Token * startOpr8rTkn) {
	// TODO Auto-generated constructor stub
  originalTkn = startOpr8rTkn;
  // Storage place for bubbled-up result of resolving ExpressionTree
  // resultTkn = Token (START_UNDEF_TKN, L"", startOpr8rTkn != NULL ? startOpr8rTkn->line_number : 0, startOpr8rTkn != NULL ? startOpr8rTkn->column_pos : 0);
	resultTkn = new Token (START_UNDEF_TKN, L"", 0, 0);
  myParent = NULL;
  _1stChild = NULL;
  _2ndChild = NULL;
  _3rdChild = NULL;

}

ExprTreeNode::~ExprTreeNode() {
	// TODO Auto-generated destructor stub
	// Clean up our allocated memory
	if (originalTkn != NULL)	{
		delete (originalTkn);
		originalTkn = NULL;
	}

	if (resultTkn != NULL)	{
		delete (resultTkn);
		resultTkn = NULL;
	}

	if (_1stChild != NULL)	{
		delete (_1stChild);
		_1stChild = NULL;
	}

	if (_2ndChild != NULL)	{
		delete (_2ndChild);
		_2ndChild = NULL;
	}

	if (_3rdChild != NULL)	{
		delete (_3rdChild);
		_3rdChild = NULL;
	}
}

/* ****************************************************************************
 * Recursive proc to fill in Token descriptions at corresponding tree levels
 * ***************************************************************************/
void ExprTreeNode::buildTreeGraph (ExprTreeNode * treeNode, std::vector<std::wstring> & treeGraph, int scopeDepth, int lftRgt3rdChild)	{

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
			if (lftRgt3rdChild == 1)
				treeGraph[nxtLvlDown].append(L"(1st)");
			else if (lftRgt3rdChild == 2)
				treeGraph[nxtLvlDown].append(L"(2nd)");
			else if (lftRgt3rdChild == 3)
				treeGraph[nxtLvlDown].append(L"(3rd)");

			treeGraph[nxtLvlDown].append(L"[");
			treeGraph[nxtLvlDown].append(treeNode->_1stChild->originalTkn->_string);
		}

		if (treeNode->_2ndChild != NULL)	{
			treeGraph[nxtLvlDown].append(L", ");
			treeGraph[nxtLvlDown].append(treeNode->_2ndChild->originalTkn->_string);
		}

		if (treeNode->_3rdChild != NULL)	{
			treeGraph[nxtLvlDown].append(L", ");
			treeGraph[nxtLvlDown].append(treeNode->_3rdChild->originalTkn->_string);
		}

		if (treeNode->_1stChild != NULL)	{
			// Close it off
			treeGraph[nxtLvlDown].append(L"]");
		}

		// Now do the recursive calls
		buildTreeGraph (treeNode->_1stChild, treeGraph, scopeDepth + 1, 1);
		buildTreeGraph (treeNode->_2ndChild, treeGraph, scopeDepth + 1, 2);
		buildTreeGraph (treeNode->_3rdChild, treeGraph, scopeDepth + 1, 3);
	}
}

/* ****************************************************************************
 * Serves as an aid for debugging.
 * After an expression tree has been built, this proc will create a crude
 * graphical tree representation so I can see if the tree matches what I
 * expected to happen.
 * ***************************************************************************/
void ExprTreeNode::showTree (ExprTreeNode * treeNode)	{

	std::vector<std::wstring> treeGraph;

	std::wcout << L"********** EXPRESSION TREE **********" << std::endl;

	buildTreeGraph (treeNode, treeGraph, 0, 0);

	for (std::vector<std::wstring>::iterator graphR8r = treeGraph.begin(); graphR8r != treeGraph.end(); graphR8r++)	{
		std::wcout << *graphR8r << std::endl;
	}
}

