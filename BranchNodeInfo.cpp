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
  allocatedLen = 0;

}
 
 BranchNodeInfo::~BranchNodeInfo() {
   // Clean up our allocated memory
   nodePtr.reset();
 }

 BranchNodeInfo& BranchNodeInfo::operator= (const BranchNodeInfo& srcBni) {
	// self-assignment check
	if (this == &srcBni)
		return *this;

	// if data exists in the current string, delete it
  nodePtr = srcBni.nodePtr;
  tokenStr.clear();
  tokenStr = srcBni.tokenStr;
  isLeftAligned = srcBni.isLeftAligned;
  centerSidePos = srcBni.centerSidePos;
  allocatedLen = srcBni.allocatedLen;

	// TODO: I don't understand the comment below
	// return the existing object so we can chain this operator
	return *this;
}

 