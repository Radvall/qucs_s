/***************************************************************************
                               sp_sens.cpp
                               ------------
    begin                : Mon Sep 18 2017
    copyright            : (C) 2017 by Vadim Kuznetsov
    email                : ra3xdh@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "sp_sens.h"
#include "extsimkernels/spicecompat.h"


SpiceSENS::SpiceSENS()
{
  isSimulation = true;
  Description = QObject::tr("DC sensitivity simulation");
  Simulator = spicecompat::simNgspice | spicecompat::simSpiceOpus;
  initSymbol(Description);
  Model = ".SENS";
  Name  = "SENS";
  SpiceModel = ".SENS";

  // The index of the first 4 properties must not changed. Used in recreate().
  Props.append(new Property("Output", "v(out)", true,
            QObject::tr("Output variable")));
  Props.append(new Property("Param", "R1", true,
        QObject::tr("parameter to sweep")));
  Props.append(new Property("Start", "5", true,
        QObject::tr("start value for sweep")));
  Props.append(new Property("Stop", "50", true,
        QObject::tr("stop value for sweep")));
  Props.append(new Property("Step", "1", true,
        QObject::tr("Simulation step")));
}

SpiceSENS::~SpiceSENS()
{
}

Component* SpiceSENS::newOne()
{
  return new SpiceSENS();
}

Element* SpiceSENS::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("DC sensitivity simulation");
  BitmapFile = (char *) "sp_sens";

  if(getNewOne)  return new SpiceSENS();
  return 0;
}

QString SpiceSENS::spice_netlist(spicecompat::SpiceDialect dialect /* = spicecompat::SPICEDefault */)
{
    QString s;
    s.clear();
    if (dialect != spicecompat::SPICEXyce) {
        QString sweepvar = Props.at(1)->Value;
        QString par = sweepvar;
        sweepvar.remove(' ');
        QString start = spicecompat::normalize_value(Props.at(2)->Value);
        QString stop = spicecompat::normalize_value(Props.at(3)->Value);
        QString step = spicecompat::normalize_value(Props.at(4)->Value);
        QString output = "spice4qucs." + Name.toLower() + ".ngspice.sens.dc.prn";
        s += QStringLiteral("echo \"Start\">%1\n").arg(output);
        s += QStringLiteral("let %1_start=%2\n").arg(sweepvar).arg(start);
        s += QStringLiteral("let %1_sweep=%1_start\n").arg(sweepvar);
        s += QStringLiteral("let %1_step=%2\n").arg(sweepvar).arg(step);
        s += QStringLiteral("let %1_stop=%2\n").arg(sweepvar).arg(stop);
        s += QStringLiteral("while %1_sweep le %1_stop\n").arg(sweepvar);
        if (sweepvar.compare("temp",Qt::CaseInsensitive)) {
            s += QStringLiteral("alter %1 = %2_sweep\n").arg(par).arg(sweepvar);
        } else {
            s += QStringLiteral("set %1 = $&%2_sweep\n").arg(par).arg(sweepvar);
        }
        s += QStringLiteral("sens %1\n").arg(Props.at(0)->Value);
        s += QStringLiteral("echo \"Sens analysis\">>%1\n").arg(output);
        s += QStringLiteral("print %1_sweep>>%2\nprint all>>%2\n").arg(sweepvar).arg(output);
        s += QStringLiteral("let %1_sweep = %1_sweep + %1_step\nend\n").arg(sweepvar);

    }

    return s;
}
