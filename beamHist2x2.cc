//g++ -o beamHist beamHist2x2.cc -std=c++1y -O3 -L${BOOST_LIB} -lboost_program_options -I${BOOST_INC} -I${DK2NU}/include -I${ROOT_INC} -L${DK2NU_LIB} -ldk2nuTree `root-config --libs`
#include <iostream>
#include <string>
#include <glob.h>
#include <thread>

#include "TFile.h"
#include "TH1F.h"
#include "TH3F.h"
#include "TChain.h"
#include "TTree.h"
#include "TRandom3.h"
#include "TVector3.h"
#include "TThread.h"

#include "dk2nu/tree/dk2nu.h"
#include "dk2nu/tree/dkmeta.h"
#include "dk2nu/tree/calcLocationWeights.h"

#include <boost/program_options.hpp>

using namespace std;
using namespace boost::program_options;

struct histpackage_t
{
  std::vector<string> filelist;
  int NREDECAY;
  std::vector<double> detpos; 
  double RDet;
  int rndSeed;
  bool countPOT;
  TH3F* hxye;
  TH1F* hFlux[4];
  TH1F* hparent[4][5];
  TH1F* hsec[4][6]; 
  double POT;
};

//thread function (has to be void* since root uses this to determine if it is detached or non-detached thread)  
void* FillHist(void* hp);

int main(int ac, char* av[])
{
  vector<double> detpos;
  detpos.resize(3);
  double RDet;
  string searchpath;
  string outputfn;
  int NREDECAY;
  double userPOT=0;
  int nthread=1;
  options_description opt("Options");
  opt.add_options()
    ("help,h", "Print help message")
    ("input,i",value<string>(&searchpath),"Path pattern for input files. Put it in quotes or escape *.")
    ("output,o",value<string>(&outputfn)->default_value("hist.root"),"Output file name.")
    ("pot,p",value<double>(&userPOT),"POT used for normalization (overides counting using info in meta tree and speeds up process). \nTotal POT should be given (number of files X POT per file).")
    ("nredecays,n",value<int>(&NREDECAY)->default_value(1.),"Number of redecays.")
    ("detector-radius,r",value<double>(&RDet)->default_value(0.),"Detector radius (in cm).")
    ("detector-position,x",value<vector<double> >(&detpos)->multitoken(),"Detector position (in cm).")
    ("thread,t",value<int>(&nthread)->default_value(1),"Number of threads to run. (max set to 8)");
    
  variables_map vm;
  
  try {
    store(parse_command_line(ac,av,opt),vm);
    notify(vm);
    if (vm.count("help")) {
      cerr<<opt<<endl;
      return 1;
    } 
    if (!vm.count("input")) {
      cerr<<"Need to provide input pattern."<<endl;
      cerr<<opt<<endl;
      return 1;
    }
    if (!vm.count("detector-position")) {
      //assume it is 2x2 from xml file, it comes in at a slight angle with a rotatio around x of -0.0582977560 radians but that is negligible as it shifts YZ
      detpos[0]=0;
      detpos[1]=0;
      detpos[2]=103648.837;
    }
  } catch (error& e) {
    cerr << e.what()<<endl<<endl;
    cerr << opt <<endl;
    return 1;
  }
  if (nthread>8) nthread=8;
  for (unsigned int i=3;i<=detpos.size();i++) {
    if (detpos[i]!=0) detpos[i-3]*=-1;
  }
  cout <<"Making histograms for detector at r=("
       <<detpos[0]<<", "
       <<detpos[1]<<", "
       <<detpos[2]<<") cm ";

  cout<<"and smearing over RDet="<<RDet<<" cm"<<endl;

  cout <<"Redecaying "<<NREDECAY<<" times."<<endl;

  glob_t glob_result;
  cout<<"Searching "<<searchpath<<endl;
  glob(searchpath.c_str(),GLOB_TILDE,NULL,&glob_result);
  vector<string> filelist;
  bool streamfiles=false;
  for(unsigned int i=0;i<glob_result.gl_pathc;++i){
    string fname=string(glob_result.gl_pathv[i]);
    //if (fname.find("/pnfs")==0) {
    //  streamfiles=true;
    //  fname.replace(0,5,"root://fndca1.fnal.gov:1094/pnfs/fnal.gov/usr");
    //}
    filelist.push_back(fname);
  }
  globfree(&glob_result);
  cout <<"Found "<<filelist.size()<<" files"<<endl;
  //if (streamfiles)
  //cout<<"Streaming root files (something like "<<filelist[0]<<")"<<endl;
  //  cout<<"Total POT: "<<hp->POT<<endl;
  //cout<<"(with redecaying "<<hp->NREDECAY<<" times)"<<endl;
  //if (dkmetaTree) {
  // cout <<"POT calculated using meta data"<<endl;
  //} else {
  // cout <<"POT set using --pot option."<<endl;
  //}

  cout<<"Starting "<<nthread<<" threads"<<endl;
  TThread::Initialize();
  TThread* t[nthread];
  histpackage_t* hp[nthread];

  //prepare filelists
  std::vector<string> tfl[nthread];
  while (filelist.size()>0) 
    for (int i=0;i<nthread;i++) {
      if (filelist.size()>0) {
	tfl[i].push_back(filelist.back());
	filelist.pop_back();
      }
    }

  for (int i=0;i<nthread;i++) {
    hp[i]=new histpackage_t();
    hp[i]->filelist=tfl[i];
    hp[i]->NREDECAY=NREDECAY;
    hp[i]->detpos=detpos;
    hp[i]->RDet=RDet;
    hp[i]->rndSeed=i;
    if (!vm.count("pot"))
      hp[i]->countPOT=true;
    else 
      hp[i]->countPOT=false;
    t[i]=new TThread(Form("Thread_%i",i),FillHist, (void*) hp[i]);
    t[i]->Run();
  }
  TThread::Ps();
  for (int i=0;i<nthread;i++) {
    t[i]->Join();
  }
  std::cerr<<"Done with threads"<<std::endl;
  //add histograms from all threads
  for (int i=1;i<nthread;i++) {
    hp[0]->hxye->Add(hp[i]->hxye);
    for (int inu=0;inu<4;inu++) {
      hp[0]->hFlux[inu]->Add(hp[i]->hFlux[inu]);
      for (int ipar=0;ipar<4;ipar++) {
	hp[0]->hparent[inu][ipar]->Add(hp[i]->hparent[inu][ipar]);
      }
      for (int isec=0;isec<5;isec++) {
	hp[0]->hsec[inu][isec]->Add(hp[i]->hsec[inu][isec]);
      } 
    }
    hp[0]->POT+=hp[i]->POT;
  }

  double totPOT=hp[0]->POT;
  if (vm.count("pot")) {
    totPOT=userPOT;
    cout <<"POT set using --pot option to "<<totPOT<<endl; 
  } else {
    cout <<"Total POT summed using meta data= "<<totPOT<<endl;
  }
  
  for (int inu=0;inu<4;inu++) {
    hp[0]->hFlux[inu]->Scale(1.1e21/totPOT);
    for (int ipar=0;ipar<4;ipar++) {
      hp[0]->hparent[inu][ipar]->Scale(1.1e21/totPOT);
    }
    for (int isec=0;isec<5;isec++) {
      hp[0]->hsec[inu][isec]->Scale(1.1e21/totPOT);
    } 
  }

  //write histograms to file
  TFile fout(outputfn.c_str(),"RECREATE");
  hp[0]->hxye->Write();
  for (int inu=0;inu<4;inu++) {
    hp[0]->hFlux[inu]->Write();
    for (int ipar=0;ipar<5;ipar++) {
      hp[0]->hparent[inu][ipar]->Write();
    }
  }  
  for (int inu=0;inu<4;inu++) {
    hp[0]->hFlux[inu]->Write(Form("h70%i",inu+1)); //same as h50x, keeping copy 
                                          //to be consistent with MB files
    for (int isec=0;isec<6;isec++) {
      hp[0]->hsec[inu][isec]->Write();
    } 
  }
  fout.Close();

  return 0;
}

void* FillHist(void* hpvoid)
{
  histpackage_t* hp=(histpackage_t*) hpvoid;
  TThread::Lock();
  TChain* dk2nuTree=new TChain("dk2nuTree");
  TChain* dkmetaTree=NULL;
  if (hp->countPOT)
    dkmetaTree=new TChain("dkmetaTree");
  for (auto ifile : hp->filelist) {
    dk2nuTree->Add(ifile.c_str());
    if (dkmetaTree)
      dkmetaTree->Add(ifile.c_str());
  }

  bsim::Dk2Nu*  dk2nu  = new bsim::Dk2Nu;
  dk2nuTree->SetBranchAddress("dk2nu",&dk2nu);
  bsim::DkMeta* dkmeta  = new bsim::DkMeta;
  if (dkmetaTree)
    dkmetaTree->SetBranchAddress("dkmeta",&dkmeta);

  TThread::UnLock();

  Long64_t ientry=0;
  hp->POT=0;
  if (dkmetaTree) {
    while (dkmetaTree->GetEntry(ientry++)) {
      hp->POT+=dkmeta->pots;
    }
  }
  hp->POT*=hp->NREDECAY;
  std::cerr<<"POT thread "<<hp->POT<<std::endl;
  string nutype[]={    "nue",        "nuebar",      "numu",         "numubar"};
  string nultx[] ={"#nu_{e}", "#bar{#nu}_{e}", "#nu_{#mu}", "#bar{#nu}_{#mu}"};
  int pdgcode[]  ={       12,             -12,          14,               -14};
  string pltx[]  ={"#mu^{#pm}","#pi^{#pm}","K^{0}_{L}","K^{#pm}","Other"};
  string secltx[]  ={"pBe->#pi^{#pm}->...->#mu^{#pm}",
		     "pBe->#pi^{#pm}->..(not #mu^{#pm})..",
		     "pBe->K^{0}_{L}->...",
		     "pBe->K^{#pm}->...",
		     "pBe->(p or n)->...",
                     "pBe->Other->..."};


  const int NEBIN=320;
  TRandom3 rndmno;
  rndmno.SetSeed(hp->rndSeed);  
  std::string suffix="";
  if (hp->rndSeed>0) 
    suffix=Form("_%i",hp->rndSeed);
  TThread::Lock();
  cout<<"Picking RDet "<<hp->RDet<<endl;
  //  hp->hxye=new TH3F(Form("h_xyE%s",suffix.c_str()),Form("Neutrino vertices at r=(%f,%f,%f)cm;x (cm);y (cm);E (GeV)",hp->detpos[0],hp->detpos[1],hp->detpos[2]),100,-hp->RDet,hp->RDet,100,-hp->RDet,hp->RDet,NEBIN,0,10);
  hp->hxye=new TH3F(Form("h_xyE%s",suffix.c_str()),Form("Neutrino vertices at r=(%f,%f,%f)cm;x (cm);y (cm);E (GeV)",hp->detpos[0],hp->detpos[1],hp->detpos[2]),120,-hp->RDet,hp->RDet,1,-50,50,NEBIN,0,40); //std::cout<<hp->detpos[0]<<","<<hp->detpos[2]<<std::endl; 
  for (int i=0;i<4;i++) {
    hp->hFlux[i]=new TH1F(Form("h50%i%s",i+1,suffix.c_str()),Form("%s (all);Energy %s (GeV);#phi(%s)/GeV/1.1#times10^{21}POT",nultx[i].c_str(),nultx[i].c_str(),nultx[i].c_str()),NEBIN,0,40);    hp->hFlux[i]->Sumw2();
  }
 
  for (int inu=0;inu<4;inu++) {
    for (int ipar=0;ipar<5;ipar++) {
      hp->hparent[inu][ipar]=new TH1F(Form("h5%i%i%s",ipar+1,inu+1,suffix.c_str()),
				  Form("...->%s->%s;Energy %s (GeV);#phi(%s)/GeV/1.1#times10^{21}POT",pltx[ipar].c_str(),nultx[inu].c_str(),nultx[inu].c_str(),nultx[inu].c_str()),
				  NEBIN,0,40);
   hp->hparent[inu][ipar]->Sumw2();
    }
    for (int isec=0;isec<6;isec++) {
      hp->hsec[inu][isec]=new TH1F(Form("h7%i%i%s",isec+1,inu+1,suffix.c_str()),
			       Form("%s->%s;Energy %s (GeV);#phi(%s)/GeV/1.1#times10^{21}POT",secltx[isec].c_str(),nultx[inu].c_str(),nultx[inu].c_str(),nultx[inu].c_str()),
			       NEBIN,0,40);
      hp->hsec[inu][isec]->Sumw2();
    }
  }
  TThread::UnLock();
  ientry=0;
  cout<<"Thread "<<hp->rndSeed<<" starting to process "<<dk2nuTree->GetNtrees()<<" files."<<endl;
  while (dk2nuTree->GetEntry(ientry++)) {
    //    if (ientry%100000==0) cout<<"Thread "<<hp->rndSeed<<" on entry "<<ientry<<endl;
    for (int ipdg=0;ipdg<4;ipdg++) {
      if (dk2nu->decay.ntype!=pdgcode[ipdg]) continue;
      
      for (int iredecay=0;iredecay<hp->NREDECAY;iredecay++) {
	double enu,wgt_xy;
	double xx;
	double yy;
	double zz=0;

	xx=rndmno.Uniform(-hp->RDet,hp->RDet);
	yy=rndmno.Uniform(-hp->RDet,hp->RDet);
	//yy=rndmno.Uniform(-50,50);
	
	while (sqrt(xx*xx+yy*yy)>hp->RDet) {
	  xx=rndmno.Uniform(-hp->RDet,hp->RDet);
	  yy=rndmno.Uniform(-hp->RDet,hp->RDet);
	}
	
	TVector3 xyz(xx+hp->detpos[0],yy+hp->detpos[1],zz+hp->detpos[2]);
	bsim::calcEnuWgt(dk2nu,xyz,enu,wgt_xy);

	double totwgh=wgt_xy*dk2nu->decay.nimpwt/3.14159;
	//account for bin size and normalize to GeV
	totwgh*=(1./hp->hFlux[0]->GetBinWidth(1));
	
	hp->hxye->Fill(xx,yy,enu,totwgh);
	hp->hFlux[ipdg]->Fill(enu,totwgh);
	
	if (dk2nu->decay.ptype==13 || dk2nu->decay.ptype==-13) //mu+-
	  hp->hparent[ipdg][0]->Fill(enu,totwgh);
	else if (dk2nu->decay.ptype==211 || dk2nu->decay.ptype==-211) //pi+-
	  hp->hparent[ipdg][1]->Fill(enu,totwgh);
	else if (dk2nu->decay.ptype==130) //K0L
	  hp->hparent[ipdg][2]->Fill(enu,totwgh);
	else if (dk2nu->decay.ptype==321 || dk2nu->decay.ptype==-321) //K+-
	  hp->hparent[ipdg][3]->Fill(enu,totwgh);
	else
	  hp->hparent[ipdg][4]->Fill(enu,totwgh);
	int firstInelastic=1;
	//while (dk2nu->ancestor[firstInelastic].proc.find("HadronInelastic")==string::npos) firstInelastic++;
      
	if (fabs(dk2nu->ancestor[firstInelastic].pdg)==211 && fabs(dk2nu->decay.ptype)==13)
	  hp->hsec[ipdg][0]->Fill(enu,totwgh);
	else if (fabs(dk2nu->ancestor[firstInelastic].pdg)==211)
	  hp->hsec[ipdg][1]->Fill(enu,totwgh);
	else if (fabs(dk2nu->ancestor[firstInelastic].pdg)==130)
	  hp->hsec[ipdg][2]->Fill(enu,totwgh);
	else if (fabs(dk2nu->ancestor[firstInelastic].pdg)==321)
	  hp->hsec[ipdg][3]->Fill(enu,totwgh);
	else if (dk2nu->ancestor[firstInelastic].pdg==2212 || dk2nu->ancestor[firstInelastic].pdg==2112)
	  hp->hsec[ipdg][4]->Fill(enu,totwgh);
	else 
	  hp->hsec[ipdg][5]->Fill(enu,totwgh);
      }
    }
  }
 
  cout<<"Thread "<<hp->rndSeed<<" processed "<<ientry<<" entries. POT = "<<hp->POT<<endl;

  return NULL;
}
