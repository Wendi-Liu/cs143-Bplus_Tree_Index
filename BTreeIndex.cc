/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */
 
#include "BTreeIndex.h"
#include "BTreeNode.h"

using namespace std;

/*
 * BTreeIndex constructor
 */
BTreeIndex::BTreeIndex()
{
    rootPid = -1;
    treeHeight = 0;
}

BTreeIndex::~BTreeIndex()
{
	if(opened)
		close();

}

int BTreeIndex::endeidofLastpage(){
    BTLeafNode *node = new BTLeafNode();
    node->read(pf.endPid()-1, pf);
    return node->getendEid();
}

PageId BTreeIndex::endPageNum()
{
    return pf.endPid();
}


/*
 * Open the index file in read or write mode.
 * Under 'w' mode, the index file should be created if it does not exist.
 * @param indexname[IN] the name of the index file
 * @param mode[IN] 'r' for read, 'w' for write
 * @return error code. 0 if no error
 */
RC BTreeIndex::readpagefilenode(PageId pid)
{
    BTLeafNode* node = new BTLeafNode();
    node->read(pid, pf);
    node->printNodeContent();
    return 0;
}
RC BTreeIndex::readpagefilenonleafnode(PageId pid)
{
    BTNonLeafNode* node = new BTNonLeafNode();
    node->read(pid, pf);
    node->printNodeContent();
    return 0;
}

RC BTreeIndex::open(const string& indexname, char mode)
{
    RC rc;
	
	if ((rc = pf.open(indexname, mode)) < 0) return rc;
	
	opened = true;
	if(pf.endPid() > 0){
		//if the index file is not empty.
		
		//metadata including rootPid, treeHeight & branchingFactor are stored sequentially in the first page of a index file
		char metadata[PageFile::PAGE_SIZE];
		
		if ((rc = pf.read(0, metadata)) < 0) {
		// an error occurred during page read
		pf.close();
		return rc;
		}
		
		memcpy(&rootPid, metadata, sizeof(PageId));
		memcpy(&treeHeight, metadata + sizeof(PageId), sizeof(int));
		memcpy(&branchingFactor, metadata + sizeof(PageId) + sizeof(int), sizeof(int));
	}
	else{
		//if the index file is empty
		branchingFactor = BRANCHING_FACTOR;
	}
	
	
	return rc;
}

/*
 * Close the index file.
 * @return error code. 0 if no error
 */
RC BTreeIndex::close()
{
    RC rc;
	
	char metadata[PageFile::PAGE_SIZE];
	memcpy(metadata, &rootPid, sizeof(PageId));
	memcpy(metadata + sizeof(PageId), &treeHeight, sizeof(int));
	memcpy(metadata + sizeof(PageId) + sizeof(int), &branchingFactor, sizeof(int));
	
	if ((rc = pf.write(0, metadata)) < 0) {
		// an error occurred during page write
		rootPid = -1;
		treeHeight = 0;
		branchingFactor = 0;
		pf.close();
		return rc;
		}
	
	rootPid = -1;
	treeHeight = 0;
	branchingFactor = 0;
	if((rc = pf.close()) >= 0){
		opened = false;
	}
	return rc;
}

/*
 * Insert (key, RecordId) pair to the index.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */
RC BTreeIndex::insert(int key, const RecordId& rid)
{
    RC rc;
	PageId nodeId = rootPid;
	int currentHeight = 1;
	int returnedKey;
	PageId returnedPid;
	bool splited;
	traverseInsert(key, rid, nodeId, currentHeight, returnedKey, returnedPid, splited);
	
	if(splited){
		//new root
		BTNonLeafNode* newRoot = new BTNonLeafNode;
		newRoot->initializeRoot(rootPid, returnedKey, returnedPid);
		
		if((rc = newRoot->write(pf.endPid(), pf)) < 0){
            fprintf(stderr, "Error, cannot write newRoot to Pagefile");
			return rc;
		}
		rootPid = pf.endPid() - 1;
		treeHeight++;
	}
	
	return 0;
}
PageId BTreeIndex::getrootpid()
{
    return rootPid;
}
/*
 * Find the leaf-node index entry whose key value is larger than or 
 * equal to searchKey, and output the location of the entry in IndexCursor.
 * IndexCursor is a "pointer" to a B+tree leaf-node entry consisting of
 * the PageId of the node and the SlotID of the index entry.
 * Note that, for range queries, we need to scan the B+tree leaf nodes.
 * For example, if the query is "key > 1000", we should scan the leaf
 * nodes starting with the key value 1000. For this reason,
 * it is better to return the location of the leaf node entry 
 * for a given searchKey, instead of returning the RecordId
 * associated with the searchKey directly.
 * Once the location of the index entry is identified and returned 
 * from this function, you should call readForward() to retrieve the
 * actual (key, rid) pair from the index.
 * @param key[IN] the key to find.
 * @param cursor[OUT] the cursor pointing to the first index entry
 *                    with the key value.
 * @return error code. 0 if no error.
 */
RC BTreeIndex::locate(int searchKey, IndexCursor& cursor)
{
    
	int currentHeight = 1;
    RC rc;
	rc = traverseLocate(searchKey, cursor, rootPid, currentHeight);
	if(rc < 0)
        return rc;
	return 0;
	
}

/*
 * Read the (key, rid) pair at the location specified by the index cursor,
 * and move foward the cursor to the next entry.
 * @param cursor[IN/OUT] the cursor pointing to an leaf-node index entry in the b+tree
 * @param key[OUT] the key stored at the index cursor location.
 * @param rid[OUT] the RecordId stored at the index cursor location.
 * @return error code. 0 if no error
 */
RC BTreeIndex::readForward(IndexCursor& cursor, int& key, RecordId& rid)
{
    RC rc;
	BTLeafNode* leaf = new BTLeafNode;
	if((rc = leaf->read(cursor.pid, pf)) < 0){
        //fprintf(stderr, "ERROR1");
		return rc;
	}
    
    if((rc = leaf->readEntry(cursor.eid, key, rid)) < 0){
        //fprintf(stderr, "ERROR2");
        return rc;
    }
	
	if(cursor.eid >= leaf->getKeyCount() - 1)
	{
		//at the last entry of this node
		cursor.pid = leaf->getNextNodePtr();
		cursor.eid = 0;
	}
	else{
		cursor.eid++;
	}
	
	return 0;
}

RC BTreeIndex::traverseInsert(int key, const RecordId& rid, PageId nodeId, int cHeight, int& returnedKey, PageId& returnedPid, bool& splited)
{
	RC rc;
	
	if(rootPid == -1){
		//new B+ tree
		BTLeafNode* leaf = new BTLeafNode;
		if((rc = leaf->insert(key, rid)) < 0){
            //fprintf(stderr, "BTreeIndex Line 224 Error");
			return rc;
		}
		if((rc = leaf->write(1, pf)) < 0){
            //fprintf(stderr, "BTreeIndex Line 227 Error");
			return rc;
		}
		rootPid = 1;
		treeHeight = 1;
        splited = false;
		return 0;
	}
	
	
	if(cHeight >= treeHeight){
		//reach leaf node
		BTLeafNode* leaf = new BTLeafNode;
		if((rc = leaf->read(nodeId, pf)) < 0){
			return rc;
		}
		
		if(leaf->getKeyCount() + 1 > branchingFactor){
			//leaf node needs split
			BTLeafNode* sibling = new BTLeafNode;
			int siblingKey;
			PageId siblingPid = pf.endPid();
			
			if((rc = leaf->insertAndSplit(key, rid, *sibling, siblingKey)) < 0){
                //fprintf(stderr, "BTreeIndex Line 252 Error");
				return rc;
			}
			leaf->setNextNodePtr(siblingPid);
			if((rc = leaf->write(nodeId, pf)) < 0){
                //fprintf(stderr, "BTreeIndex Line 256 Error");
				return rc;
			}	
			
			if((rc = sibling->write(siblingPid, pf)) < 0){
                //fprintf(stderr, "BTreeIndex Line 260 Error");
				return rc;
			}			
			returnedKey = siblingKey;
			returnedPid = siblingPid;
			splited = true;
			return 0;
		}
		
		else{
			//leaf node doesn't need split
			if((rc = leaf->insert(key, rid)) < 0){
				return rc;
				}
			if((rc = leaf->write(nodeId, pf)) < 0){
				return rc;
			}	
			splited = false;
			return 0;
		}
	}
	
	
	if(cHeight < treeHeight){
		//at non-leaf node
		BTNonLeafNode* nonLeaf = new BTNonLeafNode;
		if((rc = nonLeaf->read(nodeId, pf)) < 0){
			return rc;
		}
		
		PageId nextPid;
		nonLeaf->locateChildPtr(key, nextPid);
		
		cHeight++;
		int rKey;
		PageId rPid;
		bool childSplited;
		if((rc = traverseInsert(key, rid, nextPid, cHeight, rKey, rPid, childSplited)) < 0){
			return rc;
		}
		
		if(!childSplited)
		{
			splited = false;
		}
		else{
			if(nonLeaf->getKeyCount() + 1 > branchingFactor)
			{
				//non-leaf node needs split
				BTNonLeafNode* sibling = new BTNonLeafNode;
				int midKey;
				PageId siblingPid = pf.endPid();
				
				//if
				if((rc = nonLeaf->insertAndSplit(rKey, rPid, *sibling, midKey)) < 0){
					return rc;
				}
				if((rc = nonLeaf->write(nodeId, pf)) < 0){
					return rc;
				}	

				if((rc = sibling->write(siblingPid, pf)) < 0){
					return rc;
				}
				returnedPid = siblingPid;
				returnedKey = midKey;
				splited = true;
			}
			else{
				//non-leaf node doesn't need split
				if((rc = nonLeaf->insert(rKey, rPid)) < 0){
					return rc;
				}
				if((rc = nonLeaf->write(nodeId, pf)) < 0){
					return rc;
				}
                splited = false;
			}
		}
		
		return 0;
	}
}

RC BTreeIndex::traverseLocate(int searchKey, IndexCursor& cursor, PageId nodeId, int cHeight)
{
	RC rc;
	
	if(rootPid == -1)
		return RC_NO_SUCH_RECORD;

	if(cHeight >= treeHeight){
		//reach leaf node
		BTLeafNode* leaf = new BTLeafNode;
		if((rc = leaf->read(nodeId, pf)) < 0){
			return rc;
		}
		
		int eid;
		if((rc = leaf->locate(searchKey, eid)) < 0){
            cursor.pid = nodeId;
            cursor.eid = eid;
            if(cursor.eid >= leaf->getKeyCount())
            {
                //act the last entry of this node
                
                cursor.pid = leaf->getNextNodePtr();
                cursor.eid = 0;
            }
            return rc;
		}
        //fprintf(stdout, "%d ////NNNOOODDEEEIIIDDD %d EEEIIIIDDD  ",eid , nodeId);
		cursor.pid = nodeId;
		cursor.eid = eid;
		return 0;
	
	}
	else{
		//at non-leaf node
		BTNonLeafNode* nonLeaf = new BTNonLeafNode;
		if((rc = nonLeaf->read(nodeId, pf)) < 0){
			return rc;
		}
		
		PageId nextPid;
		if((rc = nonLeaf->locateChildPtr(searchKey, nextPid)) < 0){
			return rc;
		}
        //fprintf(stdout, "%d ////", nextPid);
		cHeight++;
		return traverseLocate(searchKey, cursor, nextPid, cHeight);
	}
}
