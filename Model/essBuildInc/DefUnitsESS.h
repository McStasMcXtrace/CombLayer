/********************************************************************* 
  CombLayer : MCNP(X) Input builder
 
 * File:   essBuildInc/DefUnitsESS.h
 *
 * Copyright (c) 2004-2017 by Stuart Ansell
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
#ifndef mainSystem_DefUnitsESS_h
#define mainSystem_DefUnitsESS_h

class Simulation;
class FuncDataBase;

namespace mainSystem
{
  class defaultConfig;
  class inputParam;

  void setDefUnits(FuncDataBase&,inputParam&);
  void setESSNeutronics(defaultConfig&,const std::string&,
			const std::string&);

  void setESS(defaultConfig&);
  void setESSPortsOnly(defaultConfig&,const std::string&,
		       const std::string&);
  void setESSFull(defaultConfig&); 
  void setESSSingle(defaultConfig&,const std::string&,
		    const std::string&,int);
  void setESSSingle(defaultConfig&,std::vector<std::string>&);
  void setESSLinac(defaultConfig&);

}


#endif
 
