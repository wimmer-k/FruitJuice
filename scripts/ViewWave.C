#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TGraph.h"
#include "TSystem.h"
#include "TAxis.h"
#include "inc/Grapedefs.h"
#include "inc/Grape.hh"
char * ffilename = (char*)"/mnt/raid/OEDO/GRAPE/test.root";
void SetFile(char* filename){
  ffilename = filename;
}
void ViewWave(int n){
  TCanvas *c = new TCanvas("c","c",0,0,1200,600);
  TPad *pad[2];
  pad[0] = new TPad("p0","p0",0,0,0.5,1);
  pad[1] = new TPad("p1","p1",0.5,0,1,1);
  pad[0]->Draw();
  pad[1]->Draw();
  TFile *f = new TFile(ffilename);
  TTree* tr = (TTree*)f->Get("gtr");
  GrapeEvent* gre = new GrapeEvent;
  tr->SetBranchAddress("grape",&gre);
  Int_t status = tr->GetEvent(n);
  int data[WAVE_LENGTH];
  int x[WAVE_LENGTH];
  vector<TGraph*> g;
  g.resize(10);
  if(gre->GetMult()==0){
    cout << "mult = 0 event, strange!" << endl;
    return;
  }
  if(gre->GetMult()==2){
    cout << "mult = 2 event, taking first hit only!" << endl;
  }
  GrapeHit* gr = gre->GetHit(0);
  gr->Print();
  for(int i=0;i<WAVE_LENGTH;i++){
    x[i] = i;
    data[i] = (int)gr->GetSumWave()[i];
    //cout << x[i] << "\t" << data[i] << endl;
  }
  pad[0]->cd();
  g[0] = new TGraph(WAVE_LENGTH,x,data);
  g[0]->SetTitle(Form("SUM E = %d",gr->GetSumPHA()));
  g[0]->Draw("AL");
  pad[1]->Divide(3,3);
  int min = +10000;
  int max = -10000;
  for(int j=0;j<NUM_SEGMENTS;j++){
    for(int i=0;i<WAVE_LENGTH;i++){
      x[i] = i;
      data[i] = (int)gr->GetSegment(j)->GetWave()[i];
      //cout << x[i] << "\t" << data[i] << endl;
      if(data[i]<min)
	min = data[i];
      if(data[i]>max)
	max = data[i];
    }
    g[1+j] = new TGraph(WAVE_LENGTH,x,data);
    g[1+j]->SetTitle(Form("SEG %d E = %d",gr->GetSegment(j)->GetSegNumber(),gr->GetSegment(j)->GetSegPHA()));
  }
  for(int j=0;j<NUM_SEGMENTS;j++){
    g[1+j]->GetYaxis()->SetRangeUser(min*0.9,max*1.1);
    pad[1]->cd(1+j);
    g[1+j]->Draw("AL");    
  }

}
