/***************************************************************************
                          dc_sim.cpp  -  description
                             -------------------
    begin                : Sat Aug 23 2003
    copyright            : (C) 2003 by Michael Margraf
    email                : michael.margraf@alumni.tu-berlin.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "dc_sim.h"

DC_Sim::DC_Sim()
{
  isSimulation = true;
  Description = QObject::tr("dc simulation");
  initSymbol(Description);
  Model = ".DC";
  Name  = "DC";
  SpiceModel = ".OP";

  Props.append(new Property("Temp", "26.85", false,
		QObject::tr("simulation temperature in degree Celsius")));
  Props.append(new Property("reltol", "0.001", false,
		QObject::tr("relative tolerance for convergence")));
  Props.append(new Property("abstol", "1 pA", false,
		QObject::tr("absolute tolerance for currents")));
  Props.append(new Property("vntol", "1 uV", false,
		QObject::tr("absolute tolerance for voltages")));
  Props.append(new Property("saveOPs", "no", false,
		QObject::tr("put operating points into dataset")+
		" [yes, no]"));
  Props.append(new Property("MaxIter", "150", false,
		QObject::tr("maximum number of iterations until error")));
  Props.append(new Property("saveAll", "no", false,
	QObject::tr("save subcircuit nodes into dataset")+
	" [yes, no]"));
  Props.append(new Property("convHelper", "none", false,
	QObject::tr("preferred convergence algorithm")+
	" [none, gMinStepping, SteepestDescent, LineSearch, Attenuation, SourceStepping]"));
  Props.append(new Property("Solver", "CroutLU", false,
	QObject::tr("method for solving the circuit matrix")+
	" [CroutLU, DoolittleLU, HouseholderQR, HouseholderLQ, GolubSVD]"));
}

DC_Sim::~DC_Sim()
{
}

Component* DC_Sim::newOne()
{
  return new DC_Sim();
}

Element* DC_Sim::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("dc simulation");
  BitmapFile = (char *) "dc";

  if(getNewOne)  return new DC_Sim();
  return 0;
}

QString DC_Sim::spice_netlist(spicecompat::SpiceDialect dialect /* = spicecompat::SPICEDefault */)
{
    QString s;
    if (dialect != spicecompat::SPICEXyce) {
        s += "op\n";
    }

    return s;
}
