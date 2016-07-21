TH2F* h2d = NULL;
TH1F* h1d = NULL;
TH1F* hpx[5];
TH1F* hpy[5];
TH1F* hpt[5];
int reb = 1;
char* filename = "/mnt/raid/OEDO/GRAPE/hist05.root";
void loadhistos();
void reset();
void setreb(int inreb){
  reb = inreb;
}
void plot(){
  loadhistos();
  h1d->Draw();
}
void sub(double from, double to, double bg0=0, double bg1=0, double bg2=0, double bg3=0, bool draw = true, bool rebinafter =false){
  loadhistos();
  if(hpx[0]){
    for(int i=0;i<5;i++){
      hpx[i] = NULL;
      hpy[i] = NULL;
      hpt[i] = NULL;
    }
  }
  TCanvas *c;
  if(draw){
   c = new TCanvas("c","c",800,800);
   c->Divide(2,3);
   c->cd(1);
   h1d->GetXaxis()->SetRangeUser(from-200,to+200);
   h1d->DrawCopy();
   h1d->GetXaxis()->SetRangeUser(0,4000);
   c->cd(2);
   h2d->Draw("colz");
   h2d->GetXaxis()->SetRangeUser(0,2000);
   h2d->GetYaxis()->SetRangeUser(0,2000);
  }
  if(bg0==0&&bg1==0){
    bg0 = from-(to-from)/2;
    bg1 = from;
  }
  if(bg2==0&&bg3==0){
    bg2 = to;
    bg3 = to+(to-from)/2;
  }
    
  int xp[2] = {h2d->GetXaxis()->FindBin(from), h2d->GetXaxis()->FindBin(to)};
  int yp[2] = {h2d->GetYaxis()->FindBin(from), h2d->GetYaxis()->FindBin(to)};

  int xl[2] = {h2d->GetXaxis()->FindBin(bg0), h2d->GetXaxis()->FindBin(bg1)};
  int yl[2] = {h2d->GetYaxis()->FindBin(bg0), h2d->GetYaxis()->FindBin(bg1)};
  int xh[2] = {h2d->GetXaxis()->FindBin(bg2), h2d->GetXaxis()->FindBin(bg3)};
  int yh[2] = {h2d->GetYaxis()->FindBin(bg2), h2d->GetYaxis()->FindBin(bg3)};
  //cout << xb[0] << " " << xb[1] << " " << yb[0] << " " << yb[1] << endl;
  //cout << xl[0] << " " << xl[1] << " " << yl[0] << " " << yl[1] << endl;

  hpx[0] = (TH1F*)h2d->ProjectionX("hpx0",yp[0],yp[1]);
  hpy[0] = (TH1F*)h2d->ProjectionY("hpy0",xp[0],xp[1]);
  if(!rebinafter){
    hpx[0]->Rebin(reb);
    hpy[0]->Rebin(reb);
  }
  
  cout << "peak " << endl;

  int enpeak[2]= {h2d->GetXaxis()->GetBinCenter(xp[0]),h2d->GetXaxis()->GetBinCenter(xp[1])};
  int enleft[2]= {h2d->GetXaxis()->GetBinCenter(xl[0]),h2d->GetXaxis()->GetBinCenter(xl[1])};
  int enrigh[2]= {h2d->GetXaxis()->GetBinCenter(xh[0]),h2d->GetXaxis()->GetBinCenter(xh[1])};

  double width[3];
  width[0] = h2d->GetXaxis()->GetBinCenter(xp[1])-h2d->GetXaxis()->GetBinCenter(xp[0]);
  cout << h2d->GetXaxis()->GetBinCenter(xp[0]) <<"\t"<<h2d->GetXaxis()->GetBinCenter(xp[1]) << " keV\twidth "<<width[0]<<" keV"<< endl;

  width[1] = h2d->GetXaxis()->GetBinCenter(xl[1])-h2d->GetXaxis()->GetBinCenter(xl[0]);
  cout << "bg1 " << endl;
  cout << h2d->GetXaxis()->GetBinCenter(xl[0]) <<" to "<<h2d->GetXaxis()->GetBinCenter(xl[1])<<" keV\twidth "<<width[1]<<" keV"<< endl;

  width[2] = h2d->GetXaxis()->GetBinCenter(xh[1])-h2d->GetXaxis()->GetBinCenter(xh[0]);
  cout << "bg2 " << endl;
  cout << h2d->GetXaxis()->GetBinCenter(xh[0]) <<" to "<<h2d->GetXaxis()->GetBinCenter(xh[1])<<" keV\twidth "<<width[2]<<" keV"<< endl;

  TBox *bp, *b1, *b2;
  if(draw){
    bp = new TBox(h2d->GetXaxis()->GetBinCenter(xp[1]),0,h2d->GetXaxis()->GetBinCenter(xp[0]),h1d->GetMaximum());
    bp->SetFillColor(3);
    bp->SetFillStyle(3002);
    b1 = new TBox(h2d->GetXaxis()->GetBinCenter(xl[0]),0,h2d->GetXaxis()->GetBinCenter(xl[1]),h1d->GetMaximum());
    b1->SetFillColor(2);
    b1->SetFillStyle(3002);
    b2 = new TBox(h2d->GetXaxis()->GetBinCenter(xh[0]),0,h2d->GetXaxis()->GetBinCenter(xh[1]),h1d->GetMaximum());
    b2->SetFillColor(2);
    b2->SetFillStyle(3002);
  }

  hpt[0] = (TH1F*)hpx[0]->Clone("hpt0");
  hpt[0]->Add(hpy[0],1);

  hpx[1] = (TH1F*)h2d->ProjectionX("hpx1",yl[0],yl[1]);
  hpy[1] = (TH1F*)h2d->ProjectionY("hpy1",xl[0],xl[1]);
  if(!rebinafter){
    hpx[1]->Rebin(reb);
    hpy[1]->Rebin(reb);
  }
  hpx[1]->Scale(0.5*width[0]/width[1]);
  hpy[1]->Scale(0.5*width[0]/width[1]);
  hpt[1] = (TH1F*)hpx[1]->Clone("hpt1");
  hpt[1]->Add(hpy[1],1);
  
  hpx[2] = (TH1F*)h2d->ProjectionX("hpx2",yh[0],yh[1]);
  hpy[2] = (TH1F*)h2d->ProjectionY("hpy2",xh[0],xh[1]);
  if(!rebinafter){
    hpx[2]->Rebin(reb);
    hpy[2]->Rebin(reb);
  }
  hpx[2]->Scale(0.5*width[0]/width[2]);
  hpy[2]->Scale(0.5*width[0]/width[2]);
  hpt[2] = (TH1F*)hpx[2]->Clone("hpt1");
  hpt[2]->Add(hpy[1],1);
  
  hpx[3] = (TH1F*)hpx[0]->Clone("hpx3");
  hpx[3]->Add(hpx[1],-1);
  hpx[3]->Add(hpx[2],-1);
  hpy[3] = (TH1F*)hpy[0]->Clone("hpy3");
  hpy[3]->Add(hpy[1],-1);
  hpy[3]->Add(hpy[2],-1);

  hpt[3] = (TH1F*)hpt[0]->Clone("hpt3");
  hpt[3]->Add(hpt[1],-1);
  hpt[3]->Add(hpt[2],-1);
  
  hpx[4] = (TH1F*)hpx[1]->Clone("hpx4");
  hpx[4]->Add(hpx[2],1);
  hpy[4] = (TH1F*)hpy[1]->Clone("hpy4");
  hpy[4]->Add(hpy[2],1);
  hpt[4] = (TH1F*)hpt[1]->Clone("hpt4");
  hpt[4]->Add(hpt[2],1);
 
  for(int i=0;i<4;i++){
    for(int b=xp[0];b<xp[1];b++){
      hpt[i]->SetBinContent(b,hpt[i]->GetBinContent(b)*0.5);
    }
  }
  
  if(draw){
    c->cd(1);
    bp->Draw();
    b1->Draw();
    b2->Draw();
  }
  // double intpeak = h1d->Integral(h2d->GetXaxis()->GetBinCenter(xb[1]),h2d->GetXaxis()->GetBinCenter(xb[0]));
  // double intleft = h1d->Integral(h2d->GetXaxis()->GetBinCenter(xl[0]),h2d->GetXaxis()->GetBinCenter(xb[0]));
  // double intrigh = h1d->Integral(h2d->GetXaxis()->GetBinCenter(xl[1]),h2d->GetXaxis()->GetBinCenter(xb[1]));
  cout << Form("%d to %d",enpeak[0],enpeak[1]) << endl;
  cout << Form("%d to %d",enleft[0],enleft[1]) << endl;
  cout << Form("%d to %d",enrigh[0],enrigh[1]) << endl;
  double intpeak = h1d->Integral(h1d->FindBin(enpeak[0]),h1d->FindBin(enpeak[1]));
  double intleft = 0.5*width[0]/width[1]*h1d->Integral(h1d->FindBin(enleft[0]),h1d->FindBin(enleft[1]));
  double intrigh = 0.5*width[0]/width[2]*h1d->Integral(h1d->FindBin(enrigh[0]),h1d->FindBin(enrigh[1]));
  
  cout << "peak = " << intpeak << "\tleft = " << intleft << "\tright = " << intrigh<< endl;
  cout << "peak = " << intpeak - intleft - intrigh<< endl;

  if(rebinafter){
    hpx[0]->Rebin(reb);
    hpx[1]->Rebin(reb);
    hpx[2]->Rebin(reb);
    hpx[4]->Rebin(reb);
    hpy[0]->Rebin(reb);
    hpy[1]->Rebin(reb);
    hpy[2]->Rebin(reb);
    hpy[4]->Rebin(reb);
    hpt[0]->Rebin(reb);
    hpt[3]->Rebin(reb);
    hpt[4]->Rebin(reb);
  }
  if(!draw)
    return;
  c->cd(3);
  hpx[0]->SetLineColor(1);
  hpx[0]->DrawCopy();
  hpx[1]->SetLineColor(2);
  hpx[1]->DrawCopy("same");
  hpx[2]->SetLineColor(2);
  hpx[2]->DrawCopy("same");
  hpx[4]->SetLineColor(3);
  hpx[4]->DrawCopy("same");
  c->cd(4);
  hpy[0]->SetLineColor(1);
  hpy[0]->DrawCopy();
  hpy[1]->SetLineColor(2);
  hpy[1]->DrawCopy("same");
  hpy[2]->SetLineColor(2);
  hpy[2]->DrawCopy("same");
  hpy[4]->SetLineColor(3);
  hpy[4]->DrawCopy("same");
  c->cd(5);
  hpt[0]->SetLineColor(1);
  hpt[0]->DrawCopy();
  hpt[3]->SetLineColor(3);
  hpt[3]->DrawCopy("same");
  hpt[4]->SetLineColor(2);
  hpt[4]->DrawCopy("same");
  c->cd(6);
  hpt[3]->SetLineColor(1);
  hpt[3]->DrawCopy();

  TCanvas *c2 = new TCanvas("c2","c2",800,800);
  c2->cd();
  hpt[3]->DrawCopy();
  
}
void reset(){
  h2d=NULL;
  h1d=NULL;
}
void loadhistos(){
  TFile *f1 = new TFile(filename);
  h2d = (TH2F*)f1->Get("egamegam");
  h1d = (TH1F*)f1->Get("egam");
}
