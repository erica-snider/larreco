////////////////////////////////////////////////////////////////////////
// Class:       MergeWireCell
// Module Type: producer
// File:        MergeWireCell_module.cc
//
// Generated at Sat Sep  5 12:50:05 2015 by Tingjun Yang using artmod
// from cetpkgsupport v1_08_06.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "RecoBase/Hit.h"
#include "RecoBase/SpacePoint.h"
#include "Geometry/Geometry.h"
#include "Utilities/AssociationUtil.h"

#include "TTree.h"
#include "TFile.h"

#include <memory>
#include <string>
#include <dirent.h>
#include <iostream>

namespace wc {
  class MergeWireCell;
}

class wc::MergeWireCell : public art::EDProducer {
public:
  explicit MergeWireCell(fhicl::ParameterSet const & p);
  // The destructor generated by the compiler is fine for classes
  // without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  MergeWireCell(MergeWireCell const &) = delete;
  MergeWireCell(MergeWireCell &&) = delete;
  MergeWireCell & operator = (MergeWireCell const &) = delete;
  MergeWireCell & operator = (MergeWireCell &&) = delete;

  // Required functions.
  void produce(art::Event & e) override;

  // Selected optional functions.
  void beginJob() override;
  void reconfigure(fhicl::ParameterSet const & p) override;

private:

  // Declare member data here.
  std::string fInput;

  static constexpr float Sqrt2Pi = 2.5066;
  static constexpr float SqrtPi  = 1.7725;

};


wc::MergeWireCell::MergeWireCell(fhicl::ParameterSet const & p) :
  fInput(p.get<std::string>("WireCellInput"))
{
  // Call appropriate produces<>() functions here.
  produces<std::vector<recob::Hit>                    >();
  produces<std::vector<recob::SpacePoint>             >();
  produces<art::Assns<recob::Hit, recob::SpacePoint>  >();
}

void wc::MergeWireCell::produce(art::Event & evt){

  art::ServiceHandle<geo::Geometry> geom;

  int run = evt.run();
  int subrun = evt.subRun();
  int event = evt.id().event();

  std::unique_ptr<std::vector<recob::Hit>> hit_coll(new std::vector<recob::Hit>);
  std::unique_ptr<std::vector<recob::SpacePoint>> spt_coll(new std::vector<recob::SpacePoint>);
  std::unique_ptr<art::Assns<recob::SpacePoint, recob::Hit> >   shassn(new art::Assns<recob::SpacePoint, recob::Hit>);

  std::string path(fInput);
  path=(path+"/");

  DIR *pDIR;
  struct dirent *entry;
  if( (pDIR=opendir(path.c_str())) != NULL ){
     
    while((entry = readdir(pDIR)) != NULL){

      if( strcmp(entry->d_name, ".")==0 || strcmp(entry->d_name, "..")==00) continue;

      std::string filename(entry->d_name);    
      //if((int)filename.find("fhc.root")==-1)
      //continue;

      std::string file = (path + entry->d_name);
      TFile *f = new TFile(file.c_str());
      TTree *Trun = (TTree*)f->Get("Trun");
      int eventNo, runNo, subRunNo;
      Trun->SetBranchAddress("eventNo",&eventNo);
      Trun->SetBranchAddress("runNo",&runNo);
      Trun->SetBranchAddress("subRunNo",&subRunNo);
      if (!Trun->GetEntries()) continue;
      Trun->GetEntry(0);
      //std::cout<<run<<" "<<subrun<<" "<<event<<" "<<runNo<<" "<<subRunNo<<" "<<eventNo<<std::endl;
      if (runNo!=run||eventNo!=event||subRunNo!=subrun) {
	f->Close();
	continue;
      }
      TTree *TC = (TTree*)f->Get("TC");
      Int_t           time_slice;
      Double_t        charge;
      Double_t        xx;
      Double_t        yy;
      Double_t        zz;
      Int_t           u_index;
      Int_t           v_index;
      Int_t           w_index;
      Double_t        u_charge;
      Double_t        v_charge;
      Double_t        w_charge;
      Double_t        u_charge_err;
      Double_t        v_charge_err;
      Double_t        w_charge_err;
      Int_t           tpc_no;
      Int_t           cryostat_no;

      TC->SetBranchAddress("time_slice", &time_slice);
      TC->SetBranchAddress("charge", &charge);
      TC->SetBranchAddress("xx", &xx);
      TC->SetBranchAddress("yy", &yy);
      TC->SetBranchAddress("zz", &zz);
      TC->SetBranchAddress("u_index", &u_index);
      TC->SetBranchAddress("v_index", &v_index);
      TC->SetBranchAddress("w_index", &w_index);
      TC->SetBranchAddress("u_charge", &u_charge);
      TC->SetBranchAddress("v_charge", &v_charge);
      TC->SetBranchAddress("w_charge", &w_charge);
      TC->SetBranchAddress("u_charge_err", &u_charge_err);
      TC->SetBranchAddress("v_charge_err", &v_charge_err);
      TC->SetBranchAddress("w_charge_err", &w_charge_err);
      TC->SetBranchAddress("tpc_no", &tpc_no);
      TC->SetBranchAddress("cryostat_no", &cryostat_no);

      for (int i = 0; i<TC->GetEntries(); ++i){
	TC->GetEntry(i);
	double xyz[3] = {xx,yy,zz};
	double err[3] = {0,0,0};
	spt_coll->push_back(recob::SpacePoint(xyz,err,charge));
	geo::WireID wireu(cryostat_no,tpc_no,0,u_index);
	raw::ChannelID_t chanu = geom->PlaneWireToChannel(wireu);
	geo::WireID wirev(cryostat_no,tpc_no,1,v_index);
	raw::ChannelID_t chanv = geom->PlaneWireToChannel(wirev);
	geo::WireID wirew(cryostat_no,tpc_no,2,w_index);
	raw::ChannelID_t chanw = geom->PlaneWireToChannel(wirew);
	size_t hitStart = hit_coll->size();
//	raw::TDCtick_t start_tick = time_slice*4;
//	raw::TDCtick_t end_tick = (time_slice+1)*4;
//	float peak_time = (time_slice+0.5)*4;
//	float sigma_peak = 2.0;
//	float rms = 2.0;
//	float peak_ampu = u_charge/(Sqrt2Pi*2);
//	float sigma_peak_a
	hit_coll->push_back(recob::Hit(chanu,
				       time_slice*4,
				       (time_slice+1)*4,
				       (time_slice+0.5)*4,
				       2.,
				       2.,
				       u_charge/(Sqrt2Pi*2),
				       u_charge_err/(Sqrt2Pi*2),
				       u_charge,
				       u_charge,
				       u_charge_err,
				       1,
				       0,
				       0,
				       0,
				       geom->View(chanu),
				       geom->SignalType(chanu),
				       wireu));
	hit_coll->push_back(recob::Hit(chanv,
				       time_slice*4,
				       (time_slice+1)*4,
				       (time_slice+0.5)*4,
				       2.,
				       2.,
				       v_charge/(Sqrt2Pi*2),
				       v_charge_err/(Sqrt2Pi*2),
				       v_charge,
				       v_charge,
				       v_charge_err,
				       1,
				       0,
				       0,
				       0,
				       geom->View(chanv),
				       geom->SignalType(chanv),
				       wirev));
	hit_coll->push_back(recob::Hit(chanw,
				       time_slice*4,
				       (time_slice+1)*4,
				       (time_slice+0.5)*4,
				       2.,
				       2.,
				       w_charge/(Sqrt2Pi*2),
				       w_charge_err/(Sqrt2Pi*2),
				       w_charge,
				       w_charge,
				       w_charge_err,
				       1,
				       0,
				       0,
				       0,
				       geom->View(chanw),
				       geom->SignalType(chanw),
				       wirew));
	size_t hitEnd = hit_coll->size();
	util::CreateAssn(*this, evt, *spt_coll, *hit_coll, *shassn, hitStart, hitEnd);
      }
      f->Close();
      break;
    }
    closedir(pDIR);

  }
  evt.put(std::move(spt_coll));
  evt.put(std::move(hit_coll));
  evt.put(std::move(shassn));
}

void wc::MergeWireCell::beginJob()
{
  // Implementation of optional member function here.
}

void wc::MergeWireCell::reconfigure(fhicl::ParameterSet const & p)
{
  // Implementation of optional member function here.
}

DEFINE_ART_MODULE(wc::MergeWireCell)
