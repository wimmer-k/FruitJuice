#include "BuildEvents.hh"
using namespace std;
using namespace TMath;

/*!
  Swap two bytes a and b
  \param a
  \param b
  \return void
*/
void swapbytes(char* a, char *b){
  char tmp=*a;
  *a=*b;
  *b=tmp;
}

/*!
  Convert the buffer from high endian to low endian format
  \param cBuf the buffer to be converted
  \param bytes length in bytes of the buffer
  \return void
*/
void  HEtoLE(char* cBuf, int bytes){
  for(int i=0; i<bytes; i+=2)
    swapbytes((cBuf+i), (cBuf+i+1));
}

/*!
  Initialize the BuildEvents. The number of detector modules and the corresponding files are read from a settings file, the event building window is set, the tree is initialized, counters and hit/event storers are cleared
  \param settings the settings file
  \return settings are valid
*/
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

  //fRead stores how many hits per file have been added to the memory, should be 0,1,2
  fRead.resize(fNdet);

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
  fCurrentTS = 0;
  fFirstTS = -1;
  return true;
}

/*!
  Read one double block (detector A and B) from each file
  \return the number of bytes read
*/
long long int BuildEvents::ReadEachFile(){
  if(fVerboseLevel>0)
    cout <<__PRETTY_FUNCTION__ << endl;
  long long int bytes_read = 0;
  for(unsigned short i=0; i<fNdet; i++){
    if(fVerboseLevel>2)
      cout << "reading from file " << i<< endl;
    bytes_read += Unpack(i);
  }
  return bytes_read;
}

/*!
  Read one double block (detector A and B) from new files. Check how many hits from each file are still in memory. If the memory is empty, each file will be read. If there are still hits in memory, it checks for each file how many hits there still are.
  \return the number of bytes read and if reading was successfull
*/
pair<long long int,bool> BuildEvents::ReadNewFiles(){
  if(fVerboseLevel>0)
    cout <<__PRETTY_FUNCTION__ << endl;

  //returns the number of bytes read, and if successfull.
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

  if(fHits.size()==0){
    //if there are no more hits in memory, the file count should be zero
    unsigned short hitsinmem =0;
    for(vector<unsigned short>::iterator read= fRead.begin();read !=fRead.end();++read){
      hitsinmem += *read;
    }
    if(hitsinmem>0){
      cerr << "fHits.size()==0, but there are still counts for files in fRead: ";
      for(vector<unsigned short>::iterator read= fRead.begin();read !=fRead.end();++read){
	cerr << *read << " ";
      }
      cerr <<  endl;
      returnvalue.second = false;
      return returnvalue;
    }
    //no more hits, read each file
    returnvalue.first = ReadEachFile();
    if(returnvalue.first<1){
      returnvalue.second = false;
    }
    return returnvalue;
  }
  
  bool triedtoread = false;
  for(vector<unsigned short>::iterator read= fRead.begin();read !=fRead.end();++read){
    if(*read==0){
      if(fVerboseLevel>0)
	cout << "no more data from file " << read-fRead.begin() << endl;
      triedtoread = true;
      bytes_read += Unpack(read-fRead.begin());
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
  return returnvalue;
}

/*!
  Uses the HitComparer Class to sort the hits in memory by their time stamp. Earlies hits will be in the beginning of fHits, later hits towards the end
  \return void
*/
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

/*!
  Builds the events, the first hit in the time sorted vector is automatically added to the event. Subsequent hits are compared in time-stamp and if they lie within the window they are added to the event.
  \return void
*/
void BuildEvents::ProcessHits(){
  if(fVerboseLevel>0)
    cout << __PRETTY_FUNCTION__ << "before fHits.size() = " << fHits.size() << endl;
  if(fHits.size()==0)
    return;
  //first hit always good:
  fEvent->Add(fHits.at(0));
  fCurrentTS = fHits.at(0)->GetSumTS();
  if(fFirstTS<0)
    fFirstTS = fHits.at(0)->GetSumTS();
  //remove this from the counter
  fRead.at(fHits.at(0)->GetFileNumber())--;
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
      fRead.at(currentHit->GetFileNumber())--;
      iter = fHits.erase(iter);
    }
    else
      break;
  }
  if(fVerboseLevel>0){
    cout << "after fHits.size() = " << fHits.size() << endl;
    cout << "fEvent->GetMult() " << fEvent->GetMult() << endl;
    cout << "curernt files in memory: ";
    for(vector<unsigned short>::iterator read= fRead.begin();read !=fRead.end();++read){
      cout << *read << " ";
    }
    cout << endl;
  }
}

/*!
  If the event multiplicity is larger than 0, the event will be written to the tree (if fWriteTree is set), and the event will be cleared.
  \return void
*/
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

/*!
  Unpacks the data from one module (detector A and B), the file number or detector module number from will be stored in fRead.
  \param det the file number or detector module number from which to read
  \return the number of bytes read
*/
long long int BuildEvents::Unpack(unsigned short det){
  //unpack two blocks from file det
  long long int bytes_read = UnpackCrystal(det);
  bytes_read +=UnpackCrystal(det);
  
  fNbuffers++;
  if(fVerboseLevel>1){
    cout << "fNbuffers " << fNbuffers << "\tfNhits " << fNhits << endl;
    cout << "bytes_read " << bytes_read << endl;
    cout << "fHits.size() " << fHits.size() << endl;
  }
  return bytes_read;
}

/*!
  Unpacks the data from one crystal (detector A and B). The binary data will be unpacked and stored in a GrapeHit, segment information will be added in a vector of GrapeSeg. Basic data integrity check are performed
  \param det the file number or detector module number from which to read
  \return the number of bytes read
*/
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
  unsigned short SAbscount[9];
  bsize = fread(SAbscount, sizeof(unsigned short), 9, fDatafiles[det]);
  HEtoLE((char*)SAbscount,18);
  bytes_read += 9*sizeof(unsigned short);

  //one word for each segment pulse height
  unsigned short Spha[9];
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
  unsigned short Swave[9][48];
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
  unsigned short wave[48];
  bsize = fread(wave, sizeof(unsigned short), 48, fDatafiles[det]);
  HEtoLE((char*)wave,96);
  bytes_read += 48*sizeof(unsigned short);
  hit->SetSumWave(vector<unsigned short>(wave, wave + sizeof wave / sizeof wave[0]));

  //perform checks here
  //eight words dummy
  bsize = fread(buffer, sizeof(unsigned short), 8, fDatafiles[det]);
  HEtoLE((char*)buffer,16);
  if(fVerboseLevel>3){
    for(int i=0;i<8;i++)
      cout <<i<<"\t"<< buffer[i] <<"\t0x"<<(hex) <<buffer[i] << (dec) << endl;
  }
  bytes_read += 8*sizeof(unsigned short);
  
  //check for0xffff
  if(!CheckBufferEnd(buffer)){
    cerr << "\ninconsistent hit length end of the buffer "<< fNbuffers <<" is not 8 times 0xffff" << endl;
    
    pair<long long int, bool> readsuccess = SkipBytes(det,buffer);
    bytes_read += readsuccess.first;
    if(readsuccess.second ==false){
      cerr << "problem with skipping bytes " << endl;
      return 0;
    }
    //the current event is:
    hit->Print();
  }
  
  if(fVerboseLevel>1){
    cout << "bytes_read: " << bytes_read << endl;
    hit->Print();
  }
  //for now taking all of them, but the data seems corrupted
//  if(hit->GetSumTS()<fCurrentTS){ 
//    cout << "------"<<fNbuffers<<"--------------->bad hit! current time is " << fCurrentTS<< endl;
//    hit->Print();
//  }

  fHits.push_back(hit);
  //store from which file I read
  fRead.at(det)++;
  if(fVerboseLevel>0){
    cout << "read a hit from file " << det << " now fRead is : ";
    for(vector<unsigned short>::iterator read= fRead.begin();read !=fRead.end();++read){
      cout << *read << " ";
    }
    cout << endl;
  }
  //count up the number of hits
  fNhits++;

  if(fVerboseLevel>2)
    cout <<__PRETTY_FUNCTION__<< " end"<<endl;
  return bytes_read;
}

/*!
  Checking the end of the hit buffer,
  \param buffer pointer to the buffer (first of the 8 words) at the end of the hit buffer
  \return hit buffer length OK
*/
bool BuildEvents::CheckBufferEnd(unsigned short *buffer){
  if(fVerboseLevel>2){
    cout <<__PRETTY_FUNCTION__<<endl;
    for(int i=0;i<8;i++){
      cout <<i<<"\t"<< buffer[i] <<"\t0x"<<(hex) <<buffer[i] << (dec) << endl;
    }
  }
  for(int i=0;i<8;i++){
    if(buffer[i]!=0xffff)
      return false;
  }
  //cout <<__PRETTY_FUNCTION__<< " end"<<endl;
  return true;
}
/*!
  If the length of the buffer is not 1024 bytes, i.e. the end of the buffer is not 8 times 0xffff, this function reads until 8 times 0xffff is found. It will record how many words (unsigned short) have been skipped
  \param det the file number or detector module number from which to read
  \param buffer[8] 8 words which are not 0xffff
  \return the number of bytes newly read and if reading was successfull
*/
pair<long long int,bool> BuildEvents::SkipBytes(unsigned short det, unsigned short buffer[8]){
  if(fVerboseLevel>2){
    cout <<__PRETTY_FUNCTION__<<endl;
  }

  //returns the number of bytes read, and if successfull.
  std::pair <long long int, bool> returnvalue;
  returnvalue.second = true;

  //put all 8 buffers into a ring
  std::deque<unsigned short> ring;
  for(int i=0;i<8;i++){
    ring.push_back(buffer[i]);
  }
  if(fVerboseLevel>2){
    cout << "ring queue: " << endl;
    int ctr =0;
    for(deque<unsigned short>::iterator el= ring.begin();el !=ring.end();++el){
      cout << ctr << "\t"<< *el <<"\t0x"<<(hex) <<*el<< (dec) << endl;
      ctr++;
    }
  }
  returnvalue.first = 0;
  size_t bsize =0;

  //check the first element of the ring, if its goo, check them all
  //if the first parameter to && is false, the rest will not be evaluated
  while(ring.front()!=0xffff&&!CheckBufferEnd(&ring.front())){
    //remove the first element
    ring.pop_front();
    fSkipped++;
    //read one new word
    unsigned short readone[1];
    bsize = fread(readone, sizeof(unsigned short), 1, fDatafiles[det]);
    if(bsize==0){
      cerr<<"end of file " << det<<" reached" << endl;
      returnvalue.second = false;
      break;
    }
    returnvalue.first += 1*sizeof(unsigned short);
    //add the new word to the ring
    ring.push_back(readone[0]);
  }
    

  cout << "skipped " << fSkipped << "words, newly read " << returnvalue.first << " bytes"<< endl;
  if(fVerboseLevel>2){
    cout <<" now the queue containd: " << endl;
    int ctr =0;
    for(deque<unsigned short>::iterator el= ring.begin();el !=ring.end();++el){
      cout << ctr << "\t"<< *el <<"\t0x"<<(hex) <<*el<< (dec) << endl;
      ctr++;
    }
  }
  return returnvalue;
}
