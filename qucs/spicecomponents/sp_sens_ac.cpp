/***************************************************************************
                               sp_sens_ac.cpp
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
#include "sp_sens_ac.h"
#include "extsimkernels/spicecompat.h"


SpiceSENS_AC::SpiceSENS_AC()
{
  isSimulation = true;
  Description = QObject::tr("AC sensitivity simulation");
  Simulator = spicecompat::simNgspice | spicecompat::simSpiceOpus;
  initSymbol(Description);
  Model = ".SENS_AC";
  Name  = "SENS";
  SpiceModel = ".SENS";

  // The index of the first 4 properties must not changed. Used in recreate().
  Props.append(new Property("Output", "v(out)", true,
            QObject::tr("Output variable")));
  Props.append(new Property("Type", "lin", true,
            QObject::tr("sweep type")+" [lin, dec, oct]"));
  Props.append(new Property("Start", "1 Hz", true,
            QObject::tr("start frequency in Hertz")));
  Props.append(new Property("Stop", "1000 Hz", true,
            QObject::tr("stop frequency in Hertz")));
  Props.append(new Property("Points", "10", true,
            QObject::tr("number of simulation steps")));
}

SpiceSENS_AC::~SpiceSENS_AC()
{
}

Component* SpiceSENS_AC::newOne()
{
  return new SpiceSENS_AC();
}

Element* SpiceSENS_AC::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("AC sensitivity simulation");
  BitmapFile = (char *) "sp_sens_ac";

  if(getNewOne)  return new SpiceSENS_AC();
  return 0;
}

QString SpiceSENS_AC::spice_netlist(spicecompat::SpiceDialect dialect /* = spicecompat::SPICEDefault */)
{
    QString s;
    s.clear();
    if (dialect != spicecompat::SPICEXyce) {
        QString fstart = spicecompat::normalize_value(Props.at(2)->Value); // Start freq.
        QString fstop = spicecompat::normalize_value(Props.at(3)->Value); // Stop freq.
        QString out = "spice4qucs." + Name.toLower() + ".sens.prn";
        s = QStringLiteral("sens %1 ac %2 %3 %4 %5\n")
                .arg(Props.at(0)->Value).arg(Props.at(1)->Value).arg(Props.at(4)->Value)
                .arg(fstart).arg(fstop);
        s += QStringLiteral("write %1 all\n").arg(out);
    }

    return s;
}
