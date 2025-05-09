/***************************************************************************
                                ac_sim.cpp
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
#include "ac_sim.h"
#include "misc.h"
#include "extsimkernels/spicecompat.h"

#include <cmath>


AC_Sim::AC_Sim()
{
  isSimulation = true;
  Description = QObject::tr("ac simulation");
  initSymbol(Description);
  Model = ".AC";
  SpiceModel = ".AC";
  Name  = "AC";

  // The index of the first 4 properties must not changed. Used in recreate().
  Props.append(new Property("Type", "lin", true,
			QObject::tr("sweep type")+" [lin, log, list, const]"));
  Props.append(new Property("Start", "1 Hz", true,
			QObject::tr("start frequency in Hertz")));
  Props.append(new Property("Stop", "10 kHz", true,
			QObject::tr("stop frequency in Hertz")));
  Props.append(new Property("Points", "200", true,
			QObject::tr("number of simulation steps")));
  Props.append(new Property("Noise", "no", false,
			QObject::tr("calculate noise voltages")+
			" [yes, no]"));
}

AC_Sim::~AC_Sim()
{
}

Component* AC_Sim::newOne()
{
  return new AC_Sim();
}

Element* AC_Sim::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("ac simulation");
  BitmapFile = (char *) "ac";

  if(getNewOne)  return new AC_Sim();
  return 0;
}

void AC_Sim::recreate()
{
  if((Props.at(0)->Value == "list") || (Props.at(0)->Value == "const")) {
    // Call them "Symbol" to omit them in the netlist.
    Props.at(1)->Name = "Symbol";
    Props.at(1)->display = false;
    Props.at(2)->Name = "Symbol";
    Props.at(2)->display = false;
    Props.at(3)->Name = "Values";
  }
  else {
    Props.at(1)->Name = "Start";
    Props.at(2)->Name = "Stop";
    Props.at(3)->Name = "Points";
  }
}

QString AC_Sim::spice_netlist(spicecompat::SpiceDialect dialect /* = spicecompat::SPICEDefault */)
{
    QString s = SpiceModel + " ";
    QString unit;
    if (Props.at(0)->Value=="log") { // convert points number for spice compatibility
        double Np,Fstart,Fstop,fac = 1.0;
        misc::str2num(Props.at(3)->Value,Np,unit,fac); // Points number
        Np *= fac;
        misc::str2num(Props.at(1)->Value,Fstart,unit,fac);
        Fstart *= fac;
        misc::str2num(Props.at(2)->Value,Fstop,unit,fac);
        Fstop *= fac;
        double Nd = ceil(log10(Fstop/Fstart)); // number of decades
        double Npd = ceil((Np - 1)/Nd); // points per decade
        s += QStringLiteral("DEC %1 ").arg(Npd);
    } else {  // no need conversion
        s += QStringLiteral("LIN %1 ").arg(Props.at(3)->Value);
    }
    QString fstart = spicecompat::normalize_value(Props.at(1)->Value); // Start freq.
    QString fstop = spicecompat::normalize_value(Props.at(2)->Value); // Stop freq.
    s += QStringLiteral("%1 %2 \n").arg(fstart).arg(fstop);
    if (dialect != spicecompat::SPICEXyce) s.remove(0,1);
    return s.toLower();

}
