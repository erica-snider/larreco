////////////////////////////////////////////////////////////////////////
// $Id: KingaClusterAna.cxx,v 1.36 2010/09/15  kpartyka Exp $
//
// KingaCluster class
//
// 
//
////////////////////////////////////////////////////////////////////////
#ifndef KingaCluster_H
#define KingaCluster_H

#include "TMath.h"
#include <vector>
#include <string>
#include "art/Persistency/Common/PtrVector.h" 


#include "art/Framework/Core/EDProducer.h"

class TH1F;
class TH2F;
class TTree;
namespace recob { class Hit; }
namespace cluster {
   
  class KingaCluster : public art::EDProducer {
    
  public:
    
    explicit KingaCluster(fhicl::ParameterSet const& pset); 
    KingaCluster();
    ~KingaCluster();
         
    void produce(art::Event& evt);
    void beginJob();
    void AngularDistribution(unsigned int tpc, unsigned int plane);
    void FindMax(unsigned int tpc, unsigned int plane);
    //void FinalPeaks();
    void FitAngularDistributions();
    void FindClusters(unsigned int tpc, unsigned int plane);
    void ReassignHitID(unsigned int tpc, unsigned int plane,unsigned int HitPosition,unsigned int WrongPeakNo);
 
  
     
  private:
  
    std::string fDBScanModuleLabel;  
    std::string fGenieGenModuleLabel;  
    std::string fEndPoint2DModuleLabel;
    std::string fClusterCheaterModuleLabel;
    std::string fLineMergerModuleLabel;
   std::vector<int> fclusters_planeNo_reco_;
 std::vector<double> fStart_pt_w_reco_;
 std::vector<double> fStart_pt_t_reco_;
    int fkingaCl_near_vertex_p0;
    int fkingaCl_near_vertex_p1;
    int fcheatedCl_p0;
    int fcheatedCl_p1;
     int flinemergerCl_p0;
    int flinemergerCl_p1;
    int fcheatedCl_near_vertex_p0;
    int fcheatedCl_near_vertex_p1;
    int flinemergerCl_near_vertex_p0;
    int flinemergerCl_near_vertex_p1;
    TH1F *fdiff_time_vtx_p0;
    TH1F *fdiff_wire_vtx_p0;
    TH1F *fdiff_wire_vtx_p1;
    TH1F *fh_theta_ind;
    TH1F *fh_theta_coll;
    TH1F *fh_theta_ind_2D;
    TH1F *fh_theta_coll_2D;
    TH1F *fh_theta_coll_Area;
    TH1F *fh_theta_ind_Area;
    TH1F *Hit_Area_Ind;
    TH1F *Hit_Area_Coll;
    TH1F *fdiff_no_vertex_clusters_p0;
    TH1F *fdiff_no_vertex_clusters_p1;
    TH1F *fdiff_no_vertex_linemergerclusters_p0;
    TH1F *fdiff_no_vertex_linemergerclusters_p1;
    
    //TTree:
     TTree* fTree;
     int frun;
    int fevent;
    int fno_clusters_true;
    int fno_clusters_reco;
    int fno_clusters_linemerger;
    double ftime_vertex_true;
    double *fwire_vertex_true;
    double *fTTree_wire_vertex_reco;
    double *fTTree_time_vertex_reco;
    int *fclusters_planeNo_true;
    int *fclusters_planeNo_reco;
    double *fStart_pt_w_true;
    double *fStart_pt_t_true;
    double *fStart_pt_w_reco;
    double *fStart_pt_t_reco;
    double *fStart_pt_t_linemerger;
    double *fStart_pt_w_linemerger;
     int *flinemergerclusters_planeNo;
    int *fcheated_cluster_size;
    int *flinemerger_cluster_size;
    
    
     //std::vector<TH1F*> fh_theta;     /**Histo for the angular distribution theta of the shower*/
    art::PtrVector<recob::Hit> allhits;
    std::vector<int> maxBin;    //stores bin # of local maximum
    std::vector<int> MaxStartPoint; //bin no of the starting point of a peak
    std::vector<int> MaxEndPoint;  //bin no of the end point of a peak
    std::vector<int> MaxStartPointTheta; //theta value of the starting point of a peak
    std::vector<int> MaxEndPointTheta; //theta value of the end point of a peak
    std::vector<int> fwire_vertex, fwire_vertex_reco;
    std::vector<double> ftime_vertex, ftime_vertex_reco;
    std::vector<unsigned int> HitsWithClusterID;
    double ftimetick; //get from parameterset
    double fdriftvelocity;  //get from paramtereset 9either k and V)
    double fpi;
    int fMC;
    double MCvertex [3];

    std::vector<double> maxBinValues;
    std::vector<double> OriginalmaxBinValues;
    std::vector<int> SortedMaxBin;
    std::vector<int> FinalPeaks;
    int fpeaks_found; //flag to determine whether the program should continue or not
    bool need_to_reassign_hitsIDs;
    bool go_ahead_at_reassign;
  protected:

    
  };
  
 // class AngCluster{
//   public:
//    AngCluster();
//    ~AngCluster();
//    void beginJob();
//    void AngularDistribution(art::PtrVector<recob::Hit>);
//   
//   private:
//   TH1F *fh_theta_ind;
//     TH1F *fh_theta_coll;
//     TH1F *fh_theta_ind_2D;
//     TH1F *fh_theta_coll_2D;
//   
//   
//   };
}



#endif // KingaCluster_H
