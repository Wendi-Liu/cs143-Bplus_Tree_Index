//
//  testcase2.cpp
//  cs143_project2
//
//  Created by wangxinlei on 14/12/4.
//  Copyright (c) 2014年 wangxinlei. All rights reserved.
//
#include <stdio.h>
#include <iostream>
#include "BTreeIndex.h"
#include "BTreeNode.h"
#include "PageFile.h"
using namespace std;
#include <stdio.h>
int main(){
BTNonLeafNode* node = new BTNonLeafNode();
//node->open("BTindex.txt", 'w');
PageId a = 1;
PageFile pf;
pf.open("pagefilefornonleaf.txt",'w');
int siblingkey;
for(int i = 0; i < 14; i+=3){
    
    node->insert(i,a);
    node->write(a, pf);
    a++;
}
BTNonLeafNode* sibling = new BTNonLeafNode();
node->insertAndSplit(20, a, *sibling, siblingkey);
node->write(0,pf);
sibling->write(1,pf);
node->read(0, pf);
sibling->read(1,pf);
std::cout<<node->getKeyCount()<<endl;
int key;
RecordId rid;
//BTLeafNode* node2 = new BTLeafNode();
//node2->read(1,pf);
node->printNodeContent();
cout<<endl;
sibling->printNodeContent();
int eid;


node->locateChildPtr(1, eid);
cout << "eid: " << eid <<endl;
int eid2;
sibling ->locateChildPtr(0, eid2);
cout << "eid2 " << eid2 <<endl;
}