#ifndef FEATUREVERTEXFINDERANA_H
#define FEATUREVERTEXFINDERANA_H
///////////////////////////////////////////////////////////////////////////////////////////
//
// FeatureVertexFinder designed to analyze 2d & 3d verticies found in the TPC
//
// jaasaadi@syr.edu
//
// Note: This ana module will utilize MC truth information for verticies and is not (yet)
// intended as a unit test for data...though many of the same methods will likely be used
// for real data (when the time comes)
//////////////////////////////////////////////////////////////////////////////////////////

// ########################
// ### LArSoft Includes ###
// ########################
#include "RecoBase/EndPoint2D.h"
#include "RecoBase/Hit.h"
#include "RecoBase/Cluster.h"
#include "RecoBase/Vertex.h"
#include "Geometry/Geometry.h"
#include "Geometry/CryostatGeo.h"
#include "Geometry/TPCGeo.h"
#include "Geometry/PlaneGeo.h"
#include "Geometry/WireGeo.h"
#include "Simulation/sim.h"
#include "Simulation/SimListUtils.h"
#include "MCCheater/BackTracker.h"
#include "Utilities/LArProperties.h"
#include "Utilities/DetectorProperties.h"
#include "Utilities/AssociationUtil.h"

// ##########################
// ### Basic C++ Includes ###
// ##########################
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <ios>
#include <sstream>
#include <fstream>
#include <math.h>
#include <algorithm>
#include <vector>

// ##########################
// ### Framework Includes ###
// ##########################
#include "art/Framework/Core/ModuleMacros.h" 
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Persistency/Common/Ptr.h"
#include "art/Persistency/Common/PtrVector.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// #####################
// ### ROOT Includes ###
// #####################
#include "TMath.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TVectorD.h"
#include "TGeoManager.h"
#include "TMath.h"
#include "TGraph.h"
#include "TF1.h"
#include "TVector3.h"

// ====================================================================================
// ====================================================================================

namespace vertex{

// Base class for creation of verticies
class FeatureVertexFinderAna: public art::EDAnalyzer{


public:
	explicit FeatureVertexFinderAna(fhicl::ParameterSet const& pset);
	virtual ~FeatureVertexFinderAna();
	
	// providing read write access to the event
	void analyze (const art::Event& evt);
	void beginJob();
	void reconfigure(fhicl::ParameterSet const& p);
	
private:
	std::string fLArG4ModuleLabel; //<---LArG4 Module Label
	std::string fGenieModuleLabel; //<---Genie Module Label
	std::string fVertexModuleLabel; //<---Vertex Module Label
	std::string fEndPoint2dModuleLabel; //<---2d Vertex Module Label (EndPoint2d)
	
	
	// Outputting histograms for analysis
	
	TH1F* fRun;
    	TH1F* fEvt;
	TH1F* fTruthVtxXPos;
	TH1F* fTruthVtxYPos;
	TH1F* fTruthVtxZPos;
	TH1F* fTruthWireNumberPlane0;
	TH1F* fTruthTimeTickPlane0;
	TH1F* fTruthWireNumberPlane1;
	TH1F* fTruthTimeTickPlane1;
	TH1F* fTruthWireNumberPlane2;
	TH1F* fTruthTimeTickPlane2;
	TH1F* fTruthWireInCmPlane0;
	TH1F* fTruthTimeInCmPlane0;
	TH1F* fTruthWireInCmPlane1;
	TH1F* fTruthTimeInCmPlane1;
	TH1F* fTruthWireInCmPlane2;
	TH1F* fTruthTimeInCmPlane2;	
	
	TH1F* fTwoDNVtxPlane0;
	TH1F* fTwoDNVtxPlane1;
	TH1F* fTwoDNVtxPlane2;
	
	TH1F* fTwoDWireNumberPlane0;
	TH1F* fTwoDTimeTickPlane0;
	TH1F* fTwoDWireNumberPlane1;
	TH1F* fTwoDTimeTickPlane1;
	TH1F* fTwoDWireNumberPlane2;
	TH1F* fTwoDTimeTickPlane2;
	TH1F* fTwoDWireInCmPlane0;
	TH1F* fTwoDTimeInCmPlane0;
	TH1F* fTwoDWireInCmPlane1;
	TH1F* fTwoDTimeInCmPlane1;
	TH1F* fTwoDWireInCmPlane2;
	TH1F* fTwoDTimeInCmPlane2;
	
	TH1F* fTwoDStrengthPlane0;
	TH1F* fTwoDStrengthPlane1;
	TH1F* fTwoDStrengthPlane2;
	
	TH1F* fRecoCheck2dWireNumPlane0;
	TH1F* fRecoCheck2dTimeTickPlane0;
	TH1F* fRecoCheck2dWireInCmPlane0;
	TH1F* fRecoCheck2dTimeInCmPlane0;
	
	TH1F* fRecoCheck2dWireNumPlane1;
	TH1F* fRecoCheck2dTimeTickPlane1;
	TH1F* fRecoCheck2dWireInCmPlane1;
	TH1F* fRecoCheck2dTimeInCmPlane1;
	
	TH1F* fRecoCheck2dWireNumPlane2;
	TH1F* fRecoCheck2dTimeTickPlane2;
	TH1F* fRecoCheck2dWireInCmPlane2;
	TH1F* fRecoCheck2dTimeInCmPlane2;
	
	TH1F* fRecoVtxXPos;
	TH1F* fRecoVtxYPos;
	TH1F* fRecoVtxZPos;
	
	TH1F* fRecoCheck3dVtxX;
	TH1F* fRecoCheck3dVtxY;
	TH1F* fRecoCheck3dVtxZ;
	
	TH2D* fRecoCheck3dVtxXvsX;
	TH2D* fRecoCheck3dVtxYvsY;
	TH2D* fRecoCheck3dVtxZvsZ;

}; // End class VertexFinderAna
// ====================================================================================
// ====================================================================================


//------------------------------------------------------------------
FeatureVertexFinderAna::FeatureVertexFinderAna(fhicl::ParameterSet const& pset) 
{
	this->reconfigure(pset);
}

//------------------------------------------------------------------
FeatureVertexFinderAna::~FeatureVertexFinderAna()
{
}


//------------------------------------------------------------------
void FeatureVertexFinderAna::reconfigure(fhicl::ParameterSet const& p)
{
	fLArG4ModuleLabel        = p.get< std::string >("LArGeantModuleLabel");
	fGenieModuleLabel	 = p.get< std::string >("GenieModuleLabel");
	fVertexModuleLabel       = p.get< std::string >("VertexModuleLabel");
	fEndPoint2dModuleLabel   = p.get< std::string >("EndPoint2dModuleLabel");
    	return;
}

//-------------------------------------------------
void FeatureVertexFinderAna::beginJob() 
{
	// get access to the TFile service
    	art::ServiceHandle<art::TFileService> tfs;
	// ====================================
    	// ==== Outputting TH1F Histograms ====
    	// ====================================
    	fRun 			= tfs->make<TH1F>("fRun", "Run Number", 1000, 0, 1000);
    	fEvt 			= tfs->make<TH1F>("fEvt", "Event Number", 1000, 0, 1000);
	fTruthVtxXPos 		= tfs->make<TH1F>("fTruthVtxXPos", "Truth Vertex X Position", 400, 0, 200);
	fTruthVtxYPos 		= tfs->make<TH1F>("fTruthVtxYPos", "Truth Vertex Y Position", 400, -100, 100);
	fTruthVtxZPos 		= tfs->make<TH1F>("fTruthVtxZPos", "Truth Vertex Z Position", 2000, 0, 1000);
	fTruthWireNumberPlane0  = tfs->make<TH1F>("fTruthWireNumberPlane0", "Truth Wire Number Plane 0", 3000, 0, 3000);
	fTruthTimeTickPlane0	= tfs->make<TH1F>("fTruthTimeTickPlane0", "Truth Time Tick Plane 0", 3200, 0, 3200);
	fTruthWireNumberPlane1  = tfs->make<TH1F>("fTruthWireNumberPlane1", "Truth Wire Number Plane 1", 3000, 0, 3000);
	fTruthTimeTickPlane1	= tfs->make<TH1F>("fTruthTimeTickPlane1", "Truth Time Tick Plane 1", 3200, 0, 3200);
	fTruthWireNumberPlane2  = tfs->make<TH1F>("fTruthWireNumberPlane2", "Truth Wire Number Plane 2", 3000, 0, 3000);
	fTruthTimeTickPlane2	= tfs->make<TH1F>("fTruthTimeTickPlane2", "Truth Time Tick Plane 2", 3200, 0, 3200);
	fTruthWireInCmPlane0    = tfs->make<TH1F>("fTruthWireInCmPlane0", "Truth Wire In CM Plane 0", 2000, 0, 1000);
	fTruthTimeInCmPlane0	= tfs->make<TH1F>("fTruthTimeInCmPlane0", "Truth Time In Cm Plane 0", 2000, 0, 1000);
	fTruthWireInCmPlane1  	= tfs->make<TH1F>("fTruthWireInCmPlane1", "Truth Wire In CM Plane 1", 2000, 0, 1000);
	fTruthTimeInCmPlane1	= tfs->make<TH1F>("fTruthTimeInCmPlane1", "Truth Time In Cm Plane 1", 2000, 0, 1000);
	fTruthWireInCmPlane2  	= tfs->make<TH1F>("fTruthWireInCmPlane2", "Truth Wire In CM Plane 2", 2000, 0, 1000);
	fTruthTimeInCmPlane2	= tfs->make<TH1F>("fTruthTimeInCmPlane2", "Truth Time In Cm Plane 2", 2000, 0, 1000);
	
	fTwoDNVtxPlane0		= tfs->make<TH1F>("fTwoDNVtxPlane0", "TwoD Number of Verticies Found in Plane 0", 200, 0, 100);
	fTwoDNVtxPlane1		= tfs->make<TH1F>("fTwoDNVtxPlane1", "TwoD Number of Verticies Found in Plane 1", 200, 0, 100);
	fTwoDNVtxPlane2		= tfs->make<TH1F>("fTwoDNVtxPlane2", "TwoD Number of Verticies Found in Plane 2", 200, 0, 100);
	
	fTwoDWireNumberPlane0  	= tfs->make<TH1F>("fTwoDWireNumberPlane0", "TwoD Wire Number Plane 0", 3000, 0, 3000);
	fTwoDTimeTickPlane0	= tfs->make<TH1F>("fTwoDTimeTickPlane0", "TwoD Time Tick Plane 0", 3200, 0, 3200);
	fTwoDWireNumberPlane1  	= tfs->make<TH1F>("fTwoDWireNumberPlane1", "TwoD Wire Number Plane 1", 3000, 0, 3000);
	fTwoDTimeTickPlane1	= tfs->make<TH1F>("fTwoDTimeTickPlane1", "TwoD Time Tick Plane 1", 3200, 0, 3200);
	fTwoDWireNumberPlane2  	= tfs->make<TH1F>("fTwoDWireNumberPlane2", "TwoD Wire Number Plane 2", 3000, 0, 3000);
	fTwoDTimeTickPlane2	= tfs->make<TH1F>("fTwoDTimeTickPlane2", "TwoD Time Tick Plane 2", 3200, 0, 3200);
	fTwoDWireInCmPlane0  	= tfs->make<TH1F>("fTwoDWireInCmPlane0", "TwoD Wire In CM Plane 0", 2000, 0, 1000);
	fTwoDTimeInCmPlane0	= tfs->make<TH1F>("fTwoDTimeInCmPlane0", "TwoD Time In Cm Plane 0", 2000, 0, 1000);
	fTwoDWireInCmPlane1  	= tfs->make<TH1F>("fTwoDWireInCmPlane1", "TwoD Wire In CM Plane 1", 2000, 0, 1000);
	fTwoDTimeInCmPlane1	= tfs->make<TH1F>("fTwoDTimeInCmPlane1", "TwoD Time In Cm Plane 1", 2000, 0, 1000);
	fTwoDWireInCmPlane2  	= tfs->make<TH1F>("fTwoDWireInCmPlane2", "TwoD Wire In CM Plane 2", 2000, 0, 1000);
	fTwoDTimeInCmPlane2	= tfs->make<TH1F>("fTwoDTimeInCmPlane2", "TwoD Time In Cm Plane 2", 2000, 0, 1000);
	
	fTwoDStrengthPlane0	= tfs->make<TH1F>("fTwoDStrengthPlane0", "TwoD Strength Plane 0", 1000, 0, 500);
	fTwoDStrengthPlane1	= tfs->make<TH1F>("fTwoDStrengthPlane1", "TwoD Strength Plane 1", 1000, 0, 500);
	fTwoDStrengthPlane2	= tfs->make<TH1F>("fTwoDStrengthPlane2", "TwoD Strength Plane 2", 1000, 0, 500);
	
	fRecoCheck2dWireNumPlane0	= tfs->make<TH1F>("fRecoCheck2dWireNumPlane0", "Reco Wire Number - True Wire Number Plane 0", 400, -200, 200);
	fRecoCheck2dTimeTickPlane0	= tfs->make<TH1F>("fRecoCheck2dTimeTickPlane0", "Reco Time Tick - True Time Tick Plane 0", 1000, -500, 500);
	fRecoCheck2dWireInCmPlane0	= tfs->make<TH1F>("fRecoCheck2dWireInCmPlane0", "Reco Wire in CM - True Wire in CM Plane 0", 200, -50, 50);	
	fRecoCheck2dTimeInCmPlane0	= tfs->make<TH1F>("fRecoCheck2dTimeInCmPlane0", "Reco Time in CM - True Time in CM Plane 0", 200, -50, 50);
	
	fRecoCheck2dWireNumPlane1	= tfs->make<TH1F>("fRecoCheck2dWireNumPlane1", "Reco Wire Number - True Wire Number Plane 1", 400, -200, 200);
	fRecoCheck2dTimeTickPlane1	= tfs->make<TH1F>("fRecoCheck2dTimeTickPlane1", "Reco Time Tick - True Time Tick Plane 1", 1000, -500, 500);
	fRecoCheck2dWireInCmPlane1	= tfs->make<TH1F>("fRecoCheck2dWireInCmPlane1", "Reco Wire in CM - True Wire in CM Plane 1", 200, -50, 50);	
	fRecoCheck2dTimeInCmPlane1	= tfs->make<TH1F>("fRecoCheck2dTimeInCmPlane1", "Reco Time in CM - True Time in CM Plane 1", 200, -50, 50);
	
	fRecoCheck2dWireNumPlane2	= tfs->make<TH1F>("fRecoCheck2dWireNumPlane2", "Reco Wire Number - True Wire Number Plane 2", 400, -200, 200);
	fRecoCheck2dTimeTickPlane2	= tfs->make<TH1F>("fRecoCheck2dTimeTickPlane2", "Reco Time Tick - True Time Tick Plane 2", 1000, -500, 500);
	fRecoCheck2dWireInCmPlane2	= tfs->make<TH1F>("fRecoCheck2dWireInCmPlane2", "Reco Wire in CM - True Wire in CM Plane 2", 200, -50, 50);	
	fRecoCheck2dTimeInCmPlane2	= tfs->make<TH1F>("fRecoCheck2dTimeInCmPlane2", "Reco Time in CM - True Time in CM Plane 2", 200, -50, 50);
	
	
	fRecoVtxXPos 		= tfs->make<TH1F>("fRecoVtxXPos", "Reco Vertex X Position", 400, -10, 200);
	fRecoVtxYPos 		= tfs->make<TH1F>("fRecoVtxYPos", "Reco Vertex Y Position", 400, -100, 100);
	fRecoVtxZPos 		= tfs->make<TH1F>("fRecoVtxZPos", "Reco Vertex Z Position", 2000, -10, 1000);
	
	fRecoCheck3dVtxX	= tfs->make<TH1F>("fRecoCheck3dVtxX", "Reco X Position - True X Postion", 800, -100, 100);
	fRecoCheck3dVtxY	= tfs->make<TH1F>("fRecoCheck3dVtxY", "Reco Y Position - True Y Postion", 800, -100, 100);
	fRecoCheck3dVtxZ	= tfs->make<TH1F>("fRecoCheck3dVtxZ", "Reco Z Position - True Z Postion", 800, -100, 100);
	
	fRecoCheck3dVtxXvsX	= tfs->make <TH2D>("fRecoCheck3dVtxXvsX", "(Reco X - True X)/True X vs True X", 400, -10, 200, 120, -20, 20);
	fRecoCheck3dVtxYvsY	= tfs->make <TH2D>("fRecoCheck3dVtxYvsY", "(Reco Y - True Y)/True Y vs True Y", 400, -100, 100, 120, -20, 20);
	fRecoCheck3dVtxZvsZ	= tfs->make <TH2D>("fRecoCheck3dVtxZvsZ", "(Reco Z - True Z)/True Z vs True Z", 1000, -10, 1000, 120, -20, 20);
	
	
	return;
}

// ====================================================================================
// ============================== Access the event ====================================
// ====================================================================================
void FeatureVertexFinderAna::analyze(const art::Event& evt)
{
  
  // ###############################
  // ### Filling Run/Event Histo ###
  // ###############################
  fRun->Fill( evt.run() );
  fEvt->Fill( evt.id().event() );
     
  // ##############################################
  // ### Getting the Geant Information Directly ###
  // ##############################################
  art::Handle< std::vector<simb::MCParticle> > mcParticleHandle;
  evt.getByLabel(fLArG4ModuleLabel, mcParticleHandle);

  // #######################################
  // ### Getting MC Truth Info from simb ###
  // #######################################
  art::Handle< std::vector<simb::MCTruth> > mctruthListHandle;
  evt.getByLabel(fGenieModuleLabel,mctruthListHandle); 
  
  // ############################################
  // ### Getting information from BackTracker ###
  // ############################################
  art::ServiceHandle<cheat::BackTracker> bt;
  
  // ####################################
  // ### Getting Geometry Information ###
  // ####################################
  art::ServiceHandle<geo::Geometry> geom;
  
  // ###################################
  // ### Getting Detector Properties ###
  // ###################################
  art::ServiceHandle<util::DetectorProperties> detp;
  
  // ##############################
  // ### Getting LAr Properties ###
  // ##############################
  art::ServiceHandle<util::LArProperties> larp;
  
  // ######################################################
  // ### Getting 2d Vertex information (vertex2dHandle) ###
  // ######################################################
  art::Handle< std::vector<recob::EndPoint2D> > vertex2dHandle;
  evt.getByLabel(fEndPoint2dModuleLabel,vertex2dHandle);
  
  // ##################################################
  // ### Getting the 3d Vertex (vertex3dListHandle) ###
  // ##################################################
  art::Handle< std::vector<recob::Vertex> > vertex3dListHandle;
  evt.getByLabel(fVertexModuleLabel,vertex3dListHandle); 
  
  
  
  // ==========================================================
  // === Detector numbers that are useful (to be set later) ===
  // ==========================================================
  size_t nplanes = 0;
  double WirePitch_CurrentPlane[3] = {0.}; //<---Setting the Wire pitch for each plane
  					   // Right now assuming only 3 planes
  // ##############################
  // ### Looping over cryostats ###
  // ##############################
  for(size_t cstat = 0; cstat < geom->Ncryostats(); ++cstat)
  	{
    	// ##########################
      	// ### Looping over TPC's ###
      	// ##########################
      	for(size_t tpc = 0; tpc < geom->Cryostat(cstat).NTPC(); ++tpc)
		{
		// ### Setting the number of planes for this detector ###
   		nplanes = geom->Cryostat(cstat).TPC(tpc).Nplanes();
		// ###############################
		// ### Looping over the planes ###
		// ###############################
		for(size_t plane = 0; plane < nplanes; plane ++)
			{
			//                              geom->WirePitch(Wire1, Wire2, Plane#, TPC#, Cyro#);
			WirePitch_CurrentPlane[plane] = geom->WirePitch(0,1,plane,tpc,cstat);
			
			
			}//<---End plane loop
		}//<---End tpc loop
	}//<---End cstat loop

  // ##################################################
  // ### Calculating the Timetick to CM conversion  ###
  // ##################################################
  float TimeTick      = detp->SamplingRate()/1000.; //<---To get units of microsecond...not nanosec
  float DriftVelocity = larp->DriftVelocity(larp->Efield(),larp->Temperature());
                
  float TimetoCm      = TimeTick*DriftVelocity;




// ===========================================================================================================
// ==================================== Looping over MC information ==========================================
// ===========================================================================================================
 
 
   // ##################################
   // ### Getting MC Truth simb Info ###
   // ##################################
   art::PtrVector<simb::MCTruth> mclist;
   for (unsigned int ii = 0; ii <  mctruthListHandle->size(); ++ii)
        {
        art::Ptr<simb::MCTruth> mctparticle(mctruthListHandle,ii);
        mclist.push_back(mctparticle);
        }
   
   // ##############################################
   // ### Variables for truth vertex information ###
   // ##############################################
   float    truth_vertex[5] = {0.};		//<---Truth x,y,z information
   
   uint32_t VtxWireNum[3] = {0.};		//<---Wire number in each plane ( WireNum[plane#] )
   double   VtxTimeTick[3] = {0.};		//<---Time tick in each plane   ( TimeTick[plane#] )
   
   double   VtxWireNum_InCM[3] = {0.};		//<---Wire number in each plane in CM ( WireNum[plane#] )
   double   VtxTimeTick_InCM[3] = {0.};		//<---Time tick in each plane in CM   ( TimeTick[plane#] )
   
   
   // ###################################
   // ### Finding the MC truth vertex ###
   // ###################################
   for( unsigned int i = 0; i < mclist.size(); ++i )
        {
        art::Ptr<simb::MCTruth> mc(mclist[i]);
        simb::MCParticle neut(mc->GetParticle(i)); 
	
	// ############################################
	// ### Filling the vertex x,y,z information ###
	// ############################################
	truth_vertex[0] =neut.Vx();
        truth_vertex[1] =neut.Vy();
        truth_vertex[2] =neut.Vz();
	
	}// end i loop
	
  // ### Filling Histograms ###
  fTruthVtxXPos->Fill( truth_vertex[0] );
  fTruthVtxYPos->Fill( truth_vertex[1] );
  fTruthVtxZPos->Fill( truth_vertex[2] );
  
  // ##############################
  // ### Looping over cryostats ###
  // ##############################
  for(size_t cstat = 0; cstat < geom->Ncryostats(); ++cstat)
  	{
    	// ##########################
      	// ### Looping over TPC's ###
      	// ##########################
      	for(size_t tpc = 0; tpc < geom->Cryostat(cstat).NTPC(); ++tpc)
		{
		// ###############################
		// ### Looping over the planes ###
		// ###############################
		for(size_t plane = 0; plane < nplanes; plane ++)
			{
			// ############################################################################
			// ### Calculating the nearest wire the vertex corresponds to in each plane ###
			// ############################################################################
			// ################################# Plane 0 ####################################
	
			//                  geom->NearestWire(worldLoc[3], Plane#, TPC#, Cyrostat#)
			VtxWireNum[plane] = geom->NearestWire(truth_vertex,plane,tpc,cstat);
			//             detp->ConvertXToTicks(xpos, plane#, TPC#, Cyrostat#)
			VtxTimeTick[plane] = detp->ConvertXToTicks(truth_vertex[0],plane,tpc,cstat) +  detp->GetXTicksOffset(plane, tpc, cstat);
			
			// ======================== Translating each of these in cm =====================
			VtxWireNum_InCM[plane]  = VtxWireNum[plane] * WirePitch_CurrentPlane[plane];
			VtxTimeTick_InCM[plane] = VtxTimeTick[plane] * TimetoCm;
			
			
			}//---End plane loop
		}//<---End tpc loop
	}//<---End cstat loop
   
   // ### Filling Histograms ###
   fTruthWireNumberPlane0->Fill( VtxWireNum[0]  );
   fTruthTimeTickPlane0->Fill( VtxTimeTick[0] );
   fTruthWireNumberPlane1->Fill( VtxWireNum[1]  );
   fTruthTimeTickPlane1->Fill( VtxTimeTick[1] );
   fTruthWireNumberPlane2->Fill( VtxWireNum[2]  );
   fTruthTimeTickPlane2->Fill( VtxTimeTick[2] );
   
   fTruthWireInCmPlane0->Fill( VtxWireNum_InCM[0] );
   fTruthTimeInCmPlane0->Fill( VtxTimeTick_InCM[0] );
   fTruthWireInCmPlane1->Fill( VtxWireNum_InCM[1] );
   fTruthTimeInCmPlane1->Fill( VtxTimeTick_InCM[1] );
   fTruthWireInCmPlane2->Fill( VtxWireNum_InCM[2] );
   fTruthTimeInCmPlane2->Fill( VtxTimeTick_InCM[2] );


// ===================================================================================================================
// ==================================== Looping over EndPoint2d information ==========================================
// ===================================================================================================================

   art::PtrVector<recob::EndPoint2D> vert2d;
   
   // Variables for Vertex2d
   
   double Vertex2d_TimeTick[1000] = {0.};	//<---Vertex2d Time Tick for the current plane ( TimeTick[#2d] )
   double Vertex2d_Wire[1000]	 = {0.};	//<---Veretx2d Wire # ( Wire[#2d] )
   
   double Vertex2d_TimeTick_InCM[1000] 	= {0.};	//<---Vertex 2d Time tick in CM ( TimeTick[#2d] )
   double Vertex2d_Wire_InCM[1000] 	= {0.};	//<---Veretx2d Wire in CM ( Wire[#2d] )
   int n2dVtx = 0;
   
   int n2dVtxPlane0 = 0, n2dVtxPlane1 = 0, n2dVtxPlane2 = 0;
   
   bool vertexWstrengthplane0 = false , vertexWstrengthplane1 = false, vertexWstrengthplane2 = false;
   
   // ######################################
   // ### Loop over the EndPoint2d List  ###
   // ######################################
   for(size_t ii = 0; ii < vertex2dHandle->size(); ++ii){
     art::Ptr<recob::EndPoint2D> vertex(vertex2dHandle, ii);
     vert2d.push_back(vertex);
   }
   
  // ############################################ 
  // ### If we have 2d vertex, loop over them ###
  // ############################################
  if(vert2d.size() > 0)
  	{

   	// ##############################
   	// ### Looping over cryostats ###
   	// ##############################
   	for(size_t cstat = 0; cstat < geom->Ncryostats(); ++cstat)
  		{
  		// ##########################
  		// ### Looping over TPC's ###
  		// ##########################
  		for(size_t tpc = 0; tpc < geom->Cryostat(cstat).NTPC(); ++tpc)
			{
			// ###############################
			// ### Looping over the planes ###
			// ###############################
			for(size_t plane = 0; plane < nplanes; plane ++)
				{
				
				for(size_t ww = 0; ww<vert2d.size(); ww++)
					{
					//std::cout<<"plane = "<<plane<<std::endl;
					//std::cout<<"vert2d[ww]->WireID().Plane = "<<vert2d[ww]->WireID().Plane<<std::endl;
					// Only look at this 2d vertex if it is in the current plane
					if (vert2d[ww]->WireID().Plane != plane){continue;}
					
					Vertex2d_TimeTick[n2dVtx] = vert2d[ww]->DriftTime();
					Vertex2d_Wire[n2dVtx]     = vert2d[ww]->WireID().Wire;
				
					// ======================== Translating each of these in cm =====================
					Vertex2d_Wire_InCM[n2dVtx]	   = Vertex2d_Wire[n2dVtx] * WirePitch_CurrentPlane[plane];	
					//std::cout<<"Vertex2d_Wire[n2dVtx] = "<<Vertex2d_Wire[n2dVtx]<<" , WirePitch_CurrentPlane[plane] = "<<WirePitch_CurrentPlane[plane]<<std::endl;
					//std::cout<<"Vertex2d_Wire_InCM[n2dVtx] = "<<Vertex2d_Wire_InCM[n2dVtx]<<std::endl;
					Vertex2d_TimeTick_InCM[n2dVtx]     = Vertex2d_TimeTick[n2dVtx] * TimetoCm;
					
					// ###########################################################################
					// ### Checking how well we did in reconstructing the vertex (Reco - True) ###
					// ###########################################################################
					
					float RecoCheck_TimeTick = Vertex2d_TimeTick[n2dVtx] - VtxTimeTick[plane];
					float RecoCheck_WireNum  = Vertex2d_Wire[n2dVtx] - VtxWireNum[plane];
					
					float RecoCheck_TimeInCm = Vertex2d_TimeTick_InCM[n2dVtx] - VtxTimeTick_InCM[plane];
					float RecoCheck_WireInCm = Vertex2d_Wire_InCM[n2dVtx] - VtxWireNum_InCM[plane];
					
					
					
					if( vert2d[ww]->Strength() > -1)
					{
					if(plane == 0)
						{
						vertexWstrengthplane0 = true;
						//std::cout<<"Filling Plane 0 "<<std::endl;
						//std::cout<<"Vertex2d_Wire[n2dVtx] = "<<Vertex2d_Wire[n2dVtx]<<std::endl;
					
						fTwoDWireNumberPlane0->Fill( Vertex2d_Wire[n2dVtx]  );
   						fTwoDTimeTickPlane0->Fill( Vertex2d_TimeTick[n2dVtx] );
						fTwoDWireInCmPlane0->Fill( Vertex2d_Wire_InCM[n2dVtx] );
   						fTwoDTimeInCmPlane0->Fill( Vertex2d_TimeTick_InCM[n2dVtx] );
						
						fTwoDStrengthPlane0->Fill( vert2d[ww]->Strength() );
						
						fRecoCheck2dWireNumPlane0->Fill( RecoCheck_WireNum );
						fRecoCheck2dTimeTickPlane0->Fill( RecoCheck_TimeTick );
						fRecoCheck2dWireInCmPlane0->Fill( RecoCheck_WireInCm );
						fRecoCheck2dTimeInCmPlane0->Fill( RecoCheck_TimeInCm );
						
						n2dVtxPlane0++;
					
						}//<---End Plane 0
				
					if(plane == 1)
						{
						vertexWstrengthplane1 = true;
						fTwoDWireNumberPlane1->Fill( Vertex2d_Wire[n2dVtx]  );
   						fTwoDTimeTickPlane1->Fill( Vertex2d_TimeTick[n2dVtx] );
						fTwoDWireInCmPlane1->Fill( Vertex2d_Wire_InCM[n2dVtx] );
   						fTwoDTimeInCmPlane1->Fill( Vertex2d_TimeTick_InCM[n2dVtx] );
						
						fTwoDStrengthPlane1->Fill( vert2d[ww]->Strength() );
						
						fRecoCheck2dWireNumPlane1->Fill( RecoCheck_WireNum );
						fRecoCheck2dTimeTickPlane1->Fill( RecoCheck_TimeTick );
						fRecoCheck2dWireInCmPlane1->Fill( RecoCheck_WireInCm );
						fRecoCheck2dTimeInCmPlane1->Fill( RecoCheck_TimeInCm );
						
						n2dVtxPlane1++;
					
						}//<---End Plane 1
					
					if(plane == 2)
						{
						vertexWstrengthplane2 = true;
						fTwoDWireNumberPlane2->Fill( Vertex2d_Wire[n2dVtx]  );
   						fTwoDTimeTickPlane2->Fill( Vertex2d_TimeTick[n2dVtx] );
						fTwoDWireInCmPlane2->Fill( Vertex2d_Wire_InCM[n2dVtx] );
   						fTwoDTimeInCmPlane2->Fill( Vertex2d_TimeTick_InCM[n2dVtx] );
						
						fTwoDStrengthPlane2->Fill( vert2d[ww]->Strength() );
						
						fRecoCheck2dWireNumPlane2->Fill( RecoCheck_WireNum );
						fRecoCheck2dTimeTickPlane2->Fill( RecoCheck_TimeTick );
						fRecoCheck2dWireInCmPlane2->Fill( RecoCheck_WireInCm );
						fRecoCheck2dTimeInCmPlane2->Fill( RecoCheck_TimeInCm );
						
						n2dVtxPlane2++;
					
					
						}//<---End Plane 2
					}
					
					
   					n2dVtx++;
   					}//<--end ww loop
				
					
				}//<---End plane loop
			}//<---End tpc loop
		}//<---End cstat loop

	}//<--End checking if we have 2d end points
	
fTwoDNVtxPlane0->Fill( n2dVtxPlane0 );
fTwoDNVtxPlane1->Fill( n2dVtxPlane1 );
fTwoDNVtxPlane2->Fill( n2dVtxPlane2 );

// =================================================================================================================
// ==================================== Looping over 3dVertex information ==========================================
// =================================================================================================================
   art::PtrVector<recob::Vertex>     Vertexlist;
   
   double xyz[3] = {0.};

   for(unsigned int ii = 0; ii < vertex3dListHandle->size(); ++ii)
   	{
      	art::Ptr<recob::Vertex> vertex3d(vertex3dListHandle, ii);
	Vertexlist.push_back(vertex3d); // class member
	
    	}//<---End ii loop
	
   if(Vertexlist.size() > 0 && vertexWstrengthplane0 && vertexWstrengthplane1)
    	{
	for(int ww = 0; ww<Vertexlist.size(); ww++)
		{
		Vertexlist[ww]->XYZ(xyz);
		
		// ###########################################################################
   		// ### Checking how well we did in reconstructing the vertex (Reco - True) ###
   		// ###########################################################################
   
   		// === Finding the Delta X, Y, Z between Reco vtx and truth ===
	
   		double DeltaX = truth_vertex[0] - xyz[0];
   		double DeltaY = truth_vertex[1] - xyz[1];
   		double DeltaZ = truth_vertex[2] - xyz[2];
   	
   		double DeltaXoverTrueX = DeltaX / xyz[0];
   		double DeltaYoverTrueY = DeltaY / xyz[1];
   		double DeltaZoverTrueZ = DeltaZ / xyz[2];
   
   		fRecoVtxXPos->Fill( xyz[0] );
   		fRecoVtxYPos->Fill( xyz[1] );
   		fRecoVtxZPos->Fill( xyz[2] );
   		
   		fRecoCheck3dVtxX->Fill( DeltaX );
   		fRecoCheck3dVtxY->Fill( DeltaY );
   		fRecoCheck3dVtxZ->Fill( DeltaZ );
   
   		fRecoCheck3dVtxXvsX->Fill( xyz[0], DeltaXoverTrueX );
   		fRecoCheck3dVtxYvsY->Fill( xyz[1], DeltaYoverTrueY );
   		fRecoCheck3dVtxZvsZ->Fill( xyz[2], DeltaZoverTrueZ );
		
		
		
		
		}
	}
   
return;  
}//<---End access the event


DEFINE_ART_MODULE(FeatureVertexFinderAna);

}// end of vertex namespace

#endif // FEATUREVERTEXFINDERANA_H