/********************************************************************* 
  CombLayer : MCNP(X) Input builder
 
 * File:   essBuild/Chicane.cxx
 *
 * Copyright (c) 2004-2016 by Konstantin Batkov
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
#include "stringCombine.h"
#include "MatrixBase.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "Quaternion.h"
#include "Surface.h"
#include "surfIndex.h"
#include "surfRegister.h"
#include "objectRegister.h"
#include "surfEqual.h"
#include "Quadratic.h"
#include "Plane.h"
#include "Cylinder.h"
#include "Line.h"
#include "Rules.h"
#include "varList.h"
#include "Code.h"
#include "FuncDataBase.h"
#include "inputParam.h"
#include "HeadRule.h"
#include "Object.h"
#include "Qhull.h"
#include "Simulation.h"
#include "ReadFunctions.h"
#include "ModelSupport.h"
#include "MaterialSupport.h"
#include "generateSurf.h"
#include "LinkUnit.h"
#include "FixedComp.h"
#include "FixedOffset.h"
#include "ContainedComp.h"
#include "BaseMap.h"
#include "FixedOffset.h"
#include "surfDBase.h"
#include "surfDIter.h"
#include "surfDivide.h"
#include "SurInter.h"
#include "mergeTemplate.h"

#include "Chicane.h"

namespace essSystem
{

Chicane::Chicane(const std::string& Key)  :
  attachSystem::ContainedComp(),
  attachSystem::FixedOffset(Key,6),
  keyName(Key),
  surfIndex(ModelSupport::objectRegister::Instance().cell(Key)),
  cellIndex(surfIndex+1)
  /*!
    Constructor BUT ALL variable are left unpopulated.
    \param Key :: Name for item in search
  */
{}

Chicane::Chicane(const Chicane& A) : 
  attachSystem::ContainedComp(A),
  attachSystem::FixedOffset(A),
  surfIndex(A.surfIndex),cellIndex(A.cellIndex),
  nSegments(A.nSegments),
  length(A.length),width(A.width),height(A.height),
  mat(A.mat)
  /*!
    Copy constructor
    \param A :: Chicane to copy
  */
{}

Chicane&
Chicane::operator=(const Chicane& A)
  /*!
    Assignment operator
    \param A :: Chicane to copy
    \return *this
  */
{
  if (this!=&A)
    {
      attachSystem::ContainedComp::operator=(A);
      attachSystem::FixedOffset::operator=(A);
      cellIndex=A.cellIndex;
      nSegments=A.nSegments;
      length=A.length;
      width=A.width;
      height=A.height;
      mat=A.mat;
    }
  return *this;
}

Chicane::~Chicane() 
  /*!
    Destructor
  */
{}

void
Chicane::populate(const FuncDataBase& Control)
  /*!
    Populate all the variables
    \param Control :: Variable data base
  */
{
  ELog::RegMethod RegA("Chicane","populate");

  FixedOffset::populate(Control);

  nSegments=Control.EvalVar<int>(keyName+"NSegments");
  for (size_t i=0; i<nSegments; i++)
    {
      const double l=Control.EvalVar<double>(StrFunc::makeString(keyName+"Length", i+1));
      length.push_back(l);
    }
  
  width=Control.EvalVar<double>(keyName+"Width");
  height=Control.EvalVar<double>(keyName+"Height");

  mat=ModelSupport::EvalMat<int>(Control,keyName+"Mat");

  return;
}
  
void
Chicane::createUnitVector(const attachSystem::FixedComp& FC)
  /*!
    Create the unit vectors
    \param FC :: object for origin
  */
{
  ELog::RegMethod RegA("Chicane","createUnitVector");

  FixedComp::createUnitVector(FC);
  applyShift(xStep,yStep,zStep);
  applyAngleRotate(xyAngle,zAngle);

  return;
}
  
void
Chicane::createSurfaces(const attachSystem::FixedComp& FC,
			const size_t& innerLP,
			const size_t& outerLP,
			const size_t& roofLP)
  /*!
    Create All the surfaces
    \param FC :: Bunker
    \param innerLP :: inner link ponit of Bunker
    \param outerLP :: outer link ponit of Bunker
    \param roofLP  :: link point to the inner roof of Bunker
  */
{
  ELog::RegMethod RegA("Chicane","createSurfaces");

  const Geometry::Plane *pRoof = SMap.realPtr<Geometry::Plane>(FC.getLinkSurf(roofLP));
  const Geometry::Cylinder *cylInner = SMap.realPtr<Geometry::Cylinder>(FC.getLinkSurf(innerLP));
  cylInner->print();

  ModelSupport::buildCylinder(SMap,surfIndex+7,cylInner->getCentre(),
			      cylInner->getNormal(),
			      cylInner->getRadius()-length[0]);

  ModelSupport::buildPlane(SMap,surfIndex+3,Origin-X*(width/2.0),X);
  ModelSupport::buildPlane(SMap,surfIndex+4,Origin+X*(width/2.0),X);

  SMap.addMatch(surfIndex+5,-FC.getLinkSurf(roofLP));
  ModelSupport::buildShiftedPlane(SMap,surfIndex+6,pRoof,height);
  ModelSupport::buildShiftedPlane(SMap,surfIndex+16,pRoof,height*2.0);
  
  return;
}
  
void
Chicane::createObjects(Simulation& System,
		       const attachSystem::FixedComp& FC,
		       const size_t& innerLP,
		       const size_t& outerLP,
		       const size_t& roofLP)
  /*!
    Adds the all the components
    \param System :: Simulation to create objects in
    \param FC :: Bunker
    \param innerLP :: inner link ponit of Bunker
    \param outerLP :: outer link ponit of Bunker
    \param roofLP  :: link point to the inner roof of Bunker
  */
{
  ELog::RegMethod RegA("Chicane","createObjects");

  const std::string innerSurf = FC.getLinkComplement(innerLP);
  const std::string outerSurf = FC.getLinkComplement(outerLP);
  const std::string roofSurf = FC.getLinkComplement(roofLP);

  ELog::EM << FC.getLinkSurf(roofLP) << " " << roofSurf << ELog::endDiag;
  
  std::string Out;
  Out=ModelSupport::getComposite(SMap,surfIndex," 3 -4 5 -6 ")
    + " " + innerSurf + " " + outerSurf;
  Out=ModelSupport::getComposite(SMap,surfIndex," 3 -4 5 -6 7 ")
    + " " + " " + outerSurf;
  System.addCell(MonteCarlo::Qhull(cellIndex++,mat,0.0,Out));

  addOuterSurf(Out);

  return;
}

  
void
Chicane::createLinks()
  /*!
    Create all the linkes
  */
{
  ELog::RegMethod RegA("Chicane","createLinks");

  //  FixedComp::setConnect(0,Origin,-Y);
  //  FixedComp::setLinkSurf(0,-SMap.realSurf(surfIndex+1));
  
  return;
}
  
  

  
void
Chicane::createAll(Simulation& System,
		   const attachSystem::FixedComp& origFC,
		   const attachSystem::FixedComp& FC,
		   const size_t& innerLP,
		   const size_t& outerLP,
		   const size_t& roofLP)
  /*!
    Generic function to create everything
    \param System :: Simulation item
    \param origFC :: Central origin
    \param FC :: Bunker
    \param innerLP :: inner link ponit of Bunker
    \param outerLP :: outer link ponit of Bunker
    \param roofLP  :: link point to the inner roof of Bunker
  */
{
  ELog::RegMethod RegA("Chicane","createAll");

  populate(System.getDataBase());
  createUnitVector(origFC);
  createSurfaces(FC, innerLP, outerLP, roofLP);
  createLinks();

  createObjects(System, FC, innerLP, outerLP, roofLP);
  insertObjects(System);              

  return;
}

}  // essSystem essSystem
