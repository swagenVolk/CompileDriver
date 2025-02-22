/*
 * BranchNodeInfo.h
 *
 *  Created on: Feb 20, 2025
 *      Author: Mike Volk
 */
 #ifndef BRANCH_NODE_INFO_H_
 #define BRANCH_NODE_INFO_H_

 #include "ExprTreeNode.h"

 class BranchNodeInfo {
  public:
    BranchNodeInfo (std::shared_ptr<ExprTreeNode> inNodePtr, std::wstring inTokenStr, bool isLefty);
    virtual ~BranchNodeInfo();
  
    std::shared_ptr<ExprTreeNode> nodePtr;
    std::wstring tokenStr;
    bool isLeftAligned;
    int centerSidePos;
    int outerSidePos;
    int numSpacesOnLeft;
    int numSpacesOnRight;
  };

 #endif /* BRANCH_NODE_INFO_H_ */
