#include <iostream>
#include <iomanip>
#include <string>
#include <sys/time.h>
#include <signal.h>
#include "TMath.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TStopwatch.h"
#include "CommandLineInterface.hh"
#include "BuildEvents.hh"

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
  int LastBuffer =-1;
  int Verbose =0;
  char *SetFile = NULL;
  char *RootFile = NULL;
  int WriteTree = 1;

  //Read in the command line arguments
  CommandLineInterface* interface = new CommandLineInterface();
  interface->Add("-lb", "last buffer to be read", &LastBuffer);  
  interface->Add("-s", "settings file", &SetFile);  
  interface->Add("-o", "output file", &RootFile);    
  interface->Add("-v", "verbose level", &Verbose);  
  interface->Add("-wt", "write the tree", &WriteTree);  
  interface->CheckFlags(argc, argv);

  //Complain about missing mandatory arguments
  if(SetFile == NULL){
    cout << "No settings file given " << endl;
    return 1;
  }
  if(RootFile == NULL){
    cout << "No output ROOT file given " << endl;
    RootFile = (char*)"test.root";
    //return 2;
  }
  //open the output files
  TFile *ofile = new TFile(RootFile,"RECREATE");
  
  BuildEvents *evt = new BuildEvents();
  evt->SetVL(Verbose);
  if(!evt->Init(SetFile)){
    cerr << "Initialization failed! Check settings file!" << endl;
    return 99;
  }

  //test histograms
  TList* hlist = new TList;
  TH1F* hpha[2];
  TH1F* hphan[2];
  TH2F* hphas[2];
  TH2F* hphasn[2];
  for(int i=0;i<2;i++){
    hpha[i] = new TH1F(Form("hpha_%d",i),Form("hpha_%d",i),8000,0,8000);hlist->Add(hpha[i]);
    hphan[i] = new TH1F(Form("hphan_%d",i),Form("hphan_%d",i),8000,0,8000);hlist->Add(hphan[i]);
    hphas[i] = new TH2F(Form("hphas_%d",i),Form("hphas_%d",i),10,0,10,4000,0,4000);hlist->Add(hphas[i]);
    hphasn[i] = new TH2F(Form("hphasn_%d",i),Form("hphasn_%d",i),10,0,10,4000,0,4000);hlist->Add(hphasn[i]);
  }
  TH1F* htdiff[3];
  for(int i=0;i<3;i++){
    htdiff[i] = new TH1F(Form("htdiff_%d",i),Form("htdiff_%d",i),9000,-100000,800000);hlist->Add(htdiff[i]);
  }
  
  int buffers=0;
  long long int bytes_read = 0;
  
  pair<long long int, bool> readsuccess (0,true);
  while(readsuccess.second && !signal_received){
    if(buffers % 1000 == 0){
      double time_end = get_time();
      cout << "\r" << buffers << " buffers read... "<<bytes_read/(1024*1024)<<" MB... "<<buffers/(time_end - time_start) << " buffers/s" << flush;
    }
    if(Verbose>1)
      cout << "------------------------------------" << endl;

    readsuccess = evt->ReadNewFiles();
    bytes_read += readsuccess.first;
    buffers = bytes_read/2048;
    if(!readsuccess.second){
      break;
    }
    evt->SortHits();
    evt->ProcessHits();
    evt->CloseEvent();
    if(LastBuffer>-1 &&buffers>LastBuffer-1)
      break;
  }

  //cleanup
  if(Verbose>1)
    cout << "Cleaning up " << endl;
  while(evt->GetHitsLeft()>0){
    evt->SortHits();
    evt->ProcessHits();
    evt->CloseEvent();
  }

  cout << endl;
  cout << "------------------------------------" << endl;
  cout << "Total of " << evt->GetNBuffers() << " data buffers ("<<bytes_read/(1024*1024)<<" MB) read." << endl;
  cout << evt->GetTree()->GetEntries() << " entries written to tree ("<<evt->GetTree()->GetZipBytes()/(1024*1024)<<" MB)"<< endl;
  ofile->cd();
  hlist->Write();
  if(WriteTree)
    evt->GetTree()->Write();
  ofile->Close();
  double time_end = get_time();
  cout << "Program Run time " << time_end - time_start << " s." << endl;
  cout << "Unpacked " << evt->GetNBuffers()/(time_end - time_start) << " buffers/s." << endl;
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
