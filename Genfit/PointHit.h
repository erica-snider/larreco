#ifndef POINTHIT_HH
#define POINTHIT_HH

// Base Class Headers ----------------
#include "GFRecoHitIfc.h"
#include "GFSpacepointHitPolicy.h"

// Collaborating Class Headers -------
#include <ostream> // remove if you do not need streaming op

// Collaborating Class Declarations --

namespace genf {

typedef GFRecoHitIfc<GFSpacepointHitPolicy> SpacepointRecoHit;

class PointHit : public SpacepointRecoHit {
public:

  // Constructors/Destructors ---------
  PointHit();
  PointHit(TVector3 point,double res);

  virtual ~PointHit();

  virtual GFAbsRecoHit* clone();
  
  // Operations ----------------------
  virtual TMatrixT<Double_t> getHMatrix(const GFAbsTrackRep* stateVector);


private:

  // Private Data Members ------------
  static const int NparHitRep = 3;

  // Private Methods -----------------

  //public:
  //ClassDef(PointHit,1)

};


} // namespace genf

#endif

//--------------------------------------------------------------
// $Log$
//--------------------------------------------------------------
