////////////////////////////////////////////////////////////////////////
//
// EndPointClusAlg class
//
// Ben Carls, bcarls@fnal.gov
//
// This is a version of the EndPointClusAlg specialized to the needs of
// fuzzy clustering.
//
////////////////////////////////////////////////////////////////////////
/// The algorithm is based on:
///C. Harris and M. Stephens (1988). "A combined corner and edge detector". Proceedings of the 4th Alvey 
///Vision Conference. pp. 147-151.
///B. Morgan (2010). "Interest Point Detection for Reconstruction in High Granularity Tracking Detectors". 
///arXiv:1006.3012v1 [physics.ins-det]
//Thanks to B. Morgan of U. of Warwick for comments and suggestions

#include <iostream>
#include <vector>
extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
}
#include <sstream>
#include <fstream>
#include <math.h>
#include <algorithm>

#include "TMath.h"

// Framework includes
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "ClusterFinder/EndPointClusAlg.h"
#include "Utilities/LArProperties.h"
#include "Utilities/DetectorProperties.h"
#include "Utilities/AssociationUtil.h"
#include "RecoBase/recobase.h"
#include "Geometry/geo.h"

//-----------------------------------------------------------------------------
cluster::EndPointClusAlg::EndPointClusAlg(fhicl::ParameterSet const& pset) 
{
  this->reconfigure(pset);
}

//-----------------------------------------------------------------------------
cluster::EndPointClusAlg::~EndPointClusAlg()
{
}

//-----------------------------------------------------------------------------
void cluster::EndPointClusAlg::reconfigure(fhicl::ParameterSet const& p)
{
  fTimeBins      = p.get< int    >("TimeBins");
  fMaxCorners    = p.get< int    >("MaxCorners");
  fGsigma        = p.get< double >("Gsigma");
  fWindow        = p.get< int    >("Window");
  fThreshold     = p.get< double >("Threshold");
  fSaveVertexMap = p.get< int    >("SaveVertexMap");
}

//-----------------------------------------------------------------------------
double cluster::EndPointClusAlg::Gaussian(int x, int y, double sigma)
{
  double Norm=1./sqrt(2*TMath::Pi()*pow(sigma,2));
  double value=Norm*exp(-(pow(x,2)+pow(y,2))/(2*pow(sigma,2)));
  return value;
}

//-----------------------------------------------------------------------------
double cluster::EndPointClusAlg::GaussianDerivativeX(int x,int y)
{
  double Norm=1./(sqrt(2*TMath::Pi())*pow(fGsigma,3));
  double value=Norm*(-x)*exp(-(pow(x,2)+pow(y,2))/(2*pow(fGsigma,2)));
  return value;
}

//-----------------------------------------------------------------------------
double cluster::EndPointClusAlg::GaussianDerivativeY(int x,int y)
{
  double Norm=1./(sqrt(2*TMath::Pi())*pow(fGsigma,3));
  double value=Norm*(-y)*exp(-(pow(x,2)+pow(y,2))/(2*pow(fGsigma,2)));
  return value;
}


//-----------------------------------------------------------------------------
//this method saves a BMP image of the vertex map space, which can be viewed with gimp
void cluster::EndPointClusAlg::VSSaveBMPFile(const char *fileName, unsigned char *pix, int dx, int dy)
{
  ofstream bmpFile(fileName, std::ios::binary);
  bmpFile.write("B", 1);
  bmpFile.write("M", 1);
  int bitsOffset = 54 +256*4; 
  int size = bitsOffset + dx*dy; //header plus 256 entry LUT plus pixels
  bmpFile.write((const char *)&size, 4);
  int reserved = 0;
  bmpFile.write((const char *)&reserved, 4);
  bmpFile.write((const char *)&bitsOffset, 4);
  int bmiSize = 40;
  bmpFile.write((const char *)&bmiSize, 4);
  bmpFile.write((const char *)&dx, 4);
  bmpFile.write((const char *)&dy, 4);
  short planes = 1;
  bmpFile.write((const char *)&planes, 2);
  short bitCount = 8;
  bmpFile.write((const char *)&bitCount, 2);
  int i, temp = 0;
  for (i=0; i<6; i++)
    bmpFile.write((const char *)&temp, 4);  // zero out optional color info
  // write a linear LUT
  char lutEntry[4]; // blue,green,red
  lutEntry[3] = 0;  // reserved part
  for (i=0; i<256; i++)
    {
      lutEntry[0] =i;
      lutEntry[1] =i+1;
      lutEntry[2] = i+2;
      bmpFile.write(lutEntry, sizeof lutEntry);
    }
  // write the actual pixels
  bmpFile.write((const char *)pix, dx*dy);
}

//......................................................
size_t cluster::EndPointClusAlg::EndPoint(art::PtrVector<recob::Hit>& hits, std::vector<unsigned int> *corners)

{

  art::ServiceHandle<geo::Geometry> geom;
  art::ServiceHandle<util::LArProperties> larp;
  art::ServiceHandle<util::DetectorProperties> detp;
   
  // function below does not link, so commenting
  //extern void SaveBMPFile(const char *f, unsigned char *pix, int dxx, int dyy);
  
  //Point to a collection of vertices to output.
  //art::PtrVector<recob::Hit> hit;
   
  int flag   = 0;
  int windex = 0;//the wire index to make sure the end point finder does not fall off the edge of the hit map
  int tindex = 0;//the time index to make sure the end point finder does not fall off the edge of the hit map
  int n      = 0; //index of window cell. There are 49 cells in the 7X7 Gaussian and Gaussian derivative windows
  unsigned int numberwires = 0;
  double numbertimesamples = 0.;
  double MatrixAAsum = 0.;
  double MatrixBBsum = 0.;
  double MatrixCCsum = 0.;
  std::vector<double> Cornerness2;
  //gaussian window definitions. The cell weights are calculated here to help the algorithm's speed
  double  w[49] = {0.};
  double wx[49] = {0.};
  double wy[49] = {0.};
  int ctr = 0;
  for(int i = -3; i < 4; ++i){
    for(int j = 3; j > -4; --j){
      w[ctr] = Gaussian(i, j, fGsigma);
      wx[ctr] = GaussianDerivativeX(i,j);
      wy[ctr] = GaussianDerivativeY(i,j);
      ++ctr;
    }
  }
  
  unsigned int channel = hits[0]->Wire()->RawDigit()->Channel();
  unsigned int plane   = 0;
  unsigned int wire    = 0;
  unsigned int wire2   = 0;
  unsigned int tpc     = 0;
  unsigned int cstat   = 0;
  unsigned int p  = 0;
  unsigned int t  = 0;
  unsigned int c = 0;
  unsigned int w2 = 0;
  geom->ChannelToWire(channel, c, t, p, w2);


	art::PtrVector<recob::Hit> vHits;
	//art::PtrVector<recob::Cluster>::const_iterator clusterIter = clusIn.begin();
	//geo::View_t view = geom->Plane(p,t,c).View();
	//size_t cinctr = 0;
	//while(clusterIter!= clusIn.end() ) {
	  //if((*clusterIter)->View() == view){
	    //hit = util::FindManyP<recob::Hit>(clusIn, evt, label, cinctr);
	  //}//end if cluster is in the correct view
	  //clusterIter++;  
	  //++cinctr;
	//} 
	
	if(hits.size() == 0) return 0;
	
	numberwires = geom->Cryostat(c).TPC(t).Plane(p).Nwires();
	numbertimesamples = hits[0]->Wire()->NSignal();
	mf::LogInfo("EndPointClusAlg") << " --- endpoints check " 
				       << numberwires  
				       << numbertimesamples  
				       << fTimeBins;
      
	std::vector < std::vector < double > > MatrixAsum(numberwires);
	std::vector < std::vector < double > > MatrixBsum(numberwires);
	std::vector < std::vector < double > > hit_map(numberwires); //the map of hits 
	
	//the index of the hit that corresponds to the potential corner
	std::vector < std::vector < int > > hit_loc(numberwires);
	
	std::vector < std::vector < double > >  Cornerness(numberwires); //the "weight" of a corner
	
	for(unsigned int wi = 0; wi < numberwires; ++wi){  
	  hit_map[wi].resize(fTimeBins,0);
	  hit_loc[wi].resize(fTimeBins,-1);
	  Cornerness[wi].resize(fTimeBins,0);
	  MatrixAsum[wi].resize(fTimeBins,0);
	  MatrixBsum[wi].resize(fTimeBins,0);
	}      
	for(unsigned int i = 0; i < hits.size(); ++i){
	  channel=hits[i]->Wire()->RawDigit()->Channel();
	  geom->ChannelToWire(channel, cstat, tpc, plane, wire);
	  //pixelization using a Gaussian
	  for(int j = 0; j <= (int)(hits[i]->EndTime()-hits[i]->StartTime()+.5); ++j)    
	    hit_map[wire][(int)((hits[i]->StartTime()+j)*(fTimeBins/numbertimesamples)+.5)] += Gaussian((int)(j-((hits[i]->EndTime()-hits[i]->StartTime())/2.)+.5),0,hits[i]->EndTime()-hits[i]->StartTime());      
	}
	
	// Gaussian derivative convolution  
	for(unsigned int wire = 1; wire < numberwires-1; ++wire){
	  
	  for(int timebin = 1; timebin < fTimeBins-1; ++timebin){
	    MatrixAsum[wire][timebin] = 0.;
	    MatrixBsum[wire][timebin] = 0.;
	    n = 0;
	    for(int i = -3; i <= 3; ++i) {
	      windex = wire+i;
	      if(windex < 0 ) windex = 0;
	      // this is ok, because the line before makes sure it's not negative
	      else if ((unsigned int)windex >= numberwires) windex = numberwires-1; 
	      
	      for(int j = -3; j <= 3; ++j){
		tindex = timebin+j;
		if(tindex < 0) tindex=0;
		else if(tindex >= fTimeBins) tindex = fTimeBins-1;
		
		MatrixAsum[wire][timebin] += wx[n]*hit_map[windex][tindex];  
		MatrixBsum[wire][timebin] += wy[n]*hit_map[windex][tindex]; 
		++n;
	      } // end loop over j
	    } // end loop over i
	  } // end loop over time bins
	}// end loop over wires

	//calculate the cornerness of each pixel while making sure not to fall off the hit map.
	for(unsigned int wire = 1; wire < numberwires-1; ++wire){
	  
	  for(int timebin = 1; timebin < fTimeBins-1; ++timebin){    
	    MatrixAAsum = 0.;
	    MatrixBBsum = 0.;
	    MatrixCCsum = 0.;
	    //Gaussian smoothing convolution
	    n = 0;
	    for(int i = -3; i <= 3; ++i){
	      windex = wire+i;
	      if(windex<0) windex = 0;
	      // this is ok, because the line before makes sure it's not negative
	      else if((unsigned int)windex >= numberwires) windex = numberwires-1; 
	      
	      for(int j = -3; j <= 3; ++j){
		tindex = timebin+j;
		if(tindex < 0) tindex = 0;
		else if(tindex >= fTimeBins) tindex = fTimeBins-1;
	
                // MatrixAAsum is Ix(x,y)^2
                // MatrixBBsum is Iy(x,y)^2
                // MatrixCCsum is Ix(x,y)*Iy(x,y)
		MatrixAAsum += w[n]*pow(MatrixAsum[windex][tindex],2);  
		MatrixBBsum += w[n]*pow(MatrixBsum[windex][tindex],2);                   
		MatrixCCsum += w[n]*MatrixAsum[windex][tindex]*MatrixBsum[windex][tindex]; 
		++n;
	      }// end loop over j
	    }// end loop over i
	   
            // Cornerness is R_N(x,y)
	    if((MatrixAAsum + MatrixBBsum) > 0)		
	      Cornerness[wire][timebin] = (MatrixAAsum*MatrixBBsum-pow(MatrixCCsum,2))/(MatrixAAsum+MatrixBBsum);
	    else
	      Cornerness[wire][timebin] = 0;
	    
	    if(Cornerness[wire][timebin] > 0){	  
	      for(unsigned int i = 0;i < hits.size(); ++i){
		channel = hits[i]->Wire()->RawDigit()->Channel();
		geom->ChannelToWire(channel, cstat, tpc, plane, wire2);	 
		//make sure the end point candidate coincides with an actual hit.
		if(wire == wire2 
		   && hits[i]->StartTime() < timebin*(numbertimesamples/fTimeBins) 
		   && hits[i]->EndTime() > timebin*(numbertimesamples/fTimeBins)){ 	       
		  //this index keeps track of the hit number
		  hit_loc[wire][timebin] = i;
		  Cornerness2.push_back(Cornerness[wire][timebin]);
                  //mf::LogInfo("EndPointClusAlg") << "Found corner!" << std::endl;
                  //mf::LogInfo("EndPointClusAlg") << "Wire: " << wire << " Time bin: " << hits[hit_loc[wire][timebin]]->StartTime() << std::endl; 
                  //mf::LogInfo("EndPointClusAlg") << " R_N: " << Cornerness[wire][timebin] << std::endl; 
		  break;
		} 	        
	      }// end loop over hits	     
	    }// end if cornerness > 0	    
	  }// end loop over time bins     
	}// end wire loop 

	std::sort(Cornerness2.rbegin(), Cornerness2.rend());

        // This is just a sanity check
	//for(int vertexnum = 0; vertexnum < fMaxCorners; ++vertexnum){
	  //for(unsigned int wire = 0; wire < numberwires && flag == 0; ++wire){
	    //for(int timebin = 0; timebin < fTimeBins && flag == 0; ++timebin){    
	      //if(Cornerness2.size() > (unsigned int)vertexnum)
		//if(Cornerness[wire][timebin] == Cornerness2[vertexnum] ){
                  //mf::LogInfo("EndPointClusAlg") << "vertexnum: " <<  vertexnum << std::endl;
                  //mf::LogInfo("EndPointClusAlg") << "wire number: " <<  wire << std::endl;
                  //mf::LogInfo("EndPointClusAlg") << "timebin: " <<  timebin << std::endl;
                  //mf::LogInfo("EndPointClusAlg") << hit_loc[wire][timebin] << std::endl;
                  //mf::LogInfo("EndPointClusAlg") << "Wire: " << hits[hit_loc[wire][timebin]]->Wire()->RawDigit()->Channel() << " Time bin: " << hits[hit_loc[wire][timebin]]->StartTime() << std::endl; 
                  //mf::LogInfo("EndPointClusAlg") << "R_N: " << Cornerness[wire][timebin] << std::endl << std::endl; 
		  //if(Cornerness2.size())
		    //if(Cornerness[wire][timebin] < (fThreshold*Cornerness2[0]))
		      //vertexnum = fMaxCorners;
		  //double drifttick  = larp->DriftVelocity(larp->Efield(),larp->Temperature());
		  //drifttick *= detp->SamplingRate()*1.e-3;
		  //double wirepitch  = geom->WirePitch(0,1,0);
		  //double corrfactor = drifttick/wirepitch;
		  
		  //for(int wireout=(int)wire-(int)((fWindow*(numbertimesamples/fTimeBins)*corrfactor)+.5);
		      //wireout <= (int)wire+(int)((fWindow*(numbertimesamples/fTimeBins)*corrfactor)+.5); ++wireout){
		    //for(int timebinout=timebin-fWindow;timebinout <= timebin+fWindow; timebinout++){
		      //if(sqrt(pow(wire-wireout,2)+pow(timebin-timebinout,2))<fWindow)//circular window 
			//Cornerness[wireout][timebinout]=0;	  
		    //}
		  //}
                //}
            //}
          //}
        //}




        //if(Cornerness2.size()) mf::LogInfo("EndPointClusAlg") << "Cornerness2[0]: " << Cornerness2[0] << std::endl;

	for(int vertexnum = 0; vertexnum < fMaxCorners; ++vertexnum){
	  flag = 0;
	  for(unsigned int wire = 0; wire < numberwires && flag == 0; ++wire){
	    for(int timebin = 0; timebin < fTimeBins && flag == 0; ++timebin){    
	      if(Cornerness2.size() > (unsigned int)vertexnum)
		if(Cornerness[wire][timebin] == Cornerness2[vertexnum] 
		   && Cornerness[wire][timebin] > 0. 
                   && hit_loc[wire][timebin] > -1){
                   ++flag;
		 
                  mf::LogVerbatim("EndPointClusAlg") << "Found corner!" << std::endl;
                  mf::LogVerbatim("EndPointClusAlg") << "vertexnum: " <<  vertexnum << std::endl;
                  //mf::LogVerbatim("EndPointClusAlg") << "wire number: " <<  wire << std::endl;
                  //mf::LogVerbatim("EndPointClusAlg") << "timebin: " <<  timebin << std::endl;
                  mf::LogVerbatim("EndPointClusAlg") << hit_loc[wire][timebin] << std::endl;
                  //mf::LogVerbatim("EndPointClusAlg") << "Wire: " << hits[hit_loc[wire][timebin]]->Wire()->RawDigit()->Channel() << " Time bin: " << hits[hit_loc[wire][timebin]]->StartTime() << std::endl; 
                  mf::LogVerbatim("EndPointClusAlg") << "Wire: " << wire << " Time bin: " << hits[hit_loc[wire][timebin]]->StartTime() << std::endl; 
                  mf::LogVerbatim("EndPointClusAlg") << "R_N: " << Cornerness[wire][timebin] << std::endl << std::endl; 

                  corners->push_back(hit_loc[wire][timebin]);

		  //thresholding
		  if(Cornerness2.size())
		    if(Cornerness[wire][timebin] < (fThreshold*Cornerness2[0]))
		      vertexnum = fMaxCorners;
		  vHits.push_back(hits[hit_loc[wire][timebin]]);


		  // get the total charge from the associated hits
		  double totalQ = 0.;
		  for(size_t vh = 0; vh < vHits.size(); ++vh) totalQ += vHits[vh]->Charge();

		  //recob::EndPoint2D endpoint(hit[hit_loc[wire][timebin]]->PeakTime(),
					     //wire,
					     //Cornerness[wire][timebin],
					     //vtxcol.size(),
					     //view,
					     //totalQ,
					     //vHits);
		  //vtxcol.push_back(endpoint);
		  //vtxHitsOut.push_back(vHits);
		  vHits.clear();
		  
		  // non-maximal suppression on a square window. The wire coordinate units are 
		  // converted to time ticks so that the window is truly square. 
		  // Note that there are 1/0.0743=13.46 time samples per 4.0 mm (wire pitch in ArgoNeuT), 
		  // assuming a 1.5 mm/us drift velocity for a 500 V/cm E-field 
		  
                  double drifttick  = larp->DriftVelocity(larp->Efield(),larp->Temperature());
                  drifttick *= detp->SamplingRate()*1.e-3;
                  double wirepitch  = geom->WirePitch(0,1,0);
                  double corrfactor = drifttick/wirepitch;
                  
                  for(int wireout=(int)wire-(int)((fWindow*(numbertimesamples/fTimeBins)*corrfactor)+.5);
                      wireout <= (int)wire+(int)((fWindow*(numbertimesamples/fTimeBins)*corrfactor)+.5); ++wireout){
                    for(int timebinout=timebin-fWindow;timebinout <= timebin+fWindow; timebinout++){
                      if(sqrt(pow(wire-wireout,2)+pow(timebin-timebinout,2))<fWindow){//circular window 

                        //if(hit_loc[wireout][timebinout] ){
                          //mf::LogInfo("EndPointClusAlg") << "Wire out: " << wireout << " Time bin out: " << timebinout << std::endl; 
                          //mf::LogInfo("EndPointClusAlg") << "R_N out: " << Cornerness[wireout][timebinout] << std::endl << std::endl; 
                        //}

                        Cornerness[wireout][timebinout]=0;	  

                      }
                    }
                  }
		}
	    }     
	  }
	}
	Cornerness2.clear();
	//hit.clear();
	//if(clusterIter != clusIn.end()) clusterIter++;







	//if((int)p == fSaveVertexMap){ 
	  //unsigned char *outPix = new unsigned char [fTimeBins*numberwires];
	  ////finds the maximum cell in the map for image scaling
	  //int cell    = 0;
	  //int pix     = 0;
	  //int maxCell = 0;
	  ////int xmaxx, ymaxx;
	  //for (int y = 0; y < fTimeBins; ++y){
	    //for (unsigned int x = 0; x < numberwires; ++x){
	      //cell = (int)(hit_map[x][y]*1000);
	      //if (cell > maxCell){
		//maxCell = cell;
		////  xmaxx=x;
		////  ymaxx=y;
	      //}
	    //}
	  //}
	  //for (int y = 0; y < fTimeBins; ++y){
	    //for (unsigned int x = 0; x<numberwires; ++x){ 
	      ////scales the pixel weights based on the maximum cell value     
	      //if(maxCell>0)
		//pix = (int)((1500000*hit_map[x][y])/maxCell);
	      //outPix[y*numberwires + x] = pix;
	    //}
	  //}
	  
	  // add 3x3 pixel squares to with the harris vertex finders to the .bmp file
	  
	  //for(unsigned int ii = 0; ii < vtxcol.size(); ++ii){
	    //if(vtxcol[ii].View() == (unsigned int)view){
	      //pix = (int)(255);
	      //outPix[(int)(vtxcol[ii].DriftTime()*(fTimeBins/numbertimesamples))*numberwires + vtxcol[ii].WireNum()] = pix;
	      //outPix[(int)(vtxcol[ii].DriftTime()*(fTimeBins/numbertimesamples))*numberwires + vtxcol[ii].WireNum()-1] = pix;
	      //outPix[(int)(vtxcol[ii].DriftTime()*(fTimeBins/numbertimesamples))*numberwires + vtxcol[ii].WireNum()+1] = pix;
	      //outPix[(int)((vtxcol[ii].DriftTime()*(fTimeBins/numbertimesamples))-1)*numberwires + vtxcol[ii].WireNum()] = pix;
	      //outPix[(int)((vtxcol[ii].DriftTime()*(fTimeBins/numbertimesamples))+1)*numberwires + vtxcol[ii].WireNum()] = pix;
	      //outPix[(int)((vtxcol[ii].DriftTime()*(fTimeBins/numbertimesamples))-1)*numberwires + vtxcol[ii].WireNum()-1] = pix;
	      //outPix[(int)((vtxcol[ii].DriftTime()*(fTimeBins/numbertimesamples))-1)*numberwires + vtxcol[ii].WireNum()-2] = pix;
	      //outPix[(int)((vtxcol[ii].DriftTime()*(fTimeBins/numbertimesamples))+1)*numberwires + vtxcol[ii].WireNum()+1] = pix;
	      //outPix[(int)((vtxcol[ii].DriftTime()*(fTimeBins/numbertimesamples))+1)*numberwires + vtxcol[ii].WireNum()+2] = pix;
	    //}
	  //}
	  
	  //VSSaveBMPFile(Form("harrisvertexmap_%d_%d.bmp",(*clusIn.begin())->ID(),p), outPix, numberwires, fTimeBins);
	  //delete [] outPix;
	//}   
  
  //return vtxcol.size();   
  return 0;   
}























//......................................................
size_t cluster::EndPointClusAlg::EndPoint(art::PtrVector<recob::Cluster>                 & clusIn, 
				      std::vector<recob::EndPoint2D>		     & vtxcol,
				      std::vector< art::PtrVector<recob::Hit> >      & vtxHitsOut,
				      art::Event                                const& evt,
				      std::string                               const& label)
{

  art::ServiceHandle<geo::Geometry> geom;
  art::ServiceHandle<util::LArProperties> larp;
  art::ServiceHandle<util::DetectorProperties> detp;
   
  // function below does not link, so commenting
  //extern void SaveBMPFile(const char *f, unsigned char *pix, int dxx, int dyy);
  
  //Point to a collection of vertices to output.
  std::vector< art::Ptr<recob::Hit> > hit;

  art::FindManyP<recob::Hit> fmh(clusIn, evt, label);
   
  int flag   = 0;
  int windex = 0;//the wire index to make sure the end point finder does not fall off the edge of the hit map
  int tindex = 0;//the time index to make sure the end point finder does not fall off the edge of the hit map
  int n      = 0; //index of window cell. There are 49 cells in the 7X7 Gaussian and Gaussian derivative windows
  unsigned int numberwires = 0;
  double numbertimesamples = 0.;
  double MatrixAAsum = 0.;
  double MatrixBBsum = 0.;
  double MatrixCCsum = 0.;
  std::vector<double> Cornerness2;
  //gaussian window definitions. The cell weights are calculated here to help the algorithm's speed
  double  w[49] = {0.};
  double wx[49] = {0.};
  double wy[49] = {0.};
  int ctr = 0;
  for(int i = -3; i < 4; ++i){
    for(int j = 3; j > -4; --j){
      w[ctr] = Gaussian(i, j, fGsigma);
      wx[ctr] = GaussianDerivativeX(i,j);
      wy[ctr] = GaussianDerivativeY(i,j);
      ++ctr;
    }
  }
  
  unsigned int channel = 0;
  unsigned int plane   = 0;
  unsigned int wire    = 0;
  unsigned int wire2   = 0;
  unsigned int tpc     = 0;
  unsigned int cstat   = 0;
  for(unsigned int c = 0; c < geom->Ncryostats(); ++c){
    for(unsigned int t = 0; t < geom->Cryostat(c).NTPC(); ++t){
      for(unsigned int p = 0; p < geom->Cryostat(c).TPC(t).Nplanes(); p++) {
	art::PtrVector<recob::Hit> vHits;
	art::PtrVector<recob::Cluster>::const_iterator clusterIter = clusIn.begin();
	geo::View_t view = geom->Plane(p,t,c).View();
	hit.clear();
	size_t cinctr = 0;
	while(clusterIter!= clusIn.end() ) {
	  if((*clusterIter)->View() == view){
	    hit = fmh.at(cinctr);
	  }//end if cluster is in the correct view
	  clusterIter++;  
	  ++cinctr;
	} 
	
	if(hit.size() == 0) continue;
	
	numberwires = geom->Cryostat(c).TPC(t).Plane(p).Nwires();
	numbertimesamples = hit[0]->Wire()->NSignal();
	mf::LogInfo("EndPointClusAlg") << " --- endpoints check " 
				       << numberwires  
				       << numbertimesamples  
				       << fTimeBins;
      
	std::vector < std::vector < double > > MatrixAsum(numberwires);
	std::vector < std::vector < double > > MatrixBsum(numberwires);
	std::vector < std::vector < double > > hit_map(numberwires); //the map of hits 
	
	//the index of the hit that corresponds to the potential corner
	std::vector < std::vector < int > > hit_loc(numberwires);
	
	std::vector < std::vector < double > >  Cornerness(numberwires); //the "weight" of a corner
	
	for(unsigned int wi = 0; wi < numberwires; ++wi){  
	  hit_map[wi].resize(fTimeBins,0);
	  hit_loc[wi].resize(fTimeBins,-1);
	  Cornerness[wi].resize(fTimeBins,0);
	  MatrixAsum[wi].resize(fTimeBins,0);
	  MatrixBsum[wi].resize(fTimeBins,0);
	}      
	for(unsigned int i = 0; i < hit.size(); ++i){
	  channel=hit[i]->Wire()->RawDigit()->Channel();
	  geom->ChannelToWire(channel, cstat, tpc, plane, wire);
	  //pixelization using a Gaussian
	  for(int j = 0; j <= (int)(hit[i]->EndTime()-hit[i]->StartTime()+.5); ++j)    
	    hit_map[wire][(int)((hit[i]->StartTime()+j)*(fTimeBins/numbertimesamples)+.5)] += Gaussian((int)(j-((hit[i]->EndTime()-hit[i]->StartTime())/2.)+.5),0,hit[i]->EndTime()-hit[i]->StartTime());      
	}
	
	// Gaussian derivative convolution  
	for(unsigned int wire = 1; wire < numberwires-1; ++wire){
	  
	  for(int timebin = 1; timebin < fTimeBins-1; ++timebin){
	    MatrixAsum[wire][timebin] = 0.;
	    MatrixBsum[wire][timebin] = 0.;
	    n = 0;
	    for(int i = -3; i <= 3; ++i) {
	      windex = wire+i;
	      if(windex < 0 ) windex = 0;
	      // this is ok, because the line before makes sure it's not negative
	      else if ((unsigned int)windex >= numberwires) windex = numberwires-1; 
	      
	      for(int j = -3; j <= 3; ++j){
		tindex = timebin+j;
		if(tindex < 0) tindex=0;
		else if(tindex >= fTimeBins) tindex = fTimeBins-1;
		
		MatrixAsum[wire][timebin] += wx[n]*hit_map[windex][tindex];  
		MatrixBsum[wire][timebin] += wy[n]*hit_map[windex][tindex]; 
		++n;
	      } // end loop over j
	    } // end loop over i
	  } // end loop over time bins
	}// end loop over wires

	//calculate the cornerness of each pixel while making sure not to fall off the hit map.
	for(unsigned int wire = 1; wire < numberwires-1; ++wire){
	  
	  for(int timebin = 1; timebin < fTimeBins-1; ++timebin){    
	    MatrixAAsum = 0.;
	    MatrixBBsum = 0.;
	    MatrixCCsum = 0.;
	    //Gaussian smoothing convolution
	    n = 0;
	    for(int i = -3; i <= 3; ++i){
	      windex = wire+i;
	      if(windex<0) windex = 0;
	      // this is ok, because the line before makes sure it's not negative
	      else if((unsigned int)windex >= numberwires) windex = numberwires-1; 
	      
	      for(int j = -3; j <= 3; ++j){
		tindex = timebin+j;
		if(tindex < 0) tindex = 0;
		else if(tindex >= fTimeBins) tindex = fTimeBins-1;
	
                // MatrixAAsum is Ix(x,y)^2
                // MatrixBBsum is Iy(x,y)^2
                // MatrixCCsum is Ix(x,y)*Iy(x,y)
		MatrixAAsum += w[n]*pow(MatrixAsum[windex][tindex],2);  
		MatrixBBsum += w[n]*pow(MatrixBsum[windex][tindex],2);                   
		MatrixCCsum += w[n]*MatrixAsum[windex][tindex]*MatrixBsum[windex][tindex]; 
		++n;
	      }// end loop over j
	    }// end loop over i
	   
            // Cornerness is R_N(x,y)
	    if((MatrixAAsum + MatrixBBsum) > 0)		
	      Cornerness[wire][timebin] = (MatrixAAsum*MatrixBBsum-pow(MatrixCCsum,2))/(MatrixAAsum+MatrixBBsum);
	    else
	      Cornerness[wire][timebin] = 0;
	    
	    if(Cornerness[wire][timebin] > 0){	  
	      for(unsigned int i = 0;i < hit.size(); ++i){
		channel = hit[i]->Wire()->RawDigit()->Channel();
		geom->ChannelToWire(channel, cstat, tpc, plane, wire2);	 
		//make sure the end point candidate coincides with an actual hit.
		if(wire == wire2 
		   && hit[i]->StartTime() < timebin*(numbertimesamples/fTimeBins) 
		   && hit[i]->EndTime() > timebin*(numbertimesamples/fTimeBins)){ 	       
		  //this index keeps track of the hit number
		  hit_loc[wire][timebin] = i;
		  Cornerness2.push_back(Cornerness[wire][timebin]);
		  break;
		} 	        
	      }// end loop over hits	     
	    }// end if cornerness > 0	    
	  } // end loop over time bins     
	}  // end wire loop 

	std::sort(Cornerness2.rbegin(), Cornerness2.rend());

	for(int vertexnum = 0; vertexnum < fMaxCorners; ++vertexnum){
	  flag = 0;
	  for(unsigned int wire = 0; wire < numberwires && flag == 0; ++wire){
	    for(int timebin = 0; timebin < fTimeBins && flag == 0; ++timebin){    
	      if(Cornerness2.size() > (unsigned int)vertexnum)
		if(Cornerness[wire][timebin] == Cornerness2[vertexnum] 
		   && Cornerness[wire][timebin] > 0. 
		   && hit_loc[wire][timebin] > -1){
		  ++flag;
		  
		  //thresholding
		  if(Cornerness2.size())
		    if(Cornerness[wire][timebin] < (fThreshold*Cornerness2[0]))
		      vertexnum = fMaxCorners;
		  vHits.push_back(hit[hit_loc[wire][timebin]]);
		  
		  // get the total charge from the associated hits
		  double totalQ = 0.;
		  for(size_t vh = 0; vh < vHits.size(); ++vh) totalQ += vHits[vh]->Charge();

		  recob::EndPoint2D endpoint(hit[hit_loc[wire][timebin]]->PeakTime(),
					     wire,
					     Cornerness[wire][timebin],
					     vtxcol.size(),
					     view,
					     totalQ);
		  vtxcol.push_back(endpoint);
		  vtxHitsOut.push_back(vHits);
		  vHits.clear();
		  
		  // non-maximal suppression on a square window. The wire coordinate units are 
		  // converted to time ticks so that the window is truly square. 
		  // Note that there are 1/0.0743=13.46 time samples per 4.0 mm (wire pitch in ArgoNeuT), 
		  // assuming a 1.5 mm/us drift velocity for a 500 V/cm E-field 
		  
		  double drifttick  = larp->DriftVelocity(larp->Efield(),larp->Temperature());
		  drifttick *= detp->SamplingRate()*1.e-3;
		  double wirepitch  = geom->WirePitch(0,1,0);
		  double corrfactor = drifttick/wirepitch;
		  
		  for(int wireout=(int)wire-(int)((fWindow*(numbertimesamples/fTimeBins)*corrfactor)+.5);
		      wireout <= (int)wire+(int)((fWindow*(numbertimesamples/fTimeBins)*corrfactor)+.5); ++wireout){
		    for(int timebinout=timebin-fWindow;timebinout <= timebin+fWindow; timebinout++){
		      if(sqrt(pow(wire-wireout,2)+pow(timebin-timebinout,2))<fWindow)//circular window 
			Cornerness[wireout][timebinout]=0;	  
		    }
		  }
		}
	    }     
	  }
	}
	Cornerness2.clear();
	hit.clear();
	if(clusterIter != clusIn.end()) clusterIter++;








	if((int)p == fSaveVertexMap){ 
	  unsigned char *outPix = new unsigned char [fTimeBins*numberwires];
	  //finds the maximum cell in the map for image scaling
	  int cell    = 0;
	  int pix     = 0;
	  int maxCell = 0;
	  //int xmaxx, ymaxx;
	  for (int y = 0; y < fTimeBins; ++y){
	    for (unsigned int x = 0; x < numberwires; ++x){
	      cell = (int)(hit_map[x][y]*1000);
	      if (cell > maxCell){
		maxCell = cell;
		//  xmaxx=x;
		//  ymaxx=y;
	      }
	    }
	  }
	  for (int y = 0; y < fTimeBins; ++y){
	    for (unsigned int x = 0; x<numberwires; ++x){ 
	      //scales the pixel weights based on the maximum cell value     
	      if(maxCell>0)
		pix = (int)((1500000*hit_map[x][y])/maxCell);
	      outPix[y*numberwires + x] = pix;
	    }
	  }
	  
	  // add 3x3 pixel squares to with the harris vertex finders to the .bmp file
	  
	  for(unsigned int ii = 0; ii < vtxcol.size(); ++ii){
	    if(vtxcol[ii].View() == (unsigned int)view){
	      pix = (int)(255);
	      outPix[(int)(vtxcol[ii].DriftTime()*(fTimeBins/numbertimesamples))*numberwires + vtxcol[ii].WireNum()] = pix;
	      outPix[(int)(vtxcol[ii].DriftTime()*(fTimeBins/numbertimesamples))*numberwires + vtxcol[ii].WireNum()-1] = pix;
	      outPix[(int)(vtxcol[ii].DriftTime()*(fTimeBins/numbertimesamples))*numberwires + vtxcol[ii].WireNum()+1] = pix;
	      outPix[(int)((vtxcol[ii].DriftTime()*(fTimeBins/numbertimesamples))-1)*numberwires + vtxcol[ii].WireNum()] = pix;
	      outPix[(int)((vtxcol[ii].DriftTime()*(fTimeBins/numbertimesamples))+1)*numberwires + vtxcol[ii].WireNum()] = pix;
	      outPix[(int)((vtxcol[ii].DriftTime()*(fTimeBins/numbertimesamples))-1)*numberwires + vtxcol[ii].WireNum()-1] = pix;
	      outPix[(int)((vtxcol[ii].DriftTime()*(fTimeBins/numbertimesamples))-1)*numberwires + vtxcol[ii].WireNum()-2] = pix;
	      outPix[(int)((vtxcol[ii].DriftTime()*(fTimeBins/numbertimesamples))+1)*numberwires + vtxcol[ii].WireNum()+1] = pix;
	      outPix[(int)((vtxcol[ii].DriftTime()*(fTimeBins/numbertimesamples))+1)*numberwires + vtxcol[ii].WireNum()+2] = pix;
	    }
	  }
	  
	  VSSaveBMPFile(Form("harrisvertexmap_%d_%d.bmp",(*clusIn.begin())->ID(),p), outPix, numberwires, fTimeBins);
	  delete [] outPix;
	}   
      } // end loop over planes
    } // end loop over tpcs
  } // end loop over cryostats
  
  return vtxcol.size();   
}

