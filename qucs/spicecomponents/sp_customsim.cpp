/***************************************************************************
                                tr_sim.cpp
                               ------------
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
#include "sp_customsim.h"


SpiceCustomSim::SpiceCustomSim()
{
  isSimulation = true;
  Description = QObject::tr("Nutmeg script");
  Simulator = spicecompat::simNgspice | spicecompat::simSpiceOpus;
  initSymbol(Description);
  Model = ".CUSTOMSIM";
  Name  = "CUSTOM";
  SpiceModel = "CUSTOM";

  // The index of the first 4 properties must not changed. Used in recreate().
  Props.append(new Property("SpiceCode", "\n"
                            "AC DEC 100 1K 10MEG\n"
                            "let K=V(out)/V(in)\n\n"
                            "* Use mouse double click on device to edit code\n\n"
                            "* Extra output\n"
                            "* A custom prefix could be placed between # #\n"
                            "* It will be prepended to all dataset variables\n"
                            "write custom#ac1#.plot K\n\n"
                            "* Scalars can be printed\n"
                            "* They will be available in the dataset\n"
                            "let Vout_max=vecmax(V(out))\n"
                            "let KdB_max=db(vecmax(K))\n"
                            "print Vout_max KdB_max > custom#ac1#.print\n",
                            true,
                                         "Insert spice code here"));
  Props.append(new Property("Vars","V(out);V(in)",false,"Vars to plot"));
  Props.append(new Property("Outputs","custom#ac1#.plot;custom#ac1#.print",false,"Extra outputs to parse"));

}

SpiceCustomSim::~SpiceCustomSim()
{
}

Component* SpiceCustomSim::newOne()
{
  return new SpiceCustomSim();
}

Element* SpiceCustomSim::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("Nutmeg script");
  BitmapFile = (char *) "sp_customsim";

  if(getNewOne)  return new SpiceCustomSim();
  return 0;
}

QString SpiceCustomSim::spice_netlist(spicecompat::SpiceDialect dialect /* = spicecompat::SPICEDefault */)
{
    QString s = "";
    if (dialect == spicecompat::SPICEXyce) return s;
    s = Props.at(0)->Value+"\n";
    return s;
}
