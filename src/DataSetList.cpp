// DataSetList
#include <cstdio> // sprintf
#include <cstring>
// This also includes basic DataSet class and dataType
#include "DataSetList.h"
#include "CpptrajStdio.h"
// Data types go here
#include "DataSet_double.h"
#include "DataSet_string.h"
#include "DataSet_integer.h"
#include "DataSet_float.h"

// CONSTRUCTOR
DataSetList::DataSetList() :
  debug_(0), 
  hasCopies_(false),
  maxFrames_(0),
  vecidx_(0) 
{
  //fprintf(stderr,"DSL Constructor\n");
}

// DESTRUCTOR
DataSetList::~DataSetList() {
  //fprintf(stderr,"DSL Destructor\n");
  if (!hasCopies_) {
    for (DataListType::iterator ds = DataList_.begin(); ds != DataList_.end(); ++ds) 
      delete *ds; 
  }
}

// DataSetList::begin()
DataSetList::const_iterator DataSetList::begin() const {
  return DataList_.begin();
}

// DataSetList::end()
DataSetList::const_iterator DataSetList::end() const {
  return DataList_.end();
}

// DataSetList::SetDebug()
void DataSetList::SetDebug(int debugIn) {
  debug_ = debugIn;
  if (debug_>0) 
    mprintf("DataSetList Debug Level set to %i\n",debug_);
}

// DataSetList::SetMax()
/** Set the max number frames expected to be read in. Used to preallocate
  * data set sizes in the list.
  */
void DataSetList::SetMax(int expectedMax) {
  maxFrames_ = expectedMax;
  if (maxFrames_<0) maxFrames_=0;
}

/* DataSetList::SetPrecisionOfDatasets()
 * Set the width and precision for all datasets in the list.
 */
void DataSetList::SetPrecisionOfDatasets(int widthIn, int precisionIn) {
  for (DataListType::iterator ds = DataList_.begin(); ds != DataList_.end(); ++ds) 
    (*ds)->SetPrecision(widthIn,precisionIn);
}

// DataSetList::Get()
DataSet *DataSetList::Get(const char *nameIn) {
  for (DataListType::iterator ds = DataList_.begin(); ds != DataList_.end(); ++ds)
    if ( (*ds)->Name().compare( nameIn )==0 )
      return *ds;
  return NULL;
}

// DataSetList::GetDataSetIdx()
DataSet *DataSetList::GetDataSetIdx(int idxIn) {
  for (DataListType::iterator ds = DataList_.begin(); ds != DataList_.end(); ++ds)
    if ((*ds)->Idx() == idxIn) 
      return *ds;
  return NULL;
}

// DataSetList::GetDataSetN()
DataSet *DataSetList::GetDataSetN(int ndataset) {
  if (ndataset < 0 || ndataset >= (int)DataList_.size()) 
    return NULL;
  return DataList_[ndataset];
}

// DataSetList::AddMultiN()
/** Add a dataset to the list, basing the dataset name on prefix,
  * suffix, and a given number. If a dataset already exists in
  * the list NULL will be returned instead. The dataset will be named
  *   <prefix>_<suffix><Nin> 
  * if <prefix> is not NULL, 
  *   <suffix><Nin> 
  * if <prefix> is blank (i.e. ""), and 
  *   <suffix><Nin>_XXXXX
  * otherwise. The intended use is for things like setting up data for a 
  * list of residues, where the residues may not be sequential or start 
  * from 0. 
  * \param inType type of dataset to set up
  * \param prefix dataset name prefix
  * \param suffix dataset name suffix
  * \param Nin Number that can be used to uniquely identify the dataset.
  * \return pointer to successfully set up dataset.
  */
// NOTE: Instead of using Nin to identify, why not use dataset name?
DataSet *DataSetList::AddMultiN(DataSet::DataType inType, const char *prefix, 
                                const char *suffix, int Nin) 
{
  char tempName[64];
  char tempSuffix[32];
  DataSet *tempDS = NULL;
  // Determine if dataset with idx Nin exists.
  tempDS = GetDataSetIdx( Nin );
  if (tempDS != NULL) return NULL; 
  sprintf(tempSuffix,"%s%i",suffix,Nin);
  if (prefix==NULL) {
    tempDS = this->Add(inType,NULL,tempSuffix);
  } else if (strcmp(prefix,"")==0) {
    tempDS = this->Add(inType,tempSuffix,tempSuffix);
  // Sanity check - make sure name is not too big
  } else {
    if (strlen(prefix)+strlen(tempSuffix)+1 > 64) {
      mprinterr("Internal Error: DataSetList::AddMultiN: size of %s+%s > 64\n",prefix,tempSuffix);
      return NULL;
    }
    sprintf(tempName,"%s_%s",prefix,tempSuffix);
    tempDS = this->Add(inType,tempName,tempSuffix);
  }
  if (tempDS!=NULL) tempDS->SetIdx( Nin );
  return tempDS;
}

// DataSetList::AddMultiN()
DataSet* DataSetList::AddMultiN(DataSet::DataType inType, std::string& nameIn, int Nin)
{
  // Determine if a dataset with idx Nin exists.
  DataSet *tempDS = GetDataSetIdx( Nin );
  if (tempDS != NULL) {
    mprinterr("Internal Error: DataSet with index %i already exists.\n",Nin);
    return NULL;
  }
  // Set up new dataset
  tempDS = this->Add(inType, nameIn.c_str(), nameIn.c_str());
  if (tempDS!=NULL) tempDS->SetIdx( Nin );
  return tempDS;
}

// DataSetList::AddMulti()
/** Works like Add, except the dataset will be named 
  *   <prefix>_<suffix>
  * if <prefix> is not NULL, and
  *   <suffix>_XXXXX
  * otherwise.
  * \param inType type of dataset to set up
  * \param prefix dataset name prefix
  * \param suffix dataset name suffix
  * \return pointer to successfully set up dataset.
  */
DataSet* DataSetList::AddMulti(DataSet::DataType inType, const char *prefix, const char *suffix) 
{
  char tempName[32];
  if (prefix==NULL)
    return this->Add(inType,NULL,suffix);
  // Sanity check - make sure name is not too big
  if (strlen(prefix)+strlen(suffix)+1 > 32) {
    mprinterr("Internal Error: DataSetList::AddMulti: size of %s+%s > 32\n",prefix,suffix);
    return NULL;
  } 
  sprintf(tempName,"%s_%s",prefix,suffix);
  return this->Add(inType,tempName,suffix);
}

// DataSetList::checkName()
/** Given a potential dataset name, check if that name exists in the
  * DataSetList. If no name is given, generate a name based on
  * a default name and the dataset number.
  * \return An acceptable dataset name, or NULL if not acceptable.
  */
char *DataSetList::checkName(const char *nameIn, const char *defaultName) {
  char *dsetName;
  size_t namesize;
  // Require all calls provide a default name
  if (defaultName==NULL) {
    mprinterr("Internal Error: DataSetList called without default name.\n");
    return NULL;
  }
  // If no name given make one up based on the given default 
  if (nameIn==NULL) {
    // Determine size of name + extension
    namesize = strlen( defaultName );
    size_t extsize = (size_t) DigitWidth( Size() ); // # digits
    if (extsize < 5) extsize = 5;                  // Minimum size is 5 digits
    extsize += 2;                                  // + underscore + null
    namesize += extsize;
    dsetName = new char[ namesize ];
    sprintf(dsetName,"%s_%05i", defaultName, Size());
    //mprintf("NAME SIZE [%s] = %lu\n",dsetName,namesize);
  } else {
    namesize = strlen( nameIn ) + 1; // + null
    dsetName = new char[ namesize ];
    strcpy(dsetName, nameIn);
  }
  // Check if dataset name is already in use
  if (Get(dsetName)!=NULL) {
    mprinterr("Error: Data set %s already defined.\n",dsetName);
    delete[] dsetName;
    return NULL;
  }
  // Otherwise this dataset name is good to go
  return dsetName;
}

// DataSetList::Add()
/** Add a DataSet of specified type, set it up and return pointer to it. 
  * Name the dataset nameIn if specified, otherwise give it a default
  * name based on the given defaultName and dataset #. This routine
  * MUST be called with a default name.
  * \param inType type of dataset to set up
  * \param nameIn dataset name, can be NULL.
  * \param defaultName default name prefix for use if nameIn not specified.
  * \return pointer to successfully set-up dataset.
  */ 
DataSet *DataSetList::Add(DataSet::DataType inType, const char *nameIn, const char *defaultName) 
{
  // Dont add to a list with copies
  if (hasCopies_) {
    mprinterr("Error: Attempting to add dataset to list with copies.\n");
    return NULL;
  }
  DataSet *D=NULL;
  char *dsetName = checkName(nameIn, defaultName);
  if (dsetName==NULL) return NULL;

  switch (inType) {
    case DataSet::DOUBLE       : D = new DataSet_double(); break;
    case DataSet::FLOAT        : D = new DataSet_float(); break;
    case DataSet::STRING       : D = new DataSet_string(); break;
    case DataSet::INT          : D = new DataSet_integer(); break;
    //case XYZ          : D = new DataSet_XYZ(); break;
    case DataSet::UNKNOWN_DATA :
    default           :
      mprinterr("Error: DataSetList::Add: Unknown set type.\n");
      return NULL;
  }
  if (D==NULL) {
    delete[] dsetName;
    return NULL;
  }
  // Set up dataset
  if ( D->Setup(dsetName,maxFrames_) ) {
    mprinterr("Error setting up data set %s.\n",dsetName);
    delete D;
    delete[] dsetName;
    return NULL;
  }

  DataList_.push_back(D); 
  //fprintf(stderr,"ADDED dataset %s\n",dsetName);
  delete[] dsetName;
  return D;
}

// DataSetList::AddDataSetCopy()
/** Add a pointer to an existing dataset to the list. Used when
  * DataSetList is being used as a container in DataFile.
  */
int DataSetList::AddDataSetCopy(DataSet *dsetIn) {
  if (dsetIn==NULL) return 1;
  if (!hasCopies_ && !DataList_.empty()) {
    mprinterr("Error: Adding dataset copy to list with non-copies!\n");
    return 1;
  }
  hasCopies_ = true;
  DataList_.push_back(dsetIn);
  return 0;
}

// DataSetList::AddDataSet()
int DataSetList::AddDataSet(DataSet* dsetIn) {
  if (dsetIn==NULL) return 1;
  DataList_.push_back(dsetIn);
  return 0;
}

// DataSetList::PrepareForWrite()
/** Prepare data sets in the list for writing. Since this is currently
  * only used when DataSetList is being used as a container in DataFile,
  * make this valid only if the DataSetList contains copies.
  * Remove DataSets that dont contain data.
  * \return Max X value in all sets, -1 on error.
  */
int DataSetList::PrepareForWrite() {
  int maxSetFrames = 0;
  int currentMax = 0;
  // If not copies exit?
  if (!hasCopies_) {
    mprintf("Info: DataSetList does not have copies.\n");
  }
  // If no data sets, exit.
  if (DataList_.empty()) return -1;
  // Remove data sets that do not contain data.
  // NOTE: CheckSet also sets up the format string for the dataset.
  DataListType::iterator Dset = DataList_.end();
  while (Dset != DataList_.begin()) {
    Dset--;
    // If set has no data, remove it
    if ( (*Dset)->CheckSet() ) {
      mprintf("Warning: Set %s contains no data - skipping.\n",(*Dset)->c_str()); 
      // Remove
      if (!hasCopies_) delete *Dset;
      DataList_.erase( Dset );
      Dset = DataList_.end();
    // If set has data, set its format to right-aligned initially. Also 
    // determine what the maximum x value for the set is.
    } else {
      if ((*Dset)->SetDataSetFormat(false)) return -1;
      maxSetFrames = (*Dset)->Xmax();
      if (maxSetFrames > currentMax) currentMax = maxSetFrames;
    }
  }
  // Reset Ndata
  //Ndata = (int)DataList_.size();

  // If all data sets are empty no need to write.
  if (DataList_.empty()) return -1;

  // Since currentMax is the last frame, the actual # of frames
  // is currentMax+1 (for use in loops).
  ++currentMax;
  return currentMax;
}

// DataSetList::AddData()
/** Add data to a specific dataset in the list
  * \param frame frame to add data to
  * \param dataIn pointer to data to add
  * \param SetNumber dataset to add data to
  * \return 0 on success, 1 on error.
  */
int DataSetList::AddData(int frame, void *dataIn, int SetNumber) {
  if (SetNumber<0 || SetNumber>=(int)DataList_.size()) 
    return 1;
  DataList_[SetNumber]->Add(frame, dataIn);
  return 0;
}

// DataSetList::Info()
/** Print information on all data sets in the list, as well as any datafiles
  * that will be written to.
  */
void DataSetList::Info() {
  mprintf("\nDATASETS:\n");
  if (DataList_.empty())
    mprintf("  There are no data sets set up for analysis.");
  else if (DataList_.size()==1)
    mprintf("  There is 1 data set: ");
  else
    mprintf("  There are %zu data sets: ", DataList_.size());

  for (unsigned int ds=0; ds<DataList_.size(); ds++) {
    if (ds>0) mprintf(",");
    mprintf("%s",DataList_[ds]->c_str());
    //DataList[ds]->Info();
  }
  mprintf("\n");

  // DataFile Info
  //DFL.Info();
}

// DataSetList::Sync()
void DataSetList::Sync() {
  // Sync datasets - does nothing if worldsize is 1
  for (DataListType::iterator ds = DataList_.begin(); ds != DataList_.end(); ++ds) {
    if ( (*ds)->Sync() ) {
      rprintf( "Error syncing dataset %s\n",(*ds)->c_str());
      //return;
    }
  }
}

// ---------- VECTOR ROUTINES
void DataSetList::VectorBegin() {
  vecidx_ = 0;
}

DataSet* DataSetList::NextVector() {
  for (int idx = vecidx_; idx < (int)DataList_.size(); ++idx) {
    if (DataList_[idx]->Type() == DataSet::VECTOR) {
      // Position vecidx at the next dataset
      vecidx_ = idx + 1;
      return DataList_[idx];
    }
  }
  return 0;
}

DataSet* DataSetList::NextMatrix() {
  for (int idx = vecidx_; idx < (int)DataList_.size(); ++idx) {
    if (DataList_[idx]->Type() == DataSet::MATRIX) {
      // Position vecidx at the next dataset
      vecidx_ = idx + 1;
      return DataList_[idx];
    }
  }
  return 0;
}

DataSet* DataSetList::NextModes() {
  for (int idx = vecidx_; idx < (int)DataList_.size(); ++idx) {
    if (DataList_[idx]->Type() == DataSet::MODES) {
      // Position vecidx at the next dataset
      vecidx_ = idx + 1;
      return DataList_[idx];
    }
  }
  return 0;
}

