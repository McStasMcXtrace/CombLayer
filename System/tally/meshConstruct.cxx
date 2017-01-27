/********************************************************************* 
  CombLayer : MCNP(X) Input builder
 
 * File:   tally/meshConstruct.cxx
 *
 * Copyright (c) 2004-2016 by Stuart Ansell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 *
 ****************************************************************************/
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cmath>
#include <complex>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <algorithm>
#include <iterator>
#include <memory>

#include "Exception.h"
#include "FileReport.h"
#include "NameStack.h"
#include "RegMethod.h"
#include "GTKreport.h"
#include "OutputLog.h"
#include "BaseVisit.h"
#include "BaseModVisit.h"
#include "MatrixBase.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "Triple.h"
#include "support.h"
#include "stringCombine.h"
#include "surfRegister.h"
#include "objectRegister.h"
#include "Rules.h"
#include "HeadRule.h"
#include "Code.h"
#include "varList.h"
#include "FuncDataBase.h"
#include "Simulation.h"
#include "LinkUnit.h"
#include "FixedComp.h"
#include "LinkSupport.h"
#include "inputParam.h"
#include "NList.h"
#include "NRange.h"
#include "pairRange.h"
#include "Tally.h"
#include "TallyCreate.h"
#include "localRotate.h"
#include "masterRotate.h"
#include "meshTally.h"
#include "fmeshTally.h"

#include "TallySelector.h" 
#include "basicConstruct.h" 
#include "meshConstruct.h" 



namespace tallySystem
{

meshConstruct::meshConstruct() :fmeshFlag(0)
  /// Constructor
{}

meshConstruct::meshConstruct(const meshConstruct& A) :
  fmeshFlag(A.fmeshFlag)
  /// Copy Constructor
{}

meshConstruct&
meshConstruct::operator=(const meshConstruct& A) 
  /// Assignment operator
{
  if (this!=&A)
    {
      fmeshFlag=A.fmeshFlag;
    }
    
  return *this;
}

void
meshConstruct::calcXYZ(const std::string& object,const std::string& linkPos,
                            Geometry::Vec3D& APos,Geometry::Vec3D& BPos) 
  /*!
    Calculate the grid positions relative to an object
    Note that for the mesh it must align on 
    \param object :: object name
    \param linkPos :: link position
    \param APos :: Lower corner
    \param BPos :: Upper corner
   */
{
  ELog::RegMethod RegA("meshConstruct","calcXYZ");

  ModelSupport::objectRegister& OR=
    ModelSupport::objectRegister::Instance();

  const attachSystem::FixedComp* FC=
    OR.getObjectThrow<attachSystem::FixedComp>(object,"FixedComp");
  const long int sideIndex=attachSystem::getLinkIndex(linkPos);

  attachSystem::FixedComp A("tmpComp",0);
  A.createUnitVector(*FC,sideIndex);

  //
  // Construct the 8 corners of the cube:
  // then calculate the maximum in all directions
  //
  std::vector<Geometry::Vec3D> Cube(8);
  Cube[0]=APos;
  Cube[7]=BPos;
  for(size_t i=0;i<3;i++)
    {
      Cube[i+1]=APos;
      Cube[i+1][i]=BPos[i];
    }
  for(size_t i=0;i<3;i++)
    {
      Cube[i+5]=BPos;
      Cube[i+5][i]=APos[i];
    }
  
  Geometry::Vec3D Pt=A.getX()*Cube[0][0]+
    A.getY()*Cube[0][1]+A.getZ()*Cube[0][2];
  Geometry::Vec3D PtMax(Pt);
  Geometry::Vec3D PtMin(Pt);
  for(size_t i=1;i<8;i++)
    {
      Pt=A.getX()*Cube[i][0]+A.getY()*Cube[i][1]+A.getZ()*Cube[i][2];
      for(size_t j=0;j<3;j++)
	{
	  if (Pt[j]>PtMax[j]) PtMax[j]=Pt[j];
	  if (Pt[j]<PtMin[j]) PtMin[j]=Pt[j];
	}
    }
  
  APos=PtMin+A.getCentre();
  BPos=PtMax+A.getCentre();

  return;
}

  
void
meshConstruct::processMesh(Simulation& System,
			   const mainSystem::inputParam& IParam,
			   const size_t Index) const
  /*!
    Add mesh tally (s) as needed
    \param System :: Simulation to add tallies
    \param IParam :: Main input parameters
    \param Index :: index of the -T card
   */
{
  ELog::RegMethod RegA("meshConstruct","processMesh");

  const size_t NItems=IParam.itemCnt("tally",Index);
  if (NItems<4)
    throw ColErr::IndexError<size_t>(NItems,4,
				     "Insufficient items for tally");

  const std::string PType(IParam.getValue<std::string>("tally",Index,1)); 

  const masterRotate& MR=masterRotate::Instance();
  
  if (PType=="object" || PType=="heatObject")
    {
      size_t itemIndex(5);
      const std::string place=
	IParam.outputItem<std::string>("tally",Index,2,"position not given");
      const std::string linkName=
	IParam.outputItem<std::string>("tally",Index,3,"front/back/side not given");      
      const std::string doseType=
	inputItem<std::string>(IParam,Index,4,"Dose type");
      Geometry::Vec3D APt=
	IParam.getCntVec3D("tally",Index,itemIndex,"Low Corner");
      Geometry::Vec3D BPt=
	IParam.getCntVec3D("tally",Index,itemIndex,"High Corner");

      size_t Nxyz[3];
      Nxyz[0]=inputItem<size_t>(IParam,Index,itemIndex++,"NXpts");
      Nxyz[1]=inputItem<size_t>(IParam,Index,itemIndex++,"NYpts");
      Nxyz[2]=inputItem<size_t>(IParam,Index,itemIndex++,"NZpts");

      calcXYZ(place,linkName,APt,BPt);
      
      if (PType=="heatObject" || PType=="heatObjectRotated")
        rectangleMesh(System,3,"void",APt,BPt,Nxyz);
      else
	rectangleMesh(System,1,doseType,APt,BPt,Nxyz);
      return;      
    }      
  else if (PType=="free" || PType=="heat" ||
      PType=="freeRotated" || PType=="heatRotated")
    {
      size_t itemIndex(2);
      const std::string doseType=
	inputItem<std::string>(IParam,Index,itemIndex++,"Dose type");
      Geometry::Vec3D APt=
	IParam.getCntVec3D("tally",Index,itemIndex,"Low Corner");
      Geometry::Vec3D BPt=
	IParam.getCntVec3D("tally",Index,itemIndex,"High Corner");
      
      // Rotation:
      std::string revStr;
      const int flag=checkItem<std::string>(IParam,Index,itemIndex,revStr);
      if (!flag && revStr!="r" &&
	  PType!="freeRotated" && PType!="heatRotated")
	{
	  ELog::EM<<"Reverse rotating"<<ELog::endDiag;
	  APt=MR.reverseRotate(APt);
	  BPt=MR.reverseRotate(BPt);
	  //	  nxyzIndex+=(flag && revStr=="r") ? 1 : 0;
	}
      
      size_t Nxyz[3];
      Nxyz[0]=inputItem<size_t>(IParam,Index,itemIndex++,"NXpts");
      Nxyz[1]=inputItem<size_t>(IParam,Index,itemIndex++,"NYpts");
      Nxyz[2]=inputItem<size_t>(IParam,Index,itemIndex++,"NZpts");
      if (PType=="heat" || PType=="heatRotated")
	rectangleMesh(System,3,"void",APt,BPt,Nxyz);
      else
	rectangleMesh(System,1,doseType,APt,BPt,Nxyz);
      return;
    }
  
  ELog::EM<<"Unknown Mesh type :"<<PType<<ELog::endWarn;
  return;
}

void 
meshConstruct::rectangleMesh(Simulation& System,const int type,
			     const std::string& KeyWords,
			     const Geometry::Vec3D& APt,
			     const Geometry::Vec3D& BPt,
			     const size_t* MPts) const
  /*!
    An amalgamation of values to determine what sort of mesh to put
    in the system.
    \param System :: Simulation to add tallies
    \param type :: type of tally[1,2,3]
    \param KeyWords :: KeyWords to add to the tally
    \param APt :: Lower point 
    \param BPt :: Upper point 
    \param MPts :: Points ot use
  */
{
  ELog::RegMethod RegA("meshConstruct","rectangleMesh");

  // Find next available number
  int tallyN(type);
  while(System.getTally(tallyN))
    tallyN+=10;

  // Create tally:
  meshTally MT(tallyN);
  if (type==1)
    MT.setParticles("n");
  MT.setCoordinates(APt,BPt);
  MT.setIndex(MPts);
  MT.setActive(1);
  if (KeyWords=="DOSE")
    {
      MT.setKeyWords("DOSE 1");
      MT.setResponse(getDoseConversion());
    }
  else if (KeyWords=="DOSEPHOTON")
    {
      MT.setParticles("p");
      MT.setKeyWords("DOSE 1");
      MT.setResponse(getPhotonDoseConversion());
    }
  else if (KeyWords=="InternalDOSE")
    {
      MT.setKeyWords("DOSE");
      MT.setIndexLine("40 1 2 1e6");
    }
  else if (KeyWords=="void")
    {
      MT.setKeyWords("");
    }
  else 
    {
      ELog::EM<<"Mesh keyword options:\n"
	      <<"  DOSE :: ICRP-116 Flux to Dose conversion (uSv/hour per n/cm2/sec)\n"
	      <<"  DOSEPHOTON :: ICRP-116 Flux to Dose conversion (uSv/hour per n/cm2/sec)\n"
	      <<"  InternalDOSE :: MCNPX Flux to Dose conversion (mrem/hour)\n"
	      <<"  void ::  Flux \n"
	      <<ELog::endDiag;
      ELog::EM<<"Using unknown keyword :"<<KeyWords<<ELog::endErr;
    }

  ELog::EM<<"Adding tally "<<ELog::endTrace;
  ELog::EM<<"Coordinates  : "<<ELog::endTrace;
  MT.writeCoordinates(ELog::EM.Estream());
  ELog::EM<<ELog::endTrace;

  System.addTally(MT);

  return;
}

void 
meshConstruct::rectangleFMesh(Simulation& System,const int type,
                              const std::string& KeyWords,
                              const Geometry::Vec3D& APt,
                              const Geometry::Vec3D& BPt,
                              const size_t* MPts) const
  /*!
    An amalgamation of values to determine what sort of mesh to put
    in the system.
    \param System :: Simulation to add tallies
    \param type :: type of tally [1,2,3]
    \param KeyWords :: KeyWords to add to the tally
    \param APt :: Lower point 
    \param BPt :: Upper point 
    \param MPts :: Points ot use
  */
{
  ELog::RegMethod RegA("meshConstruct","rectangleMesh");

  // Find next available number
  int tallyN(type);
  while(System.getTally(tallyN))
    tallyN+=10;

  // Create tally:
  fmeshTally MT(tallyN);
  if (type==1)
    MT.setParticles("n");
  MT.setCoordinates(APt,BPt);
  MT.setIndex(MPts);
  MT.setActive(1);
  if (KeyWords=="DOSE")
    {
      MT.setKeyWords("DOSE 1");
      MT.setResponse(getDoseConversion());
    }
  else if (KeyWords=="DOSEPHOTON")
    {
      MT.setParticles("p");
      MT.setKeyWords("DOSE 1");
      MT.setResponse(getPhotonDoseConversion());
    }
  else if (KeyWords=="InternalDOSE")
    {
      MT.setKeyWords("DOSE");
      MT.setIndexLine("40 1 2 1e6");
    }
  else if (KeyWords=="void")
    {
      MT.setKeyWords("");
    }
  else 
    {
      ELog::EM<<"Mesh keyword options:\n"
	      <<"  DOSE :: ICRP-116 Flux to Dose conversion (uSv/hour)\n"
	      <<"  DOSEPHOTON :: ICRP-116 Flux to Dose conversion (uSv/hour)\n"
	      <<"  InternalDOSE :: MCNPX Flux to Dose conversion (mrem/hour)\n"
	      <<"  void ::  Flux \n"
	      <<ELog::endDiag;
      ELog::EM<<"Using unknown keyword :"<<KeyWords<<ELog::endErr;
    }

  ELog::EM<<"Adding tally "<<ELog::endTrace;
  ELog::EM<<"Coordinates  : "<<ELog::endTrace;
  MT.writeCoordinates(ELog::EM.Estream());
  ELog::EM<<ELog::endTrace;

  System.addTally(MT);

  return;
}

const std::string& 
meshConstruct::getDoseConversion()
  /*!
    Return the dose string  for a mshtr
    Uses the FTD files values [Flux to Dose conversion].
    - These values are ICRP-116 conversion factors for neutrons
    in uSv/hour from neutrons/cm2/sec
    \return FTD string
  */
{
  static std::string fcdString=
    "1.00E-09  1.11E-02 1.00E-08  1.28E-02 2.50E-08  1.44E-02 1.00E-07  1.87E-02 "
    "2.00E-07  2.11E-02 5.00E-07  2.37E-02 1.00E-06  2.53E-02 2.00E-06  2.66E-02 "
    "5.00E-06  2.78E-02 1.00E-05  2.82E-02 2.00E-05  2.82E-02 5.00E-05  2.82E-02 "
    "1.00E-04  2.80E-02 2.00E-04  2.78E-02 5.00E-04  2.71E-02 1.00E-03  2.71E-02 "
    "2.00E-03  2.74E-02 5.00E-03  2.87E-02 1.00E-02  3.28E-02 2.00E-02  4.39E-02 "
    "3.00E-02  5.65E-02 5.00E-02  8.28E-02 7.00E-02  1.10E-01 1.00E-01  1.51E-01 "
    "1.50E-01  2.18E-01 2.00E-01  2.84E-01 3.00E-01  4.10E-01 5.00E-01  6.37E-01 "
    "7.00E-01  8.35E-01 9.00E-01  1.00E+00 1.00E+00  1.08E+00 1.20E+00  1.19E+00 "
    "1.50E+00  1.31E+00 2.00E+00  1.47E+00 3.00E+00  1.65E+00 4.00E+00  1.74E+00 "
    "5.00E+00  1.78E+00 6.00E+00  1.79E+00 7.00E+00  1.80E+00 8.00E+00  1.80E+00 "
    "9.00E+00  1.80E+00 1.00E+01  1.80E+00 1.20E+01  1.80E+00 1.40E+01  1.78E+00 "
    "1.50E+01  1.77E+00 1.60E+01  1.76E+00 1.80E+01  1.74E+00 2.00E+01  1.72E+00 "
    "2.10E+01  1.71E+00 3.00E+01  1.63E+00 5.00E+01  1.56E+00 7.50E+01  1.58E+00 "
    "1.00E+02  1.60E+00 1.30E+02  1.61E+00 1.50E+02  1.61E+00 1.80E+02  1.61E+00 "
    "2.00E+02  1.61E+00 3.00E+02  1.67E+00 4.00E+02  1.79E+00 5.00E+02  1.92E+00 "
    "6.00E+02  2.05E+00 7.00E+02  2.16E+00 8.00E+02  2.24E+00 9.00E+02  2.30E+00 "
    "1.00E+03  2.35E+00 2.00E+03  2.76E+00 5.00E+03  3.64E+00 1.00E+04  4.75E+00 ";
  return fcdString;
}

const std::string& 
meshConstruct::getPhotonDoseConversion()
  /*!
    Return the dose string  for a mshtr
    Uses the FtD files values. This is the H*(10) values which are higher
    than the FtD values!
    - These values are ICRP-116 conversion factors for photons
    in uSv/hour from photon/cm2/sec
    \return FCD string
  */
{
  static std::string fcdString=
    "1.00E-02  2.47E-04 1.50E-02  5.62E-04 2.00E-02  8.10E-04 3.00E-02  1.13E-03 "
    "4.00E-02  1.26E-03 5.00E-02  1.33E-03 6.00E-02  1.40E-03 7.00E-02  1.49E-03 "
    "8.00E-02  1.60E-03 1.00E-01  1.87E-03 1.50E-01  2.69E-03 2.00E-01  3.60E-03 "
    "3.00E-01  5.44E-03 4.00E-01  7.20E-03 5.00E-01  8.89E-03 5.11E-01  9.07E-03 "
    "6.00E-01  1.05E-02 6.62E-01  1.14E-02 8.00E-01  1.34E-02 1.00E+00  1.62E-02 "
    "1.12E+00  1.76E-02 1.33E+00  2.01E-02 1.50E+00  2.20E-02 2.00E+00  2.69E-02 "
    "3.00E+00  3.51E-02 4.00E+00  4.21E-02 5.00E+00  4.82E-02 6.00E+00  5.40E-02 "
    "6.13E+00  5.47E-02 8.00E+00  6.70E-02 1.00E+01  7.92E-02 1.50E+01  1.09E-01 "
    "2.00E+01  1.38E-01 3.00E+01  1.85E-01 4.00E+01  2.23E-01 5.00E+01  2.60E-01 "
    "6.00E+01  2.95E-01 8.00E+01  3.52E-01 1.00E+02  3.96E-01 1.50E+02  4.68E-01 "
    "2.00E+02  5.15E-01 3.00E+02  5.80E-01 4.00E+02  6.19E-01 5.00E+02  6.48E-01 "
    "6.00E+02  6.70E-01 8.00E+02  7.02E-01 1.00E+03  7.24E-01 1.50E+03  7.63E-01 "
    "2.00E+03  7.92E-01 3.00E+03  8.35E-01 4.00E+03  8.75E-01 5.00E+03  9.04E-01 "
    "6.00E+03  9.29E-01 8.00E+03  9.65E-01 1.00E+04  9.94E-01 ";

  return fcdString;
}

void
meshConstruct::writeHelp(std::ostream& OX) const
  /*!
    Write out help
    \param Output stream
  */
{
  OX<<
    "free dosetype Vec3D Vec3D Nx Ny Nz \n"
    "object objectName LinkPt dosetype Vec3D Vec3D Nx Ny Nz \n"
    "  -- Object-link point is used to construct basis set \n"
    "     Then the Vec3D are used as the offset points \n"
    "heat Vec3D Vec3D Nx Ny Nz";
  return;
}

}  // NAMESPACE tallySystem
