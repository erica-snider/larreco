////////////////////////////////////////////////////////////////////////
// ClusterCrawlerAlg.h
//
// ClusterCrawlerAlg class
//
// Bruce Baller
//
///////////////////////////////////////////////////////////////////////
#ifndef CLUSTERCRAWLERALG_H
#define CLUSTERCRAWLERALG_H

#include "TMath.h"

#include <vector>

#include "fhiclcpp/ParameterSet.h" 
#include "art/Persistency/Common/Ptr.h" 
#include "art/Persistency/Common/PtrVector.h" 

#include "Geometry/Geometry.h"
#include "RecoBase/Hit.h"
#include "Utilities/LArProperties.h"
#include "Utilities/DetectorProperties.h"
#include "Utilities/AssociationUtil.h"
#include "RecoAlg/CCHitFinderAlg.h"

namespace recob { class Hit; }

namespace cluster {


  class ClusterCrawlerAlg {
    public:

    // struct of temporary clusters
    struct ClusterStore {
      short ID;         // Cluster ID. ID < 0 = abandoned cluster
      short ProcCode;   // Processor code for debugging
      short Assn;       // coded pointer to associated clusters
      short StopCode;   // code for the reason for stopping cluster tracking
      unsigned short CTP; // Cryostat/TPC/Plane code
      float BeginSlp;   // beginning slope (= DS end = high wire number)
      float BeginSlpErr; // error
      unsigned short BeginWir;   // begin wire
      float BeginTim;   // begin time
      float BeginChg;   // beginning average charge
      float EndSlp;     // end slope (= US end = low  wire number)
      float EndSlpErr;  // error
      unsigned short EndWir;     // end wire
      float EndTim;     // ending time
      float EndChg;     // ending average charge
      short BeginVtx;   // ID of the begin vertex
      short EndVtx;     // ID of the end vertex
      std::vector<unsigned short> tclhits; // hits on the cluster
    };
    std::vector< ClusterStore > tcl;

    // struct of temporary vertices
    struct VtxStore {
      unsigned short Wire;
      float Time;
      float Wght;
      short Topo;
      unsigned short CTP;
    };
    std::vector< VtxStore > vtx;
    std::vector< std::pair<short, short> > vtxRange;

    ClusterCrawlerAlg(fhicl::ParameterSet const& pset);
    virtual ~ClusterCrawlerAlg();

    void reconfigure(fhicl::ParameterSet const& pset);
    void RunCrawler(std::vector<CCHitFinderAlg::CCHit>& allhits);
    
    unsigned short fNumPass;                 ///< number of passes over the hit collection
    std::vector<unsigned short> fMaxHitsFit; ///< Max number of hits fitted
    std::vector<unsigned short> fMinHits;    ///< Min number of hits to make a cluster
    std::vector<unsigned short> fNHitsAve;   ///< number of US hits used to compute fAveChg and fAveWid
                                    ///< set to > 2 to do a charge fit using fNHitsAve hits
    std::vector<float> fChiCut;     ///< stop adding hits to clusters if chisq too high
    std::vector<float> fKinkChiRat; ///< Max consecutive chisq increase for the last 
                                    ///< 3 hits on the cluster
    std::vector<float> fKinkAngCut; ///< kink angle cut made after fKinkChiRat
    std::vector<float> fWidCut;     ///< chisq cut for adding a hit to a cluster
    std::vector<float> fChgCut;     ///< charge difference cut for adding a hit to a cluster
    std::vector<unsigned short> fMaxWirSkip; ///< max number of wires that can be skipped while following
                                    ///< a cluster
    std::vector<unsigned short> fMinWirAfterSkip; ///< minimum number of hits on consecutive wires
                                    ///< after skipping
    std::vector<bool> fDoMerge;     ///< try to merge clusters?
    std::vector<float> fTimeDelta;  ///< max time difference for matching
    std::vector<bool> fDoVertex;    ///< run vertexing code after clustering?
    std::vector<bool> fLAClFollow;  ///< Follow large angle clusters (and merge hits)?

    // global cuts and parameters 
    float fHitErrFac;   ///< hit time error = fHitErrFac * (EndTime - PeakTime)
                        ///< used for cluster fit
    float fHitWidFac;   ///< hit width = fHitWidFac * (EndTime - PeakTime)
                        ///< used to decide if there is a signal near the projected
                        ///< cluster position
    short fDebugPlane;
    short fDebugWire;  ///< set to the Begin Wire and Hit of a cluster to print
    short fDebugHit;   ///< out detailed information while crawling

    // fills a wirehitrange vector for the supplied Cryostat/TPC/Plane code
    void GetHitRange(std::vector<CCHitFinderAlg::CCHit>& allhits,
      unsigned short CTP, 
      std::vector< std::pair<short, short> >& wirehitrange,
      unsigned short& firstwire, unsigned short& lastwire);
    
    private:
    
    bool prt;
    bool vtxprt;
    unsigned short NClusters;
    
    art::ServiceHandle<geo::Geometry> geom;
    art::ServiceHandle<util::LArProperties> larprop;
    art::ServiceHandle<util::DetectorProperties> detprop;

    // these variables define the cluster used during crawling
    float clpar[2];     ///< cluster parameters for the current fit with
                        ///< origin at the US wire on the cluster
    float clparerr[2];  ///< cluster parameter errors
    float clChisq;     ///< chisq of the current fit
    float clBeginSlp;  ///< begin slope (= DS end = high wire number)
    float clBeginSlpErr; 
    unsigned short clBeginWir;  ///< begin wire
    float clBeginTim;  ///< begin time
    float clBeginChg;  ///< begin average charge
    float clEndSlp;    ///< slope at the end   (= US end = low  wire number)
    float clEndSlpErr; 
    unsigned short clEndWir;    ///< begin wire
    float clEndTim;    ///< begin time
    float clEndChg;    ///< end average charge
    short clStopCode;     ///< code for the reason for stopping cluster tracking
                        ///< 0 = no signal on the next wire
                        ///< 1 = skipped too many occupied/dead wires
                        ///< 2 = failed the fMinWirAfterSkip cut
                        ///< 3 = ended on a kink. Fails fKinkChiRat
                        ///< 4 = failed the fChiCut cut
                        ///< 5 = cluster split by cl2VtxClusterSplit
    short clProcCode;     ///< Processor code = pass number
                        ///< +   10 cl2ChkMerge
                        ///< +  100 cl2ChkMerge12
                        ///< +  200 cl2ClusterFix
                        ///< 666 LAClFollow
                        ///< + 2000 failed pass N cuts but passes pass N=1 cuts
    short clAssn;         ///< index of a parent cluster. -1 if no parent.
                        ///< Parent clusters are not associated with daughters
    unsigned short clCTP; // Cryostat/TPC/Plane code
    
    unsigned short fFirstWire;    ///< the first wire with a hit
    unsigned short fFirstHit;     ///< first hit used for indexing hiterr2
    unsigned short fLastWire;      ///< the last wire with a hit
    float fAveChg;  ///< average charge at leading edge of cluster
    float fChgSlp;  ///< slope of the  charge vs wire 
    unsigned int cstat;         // the current cryostat
    unsigned int tpc;         // the current TPC
    unsigned int plane;         // the current plane
    unsigned short fNumWires;   // number of wires in the current plane
    unsigned short fMaxTime;    // number of time samples in the current plane
    float fScaleF;     ///< scale factor from Tick/Wire to dx/du
    
    unsigned short pass;

    // vector of pairs of first (.first) and last+1 (.second) hit on each wire
    // in the range fFirstWire to fLastWire. A value of -2 indicates that there
    // are no hits on the wire. A value of -1 indicates that the wire is dead
    std::vector< std::pair<short, short> > WireHitRange;
    std::vector<float> hiterr2;     ///< hit time error^2
    std::vector<float> hitwid;     ///< hit width
    
    std::vector<unsigned short> fcl2hits;  ///< vector of hits used in the cluster
    std::vector<float> chifits;   ///< fit chisq for monitoring kinks, etc
    bool followLACluster;   ///< following a Large Angle Cluster?

    std::string fhitsModuleLabel;
    
    // Sorts clusters in tcl by decreasing number of hits, while ignoring
    // abandoned clusters
    void cl2SortByLength(std::vector<ClusterStore>& tcl,
        std::map<unsigned short, unsigned short>& sortindex);
    // inits everything
    void cl2Init();
    // Loops over wires looking for seed clusters
    void cl2ClusterLoop(std::vector<CCHitFinderAlg::CCHit>& allhits);
    // Finds a hit on wire kwire, adds it to the cluster and re-fits it
    void cl2AddHit(std::vector<CCHitFinderAlg::CCHit>& allhits, 
      unsigned short kwire, bool& HitOK, bool& SigOK);
    // Fits the cluster hits in fcl2hits to a straight line
    void cl2Fit(std::vector<CCHitFinderAlg::CCHit>& allhits);
    // Fits the middle of a temporary cluster it1 using hits iht to iht + nhit
    void cl2FitMid(std::vector<CCHitFinderAlg::CCHit>& allhits, 
      std::vector<ClusterStore>& tcl, unsigned short it1, unsigned short iht, short nhit);
    // Fits the charge of the cluster hits in fcl2hits
    void cl2FitChg(std::vector<CCHitFinderAlg::CCHit>& allhits);
    // Follows a trail of hits UpStream
    void cl2FollowUS(std::vector<CCHitFinderAlg::CCHit>& allhits);
    // Follows a trail of hits UpStream - Large Angle version
    void LAClFollow(std::vector<CCHitFinderAlg::CCHit>& allhits);
    // fix up hits on large angle clusters
    void cl2MergeHits(std::vector<CCHitFinderAlg::CCHit>& allhits,
      unsigned short theHit);
    // Do a quality control check on clusters
    void cl2QACheck(std::vector<CCHitFinderAlg::CCHit>& allhits);
    // Stores cluster information in a temporary vector
    void cl2TmpStore(std::vector<CCHitFinderAlg::CCHit>& allhits, 
      std::vector<ClusterStore>& tcl);
    // Gets a temp cluster and puts it into the working cluster variables
    void cl2TmpGet(std::vector<CCHitFinderAlg::CCHit>& allhits, 
      std::vector<ClusterStore>& tcl, unsigned short it1);
    // Splits a cluster into two clusters at position pos. Associates the
    // new clusters with a vertex (optional)
    void cl2DoSplit(std::vector<CCHitFinderAlg::CCHit>& allhits,
      std::vector<ClusterStore>& tcl, unsigned short icl, unsigned short pos,
      unsigned short ivx);
    // Compares two cluster combinations to see if they should be merged
    void cl2ChkMerge(std::vector<CCHitFinderAlg::CCHit>& allhits,
      std::vector<ClusterStore>& tcl);
    // Checks merge for cluster cl2 within the bounds of cl1
    void cl2ChkMerge12(std::vector<CCHitFinderAlg::CCHit>& allhits,
      std::vector<ClusterStore>& tcl, unsigned short it1, unsigned short it2, bool& didit);
    // Merges clusters cl1 and cl2
    void cl2DoMerge(std::vector<CCHitFinderAlg::CCHit>& allhits, 
      std::vector<ClusterStore>& tcl, std::vector<VtxStore>& vtx,
      unsigned short it1, unsigned short it2, 
      short ProcCode);
    // Prints cluster information to the screen
    void cl2Print(std::vector<CCHitFinderAlg::CCHit>& allhits, 
     std::vector<ClusterStore>& tcl);
    // Split clusters that cross preliminary vertices
    void cl2VtxClusterSplit(std::vector<CCHitFinderAlg::CCHit>& allhits,
       std::vector<ClusterStore>& tcl, std::vector<VtxStore>& vtx);
    // make 2D vertices
    void cl2DoVertex(std::vector<CCHitFinderAlg::CCHit>& allhits,
      std::vector<ClusterStore>& tcl, std::vector<VtxStore>& vtx);
    // use a vertex constraint to start a cluster
    void cl2VtxConstraint(std::vector<CCHitFinderAlg::CCHit>& allhits,
      std::vector<VtxStore>& vtx, unsigned short iwire, unsigned short ihit,
      unsigned short jwire, unsigned short& useHit, bool& doConstrain);
    // check for a signal on all wires between two points
    void cl2ChkSignal(std::vector<CCHitFinderAlg::CCHit>& allhits,
      unsigned short wire1, float time1, unsigned short wire2, float time2, bool& SigOK);
    // check a vertex (vw, fvt) made with clusters it1, and it2 against the
    // vector of existing clusters
    void cl2ChkVertex(std::vector<CCHitFinderAlg::CCHit>& allhits,
        std::vector<ClusterStore>& tcl, std::vector<VtxStore>& vtx,
        short vw, float fvt, unsigned short it1, unsigned short it2, short topo);
    // try to attach a cluster to an existing vertex
    void cl2ClsVertex(std::vector<CCHitFinderAlg::CCHit>& allhits, 
        std::vector<ClusterStore>& tcl, std::vector<VtxStore>& vtx,
        unsigned short it2);
    // fit the vertex position
    void cl2VtxFit(std::vector<ClusterStore>& tcl,
        std::vector<VtxStore>& vtx, unsigned short iv, float& ChiDOF);
    // line fitter
    void LinFit(std::vector<float>& x, std::vector<float>& y, 
      std::vector<float>& ey2, float& Intercept, float& Slope, 
      float& InterceptError, float& SlopeError, float& ChiDOF);

  }; // class ClusterCrawlerAlg


} // namespace cluster

#endif // ifndef CLUSTERCRAWLERALG_H
