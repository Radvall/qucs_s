/***************************************************************************
                             volt_ac_SPICE.cpp
                              -------------
    begin                : Thu May 21 2015
    copyright            : (C) 2015 by Vadim Kuznetsov
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

#include "volt_ac_SPICE.h"
#include "extsimkernels/spicecompat.h"


Vac_SPICE::Vac_SPICE()
{
  Description = QObject::tr("AC voltage source (SPICE)");
  Simulator = spicecompat::simSpice;

  Arcs.append(new qucs::Arc(-12,-12, 24, 24,     0, 16*360,QPen(Qt::red,2)));
  Arcs.append(new qucs::Arc( -3, -7,  7,  7,16*270, 16*180,QPen(Qt::red,2)));
  Arcs.append(new qucs::Arc( -3,  0,  7,  7, 16*90, 16*180,QPen(Qt::red,2)));
  Lines.append(new qucs::Line(-30,  0,-12,  0,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line( 30,  0, 12,  0,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line( 18,  5, 18, 11,QPen(Qt::red,1)));
  Lines.append(new qucs::Line( 21,  8, 15,  8,QPen(Qt::red,1)));
  Lines.append(new qucs::Line(-18,  5,-18, 11,QPen(Qt::black,1)));

  Ports.append(new Port( 30,  0));
  Ports.append(new Port(-30,  0));

  x1 = -30; y1 = -14;
  x2 =  30; y2 =  14;

  tx = x1+4;
  ty = y2+4;
  Model = "Vac_SPICE";
  SpiceModel = "V";
  Name  = "V";

  Props.append(new Property("Vac", "", true,"Expression"));
  Props.append(new Property("Vac_Line 2", "", false,"Expression"));
  Props.append(new Property("Vac_Line 3", "", false,"Expression"));
  Props.append(new Property("Vac_Line 4", "", false,"Expression"));
  Props.append(new Property("Vac_Line 5", "", false,"Expression"));

  rotate();  // fix historical flaw
}

Vac_SPICE::~Vac_SPICE()
{
}

Component* Vac_SPICE::newOne()
{
  return new Vac_SPICE();
}

Element* Vac_SPICE::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("ac Voltage Source");
  BitmapFile = (char *) "Vac_SPICE";

  if(getNewOne)  return new Vac_SPICE();
  return 0;
}

QString Vac_SPICE::spice_netlist(spicecompat::SpiceDialect dialect /* = spicecompat::SPICEDefault */)
{
    Q_UNUSED(dialect);

    QString s = spicecompat::check_refdes(Name,SpiceModel);
    for (Port *p1 : Ports) {
        QString nam = p1->Connection->Name;
        if (nam=="gnd") nam = "0";
        s += " "+ nam+" ";   // node names
    }

    QString l0= Props.at(0)->Value;
    QString l1= Props.at(1)->Value;
    QString l2= Props.at(2)->Value;
    QString l3= Props.at(3)->Value;
    QString l4= Props.at(4)->Value;

    if(l0.length()> 0)   s += QStringLiteral("%1").arg(l0);
    if(l1.length()> 0)   s += QStringLiteral("\n%1").arg(l1);
    if(l2.length()> 0)   s += QStringLiteral("\n%1").arg(l2);
    if(l3.length()> 0)   s += QStringLiteral("\n%1").arg(l3);
    if(l4.length()> 0)   s += QStringLiteral("\n%1").arg(l4);
    s += "\n";

    return s;
}
