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
  if(fNdet<1 || fNdet>MAXNUMFILES){
    cerr << "too few or to many files to read: Number.Of.Files = " << fNdet << endl;
    return false;
  }
  cout << "reading " << fNdet << " datafiles" << endl;
  fDatafiles.resize(fNdet);
  fDetNumbers.resize(fNdet);
  for(unsigned short i=0; i<fNdet; i++){
    string InputFile = set->GetValue(Form("File.%d",i),(char*)"nofile");
    fDetNumbers[i] = set->GetValue(Form("Detector.Number.%d",i),-1);
    cout << "reading File."<<i<<" for detector " << fDetNumbers[i] <<"\t" << InputFile;
    fDatafiles[i] = fopen(InputFile.c_str(),"rb");
    fseek(fDatafiles[i] , 0 , SEEK_END);
    long long int lSize = ftell(fDatafiles[i]);
    rewind(fDatafiles[i]);
    cout << "\t size = " << lSize/(1024*1024) << " MB" << endl;
  }

  //fRead stores how many hits per file have been added to the memory, should be 0,1,2
  fRead.resize(fNdet);

  //event building window
  fEventWindow = set->GetValue("Event.Window",500);
  cout << "Event building window " << fEventWindow << " ticks" << endl;

  //DAQ coincidence gate, maximum time difference between detector A and B (set carefully)
  fMaxTSDiff = set->GetValue("Coincidence.Gate",5000000);
  cout << "Coincidence Gate " << fMaxTSDiff << " ticks" << endl;


  fEvent = new GrapeEvent;
  //setup tree
  fTree = new TTree("gtr","GRAPE built events");
  double maxtreesize = set->GetValue("MaxTreeSize.GB",2.0);
  fTree->SetMaxTreeSize((int)(maxtreesize*1000000000));
  fTree->Branch("grape",&fEvent,320000);
  fTree->BranchRef();
  fTree->SetAutoSave(-300000000);	
  fTree->SetAutoFlush(-100000000);	

  
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
  fHits.clear();

  for(unsigned short i=0;i<10;i++){
    fErrors[i] =0;
  }
  return true;
}

/*!
  Read one double block (detector A and B) from each file
  \return the number of bytes read
*/
long long int BuildEvents::ReadEachFile(){
  if(fVerboseLevel>1)
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
  if(fVerboseLevel>1)
    cout <<__PRETTY_FUNCTION__ << endl;

  //returns the number of bytes read, and if successfull.
  //it can happen that nothing is read, but there was also no attempt to read, therefore the bool is needed as well;
  std::pair <long long int, bool> returnvalue;
  returnvalue.second = true;

  long long int bytes_read = 0;

  if(fVerboseLevel>1){
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
      if(fVerboseLevel>1)
	cout << "no more data from file " << read-fRead.begin() << endl;
      triedtoread = true;
      bytes_read += Unpack(read-fRead.begin());
    }
  }

  if(fVerboseLevel>1){
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
  if(fVerboseLevel>1)
    cout <<__PRETTY_FUNCTION__ << endl;
  sort(fHits.begin(), fHits.end(), HitComparer());
  if(fVerboseLevel>0){
    cout <<GREEN<< "sorted:"<<DEFCOLOR << endl;
    for(unsigned short i=0;i<fHits.size();i++)
      fHits.at(i)->Print();
  }
}

/*!
  Builds the events, the first hit in the time sorted vector is automatically added to the event. Subsequent hits are compared in time-stamp and if they lie within the window they are added to the event.
  \return void
*/
void BuildEvents::ProcessHits(){
  if(fVerboseLevel>1)
    cout << __PRETTY_FUNCTION__ << " before fHits.size() = " << fHits.size() << endl;
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
  if(fVerboseLevel>1){
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
    cout << BLUE << "built events: "<< DEFCOLOR<<endl;
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
  long long int unpackTS =0;

  //unpack two blocks from file det
  long long int bytes_read = UnpackCrystal(det,unpackTS);
  bytes_read += UnpackCrystal(det,unpackTS);

  
  fNbuffers++;
  if(fVerboseLevel>1){
    cout << "fNbuffers " << fNbuffers << "\tfNhits " << fNhits << endl;
    cout << "bytes_read " << bytes_read << endl;
    cout << "fHits.size() " << fHits.size() << endl;
  }
  if(fVerboseLevel>0){
    cout <<MAGENTA<< "read:"<<DEFCOLOR << endl;
    for(unsigned short i=0;i<fHits.size();i++)
      fHits.at(i)->Print();
  }

  return bytes_read;
}

/*!
  Unpacks the data from one crystal (detector A and B). The binary data will be unpacked and stored in a GrapeHit, segment information will be added in a vector of GrapeSeg. Basic data integrity check are performed
  \param det the file number or detector module number from which to read
  \param prevunpackts 0 if this is for detector A, TS of A if B will be read
  \param checkonly if true, the hit will be returned no matter what, even if there is an error
  \return the number of bytes read
*/
long long int BuildEvents::UnpackCrystal(unsigned short det,long long int &prevunpackts, bool checkonly){
  long long int bytes_read = 0;
  bool hitgood = true;
  if(fVerboseLevel>1){
    cout <<__PRETTY_FUNCTION__<< endl;
    cout << "unpacking file for detector " << det << endl;
  }
  if(feof(fDatafiles[det])){
    //cout << "end of file " << endl;
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

  //48 = WAVE_LENGTH words for each segments containing the wave
  for(int i=0;i<NUM_SEGMENTS;i++){

#ifdef WRITE_WAVE
    unsigned short Swave[WAVE_LENGTH];
    bsize = fread(Swave, sizeof(unsigned short), WAVE_LENGTH, fDatafiles[det]);
    HEtoLE((char*)Swave,WAVE_LENGTH*2);
    bytes_read += WAVE_LENGTH*sizeof(unsigned short);
    segments[i].SetSegWave(vector<unsigned short>(Swave, Swave + sizeof Swave / sizeof Swave[0]));
#else  
    bsize = fseek(fDatafiles[det], WAVE_LENGTH*2, SEEK_CUR);
    bytes_read += WAVE_LENGTH*sizeof(unsigned short);
#endif
    segments[i].SetSegNumber(i);
    segments[i].SetSegTS(SAbscount[i]);
    segments[i].SetSegPHA(Spha[i]);
      
    hit->AddSegment(segments[i]);
  }
    
#ifdef WRITE_WAVE
  //48 = WAVE_LENGTH words for SUM containing the wave
  unsigned short wave[WAVE_LENGTH];
  bsize = fread(wave, sizeof(unsigned short), WAVE_LENGTH, fDatafiles[det]);
  HEtoLE((char*)wave,WAVE_LENGTH*2);
  bytes_read += WAVE_LENGTH*sizeof(unsigned short);
  hit->SetSumWave(vector<unsigned short>(wave, wave + sizeof wave / sizeof wave[0]));
#else  
  bsize = fseek(fDatafiles[det], WAVE_LENGTH*2, SEEK_CUR);
  bytes_read += WAVE_LENGTH*sizeof(unsigned short);
#endif

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
    if(fVerboseLevel>0)
      cout << "inconsistent hit length end of the buffer "<< fNbuffers <<" is not 8 times 0xffff" << endl;
    fErrors[0]++;
    hitgood = false;
    pair<long long int, bool> readsuccess = SkipBytes(det,buffer);
    bytes_read += readsuccess.first;
    if(readsuccess.second ==false){
      if(fVerboseLevel>0)
	cout << "problem with skipping bytes " << endl;
      return 0;
    }
  }
  
  if(fVerboseLevel>1){
    cout << "bytes_read: " << bytes_read << endl;
    hit->Print();
  }
  //for now taking all of them, but the data seems corrupted

  //checking if the time jumped back
  if(hitgood && hit->GetSumTS()<fCurrentTS){ 
    fErrors[1]++;
    hitgood = false;
    if(fVerboseLevel>0)
      cout <<RED<< "--------------->bad hit! current time is " << fCurrentTS << " this one " << hit->GetSumTS()<< " at buffer "<<fNbuffers<<DEFCOLOR<<endl;
  }
  //checking if the time between A and B is sufficiently small
  if(hitgood && prevunpackts>0 && abs(prevunpackts-hit->GetSumTS())>fMaxTSDiff){
    fErrors[2]++;
    hitgood = false;
    if(fVerboseLevel>0)
      cout <<RED<< "--------------->bad hit! previous hit read had time " << prevunpackts << " this one " << hit->GetSumTS()<< " at buffer "<<fNbuffers<<DEFCOLOR<<endl;    
  }
  //checking if the detector number is consistent with the setting
  if(hitgood && hit->GetDetNumber() != fDetNumbers[det]){
    fErrors[3]++;
    hitgood = false;
    cout <<RED<< "--------------->bad hit! detector number read " << hit->GetDetNumber()<< " file "<<det<<" is for detector " << fDetNumbers[det] << " at buffer "<<fNbuffers<<DEFCOLOR<<endl;
    hit->Print();
  }
  //checking if the hit had an energy
  if(hitgood && hit->GetSumPHA()<1){
    fErrors[4]++;
    cout <<RED<< "--------------->bad hit! energy is " << hit->GetSumPHA()<< " at buffer "<<fNbuffers<<DEFCOLOR<<endl;
    hitgood = false;
  }
  //checking if segments reported energies// these occurr a lot skipping for now
  // if(hitgood && (hit->GetSegMult()==0||hit->GetSegMult()>9)){
  //   fErrors[5]++;
  //   if(fVerboseLevel>0)
  //     cout <<RED<< "--------------->bad hit! number of segments is " << hit->GetSegMult()<< " at buffer "<<fNbuffers<<DEFCOLOR<<endl;
    
  //   hitgood = false;
  // }

  // if the hit fails the checks, bail out
  if(!hitgood){
    if(fVerboseLevel>0){
      cout <<RED<< "the current hit will be skipped: "<<DEFCOLOR << endl;
      hit->Print();
    }
    //only check the timestamps
    if(checkonly){
      //store the hit
      fHits.push_back(hit);
    }
    return bytes_read;
  }
  //store the hit
  fHits.push_back(hit);
  //count up the number of hits
  fNhits++;
  //store from which file I read
  fRead.at(det)++;
  if(fVerboseLevel>1){
    cout << "read a hit from file " << det << " now fRead is : ";
    for(vector<unsigned short>::iterator read= fRead.begin();read !=fRead.end();++read){
      cout << *read << " ";
    }
    cout << endl;
  }
  prevunpackts = hit->GetSumTS();
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
  return true;
}
/*!
  Read double blocks (detector A and B) from each file until a timestamp jump is detected
  \return the number of bytes read
*/
long long int BuildEvents::DetectTimestampJumps(){
  if(fVerboseLevel>1)
    cout <<__PRETTY_FUNCTION__ << endl;
  long long int bytes_read = 0;
  for(unsigned short i=0; i<fNdet; i++){
    if(fVerboseLevel>2)
      cout << "reading from file " << i<< endl;
    bytes_read += DetectTimestampJump(i);
  }
  fNbuffers = 0;
  fNhits = 0;
  fCurrentTS = 0;
  fFirstTS = -1;
  fHits.clear();

  for(unsigned short i=0;i<10;i++){
    fErrors[i] =0;
  }
  fRead.clear();
  fRead.resize(fNdet);

  return bytes_read;

}
/*!
  Detect the timestamp jump
  \return the number of bytes read
*/
long long int BuildEvents::DetectTimestampJump(unsigned short det){
  fHits.clear();
  //remember position in file
  fpos_t position;
  long long int bytes_read =0;
  long long int lastTS[2] ={0,0};
  bool jumped[2] = {false,false};
  while(true){
    long long int unpackTS[2] ={0,0};
    fgetpos(fDatafiles[det], &position);
    bytes_read += UnpackCrystal(det,unpackTS[0],true);
    bytes_read += UnpackCrystal(det,unpackTS[1],true);   
    if(unpackTS[0]<lastTS[0]){
      cout << "A jumps back by " << lastTS[0] - unpackTS[0] << endl;
      jumped[0] = true;
    }
    if(unpackTS[1]<lastTS[1]){
      cout << "B jumps back by " << lastTS[1] - unpackTS[1] << endl;
      jumped[1] = true;
    }
    if(jumped[0] && jumped[1])
      break;
    lastTS[0] = unpackTS[0];
    lastTS[1] = unpackTS[1];
  }
  fsetpos(fDatafiles[det], &position);
  
  return bytes_read;
}

/*!
  If the length of the buffer is not 1024 bytes, i.e. the end of the buffer is not 8 times 0xffff, this function reads until 8 times 0xffff is found. It will record how many words (unsigned short) have been skipped
  \param det the file number or detector module number from which to read
  \param buffer 8 words which are not 0xffff
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
  //if the first parameter to || is true, the rest will not be evaluated
  int ctr =0;
  int ctrf =0;
  while(ring.front()!=0xffff || !CheckBufferEnd(&ring.front())){
    if(ring.front()==0xffff)
      ctrf++;
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
    ctr++;
  }
  if(ctr>2048 && fVerboseLevel>0){
    cout << MAGENTA << "skipped " << ctr << " words, during that " << ctrf << " 0xffff occurred (random order)"<< DEFCOLOR << endl;
  }
  if(fVerboseLevel>0)
    cout << "skipped " << fSkipped << " words (cumulative), newly read " << returnvalue.first << " bytes"<< endl;
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

/*!
  Print the couter of different errors that occurred during unpacking, if too many happen, the data file is corrupted
*/
void BuildEvents::PrintErrors(){
  char* errorcode[10] = {
    (char*)"\tHit buffer didn't end with 8 times 0xffff",          //0
    (char*)"\tTime-stamp of hit is smaller than the current time", //1
    (char*)"\tTime-stamp difference too large",                    //2
    (char*)"\tDetector number inconsistent",                       //3
    (char*)"\tSum PHA is 0",                                       //4
    (char*)"\tNumber of segments with PHA  is 0 or larger than 9", //5    
    (char*)"\tundefined error",                                    //6
    (char*)"\tundefined error",                                    //7
    (char*)"\tundefined error",                                    //8
    (char*)"\tundefined error",                                    //9
  };
  for(unsigned short i=0;i<10;i++){
    if(fErrors[i]>0)
      cout << fErrors[i] << errorcode[i] << endl;
  }
}
