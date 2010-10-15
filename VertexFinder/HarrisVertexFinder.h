////////////////////////////////////////////////////////////////////////
/// \file  DriftElectrons.h
/// \brief Module to finde vertices
///
/// \author  joshua.spitz@yale.edu
////////////////////////////////////////////////////////////////////////

#ifndef HarrisVertexFinder_H
#define HarrisVertexFinder_H

#include "FWCore/Framework/interface/EDProducer.h"
#include "TMath.h"
#include <vector>
#include <string>
namespace edm {   
  class Event;
  class ParameterSet; 
}

class TH1F;
class TTree;
namespace vertex {
   
 class HarrisVertexFinder :  public edm::EDProducer {
    
  public:
    
    explicit HarrisVertexFinder(edm::ParameterSet const& pset); 
    ~HarrisVertexFinder();        
    double Gaussian(int x, int y, double sigma);
    double GaussianDerivativeX(int x, int y);
    double GaussianDerivativeY(int x, int y);
    void produce(edm::Event& evt, edm::EventSetup const&);
    void beginJob(edm::EventSetup const&);
    
  private:
  
    std::string fHitModuleLabel;
    std::string fInputFolder;
    std::string fOutputFolder;
    int fTimeBins;
    int fMaxCorners;
    double fGsigma;
    int fWindow;
    double fThreshold;
    int fSaveVertexMap;
  };
  
  
}



#endif // HarrisVertexFinder_H
