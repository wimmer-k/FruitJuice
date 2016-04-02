#include "ProcessEvents.hh"
using namespace std;
using namespace TMath;

/*!
  Initialize the ProcessEvents. The calibration parameters are read from a settings file
  \param settings the settings file
*/
void ProcessEvents::Init(char *settings){
  if(fVerboseLevel>2)
    cout <<__PRETTY_FUNCTION__<< endl;

  fRand = new TRandom();
  cout << "reading settings file: " << settings << endl;
  TEnv* set = new TEnv(settings);
  string calfile = set->GetValue("Calibration.File",(char*)"nofile");
  if(fVerboseLevel>1)
    cout << "calibration file " << calfile << endl;
  TEnv* cal = new TEnv(calfile.c_str());
  for(unsigned short d=0;d<MAX_NUM_DET;d++){
    for(unsigned short s=0;s<NUM_SEGMENTS;s++){
      for(unsigned short a=0;a<2;a++){
	fgain[d][a][s] = cal->GetValue(Form("Gain.Det.%d.Mod.%d.Seg.%d",d,a,s),-1.0);
	foffs[d][a][s] = cal->GetValue(Form("Offset.Det.%d.Mod.%d.Seg.%d",d,a,s),0.0);
	if(fVerboseLevel>1){
	  if(a==0)
	    cout << "det " << d << "A seg " << s << ": gain = " << fgain[d][a][s]<< ", offset = " << foffs[d][a][s] << endl;
	  else
	    cout << "det " << d << "B seg " << s << ": gain = " << fgain[d][a][s]<< ", offset = " << foffs[d][a][s] << endl;
	}
      }
    }
  }
  return;
}
void ProcessEvents::Calibrate(GrapeEvent* event){
  //cout << "before" << endl;
  //event->Print();
  for(unsigned short j=0;j<event->GetMult();j++){
    GrapeHit *hit = event->GetHit(j);
    double sum =0;
    int det = hit->GetDetNumber();
    int board = hit->GetBoardNumber();
    for(unsigned short k=0;k<hit->GetSegMult();k++){
      GrapeSeg* seg = hit->GetSegment(k);
      
      double en = 0;
      if(seg->GetSegPHA()>0){
	//assuming PHA is calculated from the waveform, not like an ADC value
	en = seg->GetSegPHA()-0.5+fRand->Uniform(0,1);
	en = fgain[det][board][seg->GetSegNumber()]*en + foffs[det][board][seg->GetSegNumber()];
	if(en>0){
	  sum+=en;
	  seg->SetSegEn(en);
	}
      }
      //cout << k << "\tsegnumber = " << seg->GetSegNumber()<< "\tpha = " << seg->GetSegPHA() << "\ten = " << seg->GetSegEn() << endl;
    }
    if(sum>0)
      hit->SetSumEn(sum);
  }
  //cout << "after" << endl;
  //event->Print();

}
