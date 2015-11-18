#ifndef __GRAPE_HH
#define __GRAPE_HH
#include <iostream>
#include <vector>
#include <cstdlib>
#include <math.h>

#include "TObject.h"
#include "Grapedefs.h"

using namespace std;
class GrapeSeg : public TObject {
public:
  GrapeSeg(){
    Clear();
  }
  void Clear(Option_t *option = ""){
    fSegNumber = sqrt(-1);
    fSegTS = sqrt(-1);
    fSegPHA = sqrt(-1);
    fSegWave.clear();
    fSegWave.resize(WAVE_LENGTH);
  }
  void SetSegNumber(unsigned short segnumber){fSegNumber = segnumber;}
  void SetSegTS(unsigned short segTS){fSegTS = segTS;}
  void SetSegPHA(unsigned short segPHA){fSegPHA = segPHA;}
  void SetSegWave(unsigned short n, unsigned short segWave){fSegWave.at(n) = segWave;}
  void SetSegWave(vector<unsigned short> segWave){fSegWave = segWave;}

  unsigned short GetSegNumber(){return fSegNumber;}
  unsigned short GetSegTS(){return fSegTS;}
  unsigned short GetSegPHA(){return fSegPHA;}
  vector <unsigned short> GetWave(){return fSegWave;}

protected:
  //! Segment number 0-8
  unsigned short fSegNumber;
  //! Timestamp of the SEG signal
  unsigned short fSegTS;
  //! Pulse height amplitude of the SEG signal
  unsigned short fSegPHA;
  //! Waveform of the SEG signal
#ifdef WRITE_WAVE
  vector <unsigned short> fSegWave;
#else
  vector <unsigned short> fSegWave; //!
#endif
  ClassDef(GrapeSeg,1);
};


class GrapeHit : public TObject {
public:
  GrapeHit(){
    Clear();
  }
  void Clear(Option_t *option = ""){
    fFileNumber = -1;
    fTrigFlag = sqrt(-1);
    fBoardNumber = sqrt(-1);
    fDetNumber = sqrt(-1);
    fSumLET = sqrt(-1);
    fSumTS = sqrt(-1);
    fSumPHA = sqrt(-1);
    fSumWave.clear();
    fSumWave.resize(WAVE_LENGTH);
    fSegments.clear();
    fSegMult = 0;
  }
  void SetFileNumber(short filenumber){fFileNumber = filenumber;}
  void SetTrigFlag(unsigned short trigflag){fTrigFlag = trigflag;}
  void SetBoardNumber(unsigned short boardnumber){fBoardNumber = boardnumber;}
  void SetDetNumber(unsigned short detnumber){fDetNumber = detnumber;}
  void SetSumLET(unsigned short sumLET){fSumLET = sumLET;}
  void SetSumTS(long long int sumTS){fSumTS = sumTS;}
  void SetSumPHA(unsigned short sumPHA){fSumPHA = sumPHA;}
  void SetSumWave(unsigned short n, unsigned short sumWave){fSumWave.at(n) = sumWave;}
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
  short GetFileNumber(){return fFileNumber;}
  unsigned short GetTrigFlag(){return fTrigFlag;}
  unsigned short GetBoardNumber(){return fBoardNumber;}
  unsigned short GetDetNumber(){return fDetNumber;}
  unsigned short GetSumLET(){return fSumLET;}
  long long int GetSumTS(){return fSumTS;}
  unsigned short GetSumPHA(){return fSumPHA;}
  vector <unsigned short> GetSumWave(){return fSumWave;}
  GrapeSeg* GetSegment(int n){return &fSegments.at(n);}
  vector<GrapeSeg>* GetSegments(){return &fSegments;}
  unsigned short GetSegMult(){return fSegMult;}
  
  bool HasSumWave() const {
    for(unsigned short i=0;i<fSumWave.size();i++){
      if(fSumWave.at(i)>0){
	return true;
      }
    }
    return false;
  }
  void Print(Option_t *option = "") const {
    cout << "detID: " << fDetNumber;
    cout << "\tBN: " << fBoardNumber;
    cout << "\tTS: " << fSumTS;
    cout << "\tPHA: "<< fSumPHA;
    if(HasSumWave())
      cout << "\tWAVE: Y";
    else 
      cout << "\tWAVE: N";
    cout << "\t#Seg: "<< fSegMult << endl;
  } 

protected:
  //! From which file was the hit read. (redundant should be equialent to the detector number fDetNumber)
  short fFileNumber; //!
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
  ClassDef(GrapeHit,1);
};

class GrapeEvent : public TObject {
public:
  GrapeEvent(){
    Clear();
  };
  void Clear(Option_t *option = ""){
    fMult = 0;
    fHits.clear();
  }
  unsigned short GetMult(){return fMult;}
  GrapeHit* GetHit(unsigned short i){return fHits.at(i);}
  void Add(GrapeHit* add){
    fHits.push_back(add);
    fMult++;
  }
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
  ClassDef(GrapeEvent,1);
};

//! compares time-stamps of the hits
class HitComparer {
public:
  bool operator() ( GrapeHit *lhs, GrapeHit *rhs) {
    return (*lhs).GetSumTS() < (*rhs).GetSumTS();
  }
};

#endif
