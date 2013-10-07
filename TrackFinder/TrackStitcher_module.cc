////////////////////////////////////////////////////////////////////////
//
// \file TrackStitcher
//
// \author echurch@fnal.gov
//
//  This algorithm is designed to join tracks that point in roughly same direction
//  and whose endpoints are suitably close.
////////////////////////////////////////////////////////////////////////

// C++ includes
#include <math.h>
#include <iostream>
#include <fstream>
#include <algorithm>

// Framework includes
#include "art/Framework/Principal/Event.h" 
#include "fhiclcpp/ParameterSet.h" 
#include "art/Framework/Principal/Handle.h" 
#include "art/Framework/Principal/View.h" 
#include "art/Persistency/Common/Ptr.h" 
#include "art/Persistency/Common/PtrVector.h" 
#include "art/Framework/Services/Registry/ServiceHandle.h" 
#include "art/Framework/Services/Optional/TFileService.h" 
#include "art/Framework/Services/Optional/TFileDirectory.h" 
#include "messagefacility/MessageLogger/MessageLogger.h" 

// LArSoft includes
#include "Geometry/Geometry.h"
#include "Geometry/PlaneGeo.h"
#include "Geometry/WireGeo.h"
#include "RecoBase/Track.h"
#include "RecoBase/SpacePoint.h"

#include "Simulation/sim.h"
#include "SimulationBase/MCTruth.h"
#include "Utilities/AssociationUtil.h"


#include "art/Framework/Core/ModuleMacros.h" 
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Services/Optional/RandomNumberGenerator.h"
#include <TTree.h>
#include <TMatrixT.h>


#include "CLHEP/Random/RandFlat.h"
#include "CLHEP/Random/RandGaussQ.h"

#include <vector>
#include <string>


// ROOT includes
#include "TVectorD.h"
#include "TFile.h"
#include "TGeoManager.h"
#include "TF1.h"
#include "TGraph.h"
#include "TMath.h"
#include "TPrincipal.h"
#include "TDatabasePDG.h"
#include "Utilities/AssociationUtil.h"

// Leave the sorting functions here, in case we want them later.
/*
static bool sp_sort_3dz(const art::Ptr<recob::SpacePoint>& h1, const art::Ptr<recob::SpacePoint>& h2)
{
  const double* xyz1 = h1->XYZ();
  const double* xyz2 = h2->XYZ();
  return xyz1[2] < xyz2[2];
}
*/

namespace trkf {

  class TrackStitcher : public art::EDProducer {
    
  public:
    
    explicit TrackStitcher(fhicl::ParameterSet const& pset);
    virtual ~TrackStitcher();
    
    //////////////////////////////////////////////////////////
    void produce(art::Event& evt); 
    void beginJob();
    void endJob();
    void reconfigure(fhicl::ParameterSet const& p);

  private:

    const recob::Track Stitch(const art::PtrVector<recob::Track> &);
    std::string     fTrackModuleLabel;// label for input collection
    std::string     fSpptModuleLabel;// label for input collection
    double fCosAngTol;
    double fSepTol;

    int ftNo;

  }; // class TrackStitcher

} // end namespace for declarations

namespace trkf {

//-------------------------------------------------
  TrackStitcher::TrackStitcher(fhicl::ParameterSet const& pset) 
    : ftNo(0)
  {
    
    this->reconfigure(pset);
    
    produces< std::vector<recob::Track>                  >();
    produces<std::vector<art::PtrVector<recob::Track> >  >(); 
    // get the random number seed, use a random default if not specified    
    // in the configuration file.  
    unsigned int seed = pset.get< unsigned int >("Seed", sim::GetRandomNumberSeed());

    createEngine( seed );

  }

//-------------------------------------------------
  void TrackStitcher::reconfigure(fhicl::ParameterSet const& pset) 
  {
    fTrackModuleLabel    = pset.get< std::string >("TrackModuleLabel");
    fSpptModuleLabel     = pset.get< std::string >("SpptModuleLabel"); 
    fCosAngTol           = pset.get< double >("CosAngTolerance", 0.05); 
    fSepTol              = pset.get< double >("SpptSepTolerance", 3.0); //cm 
  }
  
  //-------------------------------------------------
  TrackStitcher::~TrackStitcher()
  {
  }


//-------------------------------------------------
  void TrackStitcher::beginJob()
  {
    art::ServiceHandle<art::TFileService> tfs;
  }

//-------------------------------------------------
  void TrackStitcher::endJob()
  {
  }


//------------------------------------------------------------------------------------//
  void TrackStitcher::produce(art::Event& evt)
  { 

    // get services
    art::ServiceHandle<geo::Geometry> geom;

    //////////////////////////////////////////////////////
    // Make a std::unique_ptr<> for the thing you want to put into the event
    // because that handles the memory management for you
    //////////////////////////////////////////////////////
    // tcol is the collection of new tracks
    std::unique_ptr<std::vector<recob::Track> > tcol(new std::vector<recob::Track>);
    // tvcol is the collection of vectors that comprise each tcol
    std::unique_ptr<std::vector< art::PtrVector<recob::Track> > > tvcol(new std::vector< art::PtrVector<recob::Track> >);
    
    art::PtrVector <recob::Track> tcolTmp;
    // define TPC parameters
    TString tpcName = geom->GetLArTPCVolumeName();

    art::Handle< std::vector< recob::Track > > tListHandle;
    evt.getByLabel(fTrackModuleLabel,tListHandle);
    
    mf::LogWarning("TrackStitcher.beginning") << "There are " <<  tListHandle->size() << " Tracks in this event before stitching.";
    
    int ntrack = tListHandle->size();
    for(int ii = 0; ii < ntrack; ++ii) {
      art::Ptr<recob::Track> ptrack1(tListHandle, ii);
            const recob::Track& track1 = *ptrack1;
      const TVector3 start1(track1.Vertex());
      const TVector3 end1(track1.End());
      const TVector3 start1Dir(track1.VertexDirection());
      const TVector3 end1Dir(track1.EndDirection());

      // If we don't already have track1, start a new tcolTmp.
      // determination of which requires a loop over all tracks in all vectors
      // and also the current list of tracks, not yet put in a vector.
      bool alreadyHaveIt(false);
      for (std::vector< art::PtrVector<recob::Track> >::const_iterator  itv=tvcol->begin(); itv<tvcol->end() && !alreadyHaveIt; ++itv ) 
	{
	  art::PtrVector<recob::Track> tvcolTmpInner(*itv);
	  for (std::vector< art::Ptr<recob::Track> >::const_iterator  it=tvcolTmpInner.begin(); it<tvcolTmpInner.end() && !alreadyHaveIt; ++it ) 
	    {
	      if ((*it).get() == ptrack1.get()) 
		{
		  alreadyHaveIt=true;
		}
	    }
	}

      for (std::vector< art::Ptr<recob::Track> >::const_iterator  it=tcolTmp.begin(); it<tcolTmp.end() && !alreadyHaveIt; ++it ) 
	{
	  if ((*it).get() == ptrack1.get()) 
	    {
	      alreadyHaveIt=true;
	    }
	}
      

      if (!alreadyHaveIt)
	{
	  if (tcolTmp.size()!=0) 
	    {
	      // I've finished building a vector. push it back onto tvcol, 
	      // make a Stitched vector.
	      tvcol->push_back(tcolTmp);
	      const recob::Track& t = Stitch(tcolTmp);
	      tcol->push_back(t);
	      tcolTmp.erase(tcolTmp.begin(),tcolTmp.end());
	      // also make the cumulative track
	    }
	  // start with 1st elment of a new vector of tracks.
	  tcolTmp.push_back(ptrack1);
	}


      for(int jj = ii+1; jj < ntrack; ++jj) {
	art::Ptr<recob::Track> ptrack2(tListHandle, jj);
	const recob::Track& track2 = *ptrack2;
	const TVector3& start2(track2.Vertex());
	const TVector3& end2(track2.End());
	const TVector3& start2Dir(track2.VertexDirection());
	const TVector3& end2Dir(track2.EndDirection());
	/*
	std::cout << "abs(start1Dir.Dot(end2Dir)) " << std::abs(start1Dir.Dot(end2Dir)) << ", start1-end2.Mag(): " << (start1-end2).Mag() << std::endl;
	std::cout << "abs(end1Dir.Dot(start2Dir)) " << std::abs(end1Dir.Dot(start2Dir)) << ", start2-end1.Mag(): " << (start2-end1).Mag() << std::endl;
	std::cout << "abs(start1Dir.Dot(start2Dir)) " << std::abs(start1Dir.Dot(start2Dir)) << ", start1-start2.Mag(): " << (start1-start2).Mag() << std::endl;
	std::cout << "abs(end1Dir.Dot(end2Dir)) " << std::abs(end1Dir.Dot(end2Dir)) << ", end1-end2.Mag(): " << (end1-end2).Mag() << std::endl;
	*/

	if (
	    (std::abs(start1Dir.Dot(end2Dir))>fCosAngTol && ((start1-end2).Mag())<fSepTol ) ||
	    (std::abs(end1Dir.Dot(start2Dir))>fCosAngTol && ((start2-end1).Mag())<fSepTol ) ||
	    (std::abs(start1Dir.Dot(start2Dir))>fCosAngTol && ((start1-start2).Mag())<fSepTol ) ||
	    (std::abs(end1Dir.Dot(end2Dir))>fCosAngTol && ((end1-end2).Mag())<fSepTol ) 
	    )
	  {
	    tcolTmp.push_back(ptrack2);
	    break;
	  }
	
      } // jj
      
    } // ii

    if (tcolTmp.size()!=0) 
      {
	tvcol->push_back(tcolTmp);
	const recob::Track& t = Stitch(tcolTmp);
	tcol->push_back(t);
      }
    
    mf::LogWarning("TrackStitcher.end") << "There are " <<  tvcol->size() << " Tracks in this event after stitching.";
    // It is redundant and wasteful to store the tcol. We can just get the individual tracks
    // from the original Tracking module that made them.
    //    evt.put(std::move(tcol)); 
    evt.put(std::move(tvcol));
    
  }
  

  const recob::Track TrackStitcher::Stitch(const art::PtrVector<recob::Track> &tv)
  {

    // take the vector of tracks, walk through each track's vectors of xyz, dxdydz, etc 
    // and concatenate them into longer vectors. Use those to instantiate one new 
    // Stitched-together track.
    std::vector<TVector3> xyz;
    std::vector<TVector3> dxdydz;
    std::vector<TMatrixT<double> > cov;
    std::vector<double> mom;
    std::vector< std::vector <double> > dQdx;
    //art::PtrVector<recob::Track>::const_iterator
    for (auto it = tv.begin(); it!=tv.end(); ++it)
      {
	for (size_t pt = 0; pt!=(*it).get()->NumberTrajectoryPoints(); pt++)
	  {
	    xyz.push_back((*it).get()->LocationAtPoint(pt));
	    dxdydz.push_back((*it).get()->DirectionAtPoint(pt));
	    cov.push_back((*it).get()->CovarianceAtPoint(pt));
	    mom.push_back((*it).get()->MomentumAtPoint(pt));
	    std::vector <double> dum; // someone insisted this must be a vector ...
	    if (pt<(*it).get()->NumberdQdx(geo::kZ))
	      dum.push_back((*it).get()->DQdxAtPoint(pt,geo::kZ));
	    else
	      dum.push_back(0.0);
	    dQdx.push_back(dum);
	  }
      }
   
    /// TODO: sort according to spacepoint distance separation.
    /// As is, we're not sure we're not forming a stitched track with a (some) 
    /// jump(s) and a reversal(s) of direction in it.

    /// But for now we'll instantiate the stitched track faithfully.

    const recob::Track t(xyz,dxdydz,cov,dQdx,mom,ftNo++);
    return t;
  }

  DEFINE_ART_MODULE(TrackStitcher)


}  // end namespace
