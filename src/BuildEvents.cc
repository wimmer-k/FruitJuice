#include "BuildEvents.hh"
using namespace std;
using namespace TMath;

void swapbytes(char* a, char *b){
  char tmp=*a;
  *a=*b;
  *b=tmp;
}

// data is high endian format
void  HEtoLE(char* cBuf, int bytes){
  for(int i=0; i<bytes; i+=2)
    swapbytes((cBuf+i), (cBuf+i+1));
}

bool BuildEvents::Init(char *settings){
  if(fVerboseLevel>2)
    cout <<__PRETTY_FUNCTION__<< endl;

  cout << "reading settings file: " << settings << endl;
  TEnv* set = new TEnv(settings);
  
  //how many files to read
  fNdet = set->GetValue("Number.Of.Files",0);
  if(fNdet<1 || fNdet>18){
    cerr << "too few or to many files to read: Number.Of.Files = " << fNdet << endl;
    return false;
  }
  cout << "reading " << fNdet << " datafiles" << endl;
  fDatafiles.resize(fNdet);
  for(unsigned short i=0; i<fNdet; i++){
    string InputFile = set->GetValue(Form("File.%d",i),(char*)"nofile");
    cout << "reading File."<<i<<"\t" << InputFile << endl;
    fDatafiles[i] = fopen(InputFile.c_str(),"r");
  }
  //event building window
  fEventWindow = set->GetValue("Event.Window",500);
  cout << "Event building window " << fEventWindow << " ticks" << endl;


  fEvent = new GrapeEvent;
  //setup tree
  fTree = new TTree("gtr","GRAPE built events");
  fTree->Branch("grape",&fEvent,320000);
  fTree->BranchRef();
  if(!fWriteTree){
    cout << "No tree written to file" << endl;
  }
  else{
#ifdef WRITE_WAVE
    cout << "Waveforms are written to file" << endl;
#else
    cout << "Waveforms are NOT written to file" << endl;
#endif
  }

  fNbuffers = 0;
  fNhits = 0;
  fRead.clear();

  return true;
}
void BuildEvents::Run(int last){
  for(int i=0;i<last;i++){
    Unpack(0);
    cout << "------------------------------------" << endl;
    fHits.clear();
  }
}
long long int BuildEvents::ReadEachFile(){
  long long int bytes_read = 0;
  for(unsigned short i=0; i<fNdet; i++){
    if(fVerboseLevel>2)
      cout << "reading from file " << i<< endl;
    bytes_read += Unpack(i);
  }
  return bytes_read;
}
pair<long long int,bool> BuildEvents::ReadNewFiles(){
  if(fVerboseLevel>0)
    cout <<__PRETTY_FUNCTION__ << endl;

  //returns the number of bytes read, and if sucessfull.
  //it can happen that nothing is read, but there was also no attempt to read, therefore the bool is needed as well;
  std::pair <long long int, bool> returnvalue;
  returnvalue.second = true;

  long long int bytes_read = 0;
  if(fVerboseLevel>0){
    cout << "currently read in memory: "; 
    for(vector<unsigned short>::iterator read= fRead.begin();read !=fRead.end();++read){
      cout << *read << " ";
    }
    cout <<  endl;
  }
  if(fVerboseLevel>0){
    cout << "I want to read from files: "; 
    for(set<unsigned short>::iterator remo= fRemoved.begin();remo !=fRemoved.end();++remo){
      cout << *remo << " ";
    }
    cout <<  endl;
  }

  if(fRead.size()==0 && fHits.size()==0){
    returnvalue.first = ReadEachFile();
    if(returnvalue.first<1){
      returnvalue.second = false;
    }
    return returnvalue;
  }
  
  bool triedtoread = false;
  for(set<unsigned short>::iterator remo= fRemoved.begin();remo !=fRemoved.end();++remo){
    bool found = false;
    for(vector<unsigned short>::iterator read= fRead.begin();read !=fRead.end();++read){
      if(*remo == *read){
	if(fVerboseLevel>0)
	  cout << "file " << *remo << " is already there and will be removed"<< endl;
	fRead.erase(read);
	found = true;
	break;
      }
    }
    if(!found){
      if(fVerboseLevel>0)
	cout << "reading from file " << *remo<< endl;
      triedtoread = true;
      bytes_read += Unpack(*remo);
    }
  }
  if(fVerboseLevel>0){
    cout << "after reading new files read in memory: "; 
    for(vector<unsigned short>::iterator read= fRead.begin();read !=fRead.end();++read){
      cout << *read << " ";
    }
    cout <<  endl;
  }

  returnvalue.first = bytes_read;
  if(returnvalue.first<1 && triedtoread){
    returnvalue.second = false;
  }
  // cout << "returning " << returnvalue.first << " and ";
  // if(returnvalue.second)
  //   cout << "TRUE " << endl;
  // if(!returnvalue.second)
  //   cout << "FALSE " << endl;
  return returnvalue;
}

void BuildEvents::SortHits(){
  if(fVerboseLevel>0)
    cout <<__PRETTY_FUNCTION__ << endl;
  sort(fHits.begin(), fHits.end(), HitComparer());
  if(fVerboseLevel>0){
    cout << "sorted" << endl;
    for(unsigned short i=0;i<fHits.size();i++)
      fHits.at(i)->Print();
  }
}
void BuildEvents::ProcessHits(){
  if(fVerboseLevel>0)
    cout << __PRETTY_FUNCTION__ << "before fHits.size() = " << fHits.size() << endl;
  if(fHits.size()==0)
    return;
  fRemoved.clear();
  //first hit always good:
  fEvent->Add(fHits.at(0));
  fCurrentTS = fHits.at(0)->GetSumTS();
  //remove this from the unused hits
  fRemoved.insert(fHits.at(0)->GetFileNumber());
  fHits.erase(fHits.begin() + 0);

  //loop over remaining hits
  vector<GrapeHit*>::iterator iter= fHits.begin();
  while(iter!=fHits.end()){
    GrapeHit* currentHit = *iter;
    if(currentHit->GetSumTS() - fCurrentTS < fEventWindow){
      //add to event
      fEvent->Add(currentHit);
      fCurrentTS = currentHit->GetSumTS();
      //remove it
      fRemoved.insert(currentHit->GetFileNumber());
      iter = fHits.erase(iter);
    }
    else
      break;
  }
  if(fVerboseLevel>0){
    cout << "after fHits.size() = " << fHits.size() << endl;
    cout << "fEvent->GetMult() " << fEvent->GetMult() << endl;
    cout << "removed from files: ";
    for(set<unsigned short>::iterator iter= fRemoved.begin();iter !=fRemoved.end();++iter){
      cout << *iter<<" ";
    }
    cout << endl;
  }
}
void BuildEvents::CloseEvent(){
  if(fVerboseLevel>0){
    cout <<__PRETTY_FUNCTION__ << endl;
    fEvent->Print();
  }

  if(fWriteTree && fEvent->GetMult()>0){
    fTree->Fill();
  }
  fEvent->Clear();  
}
long long int BuildEvents::Unpack(unsigned short det){
  //unpack two blocks from file det
  long long int bytes_read = UnpackCrystal(det);
  bytes_read +=UnpackCrystal(det);
  
  //store from which file I read
  fRead.push_back(det);
  fNbuffers++;
  if(fVerboseLevel>1){
    cout << "fNbuffers " << fNbuffers << "\tfNhits " << fNhits << endl;
    cout << "bytes_read " << bytes_read << endl;
    cout << "fHits.size() " << fHits.size() << endl;
  }
  return bytes_read;
}
long long int BuildEvents::UnpackCrystal(unsigned short det){
  long long int bytes_read = 0;
  if(fVerboseLevel>1){
    cout <<__PRETTY_FUNCTION__<< endl;
    cout << "unpacking file for detector " << det << endl;
  }
  if(feof(fDatafiles[det])){
    cout << "end of file " << endl;
    return 0;
  }

  GrapeHit *hit = new GrapeHit();
  hit->Clear();
  hit->SetFileNumber(det);
  size_t bsize;
  unsigned short buffer[8];
  //one word with trigger flag, board number (crystal A/B), detector number
  bsize = fread(buffer, sizeof(unsigned short), 1, fDatafiles[det]);
  if(bsize == 0)
    return 0;
  int detID = (buffer[0]&0xff00)>>8;
  int trigg = (buffer[0]&0x0080)>>7;
  int board = (buffer[0]&0x0040)>>6;
  hit->SetTrigFlag(trigg);
  hit->SetDetNumber(detID);
  hit->SetBoardNumber(board);
  if(fVerboseLevel>1){
    cout << "header: " << buffer[0] <<"\t0x"<<(hex) <<buffer[0] << (dec) << endl;
    cout << "detID: " << detID;
    cout << "\ttrigg: " << trigg;
    cout << "\tboard: " << board << endl;
  }
  bytes_read += 1*sizeof(unsigned short);

  //one word with the time of the SUM signal leading edge
  bsize = fread(buffer, sizeof(unsigned short), 1, fDatafiles[det]);
  if(bsize == 0){
    cerr << "error; no data" << endl;
    return bytes_read;
  }
  HEtoLE((char*)buffer,2);
  hit->SetSumLET(buffer[0]);

  if(fVerboseLevel>1){
    cout << "LET: " << buffer[0] <<"\t0x"<<(hex) <<buffer[0] << (dec) << endl;
  }
  bytes_read += 1*sizeof(unsigned short);

  //three words containing the time-stamp (SUM Abs count)
  bsize = fread(buffer, sizeof(unsigned short), 3, fDatafiles[det]);
  if(fVerboseLevel>3){
    cout << "TS0: " << buffer[0] <<"\t0x"<<(hex) <<buffer[0] << (dec) << endl;
    cout << "TS1: " << buffer[1] <<"\t0x"<<(hex) <<buffer[1] << (dec) << endl;
    cout << "TS2: " << buffer[2] <<"\t0x"<<(hex) <<buffer[2] << (dec) << endl;
  }
  HEtoLE((char*)buffer,6);
  if(fVerboseLevel>2){
    cout << "TS0: " << buffer[0] <<"\t0x"<<(hex) <<buffer[0] << (dec) << endl;
    cout << "TS1: " << buffer[1] <<"\t0x"<<(hex) <<buffer[1] << (dec) << endl;
    cout << "TS2: " << buffer[2] <<"\t0x"<<(hex) <<buffer[2] << (dec) << endl;
  }
  long long int ts = (long long int)buffer[0]<<32;
  ts += (long long int)buffer[1]<<16;
  ts += (long long int)buffer[2];
  hit->SetSumTS(ts);
  if(fVerboseLevel>1){
    cout << "ts " << ts <<"\t0x"<<(hex) << ts << (dec) << "\t" << hit->GetSumTS() << "\t\t";
  }
  bytes_read += 3*sizeof(unsigned short);
 
  //one word containing the pulse height information
  bsize = fread(buffer, sizeof(unsigned short), 1, fDatafiles[det]);
  HEtoLE((char*)buffer,2);
  int pha = buffer[0];
  hit->SetSumPHA(pha);
  if(fVerboseLevel>1){
    cout << "PHA " << pha <<"\t0x"<<(hex) << pha << (dec) << endl;
  }
  bytes_read += 1*sizeof(unsigned short);


  //one word for each segment Abs count
  unsigned short int SAbscount[9];
  bsize = fread(SAbscount, sizeof(unsigned short), 9, fDatafiles[det]);
  HEtoLE((char*)SAbscount,18);
  bytes_read += 9*sizeof(unsigned short);

  //one word for each segment pulse height
  unsigned short int Spha[9];
  bsize = fread(Spha, sizeof(unsigned short), 9, fDatafiles[det]);
  HEtoLE((char*)Spha,18);
  if(fVerboseLevel>2){
    for(int i=0;i<9;i++){
      if(Spha[i]>0)
	cout <<i<< "\tAbs " << SAbscount[i] <<"\t0x"<<(hex) <<SAbscount[i]  << (dec) << "\tPHA " << Spha[i] <<"\t0x"<<(hex) <<Spha[i]  << (dec) << endl;     
    }
  }
  bytes_read += 9*sizeof(unsigned short);

  vector<GrapeSeg> segments;
  segments.clear();
  segments.resize(NUM_SEGMENTS);

  //48 words for each segments containing the wave
  unsigned short int Swave[9][48];
  for(int i=0;i<9;i++){
    bsize = fread(Swave[i], sizeof(unsigned short), 48, fDatafiles[det]);
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
  bsize = fread(wave, sizeof(unsigned short), 48, fDatafiles[det]);
  HEtoLE((char*)wave,96);
  bytes_read += 48*sizeof(unsigned short);
  hit->SetSumWave(vector<unsigned short>(wave, wave + sizeof wave / sizeof wave[0]));

  //eight words dummy
  bsize = fread(buffer, sizeof(unsigned short), 8, fDatafiles[det]);
  HEtoLE((char*)buffer,16);
  if(fVerboseLevel>3){
    for(int i=0;i<8;i++)
      cout <<i<<"\t"<< buffer[i] <<"\t0x"<<(hex) <<buffer[i] << (dec) << endl;
  }
  bytes_read += 8*sizeof(unsigned short);
  if(fVerboseLevel>1){
    cout << "bytes_read: " << bytes_read << endl;
    hit->Print();
  }

  //for now taking all of them, but the data seems corrupted
  fHits.push_back(hit);
  fNhits++;
  if(fVerboseLevel>2)
    cout <<__PRETTY_FUNCTION__<< " end"<<endl;
  return bytes_read;
}
