#include "BTreeNode.h"

using namespace std;

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
BTLeafNode::BTLeafNode()
{
    endEid = 0;
    memset(buffer, 0, PageFile::PAGE_SIZE);
}

RC BTLeafNode::read(PageId pid, const PageFile& pf)
{ 
    RC rc;
    if((rc = pf.read(pid, buffer)) < 0){
        fprintf(stderr, "Error, unable to read leaf node");
        return rc;
    }
    //endEid is stored in the last 4 bytes in the page.
    
    memcpy(&endEid, buffer + PageFile::PAGE_SIZE - sizeof(int), sizeof(int));
    return rc;
}
int BTLeafNode::getendEid()
{
    return endEid;
}

/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::write(PageId pid, PageFile& pf)
{ 
  RC rc;
  
  memcpy(buffer + PageFile::PAGE_SIZE - sizeof(int), &endEid, sizeof(int));
  if((rc = pf.write(pid, buffer)) < 0){
    fprintf(stderr, "Error, unable to write leaf node");
    return rc;
  }

  return rc;
}




/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount()
{ return endEid; }

/*
 * Insert a (key, rid) pair to the node.
 * @param key[IN] the key to insert
 * @param rid[IN] the RecordId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTLeafNode::insert(int key, const RecordId& rid)
{
  //each entry in leaf node is stored as (pid, sid, key) in sequence.
  //each entry takes up 3 times size of int.
  if((endEid + 1) * ENTRY_SIZE + sizeof(PageId) + sizeof(int) > PageFile::PAGE_SIZE){
    fprintf(stderr, "Error: exceed the capacity of the node");
    return RC_NODE_FULL;
  }

  //move the PageId at the end of page to the right.
  memcpy(buffer + (endEid + 1) * ENTRY_SIZE, buffer + endEid * ENTRY_SIZE, sizeof(PageId));

  int i;
  for(i = endEid - 1; i >= 0; i--){
    int entry_key;
    memcpy(&entry_key, buffer + sizeof(RecordId) + i * ENTRY_SIZE, sizeof(int));
    if(entry_key > key){
      //move the entry to the right by one size of entry
      memcpy(buffer + (i + 1) * ENTRY_SIZE, buffer + i * ENTRY_SIZE, ENTRY_SIZE); 
    }
    else
      break;
  }
  memcpy(buffer + (i + 1) * ENTRY_SIZE, &rid.pid, sizeof(PageId));
  memcpy(buffer + (i + 1) * ENTRY_SIZE + sizeof(PageId), &rid.sid, sizeof(int));
  memcpy(buffer + (i + 1) * ENTRY_SIZE + sizeof(RecordId), &key, sizeof(int));
  ++endEid;
    return 0;
 }

/*
 * Insert the (key, rid) pair to the node
 * and split the node half and half with sibling.
 * The first key of the sibling node is returned in siblingKey.
 * @param key[IN] the key to insert.
 * @param rid[IN] the RecordId to insert.
 * @param sibling[IN] the sibling node to split with. This node MUST be EMPTY when this function is called.
 * @param siblingKey[OUT] the first key in the sibling node after split.
 * @return 0 if successful. Return an error code if there is an error.
 */
 //you should set ptr in orginal node to splited node manually
RC BTLeafNode::insertAndSplit(int key, const RecordId& rid, 
                              BTLeafNode& sibling, int& siblingKey)
{
  insert(key, rid);
  int k;
  RecordId r;
  int i = int(endEid / 2);
  PageId pid;
  
  memcpy(&siblingKey, buffer + i * ENTRY_SIZE + sizeof(RecordId), sizeof(int));
    
   //// this.setNextNodePtr(&r.pid);

  //copy right half of the keys to sibling node
  for(i; i <=  endEid - 1; i++){
    memcpy(&r.pid, buffer + i * ENTRY_SIZE, sizeof(PageId));
    memcpy(&r.sid, buffer + i * ENTRY_SIZE + sizeof(PageId), sizeof(int));
    memcpy(&k, buffer + i * ENTRY_SIZE + sizeof(RecordId), sizeof(int));
    sibling.insert(k, r);
  }
  //copy the ptr to next node to sibling node
  memcpy(&pid, buffer + endEid * ENTRY_SIZE, sizeof(int));
  sibling.setNextNodePtr(pid);
  
  endEid = int(endEid / 2);
  
  return 0;
 }

/*
 * Find the entry whose key value is larger than or equal to searchKey
 * and output the eid (entry number) whose key value >= searchKey.
 * Remeber that all keys inside a B+tree node should be kept sorted.
 * @param searchKey[IN] the key to search for
 * @param eid[OUT] the entry number that contains a key larger than or equalty to searchKey
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::locate(int searchKey, int& eid)
{ 
  //Binary search
  if(endEid == 0)
	return RC_NO_SUCH_RECORD;
	
  int key;
  int i = 0;
  int j = endEid - 1;
  
  memcpy(&key, buffer + j * ENTRY_SIZE + sizeof(RecordId), sizeof(int));
    if(searchKey > key){
        eid = endEid;
        return RC_NO_SUCH_RECORD;
    }
    while(j >= i + 1){
    int mid = (i + j) / 2;
    memcpy(&key, buffer + mid * ENTRY_SIZE + sizeof(RecordId), sizeof(int));
    if(searchKey == key){
      eid = mid;
      return 0;
    }
    if(searchKey > key)
      i = mid + 1;
    else
      j = mid;
  }
    eid = j;
    memcpy(&key, buffer + eid * ENTRY_SIZE + sizeof(RecordId), sizeof(int));
    if(searchKey == key)
        return 0;
  
  return RC_NO_SUCH_RECORD;
}

/*
 * Read the (key, rid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param rid[OUT] the RecordId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::readEntry(int eid, int& key, RecordId& rid)
{
  if(eid >= endEid || eid < 0)
      return RC_INVALID_CURSOR;
      
  memcpy(&rid.pid, buffer + eid * ENTRY_SIZE, sizeof(PageId));
  memcpy(&rid.sid, buffer + eid * ENTRY_SIZE + sizeof(PageId), sizeof(int));
  memcpy(&key, buffer + eid * ENTRY_SIZE + sizeof(RecordId), sizeof(int));

  return 0;
}

/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr()
{ 
  PageId pid;
  memcpy(&pid, buffer + endEid * ENTRY_SIZE, sizeof(PageId));
  return pid; 
}



/*
 * Set the pid of the next sibling node.
 * @param pid[IN] the PageId of the next sibling node 
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid)
{
  memcpy(buffer + endEid * ENTRY_SIZE, &pid, sizeof(PageId));
  return 0; 
}

void BTLeafNode::printNodeContent()
{
	RecordId rid;
	int key;
	PageId pid;
	
	for(int i = 0; i < endEid; i++)
	{
		readEntry(i, key, rid);
		fprintf(stdout, "key: %d, rid: (%d, %d) | ", key, rid.pid, rid.sid);
	}
	fprintf(stdout, "next page id: %d\n", getNextNodePtr());
	fprintf(stdout, "number of keys: %d\n", endEid);


}

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
BTNonLeafNode::BTNonLeafNode()
{
    memset(buffer, 0, PageFile::PAGE_SIZE);
    keyCount = 0;
}
RC BTNonLeafNode::read(PageId pid, const PageFile& pf)
{ 
  RC rc;
  if((rc = pf.read(pid, buffer)) < 0){
    fprintf(stderr, "Error, unable to read nonleaf node");
    return rc;
  }
  memcpy(&keyCount, buffer + PageFile::PAGE_SIZE - sizeof(int), sizeof(int));
  return rc;
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::write(PageId pid, PageFile& pf)
{
  RC rc;
  memcpy(buffer + PageFile::PAGE_SIZE - sizeof(int), &keyCount, sizeof(int));
  if((rc = pf.write(pid, buffer)) < 0){
    fprintf(stderr, "Error, unable to write nonleaf node");
    return rc;
  }
  return rc;
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount()
{ return keyCount; }


/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid)
{ 
  //data stored in buffer is in the form of pid|key|pid|key|...|pid
  //key is sorted
  
  if((keyCount + 2) * sizeof(int) + (keyCount + 2) * sizeof(PageId) > PageFile::PAGE_SIZE){
    fprintf(stderr, "Error: exceed the capacity of the node");
    return RC_NODE_FULL;
  }

  int i;
  for(i = keyCount - 1; i>= 0; i--){
    int k;
    memcpy(&k, buffer + (i + 1) * sizeof(PageId) + i * sizeof(int), sizeof(int));
    if(k > key){
      //move the (key|pid) pair to the right
      memcpy(buffer + (i + 2) * sizeof(PageId) + (i + 1) * sizeof(int), buffer + (i + 1) * sizeof(PageId) + i * sizeof(int), sizeof(PageId) + sizeof(int));
    }
    else
      break;
  }

  //put the (key|pid) pair to be inserted into the right place
  memcpy(buffer + (i + 2) * sizeof(PageId) + (i + 1) * sizeof(int), &key, sizeof(int));
  memcpy(buffer + (i + 2) * (sizeof(PageId) + sizeof(int)), &pid, sizeof(PageId));

  ++keyCount;
    return 0;
}

/*
 * Insert the (key, pid) pair to the node
 * and split the node half and half with sibling.
 * The middle key after the split is returned in midKey.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
 * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey)
{ 
  RC rc;
  if(rc = insert(key, pid) < 0){
    fprintf(stderr, "Error: could not insert key");
    return rc;
  }

  int k;
  PageId p;
  int i = int(keyCount / 2);
  memcpy(&midKey, buffer + (i + 1) * sizeof(PageId) + i * sizeof(int), sizeof(int));

  memcpy(&p, buffer + (i + 1) * (sizeof(PageId) + sizeof(int)), sizeof(PageId));
  sibling.setFirstPid(p);
    
  for(i = i + 1; i <= keyCount - 1; i++){
    memcpy(&k, buffer + (i + 1) * sizeof(PageId) + i * sizeof(int), sizeof(int));
    memcpy(&p, buffer + (i + 1) * (sizeof(PageId) + sizeof(int)), sizeof(PageId));
    sibling.insert(k, p);
  }
  

  keyCount = int(keyCount / 2);
    return 0;
}

RC BTNonLeafNode::setFirstPid(PageId pid)
{
  memcpy(buffer, &pid, sizeof(PageId));
  return 0;
}


/*
 * Given the searchKey, find the child-node pointer to follow and
 * output it in pid.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param pid[OUT] the pointer to the child node to follow.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locateChildPtr(int searchKey, PageId& pid)
{ 
  int i = 0;
  int j = keyCount - 1;
  int mid;

  while(j >= i + 1){
    mid = (i + j) / 2;
    int key;
    memcpy(&key, buffer + (mid + 1) * sizeof(PageId) + mid * sizeof(int), sizeof(int));
    if(searchKey == key){
      memcpy(&pid, buffer + (mid + 1) * (sizeof(PageId) + sizeof(int)), sizeof(PageId));
      return 0;
    }
    if(searchKey > key)
      i = mid+1;
    else
      j = mid;
  }
  
  int keyi;
  int keyj;
  memcpy(&keyi, buffer + (i + 1) * sizeof(PageId) + i * sizeof(int), sizeof(int));
  memcpy(&keyj, buffer + (j + 1) * sizeof(PageId) + j * sizeof(int), sizeof(int));
  if(searchKey < keyi)
    memcpy(&pid, buffer + i * (sizeof(PageId) + sizeof(int)), sizeof(PageId));
  else if(searchKey >= keyj)
    memcpy(&pid, buffer + (j + 1) * (sizeof(PageId) + sizeof(int)), sizeof(PageId));
  else
    memcpy(&pid, buffer + (i + 1) * (sizeof(PageId) + sizeof(int)), sizeof(PageId));
  return 0;

}

/*
 * Initialize the root node with (pid1, key, pid2).
 * @param pid1[IN] the first PageId to insert
 * @param key[IN] the key that should be inserted between the two PageIds
 * @param pid2[IN] the PageId to insert behind the key
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::initializeRoot(PageId pid1, int key, PageId pid2)
{ 
  memcpy(buffer, &pid1, sizeof(PageId));
  memcpy(buffer + sizeof(PageId), &key, sizeof(int));
  memcpy(buffer + sizeof(PageId) + sizeof(int), &pid2, sizeof(int));
  
  keyCount = 1;

  return 0;
}

void BTNonLeafNode::printNodeContent()
{
	
	PageId pid;
	int key;

	for(int i = 0; i < keyCount; i++)
	{
		memcpy(&pid, buffer + i * sizeof(PageId) + i * sizeof(int), sizeof(PageId));
		memcpy(&key, buffer + (i + 1) * sizeof(PageId) + i * sizeof(int), sizeof(int));
		fprintf(stdout, "page id: %d | key: %d | ", pid, key);
	}
	memcpy(&pid, buffer + keyCount * sizeof(PageId) + keyCount * sizeof(int), sizeof(PageId));
	fprintf(stdout, "page id: %d\n", pid);
	fprintf(stdout, "number of keys: %d\n", keyCount);


}
