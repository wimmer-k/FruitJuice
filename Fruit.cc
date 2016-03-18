#include <iostream>
#include <iomanip>
#include <string>
#include <sys/time.h>
#include <signal.h>
#include "TMath.h"
#include "TFile.h"
#include "TTree.h"
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
  evt->SetWriteTree(WriteTree);
  if(!evt->Init(SetFile)){
    cerr << "Initialization failed! Check settings file!" << endl;
    return 99;
  }
  
  int buffers=0;
  long long int bytes_read = 0;
  bytes_read = evt->DetectTimestampJumps();
  buffers = bytes_read/2048;
  cout << "skipped "<< buffers << " buffers in the beginning." << endl;
  if(LastBuffer>-1)
    LastBuffer += buffers;

  pair<long long int, bool> readsuccess (0,true);
  while(readsuccess.second && !signal_received){
    if(buffers % 10000 == 0){
      double time_end = get_time();
      cout << "\r" << buffers << " buffers read... "<<bytes_read/(1024*1024)<<" MB... "<<buffers/(time_end - time_start) << " buffers/s" << flush;
    }
    if(Verbose>0)
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
    if(LastBuffer>-1 &&buffers>LastBuffer)
      break;
    
  }//loop over all files

  //cleanup
  if(Verbose>0){
    cout <<BLUE<< "------------------------------------" << endl;
    cout << "Cleaning up " <<DEFCOLOR<< endl;
  }
  while(evt->GetHitsLeft()>0){
    evt->SortHits();
    evt->ProcessHits();
    evt->CloseEvent();
  }

  cout << endl;
  cout << "------------------------------------" << endl;
  cout << "Total of " << evt->GetNBuffers() << " data buffers ("<<bytes_read/(1024*1024)<<" MB) read." << endl;
  cout << evt->GetTree()->GetEntries() << " entries written to tree ("<<evt->GetTree()->GetZipBytes()/(1024*1024)<<" MB)"<< endl;
  cout << "First time-stamp: " <<  evt->GetFirstTS() << ", last time-stamp: " << evt->GetCurrentTS() << ", data taking time: " << (evt->GetCurrentTS() - evt->GetFirstTS())*1e-8 << " seconds." << endl;
  cout << "problems in the data:" << endl;
  cout << "had to skip " << evt->GetSkippedWords() << " words (" << evt->GetSkippedWords()*2./1024 << " kB)" << endl;
  cout << "The following errors happend during unpacking" << endl;
  evt->PrintErrors();
  ofile->cd();

  if(WriteTree)
    evt->GetTree()->Write();
  ofile->Close();
  double time_end = get_time();
  cout << "Program Run time: " << time_end - time_start << " s." << endl;
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
