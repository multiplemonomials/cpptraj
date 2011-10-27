// ArgList
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <string> // Used in constructor to detect quoted args
#include "ArgList.h"
#include "CpptrajStdio.h"
// NOTE: Place checks on memory
// NOTE: Check Arg, ReplaceArg, SplitAt etc to see how often they are used.

// CONSTRUCTOR: Empty Arg List
ArgList::ArgList() {
  nargs=0;
  arglist=NULL;
  marked=NULL;
  argline=NULL;
  debug=0;
}

// DESTRUCTOR
ArgList::~ArgList() {
  if (arglist!=NULL){
    for (int i=0; i<nargs; i++)
      if (arglist[i]!=NULL) free(arglist[i]);
    free(arglist);
  }
  if (marked!=NULL) free(marked);
  if (argline!=NULL) free(argline);
}

/* ArgList::SetDebug()
 * Set the arglist debug level.
 */
void ArgList::SetDebug(int debugIn) {
  debug = debugIn;
  if (debug>0) 
    mprintf("ArgList debug level set to %i\n",debug);
}

/* ArgList::SetList()
 * Separate input by the characters in separator and store as separate args.
 * This overwrites any existing args and completely resets the list.
 */
int ArgList::SetList(char *input, const char *separator) {
  char *pch;
  std::string quotedArg;
  size_t inputSize;

  if (input==NULL || separator==NULL) return 1;
  // Free existing arglist
  if (arglist!=NULL){
    for (int i=0; i<nargs; i++)
      if (arglist[i]!=NULL) free(arglist[i]);
    free(arglist);
  }
  if (marked!=NULL) free(marked);
  if (argline!=NULL) free(argline);
  arglist=NULL; marked=NULL; argline=NULL; nargs=0; 

  inputSize = strlen(input);
  // Replace any trailing newline char from input with NULL
  if (inputSize>0) {
    if (input[inputSize-1]=='\n') input[inputSize-1]='\0';
  }
  if (debug>0) 
    mprintf("getArgList: Setting up arg list for [%s] with separator [%s]\n",
            input, separator);

  // Store original argument line
  argline=(char*) malloc( (inputSize+1) * sizeof(char) );
  strcpy(argline, input);

  pch=strtok(input,separator);
  if (pch!=NULL) {

    while (pch!=NULL) {
      if (debug>1) mprintf("getArgList:  Arg %i, Token [%s], ",nargs,pch);
      if ( pch[0]!='"' ) 
        Add(pch);
      else {
        // If the argument begins with a quote, place this and all subsequent
        // arguments ending with another quote into the same argument.
        quotedArg.clear();
        quotedArg.assign(pch);
        // Check if this argument itself ends with a quote
        if (quotedArg.size() == 1 || quotedArg[quotedArg.size()-1]!='"') {
          while (pch!=NULL) {
            quotedArg.append(" ");
            pch=strtok(NULL," ");
            quotedArg.append(pch);
            if (strchr(pch,'"')!=NULL) break;
          }
        }
        // Remove the quotes from the argument
        for (std::string::iterator it=quotedArg.begin(); it < quotedArg.end(); it++)
          if (*it=='"') quotedArg.erase(it);  
        Add((char*)quotedArg.c_str());
      }
      if (debug>1) mprintf("Arglist[%i]= [%s]\n",nargs-1,arglist[nargs-1]);
      pch=strtok(NULL,separator);
    }
    // Setup marked array
    ResetAll();
  }
  if (debug>0) mprintf("getArgList: Processed %i args\n",nargs);
  return 0;
}

/* ArgList::Add()
 * Add input to the argument list.
 */
void ArgList::Add(char *input) {
  // Dont store blank tokens
  if (input==NULL) return;
  if (input[0]!='\n') {
    arglist=(char**) realloc(arglist,(nargs+1)*sizeof(char*));
    arglist[nargs]=(char*) malloc( (strlen(input)+1) * sizeof(char) );
    strcpy(arglist[nargs],input);
    marked=(char*) realloc(marked,(nargs+1)*sizeof(char*));
    marked[nargs]='F';
    nargs++;
  }
}

/* ArgList::operator=()
 */
ArgList &ArgList::operator=(const ArgList &rhs) {
  // Check for self assignment
  if (this == &rhs) return *this;

  // Deallocate
  if (arglist!=NULL){
    for (int i=0; i<nargs; i++)
      if (arglist[i]!=NULL) free(arglist[i]);
    free(arglist);
  }
  if (marked!=NULL) free(marked);
  if (argline!=NULL) free(argline);
  marked=NULL;
  argline=NULL;
  arglist=NULL;

  // Allocate and copy
  debug = rhs.debug;
  nargs = rhs.nargs;
  if (rhs.arglist!=NULL) {
    arglist = (char**) malloc(nargs * sizeof(char*));
    for (int i=0; i < nargs; i++) {
      arglist[i] = (char*) malloc( (strlen(rhs.arglist[i])+1) * sizeof(char));
      strcpy(arglist[i], rhs.arglist[i]);
    }
  }
  if (rhs.marked!=NULL) {
    marked = (char*) malloc(nargs * sizeof(char));
    memcpy(marked, rhs.marked, nargs*sizeof(char));
  }
  if (argline!=NULL) {
    argline = (char*) malloc( (strlen(rhs.argline)+1) * sizeof(char));
    strcpy(argline, rhs.argline);
  }

  // return *this
  return *this;
}

/* ArgList::Copy()
 * Return a copy of this arglist
 */
/*ArgList *ArgList::Copy() {
  ArgList *temp; 
  int i;

  temp=new ArgList();

  for (i=0; i<nargs; i++) 
    temp->Add(arglist[i]);

  if (argline!=NULL) {
    temp->argline=(char*) malloc( (strlen(argline)+1) * sizeof(char));
    strcpy(temp->argline,argline);
  }

  temp->Reset();

  return temp;
}*/

/* ArgList::print()
 * Print out each arg on separate lines
 */
void ArgList::print() {
  int i;
  for (i=0; i<nargs; i++) 
    mprintf("  %i: %s\n",i,arglist[i]);
    //mprintf("  ArgList[%i]=%s\n",i,arglist[i]);
}

/* ArgList::ArgLine()
 * Return the original argument string
 */
char *ArgList::ArgLine() {
  return argline;
}

/* ArgList::Arg()
 * Return arg at specified position.
 */
char *ArgList::Arg(int pos) {
  if (pos>-1 && pos<nargs) 
    return arglist[pos];
  return NULL;
}

/* ArgList::ArgIs()
 * Return true if arg at specified position matches input.
 */
bool ArgList::ArgIs(int pos, const char *input) {
  if (pos>-1 && pos<nargs)
    return ( strcmp(input, arglist[pos])==0 );
  return false;
}

/* ArgList::Command()
 * Check the first arg for command
 * Mark and return. Return even if marked.
 */
char *ArgList::Command() {

  if (nargs==0) return NULL;
  marked[0]='T';
  return arglist[0];
}

/* ArgList::CommandIs()
 * Check if key is command, return 1 if true. Mark command no matter what.
 */
int ArgList::CommandIs(const char *key) {
  marked[0]='T';
  if (strcmp(this->Command(),key)==0) return 1;
  return 0;
}

/* ArgList::CommandIs()
 * Check if first N chars of Command are key, return 1 if true. Mark command
 * no matter what.
 */
int ArgList::CommandIs(const char *key, int N) {
  marked[0]='T';
  if (strncmp(this->Command(),key,N)==0) return 1;
  return 0;
}

/* ArgList::getNextString()
 * Return next unmarked string
 */
char *ArgList::getNextString() {
  int i;
  for (i=0; i<nargs; i++)
    if (marked[i]!='T') {
      marked[i]='T';
      return arglist[i];
    }
  return NULL;
}

/* ArgList::CheckForMoreArgs()
 * Check if all arguments have been processed. If not print a warning along
 * with all unprocessed arguments.
 */
void ArgList::CheckForMoreArgs() {
  int i;
  bool empty;

  empty=true;
  for (i=0; i<nargs; i++) {
    if (marked[i]=='F') {
      empty=false;
      break;
    }
  }
  if (!empty) {
    mprintf("Warning: [%s] Not all arguments handled: [ ",arglist[0]);
    for (i=0; i<nargs; i++) {
      if (marked[i]=='F') mprintf("%s ",arglist[i]);
    }
    mprintf(" ]\n");
  }
}

/* ArgList::getNextMask()
 * Return next unmarked Mask. A mask MUST include one of the following: 
 *   ':' residue
 *   '@' atom
 *   '*' everything
 *   NOTE: Disabling the following for now:
 *   '/' element
 *   '%' type
 */
char *ArgList::getNextMask() {
  int i;
  for (i=0; i<nargs; i++) {
    if (marked[i]!='T') {
      if ( strchr( arglist[i], ':')!=NULL ||
           strchr( arglist[i], '@')!=NULL ||
           strchr( arglist[i], '*')!=NULL //||
           //strchr( arglist[i], '/')!=NULL ||
           //strchr( arglist[i], '%')!=NULL    
         ) 
      {
        marked[i]='T';
        return arglist[i];
      }
    }
  }
  return NULL;
}

/* ArgList::getNextInteger()
 * Convert next unmarked string to int and return, otherwise return def
 */
int ArgList::getNextInteger(int def) {
  int i;
  for (i=0; i<nargs; i++)
    if (marked[i]!='T') {
      // Check that first char is indeed an integer or '-', if not then continue
      if (!isdigit(arglist[i][0]) && arglist[i][0]!='-') {
        //mprintf("WARNING: Getting integer from arg (%s) that is not digit!\n",arglist[i]);
        continue;
      }
      marked[i]='T';
      return atoi(arglist[i]);
    }
  return def;
}

/* ArgList::getNextDouble()
 * Convert next unmarked string to double and return, otherwise return def
 */
double ArgList::getNextDouble(double def) {
  for (int i=0; i<nargs; i++)
    if (marked[i]!='T') {
      // Check that first char is indeed a digit, '.', or '-', if not then continue
      if (!isdigit(arglist[i][0]) && arglist[i][0]!='.' && arglist[i][0]!='-' ) {
        //mprintf("WARNING: getKeyDouble: arg (%s) does not appear to be a number!\n",
        //        arglist[i]);
        continue;
      }
      marked[i]='T';
      return atof(arglist[i]);
    }
  return def;
}


/* ArgList::getKeyString()
 * Search for unmarked key in arglist, return if found, otherwise return def
 */
char *ArgList::getKeyString(const char *key, char *def) {
  int i;

  for (i=0; i<nargs-1; i++)
    if (marked[i]!='T' && strcmp(key,arglist[i])==0) {
      marked[i]='T';
      i++;
      marked[i]='T';
      return arglist[i];
    }
  return def;
}

/* ArgList::getKeyIndex()
 * Search for unmarked key in arglist, return its position in the ArgList
 * if found, otherwise return -1
 */
int ArgList::getKeyIndex(char *key) {
  int i;

  for (i=0; i<nargs; i++) {
    if (strcmp(key,arglist[i])==0) return i;
  }
  return -1;
}

/* ArgList::getKeyInt()
 * Search for unmarked key in arglist, return if found, otherwise returh def
 */
int ArgList::getKeyInt(const char *key, int def) {
  int i;

  for (i=0; i<nargs-1; i++)
    if (marked[i]!='T' && strcmp(key,arglist[i])==0) {
      marked[i]='T'; 
      i++;
      marked[i]='T';
      // Brief check that first char is indeed an integer or '-'
      if (!isdigit(arglist[i][0]) && arglist[i][0]!='-') {
        mprintf("WARNING: Getting integer from arg (%s) that is not digit!\n",arglist[i]);
      }
      return atoi(arglist[i]);
    }
  return def;
}

/* ArgList::getKeyDouble()
 * Search for unmarked key in arglist, return if found, otherwise return def
 */
double ArgList::getKeyDouble(const char *key, double def) {
  int i;

  for (i=0; i<nargs-1; i++)
    if (marked[i]!='T' && strcmp(key,arglist[i])==0) {
      marked[i]='T';
      i++;
      marked[i]='T';
      // Brief check that first char is indeed a digit, '.', or '-' 
      if (!isdigit(arglist[i][0]) && arglist[i][0]!='.' && arglist[i][0]!='-' ) {
        mprintf("WARNING: getKeyDouble: arg (%s) does not appear to be a number!\n",
                arglist[i]);
      }
      return atof(arglist[i]);
    }
  return def;
}

/* ArgList::hasKey()
 * Return 1 if key is found, 0 if not. Mark key if found.
 */
int ArgList::hasKey(const char *key) {
  int i;

  for (i=0; i<nargs; i++) {
    if (marked[i]!='T' && strcmp(key,arglist[i])==0) {
      marked[i]='T';
      return 1;
    }
  }
  return 0;
}

/* ArgList::Contains()
 * Like hasKey(), but key is not marked. Return 1 if key is found, 0 if not.
 * NOTE: Should this be ignoring previously marked strings?
 */
int ArgList::Contains(const char *key) {
  for (int i=0; i<nargs; i++) {
    if (marked[i]!='T' && strcmp(key,arglist[i])==0) {
      return 1;
    }
  }
  return 0;
}

/* ArgList::SplitAt()
 * Split the argument list at the specified keyword. Remove those arguments
 * from this list and return as a new list.
 */
ArgList *ArgList::SplitAt(const char *key) {
  int i, argpos;
  ArgList *split;

  argpos=-1;
  for (i=0; i<nargs; i++) {
    if (strcmp(key,arglist[i])==0) {
      argpos = i;
      break;
    }
  }
  if (argpos==-1) {
    mprinterr("Error: Arglist::SplitAt: Key %s not found in arg list beginning\n",key);
    mprinterr("       with [%s]\n",arglist[0]);
    return NULL;
  }
  // Create new arglist starting from argpos
  split = new ArgList();
  for (i=argpos; i < nargs; i++) {
    split->Add(arglist[i]);
    free(arglist[i]);
  }
  split->Reset();  
  // Resize the current arg list
  nargs = argpos;
  arglist = (char**) realloc(arglist, argpos * sizeof(char*));
  marked = (char*) realloc(marked, argpos * sizeof(char));
  // DEBUG
  //mprintf("SPLIT LIST:\n");
  //split->print();

  return split;
}

/* ArgList::RemainingArgs()
 * Create a new argument list from args that have not been marked. Mark all
 * arguments from the old list. 
 */
ArgList *ArgList::RemainingArgs() {
  ArgList *newList = new ArgList();

  for (int i=0; i<nargs; i++) {
    if (marked[i]=='F') {
      newList->Add(arglist[i]);
      marked[i]='T';
    }
  }

  // If new arglist is empty return null
  if (newList->Nargs()==0) {
    delete newList;
    newList=NULL;
  // Otherwise set up the marked list to F for the new list
  } else {
    newList->ResetAll();
  }
  return newList;
}

/* ArgList::ReplaceArg()
 * Replace argument at the given position with a copy of the given argument.
 * The memory allocated to the original argument will be freed.
 */
int ArgList::ReplaceArg(int pos, char *argIn) {
  if (pos<0 || pos>=nargs) return 1;
  if (argIn==NULL) return 1;
  free(arglist[pos]);
  arglist[pos] = (char*) malloc( (strlen(argIn)+1) * sizeof(char) );
  strcpy(arglist[pos], argIn);
  return 0;
}

/* ArgList::CopyArg()
 * Return a copy of the argument at given position.
 */
char *ArgList::CopyArg(int pos) {
  char *outArg;
  if (pos<0 || pos>=nargs) return NULL;
  outArg = (char*) malloc( (strlen(arglist[pos])+1) * sizeof(char) );
  if (outArg==NULL) return NULL;
  strcpy(outArg, arglist[pos]);
  return outArg;
}

/* ArgList::Reset()
 * Reset marked except for the Command (arg 0). Allocate memory for marked
 * if not already done.
 */
void ArgList::Reset() {
  if (nargs==0) return;
  if (marked==NULL) 
    marked=(char*) malloc(nargs*sizeof(char));
  if (marked!=NULL) {
    memset(marked,'F',nargs);
    marked[0]='T';
  }
}

/* ArgList::ResetAll()
 * Reset all arguments including Command (arg 0)
 */
void ArgList::ResetAll() {
  if (nargs==0) return;
  if (marked==NULL)
    marked=(char*) malloc(nargs*sizeof(char));
  if (marked!=NULL) 
    memset(marked,'F',nargs);
}
    
