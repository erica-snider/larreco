////////////////////////////////////////////////////////////////////////
//
// \file ShowerReco.cxx
//
// biagio.rossi@lhep.unibe.ch   (FWMK : argoneut specific)
// thomas.strauss@lhep.unibe.ch (ART  : general detector)
//
// andrzej.szelc@yale.edu (port to detector agnostic version)
//
// This algorithm is designed to reconstruct showers
// 
///////////////////////////////////////////////////////////////////////

extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
}

#include <math.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>

// Framework includes
#include "art/Framework/Principal/Event.h" 
#include "fhiclcpp/ParameterSet.h" 
#include "art/Framework/Principal/Handle.h" 
#include "art/Persistency/Common/Ptr.h" 
#include "art/Persistency/Common/PtrVector.h" 
#include "art/Framework/Services/Registry/ServiceHandle.h" 
#include "art/Framework/Services/Optional/TFileService.h" 
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "messagefacility/MessageLogger/MessageLogger.h" 

#include "TMatrixD.h"
#include "TVectorD.h"
#include "TDecompSVD.h"
#include "TH2F.h"
#include "TF1.h"
#include "TFile.h"
#include "TMath.h"
#include "TTree.h"

// LArSoft includes
#include "ShowerFinder/ShowerReco.h"
#include "Geometry/geo.h"
#include "RecoBase/recobase.h"
#include "Utilities/AssociationUtil.h"
#include "Utilities/LArProperties.h"
#include "Utilities/PhysicalConstants.h"
#include "Utilities/DetectorProperties.h"
#include "SummaryData/summary.h"


// ***************** //

//------------------------------------------------------------------------------
shwf::ShowerReco::ShowerReco(fhicl::ParameterSet const& pset)
{
  this->reconfigure(pset);
  produces< std::vector<recob::Shower>                >();
  produces< art::Assns<recob::Shower, recob::Cluster> >();
  produces< art::Assns<recob::Shower, recob::Hit>     >();
}

//------------------------------------------------------------------------------
void shwf::ShowerReco::reconfigure(fhicl::ParameterSet const& pset) 
{
  fClusterModuleLabel = pset.get< std::string >("ClusterModuleLabel");
  fVertexCLusterModuleLabel=pset.get<std::string > ("VertexClusterModuleLabel");
 
  return;
}

//------------------------------------------------------------------------------
shwf::ShowerReco::~ShowerReco()
{
}

namespace shwf {
  struct SortByWire 
  {
    bool operator() (art::Ptr<recob::Hit> const& h1, art::Ptr<recob::Hit> const& h2) const 
    { 
      return (h1->Wire()->RawDigit()->Channel() < h2->Wire()->RawDigit()->Channel());
    }
  };
}

// ***************** //
void shwf::ShowerReco::beginJob()
{



  /** Get Geometry*/
  art::ServiceHandle<geo::Geometry> geo;

  /// \todo the call to geo->Nplanes() assumes this is a single cryostat and single TPC detector
  /// \todo need to generalize to multiple cryostats and TPCs
  fNPlanes = geo->Nplanes();
  fMean_wire_pitch = geo->WirePitch(0,1,0);    //wire pitch in cm

  /**Get TFileService and define output Histograms*/
  art::ServiceHandle<art::TFileService> tfs;

  art::ServiceHandle<util::DetectorProperties> detp;
  ftimetick=detp->SamplingRate()/1000.;
  
  mf::LogInfo("ShowerReco") << "------------ timetick? " << ftimetick;
  
  
  /** Create Histos names*/
  char tit_dedx[128] = {0};
  char tit_h_theta[128] = {0};
  char tit_h_phi_act[128] = {0};
  char tit_h_thet_act[128] = {0};
  char sh_long_tit[128] = {0};
  char sh_tit[128] = {0};
  char shT_tit[128] = {0};
  
  int nbins;
  
  /**Histos for the actual distribution of the angle in 3D*/
  
  sprintf(tit_h_phi_act,"fh_phi_act");
  fh_phi_act = tfs->make<TH1F>(tit_h_phi_act,"3D phi distribution",720,-180., 180.);
  
  sprintf(tit_h_thet_act,"fh_thet_act");
  fh_thet_act = tfs->make<TH1F>(tit_h_thet_act,"3D Theta distribution",720,-180., 180.);
  
  for(unsigned int i = 0; i < fNPlanes; ++i){


    //    sprintf(&tit_dedx[0],"fh_dedx_%.4i_%.4i_%i",i);
    sprintf(&tit_dedx[0],"fh_dedx_._%i",i);
    fh_dedx[i] = tfs->make<TH1F>(tit_dedx,"dEdx vs distance from vertex",120,0, 40);
    
    /**Histos for the angular distribution theta of the shower*/
    sprintf(&tit_h_theta[0],"fh_theta_%i",i);
    fh_theta[i] = tfs->make<TH1F>(tit_h_theta,"Theta distribution",720,-180., 180.);
    
    
    /**Histos for the longitudinal energy distribution of the shower */
    /**number of wires used,min wire,max_wire you need for the anlysis*/
    sprintf(&sh_tit[0],"fsh_nrg1_%i",i);                   
    fsh_nrg[i] = tfs->make<TH1F>(sh_tit,"energy reco",240,0.,240*fMean_wire_pitch);

    /**Histos for the transverse energy distribution of the shower*/
    /**units are ticks most lickely, but how did Biagio get size of it???*/
    sprintf(&shT_tit[0],"fshT_nrg1_%i",i);                   
    fsh_Tnrg[i] = tfs->make<TH1F>(shT_tit,"energy reco",80,-40.,40.);
  
    /**Histos for the Transverse HIT distribution of the shower*/
    nbins = (int)(240*fMean_wire_pitch);
    /**nbins,min wire,max_wire you need for the analysis*/
    sprintf(&sh_long_tit[0],"fsh_long_hit_%i",i);                           
    fsh_long_hit[i] = tfs->make<TH1F>(sh_long_tit,"longitudinal hit reco",nbins, 0., 240*fMean_wire_pitch);
  }

  ftree_shwf =tfs->make<TTree>("ShowerReco","Results");/**All-knowing tree with reconstruction information*/
  
  // ftree_shwf->Branch("ftheta_Mean","std::vector<double>",&fTheta_Mean);
  // ftree_shwf->Branch("ftheta_RMS","std::vector<double>",&fTheta_RMS );
  
  ftree_shwf->Branch("run",&fRun,"run/I");
  ftree_shwf->Branch("subrun",&fSubRun,"subrun/I");
  ftree_shwf->Branch("event",&fEvent,"event/I");
  ftree_shwf->Branch("nplanes",&fNPlanes,"nplanes/I");
  ftree_shwf->Branch("nangles",&fNAngles,"nangles/I");
  
  ftree_shwf->Branch("fthetaN","std::vector<double>",&fThetaN_ang);
  ftree_shwf->Branch("fphiN","std::vector<double>",&fPhiN_ang);
  
  //    ftree_shwf->Branch("fthetaNC","std::vector<double>",&fThetaNC_ang);
  //    ftree_shwf->Branch("fphiNC","std::vector<double>",&fPhiNC_ang);
  
  
  ftree_shwf->Branch("ftotChargeADC","std::vector<double>",&fTotChargeADC);
  ftree_shwf->Branch("ftotChargeMeV","std::vector<double>",&fTotChargeMeV);
  
  
  ftree_shwf->Branch("NPitch","std::vector< std::vector<double> >", &fNPitch);
  //  ftree_shwf->Branch("Pitch","std::vector<double>", &fPitch);
  
  // this should be temporary - until the omega is sorted out.
  ftree_shwf->Branch("RMS_2cm","std::vector<double>",&fRMS_2cm);
  ftree_shwf->Branch("Npoints_2cm","std::vector<int>",&fNpoints_2cm);
  ftree_shwf->Branch("RMS_4cm","std::vector<double>",&fRMS_4cm);
  ftree_shwf->Branch("Npoints_4cm","std::vector<int>",&fNpoints_4cm);
  
  ftree_shwf->Branch("Corr_MeV_2cm","std::vector<double>",&fCorr_MeV_2cm);
  ftree_shwf->Branch("Corr_Charge_2cm","std::vector<double>",&fCorr_Charge_2cm);
  ftree_shwf->Branch("Corr_Charge_4cm","std::vector<double>",&fCorr_Charge_4cm);
  ftree_shwf->Branch("Corr_MeV_4cm","std::vector<double>",&fCorr_MeV_4cm);
  
  ftree_shwf->Branch("Npoints_corr_ADC_2cm","std::vector<int>",&fNpoints_corr_ADC_2cm);
  ftree_shwf->Branch("Npoints_corr_ADC_4cm","std::vector<int>",&fNpoints_corr_ADC_4cm);
  ftree_shwf->Branch("Npoints_corr_MeV_2cm","std::vector<int>",&fNpoints_corr_MeV_2cm);
  ftree_shwf->Branch("Npoints_corr_MeV_4cm","std::vector<int>",&fNpoints_corr_MeV_4cm);
  
  
  ftree_shwf->Branch("ChargeADC_2cm","std::vector<double>",&fChargeADC_2cm);
  ftree_shwf->Branch("ChargeMeV_2cm","std::vector<double>",&fChargeMeV_2cm);
  ftree_shwf->Branch("ChargeADC_4cm","std::vector<double>",&fChargeADC_4cm);
  ftree_shwf->Branch("ChargeMeV_4cm","std::vector<double>",&fChargeMeV_4cm);
  ftree_shwf->Branch("ChargeADC_6cm","std::vector<double>",&fChargeADC_6cm);
  ftree_shwf->Branch("ChargeMeV_6cm","std::vector<double>",&fChargeMeV_6cm);
  ftree_shwf->Branch("ChargeADC_8cm","std::vector<double>",&fChargeADC_8cm);
  ftree_shwf->Branch("ChargeMeV_8cm","std::vector<double>",&fChargeMeV_8cm);
  ftree_shwf->Branch("ChargeADC_10cm","std::vector<double>",&fChargeADC_10cm);
  ftree_shwf->Branch("ChargeMeV_10cm","std::vector<double>",&fChargeMeV_10cm);
  
  
  ftree_shwf->Branch("ChargedistributionADC","std::vector<std::vector<double>>",&fDistribChargeADC);
  
  ftree_shwf->Branch("ChargedistributionMeV","std::vector<std::vector<double>>",&fDistribChargeMeV);
  
  ftree_shwf->Branch("ChargedistributionPosition","std::vector<std::vector<double>>",&fDistribChargeposition);
  
}

// ***************** //
void shwf::ShowerReco::produce(art::Event& evt)
{ 

  /// \todo THESE VALUES SHOULD BE PARAMETERS OF THE MODULE, evtl from database
  //double Efield_SI        =  0.7;     // Electric Field between Shield and Induction planes in kV/cm
  //double Efield_IC        =  0.9;     // Electric Field between Induction and Collection planes in kV/cm
  //double Temperature      = 87.6;  // LAr Temperature in K
  //check if there can be a replacement later for the product, not needed now
  //double timepitch        = fdriftvelocity*ftimetick;                  //time sample (cm)

  /** Get Geometry */
  art::ServiceHandle<geo::Geometry> geo;
  //art::ServiceHandle<util::LArProperties> larprop;

  ///\todo:  Probably need to use the cryostat and TPC objects to 
  ///\todo:  get number of planes more reliably.  ie
  ///\todo:  fNPlanes = geo->Cryostat(cstat).TPC(tpc).Nplanes();
  fNPlanes = geo->Nplanes();
  //fdriftvelocity = larprob->DriftVelocity(Efield_SI,Temperature);
  
  //calculate factorial for number of angles
  int fact=1;
  for (unsigned int i = 1; i <= fNPlanes; ++i) fact *= i;
  
  fNAngles=fact/2;

  fPhiN.resize(0);
  fThetaN.resize(0);
 
  fPhiN_ang.resize(0);
  fThetaN_ang.resize(0);
  
  fDistribChargeADC.resize(fNPlanes); 
  fDistribChargeMeV.resize(fNPlanes);
  fDistribChargeposition.resize(fNPlanes);
  
  fNPitch.resize(fNAngles); 
  
  for(unsigned int ii = 0; ii < fNAngles; ++ii) {
    fNPitch[ii].resize(fNPlanes,-1);
  }
  
  fNpoints_corr_ADC_2cm.resize(fNAngles,-1);
  fNpoints_corr_MeV_2cm.resize(fNAngles,-1);
  fNpoints_corr_ADC_4cm.resize(fNAngles,-1);
  fNpoints_corr_MeV_4cm.resize(fNAngles,-1);
     
 
 
  for(unsigned int ii = 0; ii < fNPlanes; ++ii){  
    fDistribChargeADC[ii].resize(0);       //vector with the first De/Dx points
    fDistribChargeMeV[ii].resize(0);       //vector with the first De/Dx points
    fDistribChargeposition[ii].resize(0);  //vector with the first De/Dx points' positions 
  }
  
  
  // fPitch.resize(fNPlanes); 
  //fNPitch.resize(fNPlanes,-1); 
  fWire_vertex.resize(fNAngles,-1);
  fTime_vertex.resize(fNAngles,-1);
  fWire_last.resize(fNAngles,-1);
  fTime_last.resize(fNAngles,-1);
  fTotChargeADC.resize(fNAngles,0); 
  fTotChargeMeV.resize(fNAngles,0);  
  //  fChannel_vertex.resize(fNPlanes,-1);
  //  fChannel_last.resize(fNPlanes,-1);
  fRMS_2cm.resize(fNAngles,0); 
  fNpoints_2cm.resize(fNAngles,0); 
  
  fRMS_4cm.resize(fNAngles,0);  
  fNpoints_4cm.resize(fNAngles,0); 
  
  fCorr_MeV_2cm.resize(fNAngles,0); 
  fCorr_Charge_2cm.resize(fNAngles,0);
  
  fCorr_MeV_4cm.resize(fNAngles,0); 
  fCorr_Charge_4cm.resize(fNAngles,0);
  
  
  
  fChargeADC_2cm.resize(fNAngles,0);   //Initial charge in ADC/cm for each plane angle calculation 4cm
  fChargeMeV_2cm.resize(fNAngles,0);   //initial charge in MeV/cm for each angle calculation first 4cm
  fChargeADC_4cm.resize(fNAngles,0);   //Initial charge in ADC/cm for each plane angle calculation 4cm
  fChargeMeV_4cm.resize(fNAngles,0);   //initial charge in MeV/cm for each angle calculation first 4cm
  fChargeADC_6cm.resize(fNAngles,0);   //Initial charge in ADC/cm for each angle calculation first 6cm
  fChargeMeV_6cm.resize(fNAngles,0);   //initial charge in MeV/cm for each angle calculation first 6cm
  fChargeADC_8cm.resize(fNAngles,0);   //Initial charge in ADC/cm for each angle calculationfirst 8cm
  fChargeMeV_8cm.resize(fNAngles,0);   //initial charge in MeV/cm for each angle calculation first 8cm
  fChargeADC_10cm.resize(fNAngles,0);  //Initial charge in ADC/cm for each angle calculation first 10cm
  fChargeMeV_10cm.resize(fNAngles,0);  //initial charge in MeV/cm for each angle calculation first 10cm

  /**Get Clusters*/
  mf::LogInfo("ShowerReco") << "************ What I'm getting out " << fClusterModuleLabel;

  art::Handle< std::vector<recob::Cluster> > clusterListHandle;
  evt.getByLabel(fClusterModuleLabel,clusterListHandle);

  art::FindManyP<recob::Hit> fmh(clusterListHandle, evt, fClusterModuleLabel);

  std::vector< std::vector< art::Ptr<recob::Hit> > > hitlist_all;
  //art::PtrVector < recob::Hit> hitlistInd;
  hitlist_all.resize(fNPlanes);
  
  
  //std::auto_ptr<std::vector<recob::Shower> > Shower3DVector(new std::vector<recob::Shower>);
  
  for(size_t ii = 0; ii < clusterListHandle->size(); ++ii){
    
    art::Ptr<recob::Cluster> cl(clusterListHandle, ii);
    
    //get vertex position and slope information to start with:
    
    std::vector< art::Ptr<recob::Hit> > hitlist = fmh.at(ii);
    std::sort(hitlist.begin(), hitlist.end(), shwf::SortByWire());
    unsigned int p(0),w(0), c(0), t(0), cs(0); //c=channel, p=plane, w=wire
    
    
    if(hitlist.size() == 0) continue;
      
    geo->ChannelToWire((*hitlist.begin())->Wire()->RawDigit()->Channel(),cs, t, p, w);
    
    GetVertexAndAnglesFromCluster( cl,p);
    
      
      
    //loop over cluster hits
    for(art::PtrVector<recob::Hit>::const_iterator a = hitlist.begin(); a != hitlist.end();  a++){
      c=(*a)->Wire()->RawDigit()->Channel(); 
      geo->ChannelToWire(c,cs,t,p,w);
      
      hitlist_all[p].push_back(*a);
      
    }

  } // End loop on clusters.

  fRun = evt.id().run();
  fSubRun = evt.id().subRun();
  fEvent = evt.id().event();
  
  
  
  for(unsigned int i = 0; i < fNPlanes; ++i){
    std::sort(hitlist_all[i].begin(), hitlist_all[i].end(),shwf::SortByWire());
    // Get2DVariables(hitlist_all[i]);
  }
 
 
  
  /// \todo this has to be plane independent
  // Get3Daxis(fOmega_Mean[1], fOmega_Mean[0], fWire_vertex[1], fWire_vertex[0], fTime_vertex[0]);
  unsigned int set=0;
  for(unsigned int ii = 0; ii < fNPlanes-1; ++ii)
    for(unsigned int ij = ii+1; ij < fNPlanes; ++ij) 
      Get3DaxisN(set++,ii,ij);
  
    
  /* Get3DaxisN(0,1);
     if(fNPlanes>2)
     {	
     Get3DaxisN(0,2);
     Get3DaxisN(1,2);
     }*/	
  
  // Get3Daxis_coords();  
  
  
  for(unsigned int i = 0; i < fNAngles; ++i){
    fTotChargeADC[i]=0;
    fTotChargeMeV[i]=0;
    
    fNpoints_corr_ADC_2cm[i]=0;
    fNpoints_corr_MeV_2cm[i]=0;
    fNpoints_corr_ADC_4cm[i]=0;
    fNpoints_corr_MeV_4cm[i]=0;
     
	
    fRMS_2cm[i]=0; 
    fNpoints_2cm[i]=0; 

    fRMS_4cm[i]=0;  
    fNpoints_4cm[i]=0; 

    fCorr_MeV_2cm[i]=0; 
    fCorr_Charge_2cm[i]=0;

    fCorr_MeV_4cm[i]=0; 
    fCorr_Charge_4cm[i]=0;	
    //  fChannel_vertex.resize(fNPlanes,-1);
    //  fChannel_last.resize(fNPlanes,-1);
    fChargeADC_4cm[i]=0;  //Initial charge in ADC/cm for each plane angle calculation 4cm
    fChargeMeV_4cm[i]=0;  //initial charge in MeV/cm for each angle calculation first 4cm	
    
    fChargeADC_2cm[i]=0;  //Initial charge in ADC/cm for each plane angle calculation 4cm
    fChargeMeV_2cm[i]=0;  //initial charge in MeV/cm for each angle calculation first 4cm
    fChargeADC_4cm[i]=0;  //Initial charge in ADC/cm for each plane angle calculation 4cm
    fChargeMeV_4cm[i]=0;  //initial charge in MeV/cm for each angle calculation first 4cm
    fChargeADC_6cm[i]=0;  //Initial charge in ADC/cm for each angle calculation first 6cm
    fChargeMeV_6cm[i]=0;  //initial charge in MeV/cm for each angle calculation first 6cm
    fChargeADC_8cm[i]=0;  //Initial charge in ADC/cm for each angle calculationfirst 8cm
    fChargeMeV_8cm[i]=0;  //initial charge in MeV/cm for each angle calculation first 8cm
    fChargeADC_10cm[i]=0; //Initial charge in ADC/cm for each angle calculation first 10cm
    fChargeMeV_10cm[i]=0; //initial charge in MeV/cm for each angle calculation first 10cm
    
  }


  for(unsigned int set = 0; set < fNAngles; ++set)  
    LongTransEnergy(set,hitlist_all[2]); //temporary only plane 2. Do not use for ArgoneuT
  
  // LongTransEnergy(hitlistCol); //Longitudinal and Transverse energy profile of the Shower induction
  /// \todo this has to be plane independent 
  
  
  //////create spacepoints, and direction cosines for Shower creation
  
  //std::vector< recob::SpacePoint > 	spacepoints = std::vector<recob::SpacePoint>()	
  

         
  // make an art::PtrVector of the clusters
  art::PtrVector<recob::Cluster> prodvec;
  for(unsigned int i = 0; i < clusterListHandle->size(); ++i){
    art::Ptr<recob::Cluster> prod(clusterListHandle, i);
    prodvec.push_back(prod);
  }

  //create a singleSpacePoint at vertex.
  std::vector< recob::SpacePoint > spcpts;
  
  //art::PtrVector< recob::Hit > hits;
  
  //for(int pl=0;pl<fNPlanes;pl++)
  //{
  //for each plane create or find hit closest to the vertex... 
  //recob::Hit  sinhit(art::Ptr< recob::Wire > &wire, 0., 0., 0., 0., fTime_vertex[pl], 0., 0., 0., 0., 0., 0., 0.)
  
  //}
  
  //get direction cosines and set them for the shower  
  // TBD determine which angle to use for the actual shower
  double fPhi=fPhiN_ang[0];
  double fTheta=fThetaN_ang[0];
  
  double dcosVtx[3]={TMath::Cos(fPhi*pi/180)*TMath::Sin(fTheta*pi/180),
		     TMath::Cos(fTheta*pi/180),
		     TMath::Sin(fPhi*pi/180)*TMath::Sin(fTheta*pi/180)};
  /// \todo really need to determine the values of the arguments of the recob::Shower ctor
  // fill with bogus values for now
  double dcosVtxErr[3] = { util::kBogusD };
  double maxTransWidth[2] = { util::kBogusD };
  double distMaxWidth = util::kBogusD;
  
  recob::Shower  singShower(dcosVtx, dcosVtxErr, maxTransWidth, distMaxWidth, 1);
  
  std::auto_ptr<std::vector<recob::Shower> > Shower3DVector(new std::vector<recob::Shower>);
  std::auto_ptr< art::Assns<recob::Shower, recob::Cluster> > cassn(new art::Assns<recob::Shower, recob::Cluster>);
  std::auto_ptr< art::Assns<recob::Shower, recob::Hit>     > hassn(new art::Assns<recob::Shower, recob::Hit>);
  
  Shower3DVector->push_back(singShower);
    
  // associate the shower with its clusters
  util::CreateAssn(*this, evt, *Shower3DVector, prodvec, *cassn);
  
  // get the hits associated with each cluster and associate those with the shower
  for(size_t p = 0; p < prodvec.size(); ++p){
    std::vector< art::Ptr<recob::Hit> > hits = fmh.at(p);
    util::CreateAssn(*this, evt, *Shower3DVector, hits, *hassn);
  }


  //


  /**Fill the output tree with all information */
  ftree_shwf->Fill();
  
  //for(unsigned int iplane = 0; iplane < fNPlanes; ++iplane)
  //fh_theta[iplane]->Write(Form("fh_theta_%d_%d",iplane,evt.id().event()));
  // This needs work, clearly.  
  //for(int p=0;p<2;p++)Shower3DVector->push_back(shower);
  evt.put(Shower3DVector);
  evt.put(cassn);
  evt.put(hassn);

}



// void shwf::ShowerReco::Get2DVariables(art::PtrVector < recob::Hit> hitlist) {  
// 
//   art::ServiceHandle<geo::Geometry> geom;
//   // only needed for drawing the axis of the shower in the event display
//   
//   unsigned int channel;
//   double  wire_cm, time_cm;
// 
// 
//  double AC, BC, omega; 
//   double time;
//   unsigned int wire,plane;
// 
// for(art::PtrVector<recob::Hit>::const_iterator hitIter = hitlist.begin(); hitIter != hitlist.end();  hitIter++){
//     art::Ptr<recob::Hit> theHit = (*hitIter);
//     time = theHit->PeakTime() ;  
//     //time_C -= (presamplings+10.1);
//     art::Ptr<recob::Wire> theWire = theHit->Wire();
//     channel = theWire->RawDigit()->Channel();
//     geom->ChannelToWire(channel, fTPC, plane, wire);
//     //    if(time_C<1020)continue;
//     wire_cm = wire * fMean_wire_pitch; //in cm
//     time_cm = time *ftimetick*fdriftvelocity; //in cm
//    
//  /*  if(hitIter == hitlist.begin())
// 	{ fWire_vertex[plane] = fWire_vertex[plane] ; //in cm
//           fTime_vertex[plane] = fTime_vertex[plane] ; //in cmm        
//         }  */  
// 
//    
//     // moving to polar coordinates
//     BC = (wire_cm - fWire_vertex[plane]* fMean_wire_pitch)+fMean_wire_pitch; //in cm
//     AC = (time_cm - fTime_vertex[plane]*ftimetick*fdriftvelocity); // in cm 
//     omega = asin(AC/sqrt(pow(AC,2)+pow(BC,2)));
//     omega = 180*omega/3.14; // in deg
//     //mf::LogInfo("ShowerReco") << " WireI1=" << wireI1 << " BI= " << BI << "    ThetaI = " << thetaI <<std::endl;
//        
//     if( (omega>(tan(slope[plane])-1.0*pi/180.))&&(omega<(tan(slope[plane])-1.0*pi/180.)) ){
//       fWire_last[plane] = wire;
//       fChannel_last[plane]=  channel;  // wire coordinate of vertex for each plane
//       fTime_last[plane] = time;
//     }
// 
// }   // end of HitIter loop
// 
// 
// 
// 
// 
//   
//   // Making the two lines in the two views
// for (unsigned int pl=0;pl<fNPlanes;pl++)
//   {
//   slope_wt[pl] = (fTime_last[pl]-fTime_vertex[pl])/(ftimetick*fdriftvelocity)/((fWire_last[pl]-fWire_vertex[pl])/ fMean_wire_pitch);
//   //slope_wt[0] = (fTime_last[0]-Time_C_wt)/(fWire_last[0]-Wire_vertexC_wt);
//  
//   intercept_wt[pl] = fTime_vertex[pl]/(ftimetick*fdriftvelocity) - fWire_vertex[pl]/(fMean_wire_pitch)*slope_wt[pl];
// //  intercept_wt[0] = Time_C_wt - Wire_vertexC_wt*slope_wt[0];
//   }
//   
//   return (void)0;
// }


//------------------------------------------------------------------------------
int shwf::ShowerReco::Get3DaxisN(unsigned int set,int iplane0,int iplane1){

 double l(0),m(0),n(0);
 double angle[3];
 art::ServiceHandle<geo::Geometry> geom;
 
 // pretend collection and induction planes. 
 // "Collection" is the plane with the vertical angle equal to zero. 
 // If both are non zero collection is the one with the negative angle. 
 unsigned int Cplane=0,Iplane=1;   
 
 // don't know how to reconstruct these yet, so exit with error.
 if(slope[iplane0]==0 || slope[iplane1]==0){
   fPhiN_ang.push_back(-900);
   fThetaN_ang.push_back(-900);
   mf::LogInfo("ShowerReco") << " event parallell in one of the planes, exiting ";
   return 0;
 }
 
 //////////insert check for existence of planes.
 
 angle[iplane0]=geom->Plane(iplane0).Wire(0).ThetaZ(false)-TMath::Pi()/2.; // wire angle with respect to 
 angle[iplane1]=geom->Plane(iplane1).Wire(0).ThetaZ(false)-TMath::Pi()/2.; // wire angle with respect to
 
 mf::LogInfo("ShowerReco") <<  " actual angles " << angle[iplane0]*180/pi << " " 
			   << angle[iplane1]*180/pi;
 
 if(angle[iplane0] == 0){   
   // first plane is at 0 degrees
   Cplane=iplane0;
   Iplane=iplane1;
   mf::LogInfo("ShowerReco") << "+++++ new calc first test case 1 angle[0]==0 ";
 }
 else if(angle[iplane1] == 0){  
   // second plane is at 0 degrees
    
   Cplane = iplane1;
   Iplane = iplane0;
   mf::LogInfo("ShowerReco") << "+++++ new calc first test case 2 angle[1]==0 ";
 }
 else if(angle[iplane0] != 0 && angle[iplane1] != 0){
   //both planes are at non zero degree - find the one with deg<0
   if(angle[iplane1] < angle[iplane0]){
     Cplane = iplane1;
     Iplane = iplane0;
     mf::LogInfo("ShowerReco") << "+++++ new calc first test case 3 angle[1]< angle[0] ";
   }
   else if(angle[iplane1] > angle[iplane0]){
     Cplane = iplane0;
     Iplane = iplane1;
     mf::LogInfo("ShowerReco") << "+++++ new calc first test case 4 angle[1]> angle[0] ";
   }
   else{
     //throw error - same plane.
     return -1;
   }	

 }


 //0 -1 factor depending on if one of the planes is vertical.
 bool nfact = !(angle[Cplane]);


 l = 1;

 mf::LogInfo("ShowerReco") << "+++++ new calc first test c(I) ,I,C,I/C " 
			   << cos(angle[Iplane]) << " " <<  cos(angle[Cplane]) 
			   << " " << cos(angle[Iplane])/cos(angle[Cplane]) 
			   <<" nfact  " << nfact << " sines, I,C " << sin(angle[Iplane]) 
			   << " " << sin(angle[Cplane]);

 m = (1/(2*sin(angle[Iplane])))*((cos(angle[Iplane])/(slope[Cplane]*cos(angle[Cplane])))-(1/slope[Iplane]) 
				 +nfact*(  cos(angle[Iplane])/slope[Cplane]-1/slope[Iplane]  )     );
 
 
 n = (1/(2*cos(angle[Cplane])))*((1/slope[Cplane])+(1/slope[Iplane])
				 +nfact*((1/slope[Cplane])-(1/slope[Iplane])));
 
 // Direction angles
 fPhiN.push_back( atan(n/l));
 fThetaN.push_back( acos(m/(sqrt(pow(l,2)+pow(m,2)+pow(n,2)))) );
 
 
 mf::LogInfo("ShowerReco") << "+++++ new calc first test angles tests, Phi, Thet: " 
			   << fPhiN[fPhiN.size()-1]*180/pi << " " 
			   << fThetaN[fThetaN.size()-1]*180/pi;

 // solve the ambiguities due to tangent periodicity
 float Phi = fPhiN[fPhiN.size()-1]>0. ? (TMath::Pi()/2)-fPhiN[fPhiN.size()-1] : fabs(fPhiN[fPhiN.size()-1])-(TMath::Pi()/2) ; 
 float Theta = 0;
 if(Phi < 0)Theta = (TMath::Pi()/2)-fThetaN[fThetaN.size()-1];
 if(Phi > 0)Theta = fThetaN[fThetaN.size()-1]-(TMath::Pi()/2);
 
 mf::LogInfo("ShowerReco") << " NPhi=" <<Phi*180/pi << "   Ntheta=" << Theta*180/pi;
 
 
 fh_phi_act->Fill(Phi*180/pi);
 
 fh_thet_act->Fill(Theta*180/pi);
 
 GetPitchLength(set); //Get pitch of (two) wire planes 
 
 fPhiN_ang.push_back(Phi*180/pi);
 fThetaN_ang.push_back(Theta*180/pi);
 
 return 0;
 
}

// int shwf::ShowerReco::Get3Daxis_coords(){
// 
//  art::ServiceHandle<geo::Geometry> geom;
// 
// int nvertices=0;
// 
// if(fNPlanes==2)
//    nvertices=1;
// if(fNPlanes==3)
//    nvertices=3;
// 
// fYvertex.resize(0);fZvertex.resize(0);
// fXvertex.resize(0);fZlast.resize(0);
// fYlast.resize(0);fXlast.resize(0);
// 
// //for(int iplane=0;nvertices<;iplane++)
// //   {
// double y,z;
// bool wires_cross = geom->ChannelsIntersect(fChannel_vertex[0],fChannel_vertex[1],y,z);
// fYvertex.push_back(y);fZvertex.push_back(z);
// wires_cross = geom->ChannelsIntersect(fChannel_last[0],fChannel_last[1],y,z);
// fYlast.push_back(y);fZlast.push_back(z);
// 
// fXvertex.push_back(fTime_vertex[0]*ftimetick*fdriftvelocity);
// fXlast.push_back(fTime_last[0]*ftimetick*fdriftvelocity ); 
// 
// TVector3 XYZ0;  // track origin or interaction vertex
// XYZ0.SetXYZ(fXlast[0]-fXvertex[0],fYlast[0]-fYvertex[0],fZlast[0]-fZvertex[0]);
// 
// mf::LogInfo("ShowerReco") << "^^^^^ channels " << fChannel_vertex[0] << " " << fChannel_vertex[1] << " " << fChannel_last[0] << " " << fChannel_last[1] << std::endl;
// mf::LogInfo("ShowerReco") << "^^^^^ X,Y,Z start " << fXvertex[0] << " " << fYvertex[0] << " " << fZvertex[0] << std::endl;
// mf::LogInfo("ShowerReco") << "^^^^^ X,Y,Z end " << fXlast[0] << " " << fYlast[0] << " " << fZlast[0] << std::endl;
// mf::LogInfo("ShowerReco") << "^^^^^ X,Y,Z diff " << fXlast[0]-fXvertex[0] << " " << fYlast[0]-fYvertex[0] << " " << fZlast[0]-fZvertex[0] << std::endl;
// mf::LogInfo("ShowerReco") << "^^^^^ angle calculation  theta: " << XYZ0.Theta() << " " << XYZ0.Phi() << std::endl; 
// 
// 
// 
// wires_cross = geom->ChannelsIntersect(fChannel_vertex[0],fChannel_vertex[2],y,z);
// fYvertex.push_back(y);fZvertex.push_back(z);
// wires_cross = geom->ChannelsIntersect(fChannel_last[0],fChannel_last[2],y,z);
// fYlast.push_back(y);fZlast.push_back(z);
// 
// fXvertex.push_back(fTime_vertex[0]*ftimetick*fdriftvelocity);
// fXlast.push_back(fTime_last[0]*ftimetick*fdriftvelocity ); 
// 
// TVector3 XYZ1;  // track origin or interaction vertex
// XYZ0.SetXYZ(fXlast[1]-fXvertex[1],fYlast[1]-fYvertex[1],fZlast[1]-fZvertex[1]);
// 
// 
// mf::LogInfo("ShowerReco") << "^^^^^ channels " << fChannel_vertex[0] << " " << fChannel_vertex[2] << " " << fChannel_last[0] << " " << fChannel_last[2] << std::endl;
// mf::LogInfo("ShowerReco") << "^^^^^ X,Y,Z start " << fXvertex[1] << " " << fYvertex[1] << " " << fZvertex[1] << std::endl;
// mf::LogInfo("ShowerReco") << "^^^^^ X,Y,Z end " << fXlast[1] << " " << fYlast[1] << " " << fZlast[1] << std::endl;
// mf::LogInfo("ShowerReco") << "^^^^^ X,Y,Z diff " << fXlast[1]-fXvertex[1] << " " << fYlast[1]-fYvertex[1] << " " << fZlast[1]-fZvertex[1] << std::endl;
// mf::LogInfo("ShowerReco") << "^^^^^ angle calculation  theta: " << XYZ1.Theta() << " " << XYZ1.Phi() << std::endl; 
// 
// 
// 
// wires_cross = geom->ChannelsIntersect(fChannel_vertex[1],fChannel_vertex[2],y,z);
// fYvertex.push_back(y);fZvertex.push_back(z);
// wires_cross = geom->ChannelsIntersect(fChannel_last[1],fChannel_last[2],y,z);
// fYlast.push_back(y);fZlast.push_back(z);
// 
// fXvertex.push_back(fTime_vertex[0]*ftimetick*fdriftvelocity);
// fXlast.push_back(fTime_last[0]*ftimetick*fdriftvelocity ); 
// 
// 
// TVector3 XYZ2;  // track origin or interaction vertex
// XYZ0.SetXYZ(fXlast[2]-fXvertex[2],fYlast[2]-fYvertex[2],fZlast[2]-fZvertex[2]);
// 
// mf::LogInfo("ShowerReco") << "^^^^^ channels " << fChannel_vertex[1] << " " << fChannel_vertex[2] << " " << fChannel_last[1] << " " << fChannel_last[2] << std::endl;
// mf::LogInfo("ShowerReco") << "^^^^^ X,Y,Z start " << fXvertex[2] << " " << fYvertex[2] << " " << fZvertex[2] << std::endl;
// mf::LogInfo("ShowerReco") << "^^^^^ X,Y,Z end " << fXlast[2] << " " << fYlast[2] << " " << fZlast[2] << std::endl;
// mf::LogInfo("ShowerReco") << "^^^^^ X,Y,Z diff " << fXlast[2]-fXvertex[2] << " " << fYlast[2]-fYvertex[2] << " " << fZlast[2]-fZvertex[2] << std::endl;
// mf::LogInfo("ShowerReco") << "^^^^^ angle calculation  theta: " << XYZ2.Theta() << " " << XYZ2.Phi() << std::endl; 
// 
// 
//  //  }
// 
// return 0;
// }
 
// int shwf::ShowerReco::Get3Daxis(float thetaI, float thetaC, float Wire_vertexI, float Wire_vertexC, float Time_vertex){
//   
//   //Get theta and phi (polar angles "direction of the shower")
// 
//   //float ftimetick = 0.198;    //time sample in microsec
//  
//   slope[1] = tan((thetaI*pi/180));
//   slope[0] = tan((thetaC*pi/180));
// 
// 
// mf::LogInfo("ShowerReco") << "$$$$$$$$$$$ slopes " <<  slope[0] << " " << slope[1] << "  " << thetaI <<" "<<thetaC << std::endl << std::endl;
// 
// /*  Wire_vertexI = Wire_vertexI *fMean_wire_pitch; // in cm 
//   Wire_vertexC = Wire_vertexC *fMean_wire_pitch; // in cm
//   Time_vertex  = Time_vertex  *ftimetick*fdriftvelocity; // in cm*/
// 
//   intercept[1] = Time_vertex - (slope[1]*Wire_vertexI);
//   intercept[0] = Time_vertex - (slope[0]*Wire_vertexC);
// 
//  
//   float l(0),m(0),n(0);
//   // Get Geometry
//   art::ServiceHandle<geo::Geometry> geom;
//  
//   float Angle = geom->Plane(0).Wire(0).ThetaZ(false)-TMath::Pi()/2.; // wire angle with respect to the vertical direction
//   mf::LogInfo("ShowerReco") << "Angle= " <<  Angle<<std::endl;
//   
//   float angle_rad = 30* pi /180;
//   angle_rad = Angle;
//   
//   l = 1;
//   m = (1/(2*sin(angle_rad)))*((1/slope[0])-(1/slope[1]));
//   n = (1/(2*cos(angle_rad)))*((1/slope[0])+(1/slope[1]));
//  
//   mf::LogInfo("ShowerReco") << "________________m= " << m << std::endl;
// 
//   // Director angles
//   fPhi   = atan(n/l);
//   fTheta = acos(m/(sqrt(pow(l,2)+pow(m,2)+pow(n,2))));
// 
//   float Phi = fPhi>0. ? (TMath::Pi()/2)-fPhi : fabs(fPhi)-(TMath::Pi()/2) ; // solve the ambiguities due to tangent periodicity
//   float Theta=0;
//   if(Phi>0)Theta = fTheta-(TMath::Pi()/2);
//   if(Phi<0)Theta = (TMath::Pi()/2)-fTheta;
// 
//   if(slope[0]==0 || slope[1]==0){
//     Phi   = 0;
//     Theta = 0;
//     fTheta = 0;
//     fPhi = 0;
//   }
//   
//   mf::LogInfo("ShowerReco") << " Phi=" <<Phi*180/pi << "   theta=" << Theta*180/pi <<std::endl;
//  
//   
//   
//     fh_phi_act->Fill(Phi*180/pi);
// 
//     fh_thet_act->Fill(Theta*180/pi);
//   
// 
//   
//   //GetPitchLength(); //Get pitch of (two) wire planes (only for Argoneut)
//    
// 
//   fPhi_ang=Phi*180/pi;
//   fTheta_ang=Theta*180/pi;
// 
//   
//   return 0;
// }

//------------------------------------------------------------------------------
void shwf::ShowerReco::LongTransEnergy(unsigned int set, std::vector < art::Ptr<recob::Hit> > hitlist)
{
  // alogorithm for energy vs dx of the shower (roto-translation) COLLECTION VIEW
  // double  wire_cm, time_cm;
  // int loop_nrg = 0;

  
  double totCnrg = 0,totCnrg_corr =0 ; // tot enegry of the shower in collection
  //double CdEdx4cm = 0; // tot enegry of the shower in collection
  //int CdedxCounter = 0;
  art::ServiceHandle<geo::Geometry> geom;
  art::ServiceHandle<util::LArProperties> larp;
  art::ServiceHandle<util::DetectorProperties> detprop;
  int channel;
  double fElectronsToADC = detprop->ElectronsToADC();

  //double CdEdx4cm=0,CdEdx6cm=0,CdEdx8cm=0,CdEdx10cm=0;
  
  
  double time;
  unsigned int wire=0,plane=0;
  unsigned int cstat = 0;
  
  double chav2cm=0.,mevav2cm=0.;
  double chav4cm=0.,mevav4cm=0.;
  
  mf::LogInfo("ShowerReco") << " --- plane, " << plane << " size:::: " << hitlist.size();
  


  for(art::PtrVector<recob::Hit>::const_iterator hitIter = hitlist.begin(); hitIter != hitlist.end();  hitIter++){
    art::Ptr<recob::Hit> theHit = (*hitIter);
    time = theHit->PeakTime() ;  
    //time_C -= (presamplings+10.1);
    art::Ptr<recob::Wire> theWire = theHit->Wire();
    channel = theWire->RawDigit()->Channel();
    geom->ChannelToWire(channel, cstat, fTPC, plane, wire);
    //    if(time_C<1020)continue;
    //  wire_cm = wire * fMean_wire_pitch; //in cm
    //  time_cm = time *ftimetick*fdriftvelocity; //in cm
    
    // moving to polar coordinates
    //   mf::LogInfo("ShowerReco") <<  " fElectronsToADc " << fElectronsToADC << std::endl;
    /////////////////from Calorimetry
    double MIPs   = theHit->Charge(true);   // in ADC
    double dQdx   = MIPs/fNPitch[set][plane];           // in ADC/cm
    double dQdx_e = dQdx/fElectronsToADC;  // Conversion from ADC/cm to e/cm
    dQdx_e *= LifetimeCorrection(time);   // Lifetime Correction (dQdx_e in e/c
    double dEdx = larp->BirksCorrection(dQdx_e);   // Correction f  
    /////////////////////end from Calorimetry
    
    
    totCnrg +=dQdx; // Sum the energy of all the hits
    ///////////// !!! To be corrected for calFactor per plane? 
    totCnrg_corr += dEdx; 
    
    
    double hlimit=30.4;  //arbitrary value to get first ~30 cm of the shower.    
    
    
    double wdist_cm=(wire-fWire_vertex[plane])*fMean_wire_pitch;
    double tdist_cm=(time-fTime_vertex[plane])*ftimetick*fdriftvelocity;
    
    double wdist=((wire-fWire_vertex[plane])*fNPitch[set][plane]);
    double rdist=TMath::Sqrt( wdist_cm*wdist_cm + tdist_cm*tdist_cm );
    
    
    // the first hit is by definition removed because the vertex is defined at a hit.
    if( (wdist<hlimit)&&(wdist>0.2)){ 
      //if(CdedxCounter<=10)
      
      double ch_adc=dQdx;
      
      double Bcorr =  dEdx;
      
      
      
      if(wdist<2.4){
	fChargeADC_2cm[set]+=ch_adc; 
	fChargeMeV_2cm[set]+= Bcorr ; 
	fNpoints_2cm[set]++;
      }
      
      // fill out for 4cm preshower
      if(wdist<4.4){
	fChargeADC_4cm[set]+=ch_adc; 
	fChargeMeV_4cm[set]+= Bcorr ; 
	fNpoints_4cm[set]++;
      }
      
      if(wdist<4.4 &&  fNpoints_4cm[set] )
	mf::LogInfo("ShowerReco") << " charge calc, set " << set << " " 
				  << fNPitch[set][set] << " " << wdist << " " 
				  << ch_adc << " MeV/cm: " << dEdx << " " 
				  << " av up to now: MeV " 
				  <<  fChargeMeV_4cm[set]/fNpoints_4cm[set] 
				  << " 2cm test: " << fChargeADC_2cm[set] 
				  << " " << fChargeMeV_2cm[set] << "av up to now: MeV " 
				  << fChargeMeV_2cm[set]/fNpoints_2cm[set] 
				  << "hit: wire, time " << wire << " " << time 
				  << " " << rdist << " " << fTime_vertex[plane];
      
      // fill out for 6cm preshower
      if(wdist<6.4){
	fChargeADC_6cm[set]+=ch_adc;
	fChargeMeV_6cm[set]+= Bcorr; 
      }
        
      // fill out for 8cm preshower
      if(wdist<8.4){
	fChargeADC_8cm[set]+=ch_adc;
	fChargeMeV_8cm[set]+= Bcorr; 
      }
        
      // fill out for 10cm preshower
      if(wdist<10.4){
	fChargeADC_10cm[set]+=ch_adc;
	fChargeMeV_10cm[set]+= Bcorr; 
      }
         
      fDistribChargeADC[set].push_back(ch_adc);  //vector with the first De/Dx points
      fDistribChargeMeV[set].push_back(Bcorr);  //vector with the first De/Dx points
      fDistribChargeposition[set].push_back(wdist);  //vector with the first De/Dx points' positions 
      
      
      fh_dedx[plane]->Fill( wdist, ((((theHit->Charge()/LifetimeCorrection(time))/fNPitch[set][plane])*10/7)/7.6)*6250*23.6*pow(10,-6) );


    }
    
  }
  
  mf::LogInfo("ShowerReco") << " ------------ set nr " << set << " - hit-Vertex=";
  fTotChargeADC[set]=totCnrg; 
  fTotChargeMeV[set]=totCnrg_corr;  
  
      
       
  if(fNpoints_2cm[set]>0)	{
    mf::LogInfo("ShowerReco") << " -----------NPoints 2cm, 4cm " << fNpoints_2cm[set] 
			      << " " << fNpoints_4cm[set] <<   " average De/Dx, charge:  " 
			      << fChargeADC_2cm[set]/fNpoints_2cm[set] << " " 
			      << fChargeADC_4cm[set]/fNpoints_4cm[set] << " " 
			      << fChargeMeV_2cm[set]/fNpoints_2cm[set]
			      << " " << fChargeMeV_4cm[set]/fNpoints_4cm[set];
    
    chav2cm=fChargeADC_2cm[set]/fNpoints_2cm[set]; 
    mevav2cm=fChargeMeV_2cm[set]/fNpoints_2cm[set];
    chav4cm=fChargeADC_4cm[set]/fNpoints_4cm[set]; 
    mevav4cm=fChargeMeV_4cm[set]/fNpoints_4cm[set];
    
  }
  
  double RMS_ADC_2cm={0.},RMS_ADC_4cm={0.};
  
  //second loop to calculate RMS      
  for(art::PtrVector<recob::Hit>::const_iterator hitIter = hitlist.begin(); hitIter != hitlist.end(); hitIter++){
    art::Ptr<recob::Hit> theHit = (*hitIter);
    time = theHit->PeakTime() ;  
    //time_C -= (presamplings+10.1);
    art::Ptr<recob::Wire> theWire = theHit->Wire();
    channel = theWire->RawDigit()->Channel();
    geom->ChannelToWire(channel, cstat, fTPC, plane, wire);
    //    if(time_C<1020)continue;
    //  wire_cm = wire * fMean_wire_pitch; //in cm
    //  time_cm = time *ftimetick*fdriftvelocity; //in cm
    
    // moving to polar coordinates
    //   mf::LogInfo("ShowerReco") <<  " fElectronsToADc " << fElectronsToADC << std::endl;
    /////////////////from Calorimetry
    double MIPs   = theHit->Charge(true);   // in ADC
    double dQdx   = MIPs/fNPitch[set][plane];           // in ADC/cm
    double dQdx_e = dQdx/fElectronsToADC;  // Conversion from ADC/cm to e/cm
    dQdx_e *= LifetimeCorrection(time);   // Lifetime Correction (dQdx_e in e/c
    double dEdx = larp->BirksCorrection(dQdx_e);   // Correction f  
    /////////////////////end from Calorimetry
    
    
    
    double hlimit=30.4;  //arbitrary value to get first ~30 cm of the shower.    
    
    
    
    double wdist=((wire-fWire_vertex[plane])*fNPitch[set][plane]);
    
    
    if( (wdist<hlimit)&&(wdist>0.4)){ 
      //if(CdedxCounter<=10)
      
      double ch_adc=dQdx;
      double Bcorr =  dEdx;
      
      if(wdist<2.4){
	fRMS_2cm[set]+= (Bcorr-mevav2cm)*(Bcorr-mevav2cm); 
	RMS_ADC_2cm+= (ch_adc-chav2cm)*(ch_adc-chav2cm);
      }
      
      
      if(wdist<4.4){
	fRMS_4cm[set]+= (Bcorr-mevav4cm)*(Bcorr-mevav4cm); 
	RMS_ADC_4cm+= (ch_adc-chav4cm)*(ch_adc-chav4cm);
      }
    }
  }      
  
  if(fNpoints_2cm[set]>0)	{
    mf::LogInfo("ShowerReco") << " ####### Npoints " << fNpoints_2cm[set] << " " 
			      << fNpoints_4cm[set] <<   " RMS, ADC:  " 
			      << TMath::Sqrt(RMS_ADC_2cm/fNpoints_2cm[set]) 
			      << " " <<TMath::Sqrt(RMS_ADC_4cm/fNpoints_4cm[set]) 
			      << " MeV: " << TMath::Sqrt(fRMS_2cm[set]/fNpoints_2cm[set])
			      << " " << TMath::Sqrt(fRMS_4cm[set]/fNpoints_4cm[set]);
    
    fRMS_2cm[set]=TMath::Sqrt(fRMS_2cm[set]/fNpoints_2cm[set]);
    fRMS_4cm[set]=TMath::Sqrt(fRMS_4cm[set]/fNpoints_4cm[set]);
    RMS_ADC_2cm=TMath::Sqrt(RMS_ADC_2cm/fNpoints_2cm[set]);
    RMS_ADC_4cm=TMath::Sqrt(RMS_ADC_4cm/fNpoints_4cm[set]);
    
    
  }
  
  /// third loop to get only points inside of 1RMS of value.      
  
  
  
  
  for(art::PtrVector<recob::Hit>::const_iterator hitIter = hitlist.begin(); hitIter != hitlist.end(); hitIter++){
    art::Ptr<recob::Hit> theHit = (*hitIter);
    time = theHit->PeakTime() ;  
    //time_C -= (presamplings+10.1);
    art::Ptr<recob::Wire> theWire = theHit->Wire();
    channel = theWire->RawDigit()->Channel();
    geom->ChannelToWire(channel, cstat, fTPC, plane, wire);
    //    if(time_C<1020)continue;
    //  wire_cm = wire * fMean_wire_pitch; //in cm
    //  time_cm = time *ftimetick*fdriftvelocity; //in cm
    
    // moving to polar coordinates
    //   mf::LogInfo("ShowerReco") <<  " fElectronsToADc " << fElectronsToADC << std::endl;
    /////////////////from Calorimetry
    double MIPs   = theHit->Charge(true);   // in ADC
    double dQdx   = MIPs/fNPitch[set][plane];           // in ADC/cm
    double dQdx_e = dQdx/fElectronsToADC;  // Conversion from ADC/cm to e/cm
    dQdx_e *= LifetimeCorrection(time);   // Lifetime Correction (dQdx_e in e/c
    double dEdx = larp->BirksCorrection(dQdx_e);   // Correction f  
    /////////////////////end from Calorimetry
    
    
    
    double hlimit=30.4;  //arbitrary value to get first ~30 cm of the shower.    
    
    
    
    double wdist=((wire-fWire_vertex[plane])*fNPitch[set][plane]);
    
    
    if( (wdist < hlimit) && (wdist > 0.4)){ 
      //if(CdedxCounter<=10)
      
      double ch_adc=dQdx;
      double Bcorr =  dEdx;
      
      
      if(wdist < 2.4){
	if(Bcorr > (mevav2cm-fRMS_2cm[set]) && Bcorr < (mevav2cm+fRMS_2cm[set]) ) {
	  fCorr_MeV_2cm[set]+= Bcorr; 
	  fNpoints_corr_MeV_2cm[set]++;
	}
	if(ch_adc > (chav2cm-RMS_ADC_2cm) && ch_adc < (chav2cm+RMS_ADC_2cm) ) {
	  fCorr_Charge_2cm[set]+= ch_adc;
	  fNpoints_corr_ADC_2cm[set]++;
	}
      }
      
      if(wdist < 4.4){
	if(Bcorr > (mevav4cm-fRMS_4cm[set]) && Bcorr < (mevav4cm+fRMS_4cm[set]) ) {
	  fCorr_MeV_4cm[set]+= Bcorr; 
	  fNpoints_corr_MeV_4cm[set]++;
	}
	if(ch_adc > (chav4cm-RMS_ADC_4cm) && ch_adc < (chav4cm+RMS_ADC_4cm) ) {
	  fCorr_Charge_4cm[set]+= ch_adc;
	  fNpoints_corr_ADC_4cm[set]++;
	}
      }
    }
  }   
  
  if(fNpoints_corr_MeV_2cm[set]>0){
	mf::LogInfo("ShowerReco") << " ++ NPoints 2cm, ADC and MeV " 
				  << fNpoints_corr_MeV_2cm[set] << " " 
				  << fNpoints_corr_ADC_2cm[set] 
				  << " corrected average De/Dx, charge, MeV:  " 
				  << fCorr_Charge_2cm[set]/fNpoints_corr_ADC_2cm[set] 
				  << " " << fCorr_MeV_2cm[set]/fNpoints_corr_MeV_2cm[set];
	fCorr_Charge_2cm[set]/=fNpoints_corr_ADC_2cm[set];
	fCorr_MeV_2cm[set]/=fNpoints_corr_MeV_2cm[set];
  }
  
  if(fNpoints_corr_MeV_4cm[set] > 0){
    mf::LogInfo("ShowerReco") << " ++ NPoints 4cm, ADC and MeV "
			      << fNpoints_corr_MeV_4cm[set] << " " 
			      << fNpoints_corr_ADC_4cm[set] 
			      << " corrected average De/Dx, charge, MeV:  " 
			      << fCorr_Charge_4cm[set]/fNpoints_corr_ADC_4cm[set] 
			      << " " << fCorr_MeV_4cm[set]/fNpoints_corr_MeV_4cm[set];
    
    fCorr_Charge_4cm[set]/=fNpoints_corr_ADC_4cm[set];
    fCorr_MeV_4cm[set]/=fNpoints_corr_MeV_4cm[set];
  }
  
}

//------------------------------------------------------------------------------------//  
// automatically called by Get3Daxis
void shwf::ShowerReco::GetPitchLength(unsigned int set){
  
  art::ServiceHandle<geo::Geometry> geom;
  
  
  for(unsigned int pl = 0; pl < fNPlanes; ++pl){
    fNPitch[set][pl] = ProjectedLength(set,pl);
    mf::LogInfo("ShowerReco") << "++++++ calculating  -Pitch[" << set <<"]["<<pl<<"] =" 
			      <<fNPitch[set][pl];
 
  }
  //mf::LogInfo("ShowerReco") << std::endl; 

  // fNPitch[1]=ProjectedLength(geom->Plane(1).View());
  
  return (void)0;
}



//------------------------------------------------------------------------------------//  
void   shwf::ShowerReco::GetVertexAndAnglesFromCluster(art::Ptr< recob::Cluster > clust,unsigned int plane) 
// Get shower vertex and slopes.
{
  //convert to cm/cm units needed in the calculation
  slope[plane]=clust->dTdW()*(ftimetick*fdriftvelocity)/fMean_wire_pitch;  
  fWire_vertex[plane]=clust->StartPos()[0];
  fTime_vertex[plane]=clust->StartPos()[1];

  ////////// insert detector offset

  mf::LogInfo("ShowerReco") << "======= setting slope for view: " << plane 
			    << " " << slope[plane] << " " << fWire_vertex[plane] 
			    << " " << fTime_vertex[plane] << " " 
			    <<  fWire_vertex[plane]+50<< " "
			    << fTime_vertex[plane] + slope[plane]*(fWire_vertex[plane]+50);
  
}

//------------------------------------------------------------------------------------//  
double shwf::ShowerReco::ProjectedLength(unsigned int set,unsigned int plane ) 	 const
{
   art::ServiceHandle<geo::Geometry> geo;
   double pitch = -1.;
   if(geo->Plane(plane).View() == geo::kUnknown || geo->Plane(plane).View() == geo::k3D){
     mf::LogWarning("RecoBaseProng")<< "Warning Prong::ProjectedLength :  no Pitch foreseen for view "
				    <<geo->Plane(plane).View();
     return pitch;
   }
   else{
     // temporary override of global angles to use the new calculations.
     double fTheta=fThetaN[set];
     double fPhi=fPhiN[set];  
     
     
     double pi=TMath::Pi();
     for(unsigned int cs = 0; cs < geo->Ncryostats(); ++cs){
       for(unsigned int t = 0; t < geo->Cryostat(cs).NTPC(); ++t){
	 for(unsigned int i = 0; i < geo->Cryostat(cs).TPC(t).Nplanes(); ++i){
	   if(i == plane){
	     double wirePitch = geo->Cryostat(cs).TPC(t).WirePitch(0,1,i);
	     double angleToVert =0.5*TMath::Pi() - geo->Cryostat(cs).TPC(t).Plane(i).Wire(0).ThetaZ(false) ;

	     mf::LogInfo("ShowerReco") <<" %%%%%%%%%%  " << i << " angle " 
				       << angleToVert*180/pi << " " 
				       << geo->Plane(i).Wire(0).ThetaZ(false)*180/pi 
				       <<" wirePitch " << wirePitch
				       <<"\n %%%%%%%%%%  " << fTheta << " " << fPhi;
	   
	     //(sin(angleToVert),cos(angleToVert)) is the direction perpendicular to wire
	     //(fDCosStart[1],fDDosStart[2]) is the direction of prong in the y-z plane
	     double cosgamma = TMath::Abs(TMath::Sin(angleToVert)*TMath::Cos(fTheta)
					  +TMath::Cos(angleToVert)*TMath::Sin(fTheta)*TMath::Sin(fPhi));
	     if (cosgamma>0) pitch = wirePitch/cosgamma;     
	   } // end if the correct view
	 } // end loop over planes
       } // end loop over TPCs
     } // end loop over cryostats
   } // end if a reasonable view
   
   return pitch;
}


/////////////////////////from Calorimetry:
//------------------------------------------------------------------------------------//
double shwf::ShowerReco::LifetimeCorrection(float time){
  
  float t = time;
  art::ServiceHandle<util::LArProperties> LArProp;
  art::ServiceHandle<util::DetectorProperties> detprop;
  double timetick = detprop->SamplingRate()*1.e-3;    //time sample in microsec
  double presamplings = detprop->TriggerOffset();
  t -= presamplings;
  
  time = t * timetick;  //  (in microsec)
  double tau = LArProp->ElectronLifetime();
  double correction = exp(time/tau);
  return correction;
}
