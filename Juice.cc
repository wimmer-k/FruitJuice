#include <iostream>
#include <iomanip>
#include <string>
#include <sys/time.h>
#include <signal.h>
#include "TMath.h"
#include "TFile.h"
#include "TChain.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TStopwatch.h"
#include "CommandLineInterface.hh"
#include "Grape.hh"
using namespace TMath;
using namespace std;

bool signal_received = false;
void signalhandler(int sig);
double get_time();
int main(int argc, char* argv[]){
  double time_start = get_time();  
  TStopwatch timer;
  timer.Start();
  signal(SIGINT,signalhandler);
  int LastEvent =-1;
  int Verbose =0;
  vector<char*> InputFiles;
  char *OutFile = NULL;
  //Read in the command line arguments
  CommandLineInterface* interface = new CommandLineInterface();
  interface->Add("-i", "inputfiles", &InputFiles);
  interface->Add("-o", "output file", &OutFile);    
  interface->Add("-le", "last event to be read", &LastEvent);  
  interface->Add("-v", "verbose level", &Verbose);  
  interface->CheckFlags(argc, argv);
  //Complain about missing mandatory arguments
  if(InputFiles.size() == 0){
    cout << "No input file given " << endl;
    return 1;
  }
  if(OutFile == NULL){
    cout << "No output ROOT file given " << endl;
    OutFile = (char*)"test.root";
  }
  cout<<"input file(s):"<<endl;
  for(unsigned int i=0; i<InputFiles.size(); i++){
    cout<<InputFiles[i]<<endl;
  }
  cout<<"output file: "<<OutFile<< endl;
  
  TList *hlist = new TList();
  //histograms
  TH1F* mult = new TH1F("mult","mult",MAX_NUM_DET*2,0,MAX_NUM_DET*2);hlist->Add(mult);
  TH1F* detnumber = new TH1F("detnumber","detnumber",MAX_NUM_DET,0,MAX_NUM_DET);hlist->Add(detnumber);
  TH1F* board = new TH1F("board","board",2,0,2);hlist->Add(board);
  TH2F* detnumber_vs_board = new TH2F("detnumber_vs_board","detnumber_vs_board",2,0,2,MAX_NUM_DET,0,MAX_NUM_DET);hlist->Add(detnumber_vs_board);
  TH1F* detID = new TH1F("detID","detID",MAX_NUM_DET*2,0,MAX_NUM_DET*2);hlist->Add(detID);
  TH2F* PHA_vs_detID = new TH2F("PHA_vs_detID","PHA_vs_detID",MAX_NUM_DET*2,0,MAX_NUM_DET*2,2000,0,4000);hlist->Add(PHA_vs_detID);
  TH2F* LET_vs_detID = new TH2F("LET_vs_detID","LET_vs_detID",MAX_NUM_DET*2,0,MAX_NUM_DET*2,2000,0,16000);hlist->Add(LET_vs_detID);
  TH1F* SumPHA[MAX_NUM_DET*2];
  for(unsigned short j=0;j<MAX_NUM_DET*2;j++){
    SumPHA[j] = new TH1F(Form("SumPHA_%d",j),Form("SumPHA_%d",j),4000,0,4000);hlist->Add(SumPHA[j]);
  }
  TH1F* LET_TDiff = new TH1F("LET_TDiff","LET_TDiff",2000,-16000,16000);hlist->Add(LET_TDiff);
  TH1F* TS_TDiff = new TH1F("TS_TDiff","TS_TDiff",1200,-1000,200);hlist->Add(TS_TDiff);
  TH2F* A_vs_A = new TH2F("A_vs_A","A_vs_A",1000,0,4000,1000,0,4000);hlist->Add(A_vs_A);
  TH2F* B_vs_B = new TH2F("B_vs_B","B_vs_B",1000,0,4000,1000,0,4000);hlist->Add(B_vs_B);
  TH2F* A_vs_B = new TH2F("A_vs_B","A_vs_B",1000,0,4000,1000,0,4000);hlist->Add(A_vs_B);

  TChain* tr;
  tr = new TChain("gtr");
  for(unsigned int i=0; i<InputFiles.size(); i++){
    tr->Add(InputFiles[i]);
  }
  if(tr == NULL){
    cout << "could not find tree ctr in file " << endl;
    for(unsigned int i=0; i<InputFiles.size(); i++){
      cout<<InputFiles[i]<<endl;
    }
    return 3;
  }
  GrapeEvent* gr = new GrapeEvent;
  tr->SetBranchAddress("grape",&gr);
  Double_t nentries = tr->GetEntries();
  if(LastEvent>0)
    nentries = LastEvent;

  Int_t nbytes = 0;
  Int_t status;
  for(int i=0; i<nentries;i++){
    if(signal_received){
      break;
    }
    gr->Clear();
    if(Verbose>2)
      cout << "getting entry " << i << endl;
    status = tr->GetEvent(i);
    if(Verbose>2)
      cout << "status " << status << endl;
    if(status == -1){
      cerr<<"Error occured, couldn't read entry "<<i<<" from tree "<<tr->GetName()<<" in file "<<tr->GetFile()->GetName()<<endl;
      return 5;
    }
    else if(status == 0){
      cerr<<"Error occured, entry "<<i<<" in tree "<<tr->GetName()<<" in file "<<tr->GetFile()->GetName()<<" doesn't exist"<<endl;
      return 6;
    }
    nbytes += status;
    
    //start analysis
    mult->Fill(gr->GetMult());
    for(unsigned short j=0;j<gr->GetMult();j++){
      GrapeHit *hit = gr->GetHit(j);
      detnumber_vs_board->Fill(hit->GetBoardNumber(),hit->GetDetNumber());
      detnumber->Fill(hit->GetDetNumber());
      board->Fill(hit->GetBoardNumber());
      detID->Fill(hit->GetDetID());
      PHA_vs_detID->Fill(hit->GetDetID(),hit->GetSumPHA());
      LET_vs_detID->Fill(hit->GetDetID(),hit->GetSumLET());
      SumPHA[(int)hit->GetDetID()]->Fill(hit->GetSumPHA());
    }
    if(gr->GetMult()>1){
      GrapeHit *hit[2];
      hit[0] = gr->GetHit(0);
      for(unsigned short j=1;j<gr->GetMult();j++){
    	hit[1] = gr->GetHit(j);
    	LET_TDiff->Fill(hit[0]->GetSumLET() - hit[1]->GetSumLET());
    	TS_TDiff->Fill(hit[0]->GetSumTS() - hit[1]->GetSumTS());
      }
      for(unsigned short j=0;j<gr->GetMult();j++){
	hit[0] = gr->GetHit(j);
	for(unsigned short k=0;k<gr->GetMult();k++){
	  if(k==j)
	    continue;
	  hit[1] = gr->GetHit(k);
	  if(hit[0]->GetBoardNumber()==0){//A
	    if(hit[1]->GetBoardNumber()==0)//A
	      A_vs_A->Fill(hit[0]->GetSumPHA(),hit[1]->GetSumPHA());
	    if(hit[1]->GetBoardNumber()==1)//B
	      A_vs_B->Fill(hit[1]->GetSumPHA(),hit[0]->GetSumPHA());	    
	  }
	  if(hit[0]->GetBoardNumber()==1){//B
	    if(hit[1]->GetBoardNumber()==0)//A
	      A_vs_B->Fill(hit[0]->GetSumPHA(),hit[1]->GetSumPHA());
	    if(hit[1]->GetBoardNumber()==1)//B
	      B_vs_B->Fill(hit[0]->GetSumPHA(),hit[1]->GetSumPHA());	    
	  }
	}
      }
    }

    if(i%100000 == 0){
      double time_end = get_time();
      cout << setw(5) << setiosflags(ios::fixed) << setprecision(1) << (100.*i)/nentries <<
	" % done\t" << (Float_t)i/(time_end - time_start) << " events/s " << 
	(nentries-i)*(time_end - time_start)/(Float_t)i << "s to go \r" << flush;
    }

  }

  cout << "creating outputfile " << endl;
  TFile* ofile = new TFile(OutFile,"recreate");
  hlist->Write();
  ofile->Close();
  double time_end = get_time();
  cout << "Program Run time: " << time_end - time_start << " s." << endl;
  //cout << "Unpacked " << evt->GetNBuffers()/(time_end - time_start) << " buffers/s." << endl;
  timer.Stop();
  cout << "\n CPU time: " << timer.CpuTime() << "\tReal time: " << timer.RealTime() << endl;
 

  return 0;
}

void signalhandler(int sig){
  if (sig == SIGINT){
    signal_received = true;
  }
}

double get_time(){  
    struct timeval t;  
    gettimeofday(&t, NULL);  
    double d = t.tv_sec + (double) t.tv_usec/1000000;  
    return d;  
}  