#include <iostream>
#include <fstream>
#include <iomanip>
#include "TFile.h"
#include "TH1F.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TMath.h"
#include "TEnv.h"
#include "TSpectrum.h"
using namespace std;
using namespace TMath;
double lbinw;
Double_t gausbg(Double_t *x, Double_t *par);

void Eff(){
  TFile *f = new TFile("/mnt/raid/OEDO/GRAPE/heuropium.root");
  TH1F *h = (TH1F*)f->Get("egam"); //change egam to SUM_En** to make an individual one
  const int n = 10;
  ifstream intensity;
  intensity.open("scripts/EuDecay.txt");
  intensity.ignore(1000,'\n');
  double en[n], inten[n], eff[n];
  TF1* fu[n];
  double act = 178455;
  double runtime = 1798.08;
  
  for(int i=0;i<n;i++){
    intensity >> en[i] >> inten[i];
    h->GetXaxis()->SetRangeUser(en[i]-50,en[i]+50);
    lbinw = h->GetXaxis()->GetBinWidth(1);
    fu[i] = new TF1(Form("fu_%d",i),gausbg,en[i]-20,en[i]+20,5);
    double hint = h->Integral(h->FindBin(en[i]-20),h->FindBin(en[i]+20));
    if(hint<10)
      continue;
    double par[5] = {0,0,hint,en[i],2};
    fu[i]->SetParameters(par);
    //fu[i]->SetParLimits(2,hint*0.5,hint*2);
    //fu[i]->SetParLimits(3,en[i]-5,en[i]+5);
    fu[i]->SetParLimits(4,0.1,5);
    if(hint>100)
      h->Fit(fu[i],"Rqn");
    
    eff[i] = fu[i]->GetParameter(2)/(inten[i]*act*runtime);

  }
  TCanvas *c = new TCanvas("c","c",800,400);
  c->Divide(2,1);
  c->cd(1);
  h->GetXaxis()->SetRangeUser(0,1500);
  h->Draw();
  for(int i=0;i<n;i++){
    fu[i]->Draw("same");
  }
  TGraph * g = new TGraph(n,en,eff);
  c->cd(2);
  g->Draw("A*");
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
