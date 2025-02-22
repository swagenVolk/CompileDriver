/*
 * BranchNodeInfo.cpp
 * This class holds info on an ExprTreeNode that helps display a parse tree for a
 * compiled expression.
 * 
 *
 *  Created on: Feb 20, 2025
 *      Author: Mike Volk
 */

 #include "BranchNodeInfo.h"
 #include <memory>
 
 BranchNodeInfo::BranchNodeInfo (std::shared_ptr<ExprTreeNode> inNodePtr, std::wstring inTokenStr, bool isLefty)  {
  nodePtr = inNodePtr;
  tokenStr = inTokenStr;
  isLeftAligned = isLefty;
  centerSidePos = 0;
  outerSidePos = 0;
  numSpacesOnLeft = 0;
  numSpacesOnRight = 0;

}
 
 BranchNodeInfo::~BranchNodeInfo() {
   // Clean up our allocated memory
   nodePtr.reset();
 }
 