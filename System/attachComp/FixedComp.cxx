/********************************************************************* 
  CombLayer : MCNP(X) Input builder
 
 * File:   attachComp/FixedComp.cxx
 *
 * Copyright (c) 2004-2015 by Stuart Ansell
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
#include <memory>

#include "Exception.h"
#include "FileReport.h"
#include "GTKreport.h"
#include "NameStack.h"
#include "RegMethod.h"
#include "OutputLog.h"
#include "BaseVisit.h"
#include "BaseModVisit.h"
#include "support.h"
#include "MatrixBase.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "Quaternion.h"
#include "Surface.h"
#include "surfIndex.h"
#include "surfRegister.h"
#include "surfEqual.h"
#include "Rules.h"
#include "HeadRule.h"
#include "LinkUnit.h"
#include "FixedComp.h"

namespace attachSystem
{

FixedComp::FixedComp(const std::string& KN,const size_t NL) :
  keyName(KN),X(Geometry::Vec3D(1,0,0)),Y(Geometry::Vec3D(0,1,0)),
  Z(Geometry::Vec3D(0,0,1)),LU(NL)
 /*!
    Constructor 
    \param KN :: KeyName
    \param NL :: Number of links
  */
{}

FixedComp::FixedComp(const std::string& KN,const size_t NL,
		     const Geometry::Vec3D& O) :
  keyName(KN),X(Geometry::Vec3D(1,0,0)),Y(Geometry::Vec3D(0,1,0)),
  Z(Geometry::Vec3D(0,0,1)),Origin(O),LU(NL)
  /*!
    Constructor 
    \param KN :: KeyName
    \param NL :: Number of links
    \param O :: Origin Point
  */
{}

FixedComp::FixedComp(const std::string& KN,const size_t NL,
		     const Geometry::Vec3D& O,
		     const Geometry::Vec3D& xV,
		     const Geometry::Vec3D& yV,
		     const Geometry::Vec3D& zV) :
  keyName(KN),X(xV.unit()),Y(yV.unit()),Z(zV.unit()),
  Origin(O),LU(NL)
  /*!
    Constructor 
    \param KN :: KeyName
    \param NL :: Number of links
    \param O :: Origin Point
  */
{}

FixedComp::FixedComp(const FixedComp& A) : 
  keyName(A.keyName),SMap(A.SMap),X(A.X),Y(A.Y),Z(A.Z),
  Origin(A.Origin),beamAxis(A.beamAxis),LU(A.LU)
  /*!
    Copy constructor
    \param A :: FixedComp to copy
  */
{}

FixedComp&
FixedComp::operator=(const FixedComp& A)
  /*!
    Assignment operator
    \param A :: FixedComp to copy
    \return *this
  */
{
  if (this!=&A)
    {
      SMap=A.SMap;
      X=A.X;
      Y=A.Y;
      Z=A.Z;
      Origin=A.Origin;
      beamAxis=A.beamAxis;
      LU=A.LU;
    }
  return *this;
}

void
FixedComp::createUnitVector()
  /*!
    Create the unit vectors
  */
{
  ELog::RegMethod RegA("FixedComp","createUnitVector");
  
  ELog::EM<<"Using TS2 axis origin system"<<ELog::endErr;

  Z=Geometry::Vec3D(-1,0,0);          // Gravity axis [up]
  Y=Geometry::Vec3D(0,0,-1);
  X=Y*Z;
  beamAxis=Y;
  return;
}

void
FixedComp::createUnitVector(const FixedComp& FC)
  /*!
    Create the unit vectors
    \param FC :: Relative to another FC
  */
{
  ELog::RegMethod RegA("FixedComp","createUnitVector(FixedComp)");

  Z=FC.Z;
  Y=FC.Y;
  X=FC.X;
  Origin=FC.Origin;
  beamOrigin=FC.beamOrigin;
  beamAxis=FC.beamAxis;

  return;
}

void
FixedComp::createUnitVector(const FixedComp& FC,
			    const Geometry::Vec3D& POrigin)
  /*!
    Create the unit vectors
    \param FC :: Relative to another FC
    \param POrigin :: New Origin
  */
{
  ELog::RegMethod RegA("FixedComp","createUnitVector(FixedComp,Vec3D)");

  Z=FC.Z;
  Y=FC.Y;
  X=FC.X;
  Origin=POrigin;
  beamOrigin=FC.beamOrigin;
  beamAxis=FC.beamAxis;

  return;
}

void
FixedComp::createUnitVector(const FixedComp& FC,
			    const long int sideIndex)
  /*!
    Create the unit vectors
    \param FC :: Fixed unit for link points
    \param sideIndex :: SIGNED +1 side index
  */
{
  ELog::RegMethod RegA("FixedComp","createUnitVector(FixedComp,side)");

  if (sideIndex==0)
    {
      createUnitVector(FC);
      return;
    }

  const size_t linkIndex=
    (sideIndex>0) ? static_cast<size_t>(sideIndex-1) :
    static_cast<size_t>(-sideIndex-1) ;
  if (linkIndex>=FC.LU.size())
    throw ColErr::IndexError<size_t>
      (linkIndex,FC.LU.size(),
       "LU.size()/linkIndex in object:"+FC.getKeyName()+" to object "+
       keyName);
     
  const LinkUnit& LU=FC.getLU(linkIndex);
  const double signV((sideIndex>0) ? 1.0 : -1.0);

  const Geometry::Vec3D yTest=LU.getAxis();
  Geometry::Vec3D zTest=FC.getZ();
  if (fabs(zTest.dotProd(yTest))>1.0-Geometry::zeroTol)
    zTest=FC.getX();

  createUnitVector(LU.getConnectPt(),
		   yTest*signV,zTest);
  return;
}
  
void
FixedComp::createUnitVector(const Geometry::Vec3D& OG,
			    const Geometry::Vec3D& BeamAxis,
			    const Geometry::Vec3D& ZAxis)
  /*!
    Create the unit vectors [using beam directions]
    \param OG :: Origin
    \param BeamAxis :: Beamline axis line
    \param ZAxis :: Direction for Z
  */
{
  ELog::RegMethod RegA("FixedComp","createUnitVector(Vec3D,Vec3D,Vec3D))");

  //Geometry::Vec3D(-1,0,0);          // Gravity axis [up]
  Z=ZAxis;
  Y=BeamAxis.unit();
  X=(Y*Z);                            // horrizontal axis [across]

  Origin=OG;
  beamOrigin=OG;
  beamAxis=Y;
  return;
}

void
FixedComp::applyShift(const double xStep,
		      const double yStep,
		      const double zStep)
  /*!
    Create the unit vectors
    \param xStep :: XStep [of X vector]
    \param yStep :: YStep [of Y vector]
    \param zStep :: ZStep [of Z vector]
  */
{
  Origin+=X*xStep+Y*yStep+Z*zStep;
  return;
}


void
FixedComp::applyAngleRotate(const double xAngle,
			    const double yAngle,
			    const double zAngle)
 /*!
    Create the unit vectors
    \param xyAngle :: XY Rotation [second]
    \param zAngle :: Z Rotation [first]
  */
{
  const Geometry::Quaternion Qz=
    Geometry::Quaternion::calcQRotDeg(zAngle,Z);
  const Geometry::Quaternion Qy=
    Geometry::Quaternion::calcQRotDeg(yAngle,Y);
  const Geometry::Quaternion Qx=
    Geometry::Quaternion::calcQRotDeg(xAngle,X);
  Qz.rotate(Y);
  Qz.rotate(X);
  Qy.rotate(Y);
  Qy.rotate(X);
  Qy.rotate(Z);
  
  Qx.rotate(Y);
  Qx.rotate(X);
  Qx.rotate(Z);
  return;
}
void
FixedComp::applyAngleRotate(const double xyAngle,
			    const double zAngle)
 /*!
    Create the unit vectors
    \param xyAngle :: XY Rotation [second]
    \param zAngle :: Z Rotation [first]
  */
{
  const Geometry::Quaternion Qz=
    Geometry::Quaternion::calcQRotDeg(zAngle,X);
  const Geometry::Quaternion Qxy=
    Geometry::Quaternion::calcQRotDeg(xyAngle,Z);
  Qz.rotate(Y);
  Qz.rotate(Z);
  Qxy.rotate(Y);
  Qxy.rotate(X);
  Qxy.rotate(Z);
  return;
}

void
FixedComp::linkAngleRotate(const long int sideIndex,
			   const double xyAngle,
			   const double zAngle)
 /*!
   Rotate a linke point axis [not connection point]
   \param sideIndex :: signed ink point [sign for direction]
   \param xyAngle :: XY Rotation [second]
   \param zAngle :: Z Rotation [first]
 */
{
  ELog::RegMethod RegA("FixedComp","linkAngleRotate");

  LinkUnit& LItem=getSignedLU(sideIndex);
  const double signV=(sideIndex>0) ? 1.0 : -1.0;
  const Geometry::Quaternion Qz=
    Geometry::Quaternion::calcQRotDeg(zAngle*signV,X);
  const Geometry::Quaternion Qxy=
    Geometry::Quaternion::calcQRotDeg(xyAngle*signV,Z);

  Geometry::Vec3D Axis=LItem.getAxis();
  Qz.rotate(Axis);
  Qxy.rotate(Axis);

  LItem.setAxis(Axis);
  return;
}

void
FixedComp::applyFullRotate(const double xyAngle,
			   const double zAngle,
			   const Geometry::Vec3D& RotCent)
  /*!
    Create the unit vectors
    \param xyAngle :: XY Rotation [second]
    \param zAngle :: Z Rotation [first]
    \param RotCentre :: Z Rotation [first]
  */
{
  const Geometry::Quaternion Qz=
    Geometry::Quaternion::calcQRotDeg(zAngle,X);
  const Geometry::Quaternion Qxy=
    Geometry::Quaternion::calcQRotDeg(xyAngle,Z);
  Qz.rotate(Y);
  Qz.rotate(Z);
  Qxy.rotate(Y);
  Qxy.rotate(X);
  Qxy.rotate(Z);

  Origin-=RotCent;
  Qz.rotate(Origin);
  Qxy.rotate(Origin);
  Origin+=RotCent;

  return;
}

void
FixedComp::reverseZ()
  /*!
    Flip the Z axis keeping Y Fixed
    (could generalize but ...)
  */
{
  Z*= -1.0;
  X*= -1.0;
  return;
}

void
FixedComp::setNConnect(const size_t N) 
  /*!
    Create/Remove new links point
    \param N :: New size of link points
  */
{
  LU.resize(N);
  return;
}

void
FixedComp::copyLinkObjects(const FixedComp& A)
  /*!
    Copy all the link object from A to this FixedComp.
    Overwrites the existing link units.
    Sizes must match or be smaller
    \param A :: FixedComp to copy
  */
{
  ELog::RegMethod RegA("FixedComp","copyLinkObjects");
  if (this!=&A)
    {
      if(LU.size()!=A.LU.size())
	ELog::EM<<"Changing link size from "<<LU.size()
		<<" to "<<A.LU.size()<<ELog::endCrit;
      LU=A.LU;
    }
  return;
}

void
FixedComp::addLinkSurf(const size_t Index,const int SN) 
  /*!
    Add a surface to output
    \param Index :: Link number
    \param SN :: Surface number [inward looking]
  */
{
  ELog::RegMethod RegA("FixedComp","addLinkSurf");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"LU size/Index");
  
  LU[Index].addLinkSurf(SN);
  return;
}


void
FixedComp::addLinkSurf(const size_t Index,
		       const std::string& SList) 
  /*!
    Add a surface to output
    \param Index :: Link number
    \param SList :: String to process
  */
{
  ELog::RegMethod RegA("FixedComp","addLinkSurf");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"LU size/Index");

  LU[Index].addLinkSurf(SList);
  return;
}

void
FixedComp::setLinkSurf(const size_t Index,
		       const std::string& SList) 
  /*!
    Set a surface to output
    \param Index :: Link number
    \param SList :: String to process
  */
{
  ELog::RegMethod RegA("FixedComp","setLinkSurf");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"LU size/Index");

  LU[Index].setLinkSurf(SList);
  return;
}

void
FixedComp::setLinkSurf(const size_t Index,
		       const HeadRule& HR) 
  /*!
    Set a surface to output
    \param Index :: Link number
    \param HR :: HeadRule to add
  */
{
  ELog::RegMethod RegA("FixedComp","setLinkSurf(HR)");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"LU size/Index");

  LU[Index].setLinkSurf(HR);
  return;
}

void
FixedComp::setLinkSurf(const size_t Index,const int SN) 
  /*!
    Set  a surface to output
    \param Index :: Link number
    \param SN :: Surface number [inward looking]
  */
{
  ELog::RegMethod RegA("FixedComp","setLinkSurf");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"LU size/index");

  LU[Index].setLinkSurf(SN);
  return;
}

void
FixedComp::setLinkSurf(const size_t Index,
		       const attachSystem::FixedComp& FC,
		       const size_t otherIndex) 
  /*!
    Set  a surface to output
    \param Index :: Link number
    \param FC :: Fixed component to use as connection
    \param otherIndex :: Connecting surface on the FC
  */
{
  ELog::RegMethod RegA("FixedComp","setLinkSurf<FC>");
  if (otherIndex>=FC.LU.size())
    throw ColErr::IndexError<size_t>(otherIndex,FC.LU.size(),
				  "otherIndex/LU.size");

  setLinkSurf(Index,-FC.getLinkSurf(otherIndex));
  return;
}

void
FixedComp::setBridgeSurf(const size_t Index,const int SN) 
  /*!
    Set a surface to bridge output
    \param Index :: Link number
    \param SN :: Surface number [inward looking]
  */
{
  ELog::RegMethod RegA("FixedComp","setBridgeSurf");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"LU size/index");

  LU[Index].setBridgeSurf(SN);
  return;
}

void
FixedComp::setBridgeSurf(const size_t Index,const HeadRule& HR) 
  /*!
    Set a surface to bridge output
    \param Index :: Link number
    \param HR :: HeadRule for bridge
  */
{
  ELog::RegMethod RegA("FixedComp","setBridgeSurf");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"LU size/index");

  LU[Index].setBridgeSurf(HR);
  return;
}

void
FixedComp::setBridgeSurf(const size_t Index,
		       const attachSystem::FixedComp& FC,
		       const size_t otherIndex) 
  /*!
    Set  a surface to bridge
    \param Index :: Link number
    \param FC :: Fixed component to use as connection
    \param otherIndex :: Connecting surface on the FC
  */
{
  ELog::RegMethod RegA("FixedComp","setBridgeSurf<FC>");
  if (otherIndex>=FC.LU.size())
    throw ColErr::IndexError<size_t>(otherIndex,FC.LU.size(),
				  "otherIndex/LU.size");

  setBridgeSurf(Index,-FC.getLinkSurf(otherIndex));
  return;
}
void
FixedComp::addBridgeSurf(const size_t Index,const int SN) 
  /*!
    Add a surface to output
    \param Index :: Link number
    \param SN :: Surface number [inward looking]
  */
{
  ELog::RegMethod RegA("FixedComp","addBridgeSurf");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"LU size/Index");
  
  LU[Index].addBridgeSurf(SN);
  return;
}



void
FixedComp::addBridgeSurf(const size_t Index,
			 const std::string& SList) 
  /*!
    Add a surface to output
    \param Index :: Link number
    \param SList :: String to process
  */
{
  ELog::RegMethod RegA("FixedComp","addBridgeSurf");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"LU size/Index");

  LU[Index].addBridgeSurf(SList);
  return;
}

void
FixedComp::setConnect(const size_t Index,const Geometry::Vec3D& C,
		      const Geometry::Vec3D& A)
 /*!
   Set the axis of the linked component
   \param Index :: Link number
   \param C :: Centre coordinate
   \param A :: Axis direciton
 */
{
  ELog::RegMethod RegA("FixedComp","setConnection");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"LU.size/index");

  LU[Index].setConnectPt(C);
  LU[Index].setAxis(A);
  return;
}

void
FixedComp::setLinkComponent(const size_t Index,
			    const FixedComp& FC,
			    const size_t sideIndex)
  /*!
    Copy the opposite (as if joined) link surface 
    Note that the surfaces are complemented
    \param Index :: Link number
    \param FC :: Other Fixed component to copy object from
    \param sideIndex :: link unit of other object
  */
{
  ELog::RegMethod RegA("FixedComp","setLinkComplement");

  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"LU size/index");
  if (sideIndex>=FC.LU.size())
    throw ColErr::IndexError<size_t>(sideIndex,FC.LU.size(),"FC/index");
  
  LU[Index]=FC.LU[sideIndex];
  LU[Index].complement();
  return;
}

void
FixedComp::setLinkCopy(const size_t Index,
		       const FixedComp& FC,
		       const size_t sideIndex)
  /*!
    Copy the opposite (as if joined) link surface 
    Note that the surfaces are complemented
    \param Index :: Link number
    \param FC :: Other Fixed component to copy object from
    \param sideIndex :: link unit of other object
  */
{
  ELog::RegMethod RegA("FixedComp","setLinkSurf");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"LU size/index");
  if (sideIndex>=FC.LU.size())
    throw ColErr::IndexError<size_t>(sideIndex,FC.LU.size(),"FC/index");
  
  LU[Index]=FC.LU[sideIndex];
  return;
}

void
FixedComp::setLinkSignedCopy(const size_t Index,
			     const FixedComp& FC,
			     const long int  sideIndex)
  /*!
    Copy the opposite (as if joined) link surface 
    Note that the surfaces are complemented
    \param Index :: Link number
    \param FC :: Other Fixed component to copy object from
    \param sideIndex :: signed link unit of other object
  */
{
  ELog::RegMethod RegA("FixedComp","setLinkSurf");
  if (sideIndex>0)
    setLinkCopy(Index,FC,static_cast<size_t>(sideIndex-1));
  else if (sideIndex<0)   // complement form
    setLinkComponent(Index,FC,static_cast<size_t>(-1-sideIndex));
  else
    throw ColErr::IndexError<long int>
      (sideIndex,static_cast<long int>(FC.LU.size()),"FC/index");

  return;
}

void
FixedComp::setBasicExtent(const double XWidth,const double YWidth,
			  const double ZWidth)
 /*!
   Set the axis of the linked component
   \param XWidth :: Axis direciton Width (half)
   \param YWidth :: Axis direciton width (half)
   \param ZWidth :: Axis direciton width (half)
 */
{
  ELog::RegMethod RegA("FixedComp","setBasicExtent");
  if (LU.size()!=6)
    throw ColErr::MisMatch<int>(6,static_cast<int>(LU.size()),
				"6/LU.size");
  

  LU[0].setConnectPt(Origin-Y*XWidth);
  LU[1].setConnectPt(Origin+Y*XWidth);
  LU[2].setConnectPt(Origin-X*YWidth);
  LU[3].setConnectPt(Origin+X*YWidth);
  LU[4].setConnectPt(Origin-Z*ZWidth);
  LU[5].setConnectPt(Origin+Z*ZWidth);

  LU[0].setAxis(-Y);
  LU[1].setAxis(Y);
  LU[2].setAxis(-X);
  LU[3].setAxis(X);
  LU[4].setAxis(-Z);
  LU[5].setAxis(Z);

  return;
}

const LinkUnit&
FixedComp::operator[](const size_t Index) const
 /*!
   Get the axis of the linked component
   \param Index :: Link number
   \return LinkUnit reference
 */
{
  ELog::RegMethod RegA("FixedComp","operator[]");
  if (Index>LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"Index/LU.size");

  return LU[Index];
}

const LinkUnit&
FixedComp::getLU(const size_t Index) const
 /*!
   Get the axis of the linked component
   \param Index :: Link number
   \return LinkUnit reference
 */
{
  ELog::RegMethod RegA("FixedComp","getLinkUnit");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"Index/LU.size");

  return LU[Index];
}

const LinkUnit&
FixedComp::getSignedLU(const long int sideIndex) const
  /*!
    Accessor to the link unit
    \param sideIndex :: SIGNED +1 side index
    \return Link Unit 
  */
{
  ELog::RegMethod RegA("FixedComp","getSignedLU:"+keyName);

  if (sideIndex)
    {
      const size_t linkIndex=
	(sideIndex>0) ? static_cast<size_t>(sideIndex-1) :
	static_cast<size_t>(-sideIndex-1) ;
      if (linkIndex<LU.size())
	return LU[linkIndex];
    }
  throw ColErr::IndexError<long int>
    (sideIndex,static_cast<long int>(LU.size()),"Index/LU.size");
}

LinkUnit&
FixedComp::getSignedLU(const long int sideIndex) 
  /*!
    Accessor to the link unit
    \param sideIndex :: SIGNED +1 side index
    \return Link Unit 
  */
{
  ELog::RegMethod RegA("FixedComp","getSignedLU:"+keyName);

  if (sideIndex)
    {
      const size_t linkIndex=
	(sideIndex>0) ? static_cast<size_t>(sideIndex-1) :
	static_cast<size_t>(-sideIndex-1) ;
      if (linkIndex<LU.size())
	return LU[linkIndex];
    }
  throw ColErr::IndexError<long int>
    (sideIndex,static_cast<long int>(LU.size()),"Index/LU.size");
}

  
int
FixedComp::getLinkSurf(const size_t Index) const
  /*!
    Accessor to the link surface string
    \param Index :: Link number
    \return Surface Key number
  */
{
  ELog::RegMethod RegA("FixedComp","getLinkSurf");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"Index to big");
  
  return LU[Index].getLinkSurf();
}

const Geometry::Vec3D&
FixedComp::getLinkPt(const size_t Index) const
  /*!
    Accessor to the link point
    \param Index :: Link number
    \return Link point
  */
{
  ELog::RegMethod RegA("FixedComp","getLinkPt:"+keyName);
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),
				     "Index/LU.size");

  // this can throw:
  return LU[Index].getConnectPt();
}

Geometry::Vec3D
FixedComp::getSignedLinkPt(const long int sideIndex) const
  /*!
    Accessor to the link point
    \param sideIndex :: SIGNED +1 side index
    \return Link point
  */
{
  ELog::RegMethod RegA("FixedComp","getSignedLinkPt:"+keyName);

  if (!sideIndex) return Origin;
  const LinkUnit& LItem=getSignedLU(sideIndex);
  return LItem.getConnectPt();
}

int
FixedComp::getSignedLinkSurf(const long int sideIndex) const
  /*!
    Accessor to the link surface string
    \param sideIndex :: Link number
    \return Surface Key number
  */
{
  ELog::RegMethod RegA("FixedComp","getSignedLinkSurf");
  if (!sideIndex) return 0;
  
  const LinkUnit& LItem=getSignedLU(sideIndex);
  const int sign((sideIndex>0) ? 1 : -1);
  return sign*LItem.getLinkSurf();
}


  
const Geometry::Vec3D&
FixedComp::getLinkAxis(const size_t Index) const
  /*!
    Accessor to the link axis
    \param Index :: Link number
    \return Link Axis
  */
{
  ELog::RegMethod RegA("FixedComp","getLinkAxis");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"Index/LU.size");
  
  return LU[Index].getAxis();
}

Geometry::Vec3D
FixedComp::getSignedLinkAxis(const long int sideIndex) const
  /*!
    Accessor to the link axis
    \param sideIndex :: SIGNED +1 side index
    \return signed Link Axis [Y is sideIndex == 0]
  */
{
  ELog::RegMethod RegA("FixedComp","getSignedLinkAxis:"+keyName);

  if (sideIndex==0)
    return Y;

  const LinkUnit& LItem=getSignedLU(sideIndex);
  return (sideIndex>0)  ? LItem.getAxis() : -LItem.getAxis();
}

std::string
FixedComp::getMasterString(const size_t Index) const
  /*!
    Accessor to the master link surface string
    \param Index :: Link number
    \return String of link
  */
{
  ELog::RegMethod RegA("FixedComp","getMasterString:"+keyName);
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"Index/LU.size");
  
  return LU[Index].getMain();
}

std::string
FixedComp::getMasterComplement(const size_t Index) const
  /*!
    Accessor to the master link surface string
    \param Index :: Link number
    \return String of link
  */
{
  ELog::RegMethod RegA("FixedComp","getMasterString");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"Index/LU.size");

  HeadRule RP;
  RP.procString(LU[Index].getMain());
  RP.makeComplement();
  return RP.display();
}


std::string
FixedComp::getSignedLinkString(const long int sideIndex) const
  /*!
    Accessor to the link string
    \param sideIndex :: SIGNED +1 side index
    \return Link string 
  */
{
  ELog::RegMethod RegA("FixedComp","getSignedLinkString:"+keyName);

  if (!sideIndex) return "";
  
  const size_t linkIndex=
    (sideIndex>0) ? static_cast<size_t>(sideIndex-1) :
    static_cast<size_t>(-sideIndex-1) ;

  return (sideIndex>0) ?
    getLinkString(linkIndex) : getLinkComplement(linkIndex);
}
  
std::string
FixedComp::getLinkString(const size_t Index) const
  /*!
    Accessor to the link surface string
    \param Index :: Link number
    \return String of link
  */
{
  ELog::RegMethod RegA("FixedComp","getLinkString");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"Index/LU.size");
  
  return LU[Index].getLinkString();
}

std::string
FixedComp::getLinkComplement(const size_t Index) const
  /*!
    Accessor to the link surface string [negative]
    \param Index :: Link number
    \return String of link
  */
{
  ELog::RegMethod RegA("FixedComp","getLinkComplement");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"Index/LU.size");

  HeadRule RP;
  RP.procString(LU[Index].getMain());
  RP.makeComplement();
  
  RP.addIntersection(LU[Index].getCommon());
  return RP.display();
}

std::string
FixedComp::getCommonString(const size_t Index) const
  /*!
    Accessor to the link surface string [negative]
    \param Index :: Link number
    \return String of link
  */
{
  ELog::RegMethod RegA("FixedComp","getBridgeString");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"Index/LU.size");

  return (LU[Index].hasCommon()) ? LU[Index].getCommon() : ""; 
}

std::string
FixedComp::getCommonComplement(const size_t Index) const
  /*!
    Accessor to the common surfaces [in complement]
    \param Index :: Link number
    \return String of common link
  */
{
  ELog::RegMethod RegA("FixedComp","getBridgeString");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"Index/LU.size");

  if (LU[Index].hasCommon())
    {
      HeadRule RP;
      RP.procString(LU[Index].getCommon());
      RP.makeComplement();
      return RP.display();
    }
  return "";
}

std::string
FixedComp::getBridgeComplement(const size_t Index) const
  /*!
    Accessor to the link surface string [negative]
    \param Index :: Link number
    \return String of link
  */
{
  ELog::RegMethod RegA("FixedComp","getBridgeComplement");
  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"Index/LU.size");

  HeadRule RP;
  RP.procString(LU[Index].getMain());
  RP.makeComplement();
  if (LU[Index].hasCommon())
    RP.addIntersection(LU[Index].getCommon());

  return RP.display();
}

void 
FixedComp::setCentre(const Geometry::Vec3D& C)
  /*!
    User Interface to LU[1] to set Point + Axis
    \param C :: Centre point
  */
{
  ELog::RegMethod RegA("FixedComp","setCentre");
  Origin=C;
  return;
}

void 
FixedComp::setExit(const Geometry::Vec3D& C,
		   const Geometry::Vec3D& A) 
  /*!
    User Interface to LU[1] to set Point + Axis
    \param C :: Connect point
    \param A :: Axis
  */
{
  ELog::RegMethod RegA("FixedComp","setExit");
  if (LU.size()<2)
    throw ColErr::IndexError<size_t>(2,LU.size(),"2/LU.size");

  LU[1].setAxis(A);
  LU[1].setConnectPt(C);

  return;
}

const Geometry::Vec3D&
FixedComp::getExit() const
  /*!
    Get Exit if set / Default to Origin
    \return Exit point
  */
{
  return (LU.size()>1 && LU[1].hasConnectPt())  ? 
    LU[1].getConnectPt() : Origin;
}

const Geometry::Vec3D&
FixedComp::getExitNorm() const
  /*!
    Get exit normal if set / Default to Beam axis
    \return Exit direction
  */
{
  return (LU.size()>1 && LU[1].hasAxis())  ? 
    LU[1].getAxis() : beamAxis;
}

size_t
FixedComp::findLinkAxis(const Geometry::Vec3D& AX) const
  /*!
    Determine the Axis of the line direction
    \param AX :: Axis point to test
    \return index value
  */
{
  ELog::RegMethod RegA("FixedComp","findLinkAxis");

  size_t outIndex(0);
  double maxValue(-1.0);  
  for(size_t i=0;i<LU.size();i++)
    {
      const double MX=AX.dotProd(LU[i].getAxis());
      if (MX>maxValue)
	{
	  outIndex=i;
	  maxValue=MX;
	}
    }
  return outIndex;
}
  
void
FixedComp::selectAltAxis(const size_t Index,Geometry::Vec3D& XOut,
			 Geometry::Vec3D& YOut,Geometry::Vec3D& ZOut) const
  /*!
    From a given index find the optimal X,Y,Z axis to use for the
    output: YOut is compared with beam to find closes axis.
    \param Index :: Link surface to use
    \param XOut :: X axis
    \param YOut :: Y axis [ beam ]
    \param ZOut :: Z axis 
  */
{
  ELog::RegMethod RegA("FixedComp","selectAltAxis");
  
  YOut=getLinkAxis(Index);

  double dp[3];
  dp[0]=fabs(X.dotProd(YOut)); 
  dp[1]=fabs(Y.dotProd(YOut)); 
  dp[2]=fabs(Z.dotProd(YOut)); 
  const double* dptr=std::max_element(dp,dp+3);

  XOut=(dptr==dp) ? Y : X;
  ZOut=(dptr==dp+2) ? Y : Z;
    
  return;
}

void
FixedComp::applyRotation(const Geometry::Vec3D& Axis,
			 const double Angle)
  /*!
    Apply a rotation to the basis set
    \param Axis :: rotation axis 
    \param Angle :: rotation angle
  */
{
  ELog::RegMethod RegA("FixedComp","applyRotation");

  const Geometry::Quaternion Qrot=
    Geometry::Quaternion::calcQRotDeg(Angle,Axis.unit());
  
  Qrot.rotate(X);
  Qrot.rotate(Y);
  Qrot.rotate(beamAxis);
  Qrot.rotate(Z);
  return;
}

HeadRule
FixedComp::getSignedMainRule(const long int sideIndex) const
  /*!
    Get the main rule.
    \param Index :: Index for LinkUnit
    \return Main HeadRule
   */
{
  ELog::RegMethod RegA("FixedComp","getSignedMainRule"); 

  const LinkUnit& LObj=getSignedLU(sideIndex);
  return (sideIndex>0) ? 
    LObj.getMainRule() :
    LObj.getMainRule().complement();
}

const HeadRule&
FixedComp::getMainRule(const size_t Index) const
  /*!
    Get the main rule.
    \param Index :: Index for LinkUnit
    \return Main HeadRule
   */
{
  ELog::RegMethod RegA("FixedComp","getMainRule"); 

  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"Index/LU.size");
  
  return LU[Index].getMainRule();
}

  
HeadRule
FixedComp::getSignedCommonRule(const long int sideIndex) const
  /*!
    Get the main rule.
    \param Index :: Index for LinkUnit
    \return Main HeadRule
   */
{
  ELog::RegMethod RegA("FixedComp","getSignedCommonRule"); 

  const LinkUnit& LObj=getSignedLU(sideIndex);
  return LObj.getCommonRule();
}

const HeadRule&
FixedComp::getCommonRule(const size_t Index) const
  /*!
    Get the common rule.
    \param Index :: Index for link unit
    \return Common Headrule 
   */
{
  ELog::RegMethod RegA("FixedComp","getMainRule"); 

  if (Index>=LU.size())
    throw ColErr::IndexError<size_t>(Index,LU.size(),"Index/LU.size");
  
  return LU[Index].getCommonRule();
}

void
FixedComp::calcLinkAxis(const long int sideIndex,
			Geometry::Vec3D& XVec,
			Geometry::Vec3D& YVec,
			Geometry::Vec3D& ZVec) const
  /*!
    Given a sideIndex calculate the axises at that point.
    biasing the prefrence to select Z relative to 
    \param sideIndex :: Signed side+1 (zero for origin of FC)
    \param XVec :: Output X Vec
    \param YVec :: Output Y Vec
    \param ZVec :: Output Z Vec
  */
{
  ELog::RegMethod RegA("FixedComp","calcLinkAxis");

  if (sideIndex==0)
    {
      XVec=X;
      YVec=Y;
      ZVec=Z;
      return;
    }
  YVec=getSignedLinkAxis(sideIndex);
  // Y not parallel to Z case
  const double ZdotYVec=Z.dotProd(YVec);

  
  const Geometry::Vec3D& ZPrime=
    (fabs(ZdotYVec) < 1.0-Geometry::zeroTol) ? Z : X;

  XVec=YVec*ZPrime;
  ZVec=YVec*XVec;
  // note that ZdotYVec could have been invalidated by swapping
  // x for z so have to recalc Y.Z'.
  if ((ZVec.dotProd(ZPrime) * ZPrime.dotProd(YVec))< -Geometry::zeroTol)
    {
      ZVec*=-1.0;
      XVec*=-1.0;
    }
  return;
}

  
int
FixedComp::getMasterSurf(const size_t outIndex) const
  /*!
    Calculate the unsigned exit surface
    \param outIdnex :: Out surface direction
    \return surfNum
  */
{
  ELog::RegMethod RegA("FixedComp","getMasterSurf");
  return std::abs(SMap.realSurf(getLinkSurf(outIndex)));
}

int
FixedComp::getExitWindow(const size_t outIndex,
			 std::vector<int>& window) const
  /*!
    Generic exit window system : 
    -- Requires at least 6 surfaces
    -- Requires 3-6 to be sign surf
    \param outIndex :: Direction 0  for entry 1 for exit
    \param window :: window vector of paired planes
    \return Viewed surface
  */
{
  ELog::RegMethod RegA("FixedComp","getExitWindow");
  if (LU.size()<6)
    throw ColErr::IndexError<size_t>(LU.size(),6,"Link size too small");
  if (outIndex>LU.size())
    throw ColErr::IndexError<size_t>(outIndex,6,"outIndex too big");

  // Get four surfaces for size:
  size_t oA[4]={2,3,4,5};
  if (outIndex>=2 && outIndex<=5)
    {
      if (outIndex>4)
	{
	  oA[2]=0;
	  oA[3]=1;
	}
      else
	{
	  oA[0]=0;
	  oA[1]=1;
	}
    }

  window.clear();
  for(size_t i=0;i<4;i++)
    window.push_back(std::abs(getLinkSurf(oA[i])));

  //
  // Extremely ugly code to extract the dividing surface [if it exists]
  //   -- determine if there is a secondary surface to the linkstring
  //      if so add as the divide plane
  //      ELSE add zero
  //
  window.push_back(0);
  std::string OutSurf=getLinkString(outIndex);
  int dSurf(0);
  const int primOutSurf(getLinkSurf(outIndex));

  for(size_t i=0;i<2 && StrFunc::section(OutSurf,dSurf) 
	&& std::abs(dSurf)==std::abs(primOutSurf);i++) ;

  if (dSurf && std::abs(dSurf)!=std::abs(primOutSurf))
    window.back()=dSurf;
    
  return std::abs(SMap.realSurf(getLinkSurf(outIndex)));
}

}  // NAMESPACE attachSystem
