#ifndef __GRAPE_HH
#define __GRAPE_HH
#include <iostream>
#include <vector>
#include <cstdlib>
#include <math.h>

#include "TObject.h"
#include "Grapedefs.h"
using namespace std;

/*!
  Container for the information of one segment, contains segment number, time-stamp energy and waveform
*/
class GrapeSeg : public TObject {
public:
  //! default constructor
  GrapeSeg(){
    Clear();
  }
  //! Clear the segment information
  void Clear(Option_t *option = ""){
    fSegNumber = sqrt(-1);
    fSegTS = sqrt(-1);
    fSegPHA = sqrt(-1);
    fSegEn = sqrt(-1);
    fSegWave.clear();
    fSegWave.resize(WAVE_LENGTH);
  }
  //! Set the segment number
  void SetSegNumber(unsigned short segnumber){fSegNumber = segnumber;}
  //! Set the segment time-stamp
  void SetSegTS(unsigned short segTS){fSegTS = segTS;}
  //! Set the segment pulse height amplitude
  void SetSegPHA(unsigned short segPHA){fSegPHA = segPHA;}
  //! Set the segment energy
  void SetSegEn(float segEn){fSegEn = segEn;}
  //! Set the segment waveform a sample n
  void SetSegWave(unsigned short n, unsigned short segWave){fSegWave.at(n) = segWave;}
  //! Set the segment waveform
  void SetSegWave(vector<unsigned short> segWave){fSegWave = segWave;}

  //! Returns the segment number
  unsigned short GetSegNumber(){return fSegNumber;}
  //! Returns the segment time-stamp
  unsigned short GetSegTS(){return fSegTS;}
  //! Returns the segment pulse height amplitude
  unsigned short GetSegPHA(){return fSegPHA;}
  //! Returns the segment energy
  float GetSegEn(){return fSegEn;}
  //! Returns the segment waveform
  vector <unsigned short> GetWave(){return fSegWave;}

protected:
  //! Segment number 0-8
  unsigned short fSegNumber;
  //! Timestamp of the SEG signal
  unsigned short fSegTS;
  //! Pulse height amplitude of the SEG signal
  unsigned short fSegPHA;
  //! calibrated energy of the segment
  float fSegEn;
  //! Waveform of the SEG signal
#ifdef WRITE_WAVE
  vector <unsigned short> fSegWave;
#else
  vector <unsigned short> fSegWave; //!
#endif

  /// \cond CLASSIMP
  ClassDef(GrapeSeg,1);
  /// \endcond
};

/*!
  Container for the information of a single hit, contains identifying information, LED, time-stamp, and energy, the waveform and a vector of GrapeSeg for the segments
*/
class GrapeHit : public TObject {
public:
  //! default constructor
  GrapeHit(){
    Clear();
  }
  //! Clear the hit information, including the segments
  void Clear(Option_t *option = ""){
    fFileNumber = -1;
    fTrigFlag = sqrt(-1);
    fBoardNumber = sqrt(-1);
    fDetNumber = sqrt(-1);
    fSumLET = sqrt(-1);
    fSumTS = sqrt(-1);
    fSumPHA = sqrt(-1);
    fSumEn = sqrt(-1);
    fSumWave.clear();
    fSumWave.resize(WAVE_LENGTH);
    fSegments.clear();
    fSegMult = 0;
  }
  //! Set the file number
  void SetFileNumber(short filenumber){fFileNumber = filenumber;}
  //! Set the trigger flag
  void SetTrigFlag(unsigned short trigflag){fTrigFlag = trigflag;}
  //! Set the board number
  void SetBoardNumber(unsigned short boardnumber){fBoardNumber = boardnumber;}
  //! Set the detector/module number
  void SetDetNumber(unsigned short detnumber){fDetNumber = detnumber;}
  //! Set the Sum contact leading edge time
  void SetSumLET(unsigned short sumLET){fSumLET = sumLET;}
  //! Set the Sum contact time-stamp
  void SetSumTS(long long int sumTS){fSumTS = sumTS;}
  //! Set the Sum contact pulse height amplitude
  void SetSumPHA(unsigned short sumPHA){fSumPHA = sumPHA;}
  //! Set the Sum energy
  void SetSumEn(float sumEn){fSumEn = sumEn;}
  //! Set the Sum contact waveform a sample n
  void SetSumWave(unsigned short n, unsigned short sumWave){fSumWave.at(n) = sumWave;}
  //! Set the Sum contact waveform
  void SetSumWave(vector<unsigned short> sumWave){fSumWave = sumWave;}
  
  //! Adds one segment, if the pulse height of the segment is >0 the multiplicity is increased
  void AddSegment(GrapeSeg add){
    if(fSegments.size()>NUM_SEGMENTS){
      cout << "adding segment mult > " << NUM_SEGMENTS << endl;
      return;
    }
    fSegments.push_back(add);
    if(add.GetSegPHA()>0){
      fSegMult++;
    }
  }
  
  //! Returns the file number 
  short GetFileNumber(){return fFileNumber;}
   //! Returns the trigger flag
  unsigned short GetTrigFlag(){return fTrigFlag;}
  //! Returns the board number
  unsigned short GetBoardNumber(){return fBoardNumber;}
  //! Returns the detector/module number
  unsigned short GetDetNumber(){return fDetNumber;}
  //! Returns the detector/module ID
  unsigned short GetDetID(){return fDetNumber*2+fBoardNumber;}
  //! Returns the Sum contact leading edge time
  unsigned short GetSumLET(){return fSumLET;}
  //! Returns the Sum contact time-stamp
  long long int GetSumTS(){return fSumTS;}
  //! Returns the Sum contact pulse height amplitude
  unsigned short GetSumPHA(){return fSumPHA;}
  //! Returns the Sum energy
  float GetSumEn(){return fSumEn;}
  //! Returns the Sum contact waveform
  vector <unsigned short> GetSumWave(){return fSumWave;}
  //! Returns the segment number n
  GrapeSeg* GetSegment(int n){return &fSegments.at(n);}
  //! Returns the segments
  vector<GrapeSeg>* GetSegments(){return &fSegments;}
  //! Returns the segment multiplicity
  unsigned short GetSegMult(){return fSegMult;}
  
  //! Returns true if the Sum contact has a waveform
  bool HasSumWave() const {
    for(unsigned short i=0;i<fSumWave.size();i++){
      if(fSumWave.at(i)>0){
	return true;
      }
    }
    return false;
  }
  //! Printing information
  void Print(Option_t *option = "") const {
    cout << "file: " << fFileNumber;
    cout << "\tdetID: " << fDetNumber;
    cout << "\tBN: " << fBoardNumber;
    cout << "\tTS: " << fSumTS;
    cout << "\tPHA: "<< fSumPHA;
    cout << "\tEn: "<< fSumEn;
    if(HasSumWave())
      cout << "\tWAVE: Y";
    else 
      cout << "\tWAVE: N";
    cout << "\t#Seg: "<< fSegMult << endl;
  } 

protected:
  //! From which file was the hit read. (redundant should be equialent to the detector number fDetNumber)
  short fFileNumber; 
  //! Flag specifying external/internal trigger
  unsigned short fTrigFlag;
  //! Board number, or crystal number of one module, 0: detector A, 1: detector B
  unsigned short fBoardNumber;
  //! Detector number, derived from its IP address
  unsigned short fDetNumber;
  //! Leading Edge time of the SUM signal
  unsigned short fSumLET;
  //! Timestamp of the SUM signal
  long long int fSumTS;
  //! Pulse height amplitude of the SUM signal
  unsigned short fSumPHA;
  //! calibrated sum energy 
  float fSumEn;
  //! Waveform of the SUM signal
#ifdef WRITE_WAVE
  vector <unsigned short> fSumWave;
#else
  vector <unsigned short> fSumWave; //!
#endif
  //! a vector containing the segment information
  vector <GrapeSeg> fSegments;
  //! multiplicity of segments with net charge
  unsigned short fSegMult;

  /// \cond CLASSIMP
  ClassDef(GrapeHit,1);
  /// \endcond
};

/*!
  Container for the information of an event, contains a vector of GrapeHit for the hits and their multplicity
*/
class GrapeEvent : public TObject {
public:
  //! default constructor
  GrapeEvent(){
    Clear();
  };
  //! Clear the event information, including all hits contained
  void Clear(Option_t *option = ""){
    fMult = 0;
    fHits.clear();
  }
  //! Returns the multiplicity of the event
  unsigned short GetMult(){return fMult;}
  //! Returns the hit number i
  GrapeHit* GetHit(unsigned short i){return fHits.at(i);}
  //! Add a hit to the event
  void Add(GrapeHit* add){
    fHits.push_back(add);
    fMult++;
  }
  //! Printing information
  void Print(Option_t *option = "") const {
    cout << "multiplicity " << fMult << " event" << endl;
    for(unsigned short i=0;i<fHits.size();i++)
      fHits.at(i)->Print();
  } 
protected:
  //! a vector containing the hits which belong to the event
  vector <GrapeHit*> fHits;
  //! multiplicity of hits in the event
  unsigned short fMult;

  /// \cond CLASSIMP
  ClassDef(GrapeEvent,1);
  /// \endcond
};

/*!
  Compare two hits by their time-stamps
*/
class HitComparer {
public:
  //! compares time-stamps of the hits
  bool operator() ( GrapeHit *lhs, GrapeHit *rhs) {
    return (*lhs).GetSumTS() < (*rhs).GetSumTS();
  }
};

#endif
