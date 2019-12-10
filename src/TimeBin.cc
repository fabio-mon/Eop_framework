#include "TimeBin.h"
#include "TMath.h"
#include "FitUtils.h"

using namespace std;

TimeBin::TimeBin():
  runmin_(0),
  runmax_(0),
  lsmin_(0),
  lsmax_(0),
  timemin_(0),
  timemax_(0),
  Nev_(0),
  h_scale_(0)
  {}

TimeBin::TimeBin(const TimeBin &bincopy):
  runmin_       (bincopy.runmin_),
  runmax_       (bincopy.runmax_),
  lsmin_        (bincopy.lsmin_),
  lsmax_        (bincopy.lsmax_),
  timemin_      (bincopy.timemin_),
  timemax_      (bincopy.timemax_),
  Nev_          (bincopy.Nev_)
  {
    for (auto variableindex : bincopy.variablelist_)
      variablelist_[variableindex.first] = variableindex.second;
    if(bincopy.h_scale_)
    {
      h_scale_ = new TH1F(*(bincopy.h_scale_));
      h_scale_->SetDirectory(0);
    }
    else
      h_scale_=0;
  }


TimeBin::~TimeBin()
{
  if(h_scale_)
    delete h_scale_;
}

void TimeBin::AddEvent(const UInt_t &run,const UShort_t &ls, const UInt_t &t)
{

  if(timemax_==0 && timemin_==0)//empty bin
  {
    timemax_=timemin_=t;
    lsmax_=lsmin_=ls;
    runmax_=runmin_=run;
  }
  else
    if(t<timemin_ || run<runmin_ || (run==runmin_ && ls<lsmin_))
    {
      timemin_=t;
      lsmin_=ls;
      runmin_=run;
    }
    else
      if(t>timemax_ || run>runmax_ || (run==runmax_ && ls>lsmax_))
      {
	timemax_=t;
	lsmax_=ls;
	runmax_=run;
      }
  Nev_++;
}

void TimeBin::AddEvent(const TimeBin& other)
{

  if(timemax_==0 && timemin_==0)//empty bin
  {
    timemin_ = other.timemin_;
    timemax_ = other.timemax_; 
    lsmin_=    other.lsmin_;
    lsmax_=    other.lsmax_;
    runmin_=   other.runmin_;
    runmax_=   other.runmax_;
  }
  else
  {
    if(other.timemin_<timemin_ || other.runmin_<runmin_ || (other.runmin_==runmin_ && other.lsmin_<lsmin_))
    {
      timemin_=other.timemin_;
      lsmin_=other.lsmin_;
      runmin_=other.runmin_;
    }
    if(other.timemax_>timemax_ || other.runmax_>runmax_ || (other.runmax_==runmax_ && other.lsmax_>lsmax_))
    {
      timemax_=other.timemax_;
      lsmax_=other.lsmax_;
      runmax_=other.runmax_;
    }
  }

  Nev_ += other.Nev_;
}  

void TimeBin::SetBinRanges(const UInt_t &runmin, const UInt_t &runmax, const UShort_t &lsmin, const UShort_t &lsmax, const UInt_t &timemin, const UInt_t &timemax)
{
  runmin_=runmin;
  runmax_=runmax;
  lsmin_=lsmin;
  lsmax_=lsmax;
  timemin_=timemin;
  timemax_=timemax;
}

void TimeBin::Reset()
{
  runmin_=0;
  runmax_=0;
  lsmin_=0;
  lsmax_=0;
  timemin_=0;
  timemax_=0;
  Nev_=0;
  if(h_scale_)
    h_scale_->Reset();
}

void TimeBin::SetNev(const int &Nev_bin)
{
  Nev_=Nev_bin;
}

TimeBin& TimeBin::operator=(const TimeBin& other)
{
  runmin_       = other.runmin_;
  runmax_       = other.runmax_;
  lsmin_        = other.lsmin_;
  lsmax_        = other.lsmax_;
  timemin_      = other.timemin_;
  timemax_      = other.timemax_;
  Nev_          = other.Nev_;
  for (auto variableindex : other.variablelist_)
    variablelist_[variableindex.first] = variableindex.second;
  if(other.h_scale_)
  {
    h_scale_      = new TH1F(*(other.h_scale_));
    h_scale_ -> SetDirectory(0);
  }
  else
    h_scale_=0;
}

bool TimeBin::operator<(const TimeBin& other) const
{
  /*
  cout<<">> In function TimeBin::operator<"<<endl;
  cout<<">> Comparing"<<endl
      <<"(runmin,runmax,lsmin,lsmax,timemin,timemax)=("
      << runmin_ <<","<< runmax_ <<","<< lsmin_ <<","<< lsmax_ <<","<< timemin_ <<","<< timemax_ <<")"<<endl;
  cout<<"with"<<endl
      << other.runmin_ <<","<< other.runmax_ <<","<< other.lsmin_ <<","<< other.lsmax_ <<","<< other.timemin_ <<","<< other.timemax_ <<")"<<endl;
  */
  if(runmin_ < other.runmax_)
    return true;
  else
    if(runmax_ > other.runmin_)
      return false;
    else
      if(lsmin_ < other.lsmax_)
	return true;
      else
	if(lsmax_ > other.lsmin_)
	      return false;
	else
	  if(timemin_ < other.timemax_)
	    return true;
	  else
	    return false;

}

void TimeBin::BranchOutput(TTree* outtree)
{
  outtree->Branch("runmin",&runmin_);
  outtree->Branch("runmax",&runmax_);
  outtree->Branch("lsmin",&lsmin_);
  outtree->Branch("lsmax",&lsmax_);
  outtree->Branch("timemin",&timemin_);
  outtree->Branch("timemax",&timemax_);
  outtree->Branch("Nev",&Nev_);
  for(map<string,float>::iterator it=variablelist_.begin(); it!=variablelist_.end(); ++it)
  {
    string variablename  = it->first;
    outtree->Branch(variablename.c_str(), &variablelist_[variablename]);
  }
}

void TimeBin::BranchInput(TTree* intree)
{
  intree->SetBranchAddress("runmin",&runmin_);
  intree->SetBranchAddress("runmax",&runmax_);
  intree->SetBranchAddress("lsmin",&lsmin_);
  intree->SetBranchAddress("lsmax",&lsmax_);
  intree->SetBranchAddress("timemin",&timemin_);
  intree->SetBranchAddress("timemax",&timemax_);
  intree->SetBranchAddress("Nev",&Nev_);
  
  //loop over ttree keys to load all the other variables identified by the prefix "scale_"
  TObjArray *branchList = intree->GetListOfBranches();
  int nBranch = intree->GetNbranches();
  for(int ibranch=0; ibranch<nBranch; ibranch++)
  {
    TString branchname = branchList->At(ibranch)->GetName();
    if(branchname.BeginsWith("scale_"))
    {
      branchname.Remove(0,6);//remove "scale_" prefix from the name
      cout<<"branching "<<branchname.Data()<<endl;
      intree->SetBranchAddress(branchname.Data(),&(variablelist_[branchname.Data()])); 
    }
  }

}


bool TimeBin::Match(const UInt_t &run, const UShort_t &ls) const
{
  if(run<runmin_)
    return false;
  if(run>runmax_)
    return false;
  if(run==runmin_ && ls<lsmin_)
    return false;
  if(run==runmax_ && ls>lsmax_)
    return false;
  return true;
}

bool TimeBin::Match(const UInt_t &run, const UShort_t &ls, const UInt_t &time) const
{
  //cout<<"try to match "<<time<<" in "<<"("<<timemin_<<","<<timemax_<<")"<<endl;
  if(time>=timemin_ && time<=timemax_)
    return Match(run,ls);
  return false;
}
	
bool TimeBin::InitHisto( char* name, char* title, const int &Nbin, const double &xmin, const double &xmax)
{
  if(!h_scale_)
  {
    h_scale_=new TH1F(name,title,Nbin,xmin,xmax);
    return true;
  }
  else
    return false;
}

double TimeBin::TimeBin::TemplateFit(TF1* fitfunc)
{
  bool isgoodfit = FitUtils::PerseverantFit(h_scale_, fitfunc);
  if(isgoodfit)
    return fitfunc->GetParameter(1);
  else
    return -999;
}

double TimeBin::GetMean()
{
  if(!h_scale_)
    cerr<<"[ERROR]: histogram is not booked"<<endl;

  return h_scale_->GetMean();
}


double TimeBin::GetMedian()
{
  if(!h_scale_)
    cerr<<"[ERROR]: histogram is not booked"<<endl;

  //The median is just the 0.5 quantile
  Double_t x, q;
  q = 0.5; // 0.5 for "median"
  h_scale_->ComputeIntegral(); // just a precaution
  h_scale_->GetQuantiles(1, &x, &q);
  return x;
}

double TimeBin::TimeBin::GetIntegral(const float &xmin, const float &xmax)
{
  return h_scale_ -> Integral( h_scale_->GetXaxis()->FindBin(xmin) , h_scale_->GetXaxis()->FindBin(xmax) );
}

double TimeBin::TimeBin::GetBinWidth(const int &ibin)
{
  return h_scale_ -> GetBinWidth(ibin);
}

void TimeBin::TimeBin::SetVariable(const std::string &variablename, const float &variablevalue)
{
  cout<<"setting variablelist_["<<variablename<<"]="<<variablevalue<<endl;
  variablelist_[variablename] = variablevalue;
}

void TimeBin::PrintVariables()
{
  for(map<string,float>::iterator it=variablelist_.begin(); it!=variablelist_.end(); ++it)
    cout<<it->first<<endl;

}
