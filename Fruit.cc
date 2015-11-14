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
  
  int buffers = 0;
  long long int bytes_read = 0;
  //long long int previousts[3] ={0,0,0};
  cout << "------------------------------------" << endl;
  
  bytes_read += evt->ReadEachFile();
  buffers = evt->GetNBuffers();
  evt->SortHits();
  evt->ProcessHits();
  evt->CloseEvent();
  evt->ReadNewFiles();
  evt->SortHits();
  evt->ProcessHits();
  evt->CloseEvent();
  evt->ReadNewFiles();
  evt->SortHits();

  //evt->Run(LastBuffer);
  /*
  while(!feof(infile) && !signal_received){
    GrapeHit *hit = new GrapeHit();
    
    //Finish reading if you have read as many buffers as the user has requested.
    if(LastBuffer > 0 && buffers >= LastBuffer)
      break;
    size_t bsize;
    unsigned short buffer[8];

    //one word with trigger flag, board number (crystal A/B), detector number
    bsize = fread(buffer, sizeof(unsigned short), 1, infile);
    if(bsize == 0)
      break;

    int detID = (buffer[0]&0xff00)>>8;
    int trigg = (buffer[0]&0x0080)>>7;
    int board = (buffer[0]&0x0040)>>6;
    hit->SetTrigFlag(trigg);
    hit->SetDetNumber(detID);
    hit->SetBoardNumber(board);
    if(Verbose>1){
      cout << "header: " << buffer[0] <<"\t0x"<<(hex) <<buffer[0] << (dec) << endl;
      cout << "detID: " << detID;
      cout << "\ttrigg: " << trigg;
      cout << "\tboard: " << board << endl;
    }
    bytes_read += 1*sizeof(unsigned short);

    //one word with the time of the SUM signal leading edge
    bsize = fread(buffer, sizeof(unsigned short), 1, infile);
    if(bsize == 0){
      cout << "error; no data" << endl;
      break;
    }
    HEtoLE((char*)buffer,2);
    hit->SetSumLET(buffer[0]);

    if(Verbose>0){
      cout << "LET: " << buffer[0] <<"\t0x"<<(hex) <<buffer[0] << (dec) << endl;
    }
    bytes_read += 1*sizeof(unsigned short);

    //three words containing the time-stamp (SUM Abs count)
    bsize = fread(buffer, sizeof(unsigned short), 3, infile);
    if(Verbose>3){
      cout << "TS0: " << buffer[0] <<"\t0x"<<(hex) <<buffer[0] << (dec) << endl;
      cout << "TS1: " << buffer[1] <<"\t0x"<<(hex) <<buffer[1] << (dec) << endl;
      cout << "TS2: " << buffer[2] <<"\t0x"<<(hex) <<buffer[2] << (dec) << endl;
    }
    HEtoLE((char*)buffer,6);
    if(Verbose>2){
      cout << "TS0: " << buffer[0] <<"\t0x"<<(hex) <<buffer[0] << (dec) << endl;
      cout << "TS1: " << buffer[1] <<"\t0x"<<(hex) <<buffer[1] << (dec) << endl;
      cout << "TS2: " << buffer[2] <<"\t0x"<<(hex) <<buffer[2] << (dec) << endl;
    }
    long long int ts = (long long int)buffer[0]<<32;
    ts += (long long int)buffer[1]<<16;
    ts += (long long int)buffer[2];
    hit->SetSumTS(ts);
    if(Verbose>1){
      cout << "ts " << ts <<"\t0x"<<(hex) << ts << (dec) << "\t" << hit->GetSumTS() << "\t\t";
    }
    bytes_read += 3*sizeof(unsigned short);
 
    //one word containing the pulse height information
    bsize = fread(buffer, sizeof(unsigned short), 1, infile);
    HEtoLE((char*)buffer,2);
    int pha = buffer[0];
    hit->SetSumPHA(pha);
    if(Verbose>0){
      cout << "PHA " << pha <<"\t0x"<<(hex) << pha << (dec) << endl;
    }
    bytes_read += 1*sizeof(unsigned short);


    //one word for each segment Abs count
    unsigned short int SAbscount[9];
    bsize = fread(SAbscount, sizeof(unsigned short), 9, infile);
    HEtoLE((char*)SAbscount,18);
    bytes_read += 9*sizeof(unsigned short);

    //one word for each segment pulse height
    unsigned short int Spha[9];
    bsize = fread(Spha, sizeof(unsigned short), 9, infile);
    HEtoLE((char*)Spha,18);
    if(Verbose>0){
      for(int i=0;i<9;i++)
	if(Spha[i]>0)
	  cout <<i<< "\tAbs " << SAbscount[i] <<"\t0x"<<(hex) <<SAbscount[i]  << (dec) << "\tPHA " << Spha[i] <<"\t0x"<<(hex) <<Spha[i]  << (dec) << endl;
    }
    bytes_read += 9*sizeof(unsigned short);

    vector<GrapeSeg> segments;
    segments.clear();
    segments.resize(NUM_SEGMENTS);

    //48 words for each segments containing the wave
    unsigned short int Swave[9][48];
    for(int i=0;i<9;i++){
      bsize = fread(Swave[i], sizeof(unsigned short), 48, infile);
      HEtoLE((char*)Swave[i],96);
      bytes_read += 48*sizeof(unsigned short);
      segments[i].SetSegNumber(i);
      segments[i].SetSegTS(SAbscount[i]);
      segments[i].SetSegPHA(Spha[i]);
      segments[i].SetSegWave(vector<unsigned short>(Swave[i], Swave[i] + sizeof Swave[i] / sizeof Swave[i][0]));
      
      hit->AddSegment(segments[i]);
    }
    
    //48 words for SUM containing the wave
    unsigned short int wave[48];
    bsize = fread(wave, sizeof(unsigned short), 48, infile);
    HEtoLE((char*)wave,96);
    bytes_read += 48*sizeof(unsigned short);
    bool foundwave = false;
    for(int i=0;i<48;i++){
      if(Verbose>2)
	cout << i << "\t" << wave[i] << endl;
      if(wave[i]>0){
	foundwave = true;
      }
      hit->SetSumWave(i,wave[i]);
    }

    //eight words dummy
    bsize = fread(buffer, sizeof(unsigned short), 8, infile);
    HEtoLE((char*)buffer,16);
    if(Verbose>0){
      for(int i=0;i<8;i++)
	cout <<i<<"\t"<< buffer[i] <<"\t0x"<<(hex) <<buffer[i] << (dec) << endl;
    }
    bytes_read += 8*sizeof(unsigned short);
    if(Verbose>0)
      cout << "bytes_read: " << bytes_read << endl;

    //some diagnostics histograms, to be removed
    // if(foundwave){
    //   hpha[board]->Fill(pha);
    //   for(int i=0;i<9;i++){
    // 	if(Spha[i]>0)
    // 	  hphas[board]->Fill(i,Spha[i]);
    //   }
    // }
    // else{
    //   hphan[board]->Fill(pha);
    //   for(int i=0;i<9;i++){
    // 	if(Spha[i]>0)
    // 	  hphasn[board]->Fill(i,Spha[i]);
    //   }
    // }
    // if(foundwave){
    //   if(board==0&&previousts[1]>0){
    // 	htdiff[board]->Fill(ts-previousts[1]);
    //   }
    //   if(board==1&&previousts[0]>0){
    // 	htdiff[board]->Fill(ts-previousts[0]);
    //   }
    //   if(previousts[2]>0){
    // 	htdiff[2]->Fill(ts-previousts[2]);
    //   }
    // }
    cout << "detID: " << detID;
    cout << "\tBN: " << board;
    cout << "\tTS: " << ts;
    cout << "\tPHA: "<< pha << endl;
    if(foundwave){
      cout << "found a wave" << endl;
    }    
    buffers++;
    // if(foundwave){
    //   previousts[board] = ts;
    //   previousts[2] = ts;
    //   gr = hit;
    //   if(WriteTree)
    // 	tr->Fill();
    // }
    if(buffers % 1000 == 0){
      double time_end = get_time();
      cout << "\r" << buffers << " buffers read... "<<bytes_read/(1024*1024)<<" MB... "<<buffers/(time_end - time_start) << " buffers/s" << flush;
    }
    if(Verbose>0)
      cout << "------------------------------------" << endl;
  }
  */
  cout << endl;
  cout << "------------------------------------" << endl;
  cout << "Total of " << buffers << " data buffers ("<<bytes_read/(1024*1024)<<" MB) read." << endl;
  cout << evt->GetTree()->GetEntries() << " entries written to tree ("<<evt->GetTree()->GetZipBytes()/(1024*1024)<<" MB)"<< endl;
  ofile->cd();
  hlist->Write();
  if(WriteTree)
    evt->GetTree()->Write();
  ofile->Close();
  double time_end = get_time();
  cout << "Program Run time " << time_end - time_start << " s." << endl;
  cout << "Unpacked " << buffers/(time_end - time_start) << " buffers/s." << endl;
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
