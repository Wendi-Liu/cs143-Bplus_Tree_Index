/**
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */

#include <cstdio>
#include <iostream>
#include <fstream>
#include "Bruinbase.h"
#include "SqlEngine.h"

using namespace std;

// external functions and variables for load file and sql command parsing 
extern FILE* sqlin;
int sqlparse(void);


RC SqlEngine::run(FILE* commandline)
{
  fprintf(stdout, "Bruinbase> ");

  // set the command line input and start parsing user input
  sqlin = commandline;
  sqlparse();  // sqlparse() is defined in SqlParser.tab.c generated from
               // SqlParser.y by bison (bison is GNU equivalent of yacc)

  return 0;
}



RC SqlEngine::select(int attr, const string& table, const vector<SelCond>& cond)
{
    RecordFile rf;   // RecordFile containing the table
    RecordId   rid;  // record cursor for table scanning
    PageFile   pf;
    BTreeIndex Bindex;
    bool noindex = false;
    bool nosuchvalue = false;
    bool useindex = false;
    bool readRF = false;
    RC     rc;
    int    key;
    string value;
    int    count;
    int    diff;
    vector<bool> checked;
    
    // open the table file
    if ((rc = rf.open(table + ".tbl", 'r')) < 0) {
        fprintf(stderr, "Error: table %s does not exist\n", table.c_str());
        return rc;
    }
    
    
    if((rc = Bindex.open(table + ".idx",'r')) < 0){
        //fprintf(stderr, "Error: indextable %s does not exist\n", table.c_str());
        noindex = true;
    }
    
    // scan the table file from the beginning
    count = 0;
    IndexCursor cursor;
    cursor.pid = 1;
    cursor.eid = 0;
    IndexCursor nexttarget;
    IndexCursor Cbegin;
    Cbegin.pid = 1;
    Cbegin.eid = 0;
    IndexCursor Cend;
    Cend.pid = 0;
    Cend.eid = 0;
    rid.pid = rid.sid = 0;
    //fprintf(stdout, " NO1 %d %d\n", cursor.pid, cursor.eid);
    int Kmax=INT_MAX;
    int Kmin =INT_MIN;
    vector<SelCond> tmpcond;
    
    if(attr == 1)
        useindex = true;

    for(unsigned i = 0; i < cond.size(); i++)
    {
        
        tmpcond.push_back(cond[i]);
        if(tmpcond[i].attr == 1 && !noindex && tmpcond[i].comp!=SelCond::NE)
            //use index
        {
           int condval = atoi(tmpcond[i].value);
            if(tmpcond[i].comp == SelCond::GT)
            {
                tmpcond[i].comp = SelCond::GE;
                condval++;
                //cout<<"CONDVAL"<<condval;
            }
            if(tmpcond[i].comp == SelCond::LT)
            {
                tmpcond[i].comp = SelCond::LE;
                condval--;
            }
            
            useindex = true;
            checked.push_back(true);
            RC found;
            RC validcursor;
            IndexCursor target;
            //cout<<"CONDVAL"<<condval;
            found = Bindex.locate(condval, target);
            //fprintf(stdout, " found: %d ", found);
            //fprintf(stdout, " target.pid: %d target.eid: %d\n", target.pid, target.eid);
            nexttarget = target;
            RecordId temp;
            validcursor = Bindex.readForward(nexttarget, key, temp);
            //fprintf(stdout, " key: %d \n", key);
        
            
          
            switch (tmpcond[i].comp) {
                case SelCond::EQ:
                {
                    Cbegin = target;
                    Cend = nexttarget;
                    if(found == RC_NO_SUCH_RECORD)
                        nosuchvalue = true;
                    //setCbegin = true;
                    //setCend = true;
                }
                    break;
              /*
                case SelCond::GT:
                {
                    Cbegin = (found < 0) ? target : nexttarget;
                    if(validcursor == RC_INVALID_CURSOR)
                        nosuchvalue = true;
                    fprintf(stdout, " found: %d  validcursor: %d\n", found, validcursor);
                    Kmin = max(Kmin, condval+1);
                }
                    break;
                case SelCond::LT:
                {
                    Cend = target;
                    if(target.pid == 1&& target.eid == 0)
                        nosuchvalue = true;
                    fprintf(stdout, " Cend.pid: %d Cend.eid: %d\n", Cend.pid, Cend.eid);
                    Kmax =min(Kmax, condval-1);
                }
                    break;
               */
                case SelCond::GE:
                {
                    if(condval<=Kmin)
                        break;
                    if(found == RC_NO_SUCH_RECORD && validcursor == RC_INVALID_CURSOR)
                        nosuchvalue = true;
                    Cbegin = target;
                    Kmin = max(Kmin, condval);
                }
                    break;
                case SelCond::LE:
                {
                    if(condval>=Kmax)
                        break;
                    Cend = (found < 0) ? target : nexttarget;
                    if(found == RC_NO_SUCH_RECORD && target.pid == 1&& target.eid == 0)
                        nosuchvalue = true;
                    Kmax = min(Kmax, condval);
                }
                    break;
            }

            
            
        }
        else
            checked.push_back(false);
        
        
    }
     //fprintf(stdout, " nosuchreord?: %d\n", nosuchvalue);
  
    if(Kmax<Kmin)
    {
        if(attr != 4)
            fprintf(stdout, "INVALID KEY RANGE\n");
        else
            fprintf(stdout, "0\n");
        return 0;
    }
    if(Kmax == Kmin && Bindex.locate(Kmax, cursor)<0)
        nosuchvalue = true;
    if(nosuchvalue){
        if(attr != 4)
            fprintf(stdout, "NO SUCH RECORD\n");
        else
            fprintf(stdout, "0\n");
        return 0;
    }
    if(useindex){
        cursor = Cbegin;
        Bindex.readForward(cursor, key, rid);
    }
    //fprintf(stdout, "NO3 %d %d\n", cursor.pid, cursor.eid);
    //fprintf(stdout, "%d %d\n", Cend.pid, Cend.eid);
    
    for (unsigned i = 0; i < cond.size(); i++) {
        // if any constraint needs to read tuple value in RecordFile
        if(checked[i] == true)
            continue;
        if(cond[i].attr == 2){
            readRF = true;
            break;
        }
    }
    
    while ( rid < rf.endRid()) {

        // read the tuple
      
        if(attr == 2 || attr == 3 || readRF){
            if ((rc = rf.read(rid, key, value)) < 0) {
                fprintf(stderr, "Error: while reading a tuple from table %s\n", table.c_str());
                goto exit_select;
            }
        }
        
        
        
        // check the conditions on the tuple
        for (unsigned i = 0; i < cond.size(); i++) {
            // compute the difference between the tuple value and the condition value
            if(checked[i] == true)
                continue;
            switch (cond[i].attr) {
                case 1:
                {
                    diff = key - atoi(cond[i].value);
                    break;
                }
                case 2:
                    diff = strcmp(value.c_str(), cond[i].value);
                    break;
                    
            }
            
            //use index
      
                      // skip the tuple if any condition is not met
        //if(useindex)
                switch (cond[i].comp) {
                   
                    case SelCond::EQ:
                        if (diff != 0) goto next_cursor;
                        break;
                    case SelCond::NE:
                        if (diff == 0) goto next_cursor;
                        break;
                    case SelCond::GT:
                        if (diff <= 0) goto next_cursor;
                        break;
                    case SelCond::LT:
                        if (diff >= 0) goto next_cursor;
                        break;
                    case SelCond::GE:
                        if (diff < 0) goto next_cursor;
                        break;
                    case SelCond::LE:
                        if (diff > 0) goto next_cursor;
                        break;
                   
                }
          /*  if(!useindex)
                switch (cond[i].comp) {
                    case SelCond::EQ:
                        if (diff != 0) goto next_cursor;
                        break;
                    case SelCond::NE:
                        if (diff == 0) goto next_cursor;
                        break;
                    case SelCond::GT:
                        if (diff <= 0) goto next_cursor;
                        break;
                    case SelCond::LT:
                        if (diff >= 0) goto next_cursor;
                        break;
                    case SelCond::GE:
                        if (diff < 0) goto next_cursor;
                        break;
                    case SelCond::LE:
                        if (diff > 0) goto next_cursor;
                        break;
                }
           */
            }
        
        
        // the condition is met for the tuple.
        // increase matching tuple counte
        count++;
        //fprintf(stdout, "NO5 %d %d\n", cursor.pid, cursor.eid);
         //print the tuple
        switch (attr) {
            case 1:  // SELECT key
                fprintf(stdout, "%d\n", key);
                break;
            case 2:  // SELECT value
                fprintf(stdout, "%s\n", value.c_str());
                break;
            case 3:  // SELECT *
                fprintf(stdout, "%d '%s'\n", key, value.c_str());
                break;
        }
       
        // move to the next tuple
        
    next_cursor:
        if(useindex == true){
        if(cursor.pid==Cend.pid && cursor.eid==Cend.eid)
            break;
        Bindex.readForward(cursor, key, rid);
        }
        if(!useindex)
        ++rid;
        //fprintf(stdout, "%d %d\n", cursor.pid, cursor.eid);
        //fprintf(stdout, "%d %d\n", rid.pid, rid.sid);
      
    
    }
    if(count==0&&attr!=4)
        fprintf(stdout, "NO SUCH RECORD\n");
    // print matching tuple count if "select count(*)"
    if (attr == 4) {
        fprintf(stdout, "%d\n", count);
    }
    rc = 0;
    
    // close the table file and return
exit_select:
    rf.close();
    return rc;
}


RC SqlEngine::load(const string& table, const string& loadfile, bool index)
{
  /* your code here */
  ifstream infile;
  infile.open(loadfile.c_str(), ifstream::in);
  RecordFile rf;
  RecordId rid;
  BTreeIndex Bindex;
 
  RC rc;
  string line;
  int key;
  string value;
  
  //open the table file
  if((rc = rf.open(table + ".tbl", 'w')) < 0) {
    fprintf(stderr, "Error: could not open table %s, error code: %d\n", table.c_str(), rc);
    return rc;
    }
    if(index == true){
        //fprintf(stdout, "USING INDEX");
        if(rc =Bindex.open(table + ".idx", 'w') < 0){
            fprintf(stderr, "Error: could not open indextable %s, error code: %d\n", table.c_str(), rc);
            return rc;

        }
    }
    
  //rf.open(tablename, 'w');

  //load the tuples line by line
  while(getline(infile, line))
  {
    parseLoadLine(line, key, value);
    if((rc = rf.append(key, value, rid)) < 0) {
      fprintf(stderr, "Error: could not load tuple (%d, %s), error code : %d\n", key, value.c_str(), rc);
      return rc;
    }
      if(index == true)
          Bindex.insert(key, rid);
          
  }
  rc = 0;
  rf.close();
  Bindex.close();
  return rc;
}

RC SqlEngine::parseLoadLine(const string& line, int& key, string& value)
{
    const char *s;
    char        c;
    string::size_type loc;
    
    // ignore beginning white spaces
    c = *(s = line.c_str());
    while (c == ' ' || c == '\t') { c = *++s; }

    // get the integer key value
    key = atoi(s);

    // look for comma
    s = strchr(s, ',');
    if (s == NULL) { return RC_INVALID_FILE_FORMAT; }

    // ignore white spaces
    do { c = *++s; } while (c == ' ' || c == '\t');
    
    // if there is nothing left, set the value to empty string
    if (c == 0) { 
        value.erase();
        return 0;
    }

    // is the value field delimited by ' or "?
    if (c == '\'' || c == '"') {
        s++;
    } else {
        c = '\n';
    }

    // get the value string
    value.assign(s);
    loc = value.find(c, 0);
    if (loc != string::npos) { value.erase(loc); }

    return 0;
}
