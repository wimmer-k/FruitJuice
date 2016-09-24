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
  if(fVerboseLevel>2)
    cout <<__PRETTY_FUNCTION__<< endl;
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
void ProcessEvents::AddBack(GrapeEvent* event){
  if(fVerboseLevel>2)
    cout <<__PRETTY_FUNCTION__<< endl;
  vector<GrapeHit*> unusedhits;
  for(unsigned short j=0;j<event->GetMult();j++){
    GrapeHit *hit = event->GetHit(j);
    int det = hit->GetDetNumber();
    int board = hit->GetBoardNumber();
    long long int sumTS = hit->GetSumTS();
    float sumEn = hit->GetSumEn();
    unusedhits.push_back(new GrapeHit(board,det, sumTS, sumEn));
  }
  while(unusedhits.size() > 0){
    //first one is automatically good
    GrapeHit* thishit = unusedhits.back();
    unusedhits.pop_back();
    
    bool added = false;
    do{
      added = false;
      for(unsigned short j=0;j<unusedhits.size();j++){
	if(AddBack(thishit,unusedhits.at(j))){
	  if(thishit->GetSumEn()>unusedhits.at(j)->GetSumEn()){
	    thishit->SetDetNumber(thishit->GetDetNumber());
	    thishit->SetBoardNumber(thishit->GetBoardNumber());
	    thishit->SetSumTS(thishit->GetSumTS());
	  }
	  else{
	    thishit->SetDetNumber(unusedhits.at(j)->GetDetNumber());
	    thishit->SetBoardNumber(unusedhits.at(j)->GetBoardNumber());
	    thishit->SetSumTS(unusedhits.at(j)->GetSumTS());	  
	  }
	  thishit->SetSumEn(thishit->GetSumEn()+unusedhits.at(j)->GetSumEn());
	  added = true;
	  unusedhits.erase(unusedhits.begin() + j);
	  break;
	}
      }
    }while(added);
    event->AddAB(thishit);
  }//while
  if(fVerboseLevel>2)
    event->Print();
}
bool ProcessEvents::AddBack(GrapeHit* hit0, GrapeHit* hit1){
  if(hit0->GetDetNumber()==hit1->GetDetNumber()){
    if(hit0->GetBoardNumber()!=hit1->GetBoardNumber())
      return true;
    return false;
  }
  return false;
}
