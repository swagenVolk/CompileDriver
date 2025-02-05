/*
 * ExprTreeNode.cpp
 * This class holds a Token that originally came from the user's source file.  It has multiple child pointers that allow for linking
 * multiple ExprTreeNode instances together in hierarchical tree structure.
 *
 *  Created on: Jun 5, 2024
 *      Author: Mike Volk
 */

#include "ExprTreeNode.h"
#include <memory>

ExprTreeNode::ExprTreeNode(std::shared_ptr<Token> startOpr8rTkn) {
	// Attach this shared_ptr to an already existing shared_ptr of type Token
	originalTkn = startOpr8rTkn;
  myParentScope = NULL;
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

		if (treeGraph[scopeDepth].length() > 0)
			treeGraph[scopeDepth].append (L"     ");

		treeGraph[scopeDepth].append (L"[");
		if (_1stOr2ndChild == 1)
			treeGraph[scopeDepth].append(L"(1st)");
		else if (_1stOr2ndChild == 2)	{
			treeGraph[scopeDepth].append(L"(2nd)");
		}
		treeGraph[scopeDepth].append (treeNode->originalTkn->getValueStr());
		treeGraph[scopeDepth].append (L"]");

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
void ExprTreeNode::showTree (std::wstring fileName, int lineNumber)	{

	std::vector<std::wstring> treeGraph;

	std::wcout << L"********** ExprTreeNode::showTree called from "<< fileName << L":" << lineNumber << L" **********" << std::endl;

	// Handle the ROOT node here
	treeGraph.push_back(L"");
	treeGraph[0].append(L"ROOT[");
	treeGraph[0].append (originalTkn->getValueStr());
	treeGraph[0].append(L"]");

	// Now do recursive calls
	buildTreeGraph (_1stChild, treeGraph, 1, 1);
	buildTreeGraph (_2ndChild, treeGraph, 1, 2);

	for (std::vector<std::wstring>::iterator graphR8r = treeGraph.begin(); graphR8r != treeGraph.end(); graphR8r++)	{
		std::wcout << L"[" << *graphR8r << L"]" << std::endl;
	}
	
	std::wcout << std::endl << L"********** </ExprTreeNode::showTree> **********" << std::endl;
}

