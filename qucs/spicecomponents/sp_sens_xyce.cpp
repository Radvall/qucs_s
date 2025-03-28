/***************************************************************************
                               sp_sens_xyce.cpp
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
#include "sp_sens_xyce.h"
#include "extsimkernels/spicecompat.h"


SpiceSENS_Xyce::SpiceSENS_Xyce()
{
  isSimulation = true;
  Description = QObject::tr("DC .SENS simulation with Xyce");
  Simulator = spicecompat::simXyce;
  initSymbol(Description);
  Model = ".SENS_XYCE";
  Name  = "SENS";
  SpiceModel = ".SENS";

  // The index of the first 4 properties must not changed. Used in recreate().
  Props.append(new Property("Objfunc", "v(out)", true,
            QObject::tr("Output expressions")));
  Props.append(new Property("RefParam", "R1:R", true,
        QObject::tr("Reference parameter for .SENS analysis")));
  Props.append(new Property("Param", "V1", true,
        QObject::tr("Parameter for DC sweep")));
  Props.append(new Property("Start", "5", true,
        QObject::tr("start value for DC sweep")));
  Props.append(new Property("Stop", "50", true,
        QObject::tr("stop value for DC sweep")));
  Props.append(new Property("Step", "1", true,
        QObject::tr("Simulation step for DC sweep")));
}

SpiceSENS_Xyce::~SpiceSENS_Xyce()
{
}

Component* SpiceSENS_Xyce::newOne()
{
  return new SpiceSENS_Xyce();
}

Element* SpiceSENS_Xyce::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("DC sensitivity simulation");
  BitmapFile = (char *) "sp_sens_xyce";
  if(getNewOne)  return new SpiceSENS_Xyce();
  return 0;
}

QString SpiceSENS_Xyce::spice_netlist(spicecompat::SpiceDialect dialect /* = spicecompat::SPICEDefault */)
{
    QString s;
    s.clear();
    if (dialect == spicecompat::SPICEXyce) {
        QString start = spicecompat::normalize_value(Props.at(3)->Value);
        QString stop = spicecompat::normalize_value(Props.at(4)->Value);
        QString step = spicecompat::normalize_value(Props.at(5)->Value);
        s = QStringLiteral(".dc %3 %4 %5 %6\n"
                    ".sens objfunc={%1} param=%2\n"
                    ".print sens\n").arg(Props.at(0)->Value).arg(Props.at(1)->Value)
                .arg(Props.at(2)->Value).arg(start).arg(stop).arg(step);
    }

    return s;
}
