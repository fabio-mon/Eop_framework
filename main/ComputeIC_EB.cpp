#include "utils.h"
#include "CfgManager.h"
#include "CfgManagerT.h"
#include "calorimeter.h"
#include "crystal.h"

#include <iostream>
#include <string>

#include "TROOT.h"
#include "TStyle.h"
#include "TFile.h"
#include "TF1.h"
#include "TH1.h"
#include "TH2.h"
#include "TProfile.h"
#include "TCanvas.h"
#include "TGraphErrors.h"
#include "TPaveStats.h"
#include "TLegend.h"
#include "TChain.h"
#include "TVirtualFitter.h"
#include "TLorentzVector.h"
#include "TLatex.h"
#include "TAxis.h"
#include "TMath.h"

using namespace std;

void PrintUsage()
{
  cerr << ">>>>> usage:  ComputeIC_EB --cfg <configFileName> --inputIC <objname> <filename> --Eopweight <objtype> <objname> <filename> --ComputeIC_output <outputFileName> --odd[or --even]" << endl;
  cerr << "               " <<            " --cfg                MANDATORY"<<endl;
  cerr << "               " <<            " --inputIC            OPTIONAL, can be also provided in the cfg"<<endl;
  cerr << "               " <<            " --Eopweight          OPTIONAL, can be also provided in the cfg" <<endl;
  cerr << "               " <<            " --ComputeIC_output    OPTIONAL, can be also provided in the cfg" <<endl;
  cerr << "               " <<            " --odd[or --even]     OPTIONAL" <<endl;
}

int main(int argc, char* argv[])
{
  string cfgfilename="";
  vector<string> ICcfg;
  vector<string> weightcfg;
  string outfilename="";
  string splitstat="";

  for(int iarg=1; iarg<argc; ++iarg)
  {
    if(string(argv[iarg])=="--cfg")
      cfgfilename=argv[iarg+1];
    if(string(argv[iarg])=="--inputIC")
    {
      ICcfg.push_back(argv[iarg+1]);
      ICcfg.push_back(argv[iarg+2]);
    }
    if(string(argv[iarg])=="--Eopweight")
    {
      weightcfg.push_back(argv[iarg+1]);
      weightcfg.push_back(argv[iarg+2]);
      weightcfg.push_back(argv[iarg+3]);
    }
    if(string(argv[iarg])=="--ComputeIC_output")
      outfilename=argv[iarg+1];
    if(string(argv[iarg])=="--odd")
      splitstat="odd";
    if(string(argv[iarg])=="--even")
      splitstat="even";
  }

  if(cfgfilename=="")
  {
    PrintUsage();
    return -1;
  }

  // parse the config file
  CfgManager config;
  config.ParseConfigFile(cfgfilename.c_str());
  
  //define the calorimeter object to easily access to the ntuples data
  calorimeter EB(config);

  //set the options directly given as input to the executable, overwriting, in case, the corresponding ones contained in the cfg
  if(weightcfg.size()>0)
    EB.LoadEopWeight(weightcfg);
  if(ICcfg.size()>0)
    EB.LoadIC(ICcfg);

  //eta, phi boundaries are taken by the calorimeter constructor from configfile 
  float ietamin, ietamax, iphimin, iphimax;
  int Neta, Nphi;
  EB.GetEtaboundaries(ietamin, ietamax);
  EB.GetPhiboundaries(iphimin, iphimax);
  Neta=EB.GetNeta();
  Nphi=EB.GetNphi();

  //define the output 
  if(outfilename == "")
    if(config.OptExist("Output.ComputeIC_output"))
      outfilename = config.GetOpt<string> ("Output.ComputeIC_output");
    else
      outfilename = "IC.root";
  TFile *outFile = new TFile(outfilename.c_str(),"RECREATE");
  TH2D* numerator = new TH2D("numerator","numerator", Nphi, iphimin, iphimax+1, Neta, ietamin, ietamax+1);
  TH2D* denominator = new TH2D("denominator","denominator", Nphi, iphimin, iphimax+1, Neta, ietamin, ietamax+1);
  TH2D* ICpull = new TH2D("ICpull","ICpull", Nphi, iphimin, iphimax+1, Neta, ietamin, ietamax+1);
  TH2D* temporaryIC = new TH2D("temporaryIC","temporaryIC", Nphi, iphimin, iphimax+1, Neta, ietamin, ietamax+1);

  double *numerator1D = new double[Neta*Nphi];
  double *denominator1D = new double[Neta*Nphi]; 
  
  //initialize numerator and denominator
  for( int index=0; index<Neta*Nphi; ++index)
  {
    numerator1D[index]=0;
    denominator1D[index]=0;
  }

  //loop over entries to fill the histo  
  Long64_t Nentries=EB.GetEntries();
  cout<<Nentries<<" entries"<<endl;
  float E,p,eta,IC,weight,regression;
  int iEle, index, ieta, iphi, ietaSeed;
  vector<float>* ERecHit;
  vector<float>* fracRecHit;
  vector<int>*   XRecHit;
  vector<int>*   YRecHit;
  vector<int>*   ZRecHit;
  vector<int>*   recoFlagRecHit;


  //set the iteration start and the increment accordingly to splitstat
  int ientry0=0;
  int ientry_increment=1;
  if(splitstat=="odd")
  {
    ientry0=1;
    ientry_increment=2;
  }
  else
    if(splitstat=="even")
    {
      ientry0=0;
      ientry_increment=2;
    }
  
  for(Long64_t ientry=ientry0 ; ientry<Nentries ; ientry+=ientry_increment)
  {
    if( ientry%100000==0 || (ientry-1)%100000==0)
      std::cout << "Processing entry "<< ientry << "\r" << std::flush;
    EB.GetEntry(ientry);
    for(iEle=0;iEle<2;++iEle)
    {
      if(EB.isSelected(iEle))
      {
	ERecHit=EB.GetERecHit(iEle);
	fracRecHit=EB.GetfracRecHit(iEle);
	XRecHit=EB.GetXRecHit(iEle);
	YRecHit=EB.GetYRecHit(iEle);
	//ZRecHit=EB.GetZRecHit(iEle);
	recoFlagRecHit=EB.GetrecoFlagRecHit(iEle);
	E=EB.GetICEnergy(iEle);
	p=EB.GetPcorrected(iEle);
	//eta=EB.GetEtaSC(iEle);
	ietaSeed=EB.GetietaSeed(iEle);
	weight=EB.GetWeight(ietaSeed,E/p);
	regression=EB.GetRegression(iEle);
	//cout<<"E="<<E<<"\tp="<<p<<"\teta="<<eta<<"\tweight="<<weight<<"\tregression="<<regression<<endl;
	for(unsigned iRecHit=0; iRecHit<ERecHit->size(); ++iRecHit)
	{
	  if(recoFlagRecHit->at(iRecHit) >= 4)
	    continue;
	  ieta=XRecHit->at(iRecHit);
	  iphi=YRecHit->at(iRecHit);
	  index = fromIetaIphito1Dindex(ieta, iphi, Neta, Nphi, ietamin, iphimin);
	  IC=EB.GetIC(index);
	  //cout<<"entry="<<ientry<<endl;
	  //cout<<"ieta="<<ieta<<"\tiphi="<<iphi<<"\tindex="<<index<<"\tIC="<<IC<<endl;
	  //cout<<"ERH="<<ERecHit->at(iRecHit)<<"\tfracRH="<<fracRecHit->at(iRecHit)<<endl;
	  //cout<<"regression="<<regression<<"\tE="<<E<<"\tp="<<p<<"\tweight="<<weight<<endl;
	  //if(E>15. && p>15.)
	  {
	    numerator1D[index]   += ERecHit->at(iRecHit) * fracRecHit->at(iRecHit) * regression * IC / E * p / E * weight;
	    denominator1D[index] += ERecHit->at(iRecHit) * fracRecHit->at(iRecHit) * regression * IC / E * weight;
	    //cout<<"numerator="<<numerator1D[index]<<"\tdenominator="<<denominator1D[index]<<endl;
	    //if(ieta>70) getchar();
	  }

	  //else
	  //  cout<<"[WARNING]: E="<<E<<" and p="<<p<<" for event "<<ientry<<endl;
	}
      }
    }
  }	  

  //fill numerator and denominator histos
  for(int xbin=1; xbin<numerator->GetNbinsX()+1; ++xbin)
    for(int ybin=1; ybin<numerator->GetNbinsY()+1; ++ybin)
    {
      index = fromTH2indexto1Dindex(xbin, ybin, Nphi, Neta);
      numerator->SetBinContent(xbin,ybin,numerator1D[index]);
      denominator->SetBinContent(xbin,ybin,denominator1D[index]);
      if (denominator1D[index]!=0)
      {
	ICpull->SetBinContent(xbin,ybin,numerator1D[index]/denominator1D[index]);	
	temporaryIC->SetBinContent(xbin,ybin, EB.GetIC(index) * numerator1D[index]/denominator1D[index]);	
      }
    }


  //save and close
  outFile->cd();
  numerator->Write();
  denominator->Write();
  ICpull->Write();
  temporaryIC->Write();
  outFile->Close();
  return 0;
}