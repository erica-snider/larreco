////////////////////////////////////////////////////////////////////////
//
// \file SmallClusterFinder.cxx
//
// andrzej.szelc@yale.edu 
// corey.adams@yale.edu
//
// This algorithm is designed to fid small clusters that could correspond to gama or low energy e
//
/*	There are two parameters that matter from the fcl file:
		fNHitsInClust is the number of hits that should be in these small clusters
			^-- Gamma events seem to rarely have more than 4 hits in the cluster
			^-- SN events are unclear.  Should this even be used for SN?
		fRadiusSizePar is the distance (in cm) between the small clusters and any other hits.
	
	This algorithm sorts the hits by plane, and then looks at each hit individually.  If
	there is a hit within RadiusSizePar, it gets added to a local list.  All other hits
	are ignored.  Then, if the number of hits that got added to the local list is greater 
	then NHitsInClust, the original hit is ignored.  If it's less, the original hit is 
	presumed to be part of a very small (or single hit) cluster.  So its added to the list
	of hits in the small cluster.
	
	This algorithm dumps all the hits that could be part of a small cluster into one cluster
	There is no grouping done between individual hits or anything like that.
	
	The scheme for cluster identification was:
	Clusters with ID 0 to nPlanes-1 are the gammas on planes 0, 1, ... nPlanes-1
	Clusters with ID nPlanes to 2*nPlanes -1 are the leftover hits on the planes 0, 1, ... nPlanes-1
	
	But has been updated to:
	Clusters are ID'd with numbers like 107, meaning the 7th cluster on plane 1.  The formula
	is ID = 100*iPlane + Cluster on that plane.  
	
	By Convention, cluster 000, 100, 200, etc. are the leftover hits that aren't gammas.
	
	-Corey
*/
//
// 
///////////////////////////////////////////////////////////////////////

#ifndef SMALLCLUSTERFINDERALG_H
#define SMALLCLUSTERFINDERALG_H

#include "art/Framework/Core/EDProducer.h" // include the proper bit of the framework
#include "art/Framework/Services/Registry/ActivityRegistry.h"
#include <vector>
#include <string>

#include "RecoBase/Cluster.h"
#include "RecoBase/Hit.h"
#include "Utilities/LArProperties.h"
#include "Utilities/GeometryUtilities.h"
#include "Utilities/DetectorProperties.h"



//class recob::Hit;
// class TH1F;
// class TF1;
// class TTree;

struct wire {
	std::vector<art::Ptr<recob::Hit> > hits;
};


namespace cluster {

  class SmallClusterFinderAlg {
    
  public:

    /**METHODS global*/
    SmallClusterFinderAlg(); //default constructor
    SmallClusterFinderAlg(fhicl::ParameterSet const& pset);/**Constructor*/
   // ~SmallClusterFinderAlg();                               /**Destructor*/
    void reconfigure(fhicl::ParameterSet const& pset);
  //  void produce(art::Event& evt);                       /**Routine that finds the cluster and sets the dTdW of the 2D shower*/
   
    
    void SelectLocalHitlist(std::vector< art::Ptr < recob::Hit> > hitlist, std::vector < art::Ptr<recob::Hit> > &hitlistlocal, double  wire_start,double time_start, double radlimit);
    void SelectLocalHitlist(std::vector< art::Ptr < recob::Hit> > hitlist, std::vector < art::Ptr<recob::Hit> > &hitlistlocal,
    				 double  wire_start,double time_start, double radlimit, std::vector<int> &);

	//pass in a hit, all the others are filled in the function.
    int GetPlaneAndTPC(	art::Ptr<recob::Hit> a, //a hit
    					unsigned int &p, //the plane
    					unsigned int &cs, //cryostat
    					unsigned int &t, //time
    					unsigned int &w); //wire
    
    //passes in a hitlist, returns a list of gammas and puts everything else in hitlistleftover
    std::vector< art::Ptr<recob::Hit> > CreateHighHitlist(std::vector< art::Ptr<recob::Hit> > hitlist,
    													  std::vector< art::Ptr<recob::Hit> > &hitlistleftover);
    void FindSmallClusters(std::vector<art::Ptr<recob::Hit> > allHits); //this routine actually finds the clusters
    	//It stores them in smallClustList
    	//It puts the leftover hits in hitlistleftover
    
    void sortLeftoverHits();
    	//This function is solely to sort the leftover hits.  It's not done automatically
    	//because it's not a short process if the number of hits is big.  For a few hits
    	//its not that bad.
    	//not yet functional!
    	
    	
    std::vector< std::vector <art::Ptr< recob::Hit> > > GetSmallClustersByPlane(unsigned int iPlane);
		//This function just returns the clusters by plane, and they are split already.
		
	std::vector< art::Ptr<recob::Hit> > GetLeftoversByPlane(unsigned int iPlane);
		//This function returns the leftover hits.
		
  private:

	//Special function to make sure the vectors are all the right size
	//can't do the hits, but can clear them and do the planes.
	//Gets called each time FindSmallClusters is called, as well as in the constructor.
    void ClearandResizeVectors();

    //**************All of the constants needed to do this process:
    double fWiretoCm,fTimetoCm,fWireTimetoCmCm;
    float fTimeTick; 							// time sample in us
    float fDriftVelocity;						// used to convert time to cm
    double fWirePitch ;   						// wire pitch in cm
    std::vector< unsigned int >fNWires;			// Number of wires on each plane
	unsigned int fNPlanes; // number of planes  
    double fNTimes;

    //The various art grabbed Larsoft stuff to get physical properties.
    art::ServiceHandle<geo::Geometry> geom;
    art::ServiceHandle<util::DetectorProperties> detp;
    art::ServiceHandle<util::LArProperties> larp;
    util::GeometryUtilities gser;
   
   	//The vectors that actually get used to hold hits, sets of hits, etc.
   	//All have the plane as the first index
   	std::vector < std::vector< std::vector< art::Ptr<recob::Hit> > > > smallClustList;
   		//This list is meant to contain the small clusters.
   		//It should be indexed by smallClustList[plane][smallcluster][hit]
    std::vector < std::vector< art::Ptr<recob::Hit> > > hitlistrefined;  //list, by plane, of hits that ought to be gammas
    std::vector < std::vector< art::Ptr<recob::Hit> > > hitlistbyplane;  //list of all hits on each plane
    std::vector < std::vector< art::Ptr<recob::Hit> > > hitlistleftover;  //list of all hits on each plane that are NOT gammas
    
    //input parameters, grabbed from the parameter set
    bool verbose;		//Make lots of printouts?
    double fRadiusSizePar;		//Determines the max radius of the cluster, must be separated
    double fNHitsInClust;		//Forces cluster to have a max number of hits
    							// Remember, this is the *small* cluster finder       
  }; // class SmallAngleFinder
}
#endif // SHOWERANGLECLUSTER_H
