/********************************************************************* 
  CombLayer : MCNP(X) Input builder
 
 * File:   photon/PhotonVariables.cxx
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

#include "Exception.h"
#include "FileReport.h"
#include "NameStack.h"
#include "RegMethod.h"
#include "GTKreport.h"
#include "OutputLog.h"
#include "support.h"
#include "MatrixBase.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "Code.h"
#include "varList.h"
#include "FuncDataBase.h"
#include "variableSetup.h"

namespace setVariable
{

void
PhotonVariables(FuncDataBase& Control)
  /*!
    Function to set the control variables and constants
    -- This version is for Photon Moderator
    \param Control :: Function data base to add constants too
  */
{
  ELog::RegMethod RegA("PhotonVariables[F]","PhotonVariables");

  Control.addVariable("laserSourceParticleType",1);     

  Control.addVariable("laserSourceXStep",0.0);       
  Control.addVariable("laserSourceYStep",-5.0);
  Control.addVariable("laserSourceZStep",0.0);       

  Control.addVariable("laserSourceShape",0);
  Control.addVariable("laserSourceRadius",0.1);       
  Control.addVariable("laserSourceASpread",3.0);       
  Control.addVariable("laserSourceEStart",1.0);       
  Control.addVariable("laserSourceNE",10);       
  //  Control.addVariable("laserSourceEEnd",80.0);     

  Control.addVariable("laserSourceEnergy",
		      "1.5743253000e-01 1.7496055000e-01 1.9447280000e-01 "
		      "2.1294111000e-01 2.2970930000e-01 2.5530586000e-01 "
		      "2.8806970000e-01 3.2498354000e-01 3.7220290000e-01 "
		      "4.4595280000e-01 5.2631220000e-01 6.4986813000e-01 "
		      "7.7863586000e-01 1.0051130000e+00 1.2597324000e+00 "
		      "1.5545493000e+00 2.0373990000e+00 2.4402770000e+00 "
		      "2.9213471000e+00 3.3458118000e+00 3.8306620000e+00 "
		      "4.2575135000e+00 4.9498553000e+00 5.6690580000e+00 "
		      "6.4911210000e+00 7.7779536000e+00 8.6461070000e+00 "
		      "9.4671930000e+00 9.6176280000e+00 ");
  Control.addVariable("laserSourceEProb",
		      "1.1414313932e+08 8.3536723912e+07 6.4857300459e+07 "
		      "4.3851755669e+07 2.8437764164e+07 3.1014027386e+07 "
		      "2.7031914640e+07 2.1761931266e+07 1.8958009430e+07 "
		      "1.8320349539e+07 1.2958970279e+07 1.1471860693e+07 "
		      "7.3973640814e+06 8.2518069286e+06 5.3420942641e+06 "
		      "4.2153288883e+06 4.0740240641e+06 2.3159351182e+06 "
		      "2.1770310225e+06 1.3081641259e+06 1.1205574185e+06 "
		      "7.0480772070e+05 7.6011160786e+05 5.3774016332e+05 "
		      "4.4995782718e+05 4.2542994596e+05 1.9541100800e+05 "
		      "1.3202298449e+05 1.9013627678e+04 ");

  // PHOTONMOD2:
 
  Control.addVariable("PModOuterHeight",7.0);
  Control.addVariable("PModOuterWidth",7.0);
  Control.addVariable("PModInnerHeight",5.0);
  Control.addVariable("PModInnerWidth",5.0);
  Control.addVariable("PModOuterMat","Aluminium");

  Control.addVariable("PModNLayer",6);
  Control.addVariable("PModThick0",0.3);    // void
  Control.addVariable("PModThick1",2.5);    // lead
  Control.addVariable("PModThick2",1.2);    // W
  Control.addVariable("PModThick3",4.0);    // Polystyrene
  Control.addVariable("PModThick4",0.2);    // Aluminium [with hole]
  Control.addVariable("PModThick5",0.2);    // Void

  Control.addVariable("PModVHeight0",3.0);
  Control.addVariable("PModVHeight1",0.0);
  Control.addVariable("PModVHeight2",0.0);
  Control.addVariable("PModVHeight3",0.0);
  Control.addVariable("PModVHeight4",3.0);
  Control.addVariable("PModVHeight5",0.0);

  Control.addVariable("PModVWidth0",3.0);
  Control.addVariable("PModVWidth1",0.0);
  Control.addVariable("PModVWidth1",0.0);
  Control.addVariable("PModVWidth2",0.0);
  Control.addVariable("PModVWidth3",0.0);
  Control.addVariable("PModVWidth4",3.0);
  Control.addVariable("PModVWidth5",0.0);    

  Control.addVariable("PModMat0","Void");
  Control.addVariable("PModMat1","Lead");
  Control.addVariable("PModMat2","Tungsten");
  Control.addVariable("PModMat3","Polystyrene");
  Control.addVariable("PModMat4","Aluminium");
  Control.addVariable("PModMat5","Void");

  Control.addVariable("TPlateYStep",22.0);
  Control.addVariable("TPlateHeight",5.40);
  Control.addVariable("TPlateWidth",5.40);
  Control.addVariable("TPlateDepth",0.1);
  Control.addVariable("TPlateDefMat","Void");

  Control.addVariable("THoldYStep",22.0);
  Control.addVariable("THoldRadius",6.0);
  Control.addVariable("THoldDefMat","Void");


  
  return;
}

}  // NAMESPACE setVariable