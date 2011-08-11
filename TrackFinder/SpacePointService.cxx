///////////////////////////////////////////////////////////////////////
///
/// \file   SpacePointService.cxx
///
/// \brief  Service for generating space points from hits.
///
/// \author H. Greenlee 
///
////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <sstream>
#include <cmath>
#include <map>
#include <algorithm>
#include "cetlib/exception.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "TrackFinder/SpacePointService.h"
#include "Geometry/geo.h"
#include "Utilities/LArProperties.h"
#include "art/Persistency/Common/PtrVector.h"
#include "art/Framework/Services/Optional/TFileService.h" 
#include "Simulation/SimChannel.h"
#include "Simulation/Electrons.h"
#include "art/Framework/Core/View.h"
#include "Utilities/DetectorProperties.h"
#include "MCCheater/BackTracker.h"
#include "TH1F.h"

namespace {

  // Function classes for sorting pointer to Electrons according to
  // embedded position TVector3.

  class ElectronsLess {
  public:
    bool operator()(const sim::Electrons* p1, const sim::Electrons* p2) {
      TVector3 v1 = p1->XYZ();
      TVector3 v2 = p2->XYZ();
      bool result = (v1.x() < v2.x() || 
		     (v1.x() == v2.x() && 
		      (v1.y() < v2.y() || 
		       (v1.y() == v2.y() && v1.z() < v2.z()))));
      return result;
    }
  };

  class ElectronsEqual {
  public:
    bool operator()(const sim::Electrons* p1, const sim::Electrons* p2) {
      TVector3 v1 = p1->XYZ();
      TVector3 v2 = p2->XYZ();
      bool result = (v1.x() == v2.x() && v1.y() == v2.y() && v1.z() < v2.z());
      return result;
    }
  };

}

//----------------------------------------------------------------------
// Constructor.
//
trkf::SpacePointService::SpacePointService(const fhicl::ParameterSet& pset,
					   art::ActivityRegistry& reg) :
  fUseMC(false),
  fMaxDT(0.),
  fMaxS(0.),
  fTimeOffsetU(0.),
  fTimeOffsetV(0.),
  fTimeOffsetW(0.),
  fMinViews(1000),
  fEnableU(false),
  fEnableV(false),
  fEnableW(false),
  fTimePitch(0.)
{
  reconfigure(pset);
}

//----------------------------------------------------------------------
// Destructor.
//
trkf::SpacePointService::~SpacePointService()
{
}

//----------------------------------------------------------------------
// Update configuration parameters.
//
void trkf::SpacePointService::reconfigure(const fhicl::ParameterSet& pset)
{
  // Get configuration parameters.

  fUseMC = pset.get<bool>("UseMC", false);
  fMaxDT = pset.get<double>("MaxDT", 0.);
  fMaxS = pset.get<double>("MaxS", 0.);

  fTimeOffsetU = pset.get<double>("TimeOffsetU", 0.);
  fTimeOffsetV = pset.get<double>("TimeOffsetV", 0.);
  fTimeOffsetW = pset.get<double>("TimeOffsetW", 0.);

  fMinViews = pset.get<int>("MinViews", 1000);

  fEnableU = pset.get<bool>("EnableU", false);
  fEnableV = pset.get<bool>("EnableV", false);
  fEnableW = pset.get<bool>("EnableW", false);

  // Report.

  mf::LogInfo("SpacePointService") 
    << "SpacePointService configured with the following parameters:\n"
    << "  UseMC = " << fUseMC << "\n"
    << "  MaxDT = " << fMaxDT << "\n"
    << "  MaxS = " << fMaxS << "\n"
    << "  TimeOffsetU = " << fTimeOffsetU << "\n"
    << "  TimeOffsetV = " << fTimeOffsetV << "\n"
    << "  TimeOffsetW = " << fTimeOffsetW << "\n" 
    << "  MinViews = " << fMinViews << "\n"
    << "  EnableU = " << fEnableU << "\n"
    << "  EnableV = " << fEnableV << "\n"
    << "  EnableW = " << fEnableW;
}

//----------------------------------------------------------------------
// Update geometry constants.
//
void trkf::SpacePointService::update() const
{
  // Generate info report on first call only.

  bool report = (fTimePitch == 0.);

  // Get services.

  art::ServiceHandle<geo::Geometry> geom;
  art::ServiceHandle<util::DetectorProperties> detprop;
  art::ServiceHandle<util::LArProperties> larprop;

  // Reset geometry constants so that we don't
  // accidentally inherit from previous update.

  fTimeOffset.clear();
  fTimePitch = 0.;

  // Calculate and print geometry information.

  mf::LogInfo log("SpacePointService");
  if(report)
    log << "Updating geometry constants.\n";

  // Loop over TPCs.

  int ntpc = geom->NTPC();

  fTimeOffset.resize(ntpc);

  for(int tpc = 0; tpc < ntpc; ++tpc) {
    const geo::TPCGeo& tpcgeom = geom->TPC(tpc);

    // Loop over planes.

    int nplane = tpcgeom.Nplanes();
    fTimeOffset[tpc].resize(nplane, 0.);

    for(int plane = 0; plane < nplane; ++plane) {
      const geo::PlaneGeo& pgeom = tpcgeom.Plane(plane);

      // Fill view-dependent quantities.

      geo::View_t view = pgeom.View();
      std::string viewname = "?";
      if(view == geo::kU) {
	fTimeOffset[tpc][plane] = fTimeOffsetU;
	viewname = "U";
      }
      else if(view == geo::kV) {
	fTimeOffset[tpc][plane] = fTimeOffsetV;
	viewname = "V";
      }
      else if(view == geo::kW) {
	fTimeOffset[tpc][plane] = fTimeOffsetW;
	viewname = "W";
      }
      else
	throw cet::exception("SpacePointService") << "Bad view = " 
						  << view << "\n";

      std::string sigtypename = "?";
      geo::SigType_t sigtype = pgeom.SignalType();
      if(sigtype == geo::kInduction)
	sigtypename = "Induction";
      else if(sigtype == geo::kCollection)
	sigtypename = "Collection";
      else
	throw cet::exception("SpacePointService") << "Bad signal type = " 
						  << sigtype << "\n";

      std::string orientname = "?";
      geo::Orient_t orient = pgeom.Orientation();
      if(orient == geo::kVertical)
	orientname = "Vertical";
      else if(orient == geo::kHorizontal)
	orientname = "Horizontal";
      else
	throw cet::exception("SpacePointService") << "Bad orientation = " 
						  << orient << "\n";

      if(report) {
	log << "\nTPC, Plane: " << tpc << ", " << plane << "\n"
	    << "  View: " << viewname << "\n"
	    << "  SignalType: " << sigtypename << "\n"
	    << "  Orientation: " << orientname << "\n";
      }

      if(orient != geo::kVertical)
	throw cet::exception("SpacePointService") 
	  << "Horizontal wire geometry not implemented.\n";
    }
  }


  // Update detector properties.

  double samplingRate = detprop->SamplingRate();
  double triggerOffset = detprop->TriggerOffset();
  if(report) {
    log << "\nDetector properties:\n"
	<< "  Sampling Rate = " << samplingRate << " ns/tick\n"
	<< "  Trigger offset = " << triggerOffset << " ticks\n";
  }

  // Update LArProperties.

  double efield = larprop->Efield();
  double temperature = larprop->Temperature();
  double driftVelocity = larprop->DriftVelocity(efield, temperature);
  fTimePitch = 0.001 * driftVelocity * samplingRate;
  if(report) {
    log << "\nLAr propertoes:\n"
	<< "  E field = " << efield << " kV/cm\n"
	<< "  Temperature = " << temperature << " K\n"
	<< "  Drift velocity = " << driftVelocity << " cm/us\n"
	<< "  Time pitch = " << fTimePitch << " cm/tick";
  }
}

//----------------------------------------------------------------------
// Get time offset for the specified view.
double trkf::SpacePointService::getTimeOffset(geo::View_t view)
{
  if(view == geo::kU)
    return fTimeOffsetU;
  else if(view == geo::kV)
    return fTimeOffsetV;
  else if(view == geo::kW)
    return fTimeOffsetW;
  else
    throw cet::exception("SpacePointService") << "Bad view = " << view << "\n";
}

//----------------------------------------------------------------------
// Get corrected time for the specified hit.
double trkf::SpacePointService::correctedTime(const recob::Hit& hit)
{
  return hit.PeakTime() - getTimeOffset(hit.View());
}

//----------------------------------------------------------------------
// Spatial separation of hits (zero if two or fewer).
double trkf::SpacePointService::separation(const art::PtrVector<recob::Hit>& hits)
{
  // Get geometry service.

  art::ServiceHandle<geo::Geometry> geom;

  // Trivial case - fewer than three hits.

  if(hits.size() < 3)
    return 0.;

  // Error case - more than three hits.

  if(hits.size() > 3) {
    mf::LogError("SpacePointService") << "Method separation called with more than three htis.";
      return 0.;
  }

  // Got exactly three hits.

  assert(hits.size() == 3);

  // Calculate angles and distance of each hit from origin.

  double dist[3] = {0., 0., 0.};
  double sinth[3] = {0., 0., 0.};
  double costh[3] = {0., 0., 0.};
  unsigned int tpcs[3];
  unsigned int planes[3];

  for(int i=0; i<3; ++i) {

    // Get tpc, plane, wire.

    const recob::Hit& hit = *(hits[i]);
    unsigned short channel = hit.Channel();
    unsigned int tpc, plane, wire;
    const geo::WireGeo& wgeom = geom->ChannelToWire(channel, tpc, plane, wire);
    tpcs[i] = tpc;
    planes[i] = plane;

    // Check tpc and plane errors.

    for(int j=0; j<i; ++j) {

      if(tpcs[j] != tpc) {
	mf::LogError("SpacePointService") << "Method separation called with hits from multiple tpcs..";
	return 0.;
      }

      if(planes[j] == plane) {
	mf::LogError("SpacePointService") << "Method separation called with hits from the same plane..";
	return 0.;
      }
    }

    // Get angles and distance of wire.

    double hl = wgeom.HalfL();
    double xyz[3];
    double xyz1[3];
    wgeom.GetCenter(xyz);
    wgeom.GetCenter(xyz1, hl);
    double s  = (xyz1[1] - xyz[1]) / hl;
    double c = (xyz1[2] - xyz[2]) / hl;
    sinth[plane] = s;
    costh[plane] = c;
    dist[plane] = xyz[2] * s - xyz[1] * c;
  }

  double S = ((sinth[1] * costh[2] - costh[1] * sinth[2]) * dist[0] 
	      +(sinth[2] * costh[0] - costh[2] * sinth[0]) * dist[1] 
	      +(sinth[0] * costh[1] - costh[0] * sinth[1]) * dist[2]);
  return S;
}

//----------------------------------------------------------------------
// Check hits for compatibility.
// Check hits pairwise for different views and maximum time difference.
// Check three hits for spatial compatibility.
bool trkf::SpacePointService::compatible(const art::PtrVector<recob::Hit>& hits,
					 double maxDT, double maxS) const
{
  // Get geometry service.

  art::ServiceHandle<geo::Geometry> geom;

  // Get cuts.

  if(maxDT == 0.)
    maxDT = fMaxDT;
  if(maxS == 0.)
    maxS = fMaxS;  

  int nhits = hits.size();

  // Fewer than two or more than three hits can never be compatible.

  bool result = nhits >= 2 && nhits <= 3;
  bool mc_ok = true;
  unsigned int tpc = 0;

  if(result) {

    // First do pairwise tests.
    // Do double loop over hits.

    for(int ihit1 = 0; result && ihit1 < nhits-1; ++ihit1) {
      const recob::Hit& hit1 = *(hits[ihit1]);
      unsigned short channel1 = hit1.Channel();
      unsigned int tpc1, plane1, wire1;
      geom->ChannelToWire(channel1, tpc1, plane1, wire1);
      geo::View_t view1 = hit1.View();
      double t1 = hit1.PeakTime() - fTimeOffset[tpc1][plane1];

      // If using mc information, make a collection of electrons for hit 1.

      const std::vector<const sim::Electrons*>& electrons1 = fElecMap[&hit1];

      // Loop over second hit.

      for(int ihit2 = ihit1+1; result && ihit2 < nhits; ++ihit2) {
	const recob::Hit& hit2 = *(hits[ihit2]);
	unsigned short channel2 = hit2.Channel();
	unsigned int tpc2, plane2, wire2;
	geom->ChannelToWire(channel2, tpc2, plane2, wire2);
	geo::View_t view2 = hit2.View();

	// Test for same tpc and different views.

	result = result && tpc1 == tpc2 && view1 != view2;
	if(result) {

	  // Remember which tpc we are in.

	  tpc = tpc1;

	  double t2 = hit2.PeakTime() - fTimeOffset[tpc2][plane2];
    
	  // Test maximum time difference.

	  result = result && std::abs(t1-t2) <= maxDT;

	  // If using mc information, make a collection of electrons for hit 2.

	  if(fUseMC) {

	    // Find electrons that are in time with hit 2.

	    std::vector<const sim::Electrons*> electrons2 = fElecMap[&hit2];
	    std::vector<const sim::Electrons*>::iterator it =
	      std::set_intersection(electrons1.begin(), electrons1.end(),
				    electrons2.begin(), electrons2.end(),
				    electrons2.begin(), ElectronsLess());
	    electrons2.resize(it - electrons2.begin());

	    // Hits are compatible if they have points in common.

	    mc_ok = electrons2.size() > 0;
	    result = result && mc_ok;
	  }
	}
      }
    }

    // If there are exactly three hits, and they pass pairwise tests, check
    // for spatial compatibility.

    if(result && nhits == 3) {

      // Loop over hits.

      double dist[3] = {0., 0., 0.};
      double sinth[3] = {0., 0., 0.};
      double costh[3] = {0., 0., 0.};
      double time[3];
      geo::View_t view[3];

      for(int i=0; i<3; ++i) {

	// Get tpc, plane, wire.

	const recob::Hit& hit = *(hits[i]);
	unsigned short channel = hit.Channel();
	unsigned int tpc0, plane, wire;
	const geo::WireGeo& wgeom = geom->ChannelToWire(channel, tpc0, plane, wire);
	assert(tpc0 == tpc);
	view[i] = hit.View();

	// Get corrected time.

	time[i] = hit.PeakTime() - fTimeOffset[tpc][plane];

	// Get angles and distance of wire.

	double hl = wgeom.HalfL();
	double xyz[3];
	double xyz1[3];
	wgeom.GetCenter(xyz);
	wgeom.GetCenter(xyz1, hl);
	double s  = (xyz1[1] - xyz[1]) / hl;
	double c = (xyz1[2] - xyz[2]) / hl;
	sinth[plane] = s;
	costh[plane] = c;
	dist[plane] = xyz[2] * s - xyz[1] * c;
      }

      // Do space cut.

      double S = ((sinth[1] * costh[2] - costh[1] * sinth[2]) * dist[0] 
		  +(sinth[2] * costh[0] - costh[2] * sinth[0]) * dist[1] 
		  +(sinth[0] * costh[1] - costh[0] * sinth[1]) * dist[2]);

      result = result && std::abs(S) < maxS;
    }
  }

  // Done.
    
  return result;
}

//----------------------------------------------------------------------
// Fill one space point using a colleciton of hits.
// Assume points have already been tested for compatibility.
//
void trkf::SpacePointService::fillSpacePoint(const art::PtrVector<recob::Hit>& hits,
					     recob::SpacePoint& spt) const
{
  // Get services.

  art::ServiceHandle<geo::Geometry> geom;
  art::ServiceHandle<util::DetectorProperties> detprop;
  art::ServiceHandle<util::LArProperties> larprop;

  double triggerOffset = detprop->TriggerOffset();

  // Store hits in SpacePoint.

  spt = recob::SpacePoint(recob::SpacePoint(hits));
  int nhits = hits.size();

  // Calculate position.

  double xyz[3] = {0., 0., 0.};

  // Calculate x using drift times.
  // Loop over all hits and calculate the weighted average drift time.

  double sumtw = 0.;
  double sumw = 0.;

  for(art::PtrVector<recob::Hit>::const_iterator ihit = hits.begin();
      ihit != hits.end(); ++ihit) {

    const recob::Hit& hit = **ihit;
    unsigned short channel = hit.Channel();
    unsigned int tpc, plane, wire;
    geom->ChannelToWire(channel, tpc, plane, wire);

    // Correct time for trigger offset and view-dependent time offsets.
    // Assume time error is proportional to (end time - start time).

    double t = hit.PeakTime() - triggerOffset - fTimeOffset[tpc][plane];
    double et = hit.EndTime() - hit.StartTime();
    double w = 1./(et*et);

    sumtw += w*t;
    sumw += w;
  }

  double drift_time = 0.;
  if(sumw != 0.)
    drift_time = sumtw / sumw;
  xyz[0] = drift_time * fTimePitch;

  // Calculate y, z using wires (need at least two hits).

  if(nhits >= 2) {

    // Calculate y and z by chisquare minimization of wire coordinates.

    double sus = 0.;   // sum u_i sin_th_i
    double suc = 0.;   // sum u_i cos_th_i
    double sc2 = 0.;   // sum cos2_th_i
    double ss2 = 0.;   // sum sin2_th_i
    double ssc = 0.;   // sum sin_th_i cos_th_i

    // Loop over points.

    for(art::PtrVector<recob::Hit>::const_iterator ihit = hits.begin();
	ihit != hits.end(); ++ihit) {

      const recob::Hit& hit = **ihit;
      unsigned short channel = hit.Channel();
      unsigned int tpc, plane, wire;
      const geo::WireGeo& wgeom = geom->ChannelToWire(channel, tpc, plane, wire);

      // Calculate angle and wire coordinate in this view.
    
      double hl = wgeom.HalfL();
      double xyz[3];
      double xyz1[3];
      wgeom.GetCenter(xyz);
      wgeom.GetCenter(xyz1, hl);
      double s  = (xyz1[1] - xyz[1]) / hl;
      double c = (xyz1[2] - xyz[2]) / hl;
      double u = xyz[2] * s - xyz[1] * c;

      // Summations

      sus += u*s;
      suc += u*c;
      sc2 += c*c;
      ss2 += s*s;
      ssc += s*c;
    }

    // Calculate y,z

    double denom = sc2 * ss2 - ssc * ssc;
    if(denom != 0.) {
      xyz[1] = (-suc * ss2 + sus * ssc) / denom;
      xyz[2] = (sus * sc2 - suc * ssc) / denom;
    }

    // Set coordintates in space point.

    spt.SetXYZ(xyz);
  }
}

//----------------------------------------------------------------------
// Fill a vector of space points for all compatible combinations of hits
// from an input vector of hits (non-mc-truth version).
//
void trkf::SpacePointService::makeSpacePoints(const art::PtrVector<recob::Hit>& hits,
					      std::vector<recob::SpacePoint>& spts,
					      double maxDT, double maxS) const
{
  std::vector<art::Ptr<sim::SimChannel> > empty;
  makeSpacePoints(hits, spts, empty, maxDT, maxS);
}

//----------------------------------------------------------------------
// Fill a vector of space points for all compatible combinations of hits
// from an input vector of hits (mc truth version).
//
void trkf::SpacePointService::makeSpacePoints(const art::PtrVector<recob::Hit>& hits,
					      std::vector<recob::SpacePoint>& spts,
					      const std::vector<art::Ptr<sim::SimChannel> >& simchans,
					      double maxDT, double maxS) const
{
  // Get cuts.

  if(maxDT == 0.)
    maxDT = fMaxDT;
  if(maxS == 0.)
    maxS = fMaxS;  

  // Get geometry service.

  art::ServiceHandle<geo::Geometry> geom;

  // Precalculate certain geometry constants and properties.

  update();

  // First make result vector is empty.

  spts.erase(spts.begin(), spts.end());

  // Statistics.

  int n2 = 0;  // Number of two-hit space points.
  int n3 = 0;  // Number of three-hit space points.

  // If fUseMC is true, verify that channels are sorted by channel number.

  if(fUseMC) {

    unsigned int nsc = simchans.size();
    for(unsigned int isc = 0; isc < nsc; ++isc) {
      const sim::SimChannel& sc = *simchans[isc];
      if(isc != sc.Channel())
	throw cet::exception("SpacePointService") << "MC channels not sorted.\n";
    }
  }

  // Sort hits into maps indexed by [tpc][plane][time].
  // If using mc information, also generate maps of electrons and mc 
  // position indexed by hit.

  std::vector<std::vector<std::map<double, art::Ptr<recob::Hit> > > > hitmap;
  fElecMap.clear();

  int ntpc = geom->NTPC();
  hitmap.resize(ntpc);
  for(int tpc = 0; tpc < ntpc; ++tpc) {
    int nplane = geom->Nplanes(tpc);
    hitmap[tpc].resize(nplane);
  }

  for(art::PtrVector<recob::Hit>::const_iterator ihit = hits.begin();
      ihit != hits.end(); ++ihit) {
    const art::Ptr<recob::Hit>& phit = *ihit;
    geo::View_t view = phit->View();
    if((view == geo::kU && fEnableU) ||
       (view == geo::kV && fEnableV) ||
       (view == geo::kW && fEnableW)) {

      unsigned short channel = phit->Channel();
      unsigned int tpc, plane, wire;
      geom->ChannelToWire(channel, tpc, plane, wire);

      double t = phit->PeakTime() - fTimeOffset[tpc][plane];
      hitmap[tpc][plane][t] = phit;
      const recob::Hit& hit = *phit;

      // Get Electrons.

      if(fUseMC) {
	std::vector<const sim::Electrons*>& electrons = fElecMap[&hit];   // Empty vector.
	cheat::BackTracker::HitToElectrons(simchans, phit, electrons);
	std::sort(electrons.begin(), electrons.end(), ElectronsLess());
	std::vector<const sim::Electrons*>::iterator it = 
	  std::unique(electrons.begin(), electrons.end(), ElectronsEqual());
	electrons.resize(it - electrons.begin());
      }
    }
  }

  mf::LogDebug debug("SpacePointService");
  debug << "Total hits = " << hits.size() << "\n\n";

  for(int tpc = 0; tpc < ntpc; ++tpc) {
    int nplane = hitmap[tpc].size();
    for(int plane = 0; plane < nplane; ++plane) {
      debug << "TPC, Plane: " << tpc << ", " << plane 
	    << ", hits = " << hitmap[tpc][plane].size() << "\n";
    }
  }

  // Loop over TPCs.

  for(int tpc = 0; tpc < ntpc; ++tpc) {

    // Sort maps in increasing order of number of hits.
    // This is so that we can do the outer loops over hits 
    // over the views with fewer hits.

    int nplane = hitmap[tpc].size();
    std::vector<int> index(nplane);

    for(int i=0; i<nplane; ++i)
      index[i] = i;

    for(int i=0; i<nplane-1; ++i) {
      for(int j=i+1; j<nplane; ++j) {
	if(hitmap[tpc][index[i]].size() > hitmap[tpc][index[j]].size()) {
	  int temp = index[i];
	  index[i] = index[j];
	  index[j] = temp;
	}
      }
    }

    // If two-view space points are allowed, make a double loop
    // over hits and produce space points for compatible hit-pairs.

    if(fMinViews <= 2) {

      // Loop over pairs of views.

      for(int i=0; i<nplane-1; ++i) {
	int plane1 = index[i];
	for(int j=i+1; j<nplane; ++j) {
	  int plane2 = index[j];

	  assert(hitmap[tpc][plane1].size() <= hitmap[tpc][plane2].size());

	  // Loop over pairs of hits.

	  art::PtrVector<recob::Hit> hitvec;
	  hitvec.reserve(2);

	  for(std::map<double, art::Ptr<recob::Hit> >::const_iterator
		ihit1 = hitmap[tpc][plane1].begin();
	      ihit1 != hitmap[tpc][plane1].end(); ++ihit1) {

	    const art::Ptr<recob::Hit>& phit1 = ihit1->second;

	    double t1 = phit1->PeakTime() - fTimeOffset[tpc][plane1];
	    double t2min = t1 - maxDT;
	    double t2max = t1 + maxDT;

	    for(std::map<double, art::Ptr<recob::Hit> >::const_iterator 
		  ihit2 = hitmap[tpc][plane2].lower_bound(t2min);
		ihit2 != hitmap[tpc][plane2].upper_bound(t2max); ++ihit2) {

	      const art::Ptr<recob::Hit>& phit2 = ihit2->second;

	      // Check current pair of hits for compatibility.
	      // By construction, hits should always have compatible views 
	      // and times, but may not have compatible mc information.

	      hitvec.clear();
	      hitvec.push_back(phit1);
	      hitvec.push_back(phit2);
	      bool ok = compatible(hitvec, maxDT, maxS);
	      if(ok) {

		// Add a space point.

		++n2;
		spts.push_back(recob::SpacePoint());
		fillSpacePoint(hitvec, spts.back());
	      }
	    }
	  }
	}
      }
    }

    // If three-view space points are allowed, make a tripe loop
    // over hits and produce space points for compatible triplets.

    if(nplane >= 3 && fMinViews <= 3) {

      // Loop over triplets of hits.

      art::PtrVector<recob::Hit> hitvec;
      hitvec.reserve(3);

      int plane1 = index[0];
      for(std::map<double, art::Ptr<recob::Hit> >::const_iterator
	    ihit1 = hitmap[tpc][plane1].begin();
	  ihit1 != hitmap[tpc][plane1].end(); ++ihit1) {

	const art::Ptr<recob::Hit>& phit1 = ihit1->second;

	double t1 = phit1->PeakTime() - fTimeOffset[tpc][plane1];
	double t2min = t1 - maxDT;
	double t2max = t1 + maxDT;

	int plane2 = index[1];
	for(std::map<double, art::Ptr<recob::Hit> >::const_iterator 
	      ihit2 = hitmap[tpc][plane2].lower_bound(t2min);
	    ihit2 != hitmap[tpc][plane2].upper_bound(t2max); ++ihit2) {

	  const art::Ptr<recob::Hit>& phit2 = ihit2->second;

	  // Test first two hits for compatibility before looping 
	  // over third hit.

	  hitvec.clear();
	  hitvec.push_back(phit1);
	  hitvec.push_back(phit2);
	  bool ok = compatible(hitvec, maxDT, maxS);
	  if(ok) {

	    double t2 = phit2->PeakTime() - fTimeOffset[tpc][plane2];
	    double t3min = std::max(t1, t2) - maxDT;
	    double t3max = std::min(t1, t2) + maxDT;

	    int plane3 = index[2];
	    for(std::map<double, art::Ptr<recob::Hit> >::const_iterator 
		  ihit3 = hitmap[tpc][plane3].lower_bound(t3min);
		ihit3 != hitmap[tpc][plane3].upper_bound(t3max); ++ihit3) {

	      const art::Ptr<recob::Hit>& phit3 = ihit3->second;

	      // Test triplet for compatibility.

	      hitvec.clear();
	      hitvec.push_back(phit1);
	      hitvec.push_back(phit2);
	      hitvec.push_back(phit3);
	      bool ok = compatible(hitvec, maxDT, maxS);

	      if(ok) {

		// Add a space point.

		++n3;
		spts.push_back(recob::SpacePoint());
		fillSpacePoint(hitvec, spts.back());
	      }
	    }
	  }
	}
      }
    }
  }

  debug << "\n2-hit space points = " << n2 << "\n"
	<< "3-hit space points = " << n3;
}
