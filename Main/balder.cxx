/********************************************************************* 
  CombLayer : MCNP(X) Input builder
 
 * File:   Main/balder.cxx
 *
 * Copyright (c) 2004-2018 by Stuart Ansell
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
#include <array>

#include "Exception.h"
#include "MersenneTwister.h"
#include "FileReport.h"
#include "NameStack.h"
#include "RegMethod.h"
#include "GTKreport.h"
#include "OutputLog.h"
#include "BaseVisit.h"
#include "BaseModVisit.h"
#include "surfRegister.h"
#include "objectRegister.h"
#include "InputControl.h"
#include "MatrixBase.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "inputParam.h"
#include "Transform.h"
#include "Quaternion.h"
#include "Surface.h"
#include "Quadratic.h"
#include "Rules.h"
#include "surfIndex.h"
#include "Code.h"
#include "varList.h"
#include "FuncDataBase.h"
#include "HeadRule.h"
#include "Object.h"
#include "Qhull.h"
#include "MainProcess.h"
#include "MainInputs.h"
#include "SimProcess.h"
#include "Simulation.h" 
#include "SimPHITS.h"
#include "ContainedComp.h"
#include "LinkUnit.h"
#include "FixedComp.h"
#include "mainJobs.h"
#include "Volumes.h"
#include "DefPhysics.h"
#include "variableSetup.h"
#include "ImportControl.h"
#include "World.h"
#include "SimInput.h"

#include "makeBalder.h"

MTRand RNG(12345UL);

///\cond STATIC
namespace ELog 
{
  ELog::OutputLog<EReport> EM;
  ELog::OutputLog<FileReport> FM("Spectrum.log");
  ELog::OutputLog<FileReport> RN("Renumber.txt");   ///< Renumber
  ELog::OutputLog<StreamReport> CellM;
}
///\endcond STATIC

int 
main(int argc,char* argv[])
{
  int exitFlag(0);                // Value on exit
  ELog::RegMethod RControl("","main");
  mainSystem::activateLogging(RControl);

  std::string Oname;
  std::vector<std::string> Names;  

  Simulation* SimPtr(0);
  try
    {
      // PROCESS INPUT:
      InputControl::mainVector(argc,argv,Names);
      mainSystem::inputParam IParam;
      createXrayInputs(IParam);

      SimPtr=createSimulation(IParam,Names,Oname);
      if (!SimPtr) return -1;

      // The big variable setting
      setVariable::balderVariables(SimPtr->getDataBase());
      InputModifications(SimPtr,IParam,Names);
      mainSystem::setMaterialsDataBase(IParam);

      SimPtr->setMCNPversion(IParam.getValue<int>("mcnp"));
      
      xraySystem::makeBalder BObj;
      World::createOuterObjects(*SimPtr);
      BObj.build(*SimPtr,IParam);
      
      mainSystem::buildFullSimulation(SimPtr,IParam,Oname);
          
      exitFlag=SimProcess::processExitChecks(*SimPtr,IParam);
      ModelSupport::calcVolumes(SimPtr,IParam);

      ModelSupport::objectRegister::Instance().write("ObjectRegister.txt");
    }
  catch (ColErr::ExitAbort& EA)
    {
      if (!EA.pathFlag())
	ELog::EM<<"Exiting from "<<EA.what()<<ELog::endCrit;
      exitFlag=-2;
    }
  catch (ColErr::ExBase& A)
    {
      ELog::EM<<"EXCEPTION FAILURE :: "
	      <<A.what()<<ELog::endCrit;
      exitFlag= -1;
    }
  catch (...)
    {
      ELog::EM<<"GENERAL EXCEPTION"<<ELog::endCrit;
      exitFlag= -3;
    }

  delete SimPtr;
  ModelSupport::objectRegister::Instance().reset();
  ModelSupport::surfIndex::Instance().reset();
  return exitFlag;
}
