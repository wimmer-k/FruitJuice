#include <iostream>
#include <iomanip>
#include "TFile.h"
#include "TH2F.h"
#include "TH1F.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TMath.h"
#include "TEnv.h"
#include "TSpectrum.h"
#include "inc/Grapedefs.h"
using namespace std;
using namespace TMath;
double lbinw;
Double_t gausbg(Double_t *x, Double_t *par);
void Calibrate(TH2F* h, int det, int board);
void Calibrate(char* file = (char*)"/mnt/raid/OEDO/GRAPE/htest.root"){
  TFile *f = new TFile(file);
  for(unsigned short m=0;m<MAX_NUM_DET;m++){
    TH2F* hA = NULL;
    TH2F* hB = NULL;
    hA = (TH2F*)f->Get(Form("SegPHA_%d",m*2));
    hB = (TH2F*)f->Get(Form("SegPHA_%d",m*2+1));
    if(hA!=NULL){
      cout << "calibrating " << m << " A"<< endl;
      Calibrate(hA,m,0);
      Calibrate(hB,m,1);
    }
    if(hB!=NULL){
      cout << "calibrating " << m << " B"<< endl;
    }
  }
}
void test(){
  TFile *f = new TFile("/mnt/raid/OEDO/GRAPE/htest.root");
  TH2F* hA = (TH2F*)f->Get("SegPHA_20");
  Calibrate(hA,10,0);
}
void Calibrate(TH2F* h, int det, int board){
  TEnv *calfile = new TEnv("/mnt/raid/OEDO/GRAPE/cal.dat");
  TCanvas * c = new TCanvas("c","c");
  c->Divide(3,3);
  TH1F* hs[NUM_SEGMENTS];
  TF1* fs[NUM_SEGMENTS][2];
  for(unsigned short s=0;s<NUM_SEGMENTS;s++){
    c->cd(1+s);
    hs[s] = (TH1F*)h->ProjectionY(Form("seg_%d",s),s+1,s+1);
    if(hs[s]->Integral(2,4000)<100)
      continue;    
    TSpectrum *sp = new TSpectrum(2);
    hs[s]->GetXaxis()->SetRangeUser(900,1200);
    Int_t nfound = sp->Search(hs[s],2,"nobackground",0.2);
    
    cout << "Found " << nfound << " peaks in spectrum" << endl;
    hs[s]->GetXaxis()->SetRangeUser(1,4000);
    hs[s]->Draw();
    if(nfound!=2)
      continue;
    float *xpeaks = sp->GetPositionX();
    if(xpeaks[0]>xpeaks[1]){
      double tmp = xpeaks[1];
      xpeaks[1] = xpeaks[0];
      xpeaks[0] = tmp;
    }
    for(unsigned short p=0;p<2;p++){//peaks
      lbinw = hs[s]->GetBinWidth(1);
      fs[s][p] = new TF1(Form("fseg_%d_%d",s,p),gausbg,xpeaks[p]-20,xpeaks[p]+20,5);
      double hint = hs[s]->Integral(hs[s]->FindBin(xpeaks[p]-20),hs[s]->FindBin(xpeaks[p]+20));
      if(hint<10)
	continue;
      double par[5] = {0,0,hint,xpeaks[p],2};
      fs[s][p]->SetParameters(par);
      fs[s][p]->SetParLimits(2,hint*0.5,hint*2);
      fs[s][p]->SetParLimits(3,xpeaks[p]-5,xpeaks[p]+5);
      fs[s][p]->SetParLimits(4,0.1,5);
      if(hint>100)
	hs[s]->Fit(fs[s][p],"Rqn");
      else
	hs[s]->Fit(fs[s][p],"RqnL");
      //cout << hint << "\t" << fs[s][p]->GetParameter(4) << endl;
      fs[s][p]->SetLineColor(2);
      fs[s][p]->Draw("same");
    }
    double gain = (1332.5 - 1173.2)/(fs[s][1]->GetParameter(3) - fs[s][0]->GetParameter(3));
    double offset = 1173.2 - gain*fs[s][0]->GetParameter(3);
    if(!isnan(gain)&&!isnan(offset)){
      calfile->SetValue(Form("Gain.Det.%d.Mod.%d.Seg.%d",det,board,s),gain);
      calfile->SetValue(Form("Offset.Det.%d.Mod.%d.Seg.%d",det,board,s),offset);
    }
    cout << gain <<"\t" << offset << "\t" << 1332.5- gain*fs[s][1]->GetParameter(3)<<endl;
  }
  calfile->SaveLevel(kEnvLocal);
}
void PlotRaw(char* file = (char*)"/mnt/raid/OEDO/GRAPE/htest.root"){
  TFile *f = new TFile(file);
  vector<TH2F*> h;
  for(unsigned short m=0;m<MAX_NUM_DET;m++){
    TH2F* hA = NULL;
    TH2F* hB = NULL;
    hA = (TH2F*)f->Get(Form("SegPHA_%d",m*2));
    hB = (TH2F*)f->Get(Form("SegPHA_%d",m*2+1));
    if(hA!=NULL){
      h.push_back(hA);
    }
    if(hB!=NULL){
      h.push_back(hB);
    }
  }
  cout << h.size() << " crystals found" << endl;
  TCanvas * c = new TCanvas("c","c");
  if(h.size()<=2)
    c->Divide(2,1);
  else if(h.size()<=4)
    c->Divide(2,2);
  else if(h.size()<=6)
    c->Divide(2,3);
  else if(h.size()<=9)
    c->Divide(3,3);
  else if(h.size()<=12)
    c->Divide(3,4);
  else if(h.size()<=16)
    c->Divide(4,4);
  else if(h.size()<=20)
    c->Divide(4,5);
  else if(h.size()<=30)
    c->Divide(5,6);
  else if(h.size()<=36)
    c->Divide(6,6);
  else{
    cout << "too many detectors found" <<endl;
    return;
  }
  for(unsigned short j=0;j<h.size();j++){
    c->cd(1+j);
    h.at(j)->Draw("colz");
    gPad->SetLogz();
  }
}
void PlotCal(char* file = (char*)"/mnt/raid/OEDO/GRAPE/htestcal.root"){
  TFile *f = new TFile(file);
  vector<TH2F*> h;
  for(unsigned short m=0;m<MAX_NUM_DET;m++){
    TH2F* hA = NULL;
    TH2F* hB = NULL;
    hA = (TH2F*)f->Get(Form("SegEn_%d",m*2));
    hB = (TH2F*)f->Get(Form("SegEn_%d",m*2+1));
    if(hA!=NULL){
      h.push_back(hA);
    }
    if(hB!=NULL){
      h.push_back(hB);
    }
  }
  cout << h.size() << " crystals found" << endl;
  TCanvas * c = new TCanvas("c","c");
  if(h.size()<=2)
    c->Divide(2,1);
  else if(h.size()<=4)
    c->Divide(2,2);
  else if(h.size()<=6)
    c->Divide(2,3);
  else if(h.size()<=9)
    c->Divide(3,3);
  else if(h.size()<=12)
    c->Divide(3,4);
  else if(h.size()<=16)
    c->Divide(4,4);
  else if(h.size()<=20)
    c->Divide(4,5);
  else if(h.size()<=30)
    c->Divide(5,6);
  else if(h.size()<=36)
    c->Divide(6,6);
  else{
    cout << "too many detectors found" <<endl;
    return;
  }
  for(unsigned short j=0;j<h.size();j++){
    c->cd(1+j);
    h.at(j)->Draw("colz");
    gPad->SetLogz();
  }
}
Double_t gausbg(Double_t *x, Double_t *par){
  static Float_t sqrt2pi = TMath::Sqrt(2*TMath::Pi()), sqrt2 = TMath::Sqrt(2.);
  Double_t arg;
  /*
  par[0]   background constant
  par[1]   background slope
  par[2]   gauss0 content
  par[3]   gauss0 mean
  par[4]   gauss0 width
  */
  Double_t result = par[0] + par[1]*x[0];

  Double_t norm  = par[2];
  Double_t mean  = par[3];
  Double_t sigma = par[4];
  if(sigma==0)
    return 0;
  arg = (x[0]-mean)/(sqrt2*sigma);
  result += lbinw/(sqrt2pi*sigma) * norm * exp(-arg*arg);

  return result;
}
