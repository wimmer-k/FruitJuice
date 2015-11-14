#ifndef __BUILDEVENTS_HH
#define __BUILDEVENTS_HH
#include <iostream>
#include <iomanip>
#include <vector>
#include <set>
#include <algorithm>

#include "TTree.h"
#include "TEnv.h"
#include "TObject.h"

#include "Grapedefs.h"
#include "Grape.hh"

class BuildEvents {
public:
  BuildEvents(){};
  ~BuildEvents(){
    delete fEvent;
  };
  //! Set the verbose level
  void SetVL(unsigned short vl){
    fVerboseLevel = vl;
  }
  //! Initialyze the event builder, read the settings, and create the tree
  bool Init(char* settings);
  //! Read and unpack two blocks from each detector file
  long long int ReadEachFile();
  //! Read and unpack two blocks from detectors that have been previously added to the event
  long long int ReadNewFiles();
  //! Unpack one detector, read and unpack from one file
  long long int Unpack(unsigned short det);
  //! Unpack one crystal, create a hit, add it to the storage vector
  long long int UnpackCrystal(unsigned short det);
  //! Sort the hits according to their time stamp
  void SortHits();
  //! Process the hits and build one event
  void ProcessHits();
  //! Close the current event
  void CloseEvent();
  //! Returns the tree for writing to file
  TTree* GetTree(){return fTree;}
  //! Returns the number of buffers read
  unsigned int GetNBuffers(){return fNbuffers;}
  
  //! dummy method to be delete
  void Run(int last);
protected:
  //! Verbose level
  unsigned short fVerboseLevel;
  //! should the tree be written to file
  bool fWriteTree;
  //! Tree which will be filled
  TTree *fTree;
  //! The number of buffers read
  unsigned int fNbuffers;
  //! The number of hits read (should be 2*fNbuffers)
  unsigned int fNhits;

  //! The number of detectors or data files
  unsigned short fNdet;
  //! The raw datafiles 
  vector <FILE*> fDatafiles;
  //! The current time-stamp
  long long int fCurrentTS;
  //! The first time-stamp
  long long int fFirstTS;

  //! The event building window
  unsigned int fEventWindow;
  //! which detectors have been removed from the vector, a set will be unique 
  set<unsigned short> fRemoved;

  //! The built event, 
  GrapeEvent *fEvent;
  //! Vector containing the individual hits, time sorted
  vector<GrapeHit*> fHits;
};

#endif
