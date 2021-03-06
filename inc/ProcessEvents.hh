#ifndef __PROCESSEVENTS_HH
#define __PROCESSEVENTS_HH
#include <iostream>
#include <iomanip>

#include "TTree.h"
#include "TEnv.h"
#include "TObject.h"
#include "TRandom.h"

#include "Globaldefs.h"
#include "Grapedefs.h"
#include "Grape.hh"

/*!
  A class for calibrating and processing of GRAPE data
*/
class ProcessEvents {
public:
  //! dummy constructor
  ProcessEvents(){};
  //! dummy destructor
  ~ProcessEvents(){
  };
  //! Set the verbose level
  void SetVL(unsigned short vl){
    fVerboseLevel = vl;
  }
  //! Initialyze the event processor, read the settings
  void Init(char *settings);
  //! Calibrate the event
  void Calibrate(GrapeEvent* event);
  //! Perform the add-back
  void AddBack(GrapeEvent* event);
  //! Check if two hits are supposed to be added
  bool AddBack(GrapeHit* hit0, GrapeHit* hit1);
protected:
  //! Verbose level
  unsigned short fVerboseLevel;
  //! Random number generator for the calibration
  TRandom* fRand;
  //! The gain parameters for all segments
  float fgain[MAX_NUM_DET][2][NUM_SEGMENTS];
  //! The offsets for the segments
  float foffs[MAX_NUM_DET][2][NUM_SEGMENTS];
};

#endif
