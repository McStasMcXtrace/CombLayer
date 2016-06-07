/********************************************************************* 
  CombLayer : MCNP(X) Input builder
 
 * File:   commonVarInc/ChopperGenerator.h
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
#ifndef essSystem_ChopperGenerator_h
#define essSystem_ChopperGenerator_h

class Simulation;

namespace setVariable
{

/*!
  \class ChopperGenerator
  \version 1.0
  \author S. Ansell
  \date May 2016
  \brief ChopperGenerator for variables
*/

class ChopperGenerator
{
 private:

  double mainZStep;        ///< Offset of centre axis [drawing 5962.2]
  double height;           ///< Full height
  double width;            ///< Full width
  double shortWidth;       ///< ???
  double mainRadius;       ///< full chopper void radius
  double windowThick;      ///< window material thickness

  std::string wallMat;     ///< Main body material
  std::string sealMat;     ///< seal material
  std::string windowMat;     ///< seal material
  
 public:

  ChopperGenerator();
  ChopperGenerator(const ChopperGenerator&);
  ChopperGenerator& operator=(const ChopperGenerator&);
  ~ChopperGenerator();

  void generateChopper(FuncDataBase&,
		       const std::string&,
		       const double,const double,
		       const double);
  

  
};

}

#endif
 
