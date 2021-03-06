#ifndef __BUILDEVENTS_HH
#define __BUILDEVENTS_HH
#include <iostream>
#include <iomanip>
#include <vector>
#include <set>
#include <algorithm>
#include <queue>

#include "TTree.h"
#include "TEnv.h"
#include "TObject.h"

#include "Globaldefs.h"
#include "Grapedefs.h"
#include "Grape.hh"

/*!
  A class for unpacking and event building of GRAPE data
*/
class BuildEvents {
public:
  //! dummy constructor
  BuildEvents(){};
  //! dummy destructor
  ~BuildEvents(){
    delete fEvent;
    delete fTree;
  };
  //! Set the verbose level
  void SetVL(unsigned short vl){
    fVerboseLevel = vl;
  }
  //! Set if the tree is to be written
  void SetWriteTree(bool writetree){
    fWriteTree = writetree;
  }
  //! Initialyze the event builder, read the settings, and create the tree
  bool Init(char* settings);
  //! Read and unpack two blocks from each detector file
  long long int ReadEachFile();
  //! Read and unpack two blocks from detectors that have been previously added to the event
  pair<long long int,bool> ReadNewFiles();
  //! Unpack one detector, read and unpack from one file
  long long int Unpack(unsigned short det);
  //! Unpack one crystal, create a hit, add it to the storage vector
  long long int UnpackCrystal(unsigned short det, long long int &ts, bool checkonly = false);
  //! Sort the hits according to their time stamp
  void SortHits();
  //! Process the hits and build one event
  void ProcessHits();
  //! Close the current event
  void CloseEvent();
  //! Print error counters
  void PrintErrors();
  //! Check the end of the hit buffer, this should be 8 times 0xffff
  bool CheckBufferEnd(unsigned short *buffer);
  //! Skip bytes until the next  8 times 0xffff occurs
  pair<long long int,bool> SkipBytes(unsigned short det, unsigned short buffer[8]);
  //! Forward all files untile the timestampjump occurs
  long long int DetectTimestampJumps();
  //! Skip events until the timestamp jumps back significantly
  long long int DetectTimestampJump(unsigned short det);
  //! Returns pointer to the current event
  GrapeEvent* GetEvent(){return fEvent;}
  //! Returns the tree for writing to file
  TTree* GetTree(){return fTree;}
  //! Returns the number of buffers read
  unsigned int GetNBuffers(){return fNbuffers;}
  //! Get the size of the vector of stored hits
  unsigned short GetHitsLeft(){return fHits.size();}
  //! Returns the first time-stamp
  long long int GetFirstTS(){return fFirstTS;}
  //! Returns the current time-stamp
  long long int GetCurrentTS(){return fCurrentTS;}
  //! Returns the number of skipped words
  long long int GetSkippedWords(){return fSkipped;}
  
protected:
  //! Verbose level
  unsigned short fVerboseLevel;
  //! should the tree be written to file
  bool fWriteTree;
  //! Tree which will be filled
  TTree* fTree;
  //! The number of buffers read
  unsigned int fNbuffers;
  //! The number of hits read (should be 2*fNbuffers, if there are no errors)
  unsigned int fNhits;
  //! The number of skipped words
  long long int fSkipped;
  //! The error counter
  unsigned int fErrors[10];

  //! The number of detectors or data files
  unsigned short fNdet;
  //! The raw datafiles 
  vector <FILE*> fDatafiles;
  //! The detector numbers
  vector <unsigned short> fDetNumbers;
  //! The current time-stamp
  long long int fCurrentTS;
  //! The first time-stamp
  long long int fFirstTS;

  //! The event building window
  unsigned int fEventWindow;
  //! Maximum time-stamp difference between module A and B
  unsigned int fMaxTSDiff;
  //! How many hits from each file already read and in memory
  vector<unsigned short> fRead;

  //! The built event, 
  GrapeEvent* fEvent;
  //! Vector containing the individual hits, time sorted
  vector<GrapeHit*> fHits;
};

#endif
