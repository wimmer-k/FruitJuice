#ifndef __GRAPE_HH
#define __GRAPE_HH
#include <iostream>
#include <vector>
#include <cstdlib>
#include <math.h>

#include "TObject.h"
#include "Grapedefs.h"

using namespace std;
class GrapeSeg : public TObject{
public:
  GrapeSeg(){
    Clear();
  };
  void Clear(){
    fSegNumber = sqrt(-1);
    fSegTS = sqrt(-1);
    fSegPHA = sqrt(-1);
    fSegWave.clear();
    fSegWave.resize(WAVE_LENGTH);
  }
  void SetSegNumber(unsigned short segnumber){fSegNumber = segnumber;}
  void SetSegTS(unsigned short segTS){fSegTS = segTS;}
  void SetSegPHA(unsigned short segPHA){fSegPHA = segPHA;}
  void SetSegWave(unsigned short n, unsigned short segWave){fSegWave[n] = segWave;}
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
  vector <unsigned short int> fSegWave;
  ClassDef(GrapeSeg,1);
};


class GrapeHit : public TObject{
public:
  GrapeHit(){
    Clear();
  };
  void Clear(){
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
  void SetTrigFlag(unsigned short trigflag){fTrigFlag = trigflag;}
  void SetBoardNumber(unsigned short boardnumber){fBoardNumber = boardnumber;}
  void SetDetNumber(unsigned short detnumber){fDetNumber = detnumber;}
  void SetSumLET(unsigned short sumLET){fSumLET = sumLET;}
  void SetSumTS(long long int sumTS){fSumTS = sumTS;}
  void SetSumPHA(unsigned short sumPHA){fSumPHA = sumPHA;}
  void SetSumWave(unsigned short n, unsigned short sumWave){fSumWave[n] = sumWave;}
  void SetSumWave(vector<unsigned short> sumWave){fSumWave = sumWave;}
  
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

  unsigned short GetTrigFlag(){return fTrigFlag;}
  unsigned short GetBoardNumber(){return fBoardNumber;}
  unsigned short GetDetNumber(){return fDetNumber;}
  unsigned short GetSumLET(){return fSumLET;}
  long long int GetSumTS(){return fSumTS;}
  unsigned short GetSumPHA(){return fSumPHA;}
  vector <unsigned short> GetWave(){return fSumWave;}
  GrapeSeg* GetSegment(int n){return &fSegments[n];}
  vector<GrapeSeg>* GetSegments(){return &fSegments;}
  unsigned short GetSegMult(){return fSegMult;}

protected:
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
  vector <unsigned short> fSumWave;
  //! a vector containing the segment information
  vector <GrapeSeg> fSegments;
  //! multiplicity of segments with net charge
  unsigned short fSegMult;
  ClassDef(GrapeHit,1);
};

#endif
