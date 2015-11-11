#include <iostream>
#include <iomanip>
#include <string>
#include <sys/time.h>
#include <signal.h>
#include "TMath.h"
#include "TFile.h"
#include "TH1F.h"
#include "TStopwatch.h"
#include "CommandLineInterface.hh"
using namespace TMath;
using namespace std;

bool signal_received = false;
void signalhandler(int sig);
double get_time();
void swapbytes(char* a, char *b);
void  HEtoLE(char* cBuf, int bytes);
int main(int argc, char* argv[]){
  double time_start = get_time();  
  TStopwatch timer;
  timer.Start();
  signal(SIGINT,signalhandler);
  int LastBuffer =-1;
  int Verbose =0;
  char *InputFile = NULL;
  char *RootFile = NULL;
  //Read in the command line arguments
  CommandLineInterface* interface = new CommandLineInterface();
  interface->Add("-lb", "last buffer to be read", &LastBuffer);  
  interface->Add("-i", "input file", &InputFile);  
  interface->Add("-o", "output file", &RootFile);    
  interface->Add("-v", "verbose level", &Verbose);  
  interface->CheckFlags(argc, argv);

  //Complain about missing mandatory arguments
  if(InputFile == NULL){
    cout << "No input file given " << endl;
    return 1;
  }
  if(RootFile == NULL){
    cout << "No output ROOT file given " << endl;
    RootFile = (char*)"test.root";
    //return 2;
  }
  //open the input and the output files
  cout<<"input file: "<<InputFile<< endl;
  FILE *infile = fopen(InputFile,"r");
  
  TFile *ofile = new TFile(RootFile,"RECREATE");

  //test histograms
  TList* hlist = new TList;
  TH1F* hpha[2];
  for(int i=0;i<2;i++){
    hpha[i] = new TH1F(Form("hpha_%d",i),Form("hpha_%d",i),8000,0,8000);hlist->Add(hpha[i]);
  }

  int buffers = 0;
  long long int bytes_read = 0;
  cout << "------------------------------------" << endl;
  while(!feof(infile) && !signal_received){

    //Finish reading if you have read as many buffers as the user has requested.
    if(LastBuffer > 0 && buffers >= LastBuffer)
      break;
    size_t bsize;
    unsigned short buffer[8];

    //one word with trigger flag, board number (crystal A/B), detector number
    bsize = fread(buffer, sizeof(unsigned short), 1, infile);
    int detID = (buffer[0]&0xff00)>>8;
    int trigg = (buffer[0]&0x0080)>>7;
    int board = (buffer[0]&0x0040)>>6;
    if(Verbose>0){
      cout << "header: " << buffer[0] <<"\t0x"<<(hex) <<buffer[0] << (dec) << endl;
      cout << "detID: " << detID;
      cout << "\ttrigg: " << trigg;
      cout << "\tboard: " << board << endl;
    }
    bytes_read += 1*sizeof(unsigned short);

    //one word with the time of the SUM signal leading edge
    bsize = fread(buffer, sizeof(unsigned short), 1, infile);
    HEtoLE((char*)buffer,1);
    if(Verbose>0){
      cout << "LET: " << buffer[0] <<"\t0x"<<(hex) <<buffer[0] << (dec) << endl;
    }
    bytes_read += 1*sizeof(unsigned short);

    //three words containing the timestamp (SUM Abs count)
    bsize = fread(buffer, sizeof(unsigned short), 3, infile);
    HEtoLE((char*)buffer,3);
    if(Verbose>2){
      cout << "TS0: " << buffer[0] <<"\t0x"<<(hex) <<buffer[0] << (dec) << endl;
      cout << "TS1: " << buffer[1] <<"\t0x"<<(hex) <<buffer[1] << (dec) << endl;
      cout << "TS2: " << buffer[2] <<"\t0x"<<(hex) <<buffer[2] << (dec) << endl;
    }
    long long int ts = (long long int)buffer[0]<<32;
    ts += (long long int)buffer[1]<<16;
    ts += (long long int)buffer[2];
    if(Verbose>0){
      cout << "ts " << ts <<"\t0x"<<(hex) << ts << (dec) << endl;
    }
    bytes_read += 3*sizeof(unsigned short);
 
    //one word containing the pulse height information
    bsize = fread(buffer, sizeof(unsigned short), 1, infile);
    HEtoLE((char*)buffer,1);
    int pha = buffer[0];
    if(Verbose>0){
      cout << "PHA " << pha <<"\t0x"<<(hex) << pha << (dec) << endl;
    }
    bytes_read += 1*sizeof(unsigned short);
    hpha[board]->Fill(pha);

    //one word for each segment Abs count
    unsigned short int SAbscount[9];
    bsize = fread(SAbscount, sizeof(unsigned short), 9, infile);
    HEtoLE((char*)SAbscount,9);
    bytes_read += 9*sizeof(unsigned short);

    //one word for each segment pulse height
    unsigned short int Spha[9];
    bsize = fread(Spha, sizeof(unsigned short), 9, infile);
    HEtoLE((char*)Spha,9);
    if(Verbose>0){
      for(int i=0;i<9;i++)
	cout <<i<< "\tAbs " << SAbscount[i] <<"\t0x"<<(hex) <<SAbscount[i]  << (dec) << "\tPHA " << Spha[i] <<"\t0x"<<(hex) <<Spha[i]  << (dec) << endl;
    }
    bytes_read += 9*sizeof(unsigned short);

    //48 words for each segments containing the wave
    unsigned short int Swave[9][48];
    for(int i=0;i<9;i++){
      bsize = fread(Swave[i], sizeof(unsigned short), 48, infile);
      HEtoLE((char*)Swave[i],9);
      bytes_read += 48*sizeof(unsigned short);
    }
    
    //48 words for SUM containing the wave
    unsigned short int wave[48];
    bsize = fread(wave, sizeof(unsigned short), 48, infile);
    HEtoLE((char*)wave,9);
    bytes_read += 48*sizeof(unsigned short);
    
    //eight words dummy
    bsize = fread(buffer, sizeof(unsigned short), 8, infile);
    HEtoLE((char*)buffer,8);
    if(Verbose>0){
      for(int i=0;i<8;i++)
	cout <<i<<"\t"<< buffer[i] <<"\t0x"<<(hex) <<buffer[i] << (dec) << endl;
    }
    bytes_read += 8*sizeof(unsigned short);

    if(Verbose>0)
      cout << "bytes_read: " << bytes_read << endl;

    buffers++;
    if(buffers % 1000 == 0){
      double time_end = get_time();
      cout << "\r" << buffers << " buffers read... "<<bytes_read/(1024*1024)<<" MB... "<<buffers/(time_end - time_start) << " buffers/s" << flush;
    }
    if(Verbose>0)
      cout << "------------------------------------" << endl;
  }
  cout << "------------------------------------" << endl;
  cout << "Total of " << buffers << " data buffers ("<<bytes_read/(1024*1024)<<" MB) read." << endl;
  ofile->cd();
  hlist->Write();
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
void swapbytes(char* a, char *b){
  char tmp=*a;
  *a=*b;
  *b=tmp;
}

// Mode 3 data is high endian format
void  HEtoLE(char* cBuf, int bytes){
  for(int i=0; i<bytes; i+=2)
    swapbytes((cBuf+i), (cBuf+i+1));
}
