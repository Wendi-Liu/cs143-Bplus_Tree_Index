//
//  testcase1.cpp
//  
//
//  Created by wangxinlei on 14/12/1.
//
//

#include <stdio.h>
#include <iostream>
#include "BTreeIndex.h"
#include "BTreeNode.h"
#include "PageFile.h"
using namespace std;
int main()
{
    /*
    BTNonLeafNode* test = new BTNonLeafNode();
    PageId pid = 1;
    //RecordId rid;
    //rid.pid = 1;
    //rid.sid = 2;
    PageFile pf;
    pf.open("test.txt", 'w');
    for(int i = 1; i < 50; i++){
        test->insert(i, pid);
        pid++;
    }
    test->write(0, pf);
    test->printNodeContent();
    BTNonLeafNode* tt = new BTNonLeafNode();
    tt->read(1, pf);
    
    */
   /*
    
    BTLeafNode* node = new BTLeafNode();
    BTreeIndex* index = new BTreeIndex();
  
    //node->setNextNodePtr(1);
    PageId a = 0;
    RecordId b;
    b.pid = a;
    b.sid = 5;
    PageFile pf;
    pf.open("pagefile.txt",'w');
    int siblingkey;
   
    for(int i = 14; i > 0; i-=3){
  
        node->insert(i,b);
        //index->insert(i,b);
        b++;
    }
    /*
    BTLeafNode* sibling = new BTLeafNode();
    node->setNextNodePtr(11);
    node->insertAndSplit(20, b, *sibling, siblingkey);
    node->setNextNodePtr(2);
    
    node->write(0,pf);
    sibling->write(2,pf);
    BTLeafNode *node_test = new BTLeafNode();
    node_test->read(0, pf);
    cout<<"node_test"<<"  ";
    node_test->printNodeContent();
    
    BTLeafNode* sib_test = new BTLeafNode();
    sib_test->read(2,pf);
    cout<<"sib_test"<<"  ";
    sib_test->printNodeContent();

    std::cout<<node->getKeyCount()<<endl;
    int key;
    RecordId rid;
     */
    //BTLeafNode* node2 = new BTLeafNode();
    //node2->read(1,pf);
    //node->printNodeContent();
    //cout<<endl;
   // sibling->printNodeContent();
    //int eid;
    
    

   /*
    
    node->locate(1, eid);
    cout << "eid: " << eid <<endl;
    int eid2;
    sibling ->locate(15, eid2);
    cout << "eid2 " << eid2 <<endl;
    */
    //pf.close();
    //index->close();
    
    
    
    
    
    BTreeIndex* index = new BTreeIndex();
    cout << " rootPid1 "<<index->getrootpid()<<endl;
    PageId a = 0;
    RecordId b;
    b.pid = a;
    b.sid = 5;
    PageFile pf;
    index->open("BTindex32.txt", 'w');
    
    
    for(int i = 0; i < 23; i+=1){
        
        index->insert(i,b);
        cout << " rootPid2 "<<index->getrootpid()<<endl;
        b++;
    }
    IndexCursor cursor;
    RC what;
    int key;
    RecordId rid;
    index->locate(3, cursor);
     cout<<"cursor :"<<cursor.pid<<" "<<cursor.eid;
    //index->readForward(cursor, key, rid);
    //cout << "key :"<<key<<endl;
    //cout<<"rid :"<<rid.pid<<" "<<rid.sid<<endl;
    what = index->locate(-1, cursor);
    cout << "-1 record?" << what;
    cout<<"cursor :"<<cursor.pid<<" "<<cursor.eid;
    //index->readForward(cursor, key, rid);
    //cout << "key :"<<key<<endl;
    //cout<<"rid :"<<rid.pid<<" "<<rid.sid<<endl;
    index->locate(25, cursor);
    cout<<"cursor :"<<cursor.pid<<" "<<cursor.eid;
    //index->readForward(cursor, key, rid);
   // cout << "key :"<<key<<endl;
    //cout<<"rid :"<<rid.pid<<" "<<rid.sid<<endl;
    index->locate(12, cursor);
     cout<<"cursor :"<<cursor.pid<<" "<<cursor.eid;
    //index->readForward(cursor, key, rid);
    //cout << "key :"<<key<<endl;
    //cout<<"rid :"<<rid.pid<<" "<<rid.sid<<endl;
    
    
    
    
    /*
    for(int i = 0; i < 15; i++)
    {
    //index->readForward(cursor, key, rid);
    //cout << "key: "<<key<<endl;
    //cout<<"rid: "<<rid.pid<<" "<<rid.sid<<endl;
        if(i==3||i==9||i==10)
        {
            cout<<"current pid "<<i<<" ";
            index->readpagefilenonleafnode(i);
            cout<<endl;
            continue;

        }
        cout<<"current pid "<<i<<" ";
        index->readpagefilenode(i);
        cout<<endl;
    }
    */
    //index -> insert(20,b);
    //cout << " rootPid2 "<<index->getrootpid()<<endl;
   // IndexCursor cursor;
    //index -> locate(3, cursor);
   // cout << cursor.pid << " "<< cursor.eid;
   // pf.open("aaa",'w');
    //cout<<pf.endPid()<<endl;
    
    /*
    IndexCursor c;
    index->locate(3, c);
    cout<<c.pid<<" "<<c.eid;
    */
     
    /*
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
    
    
    node->locateChildPtr(3, eid);
    cout << "eid: " << eid <<endl;
    int eid2;
    sibling ->locateChildPtr(12, eid2);
    cout << "eid2 " << eid2 <<endl;
    */
     
    /*
    BTreeIndex* index = new BTreeIndex();
    index->open("BTindex.txt", 'w');
    PageId aa = 3;
    RecordId bb;
    bb.pid = aa;
    bb.sid = 0;
    for(int i = 0; i < 10; i++){
        node->insert(aa,bb);
        bb++;
        aa++;
    }
    */
    
}