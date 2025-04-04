/***************************************************************************
                                relais.cpp
                               ------------
    begin                : Sat Feb 25 2006
    copyright            : (C) 2006 by Michael Margraf
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

#include "relais.h"
#include "node.h"
#include "misc.h"
#include "extsimkernels/spicecompat.h"


Relais::Relais()
{
  Description = QObject::tr("relay");

  // driver contacts
  Lines.append(new qucs::Line(-30,-30,-30, -8,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(-30,  8,-30, 30,QPen(Qt::darkBlue,2)));
  // driver rect
  Lines.append(new qucs::Line(-45, -8,-15, -8,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(-45,  8,-15,  8,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(-45, -8,-45,  8,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(-15, -8,-15,  8,QPen(Qt::darkBlue,2)));
  // diagonal
  Lines.append(new qucs::Line(-45,  8,-15, -8,QPen(Qt::darkBlue,2, Qt::SolidLine, Qt::FlatCap)));
  // plus
  Lines.append(new qucs::Line(-43, -3,-37, -3,QPen(Qt::red,1)));
  Lines.append(new qucs::Line(-40, -6,-40,  0,QPen(Qt::red,1)));
  // minus
  Lines.append(new qucs::Line(-23,  3,-17,  3,QPen(Qt::black,1)));

  Lines.append(new qucs::Line(-15,  0, 35,  0,QPen(Qt::darkBlue,1,Qt::DotLine)));

  Lines.append(new qucs::Line( 30,-30, 30,-18,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line( 30, 15, 30, 30,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line( 30, 15, 45,-15,QPen(Qt::darkBlue,2)));
  Arcs.append(new qucs::Arc( 27,-18, 5, 5, 0, 16*360,QPen(Qt::darkBlue,2)));
  Ellipses.append(new qucs::Ellips( 27, 12, 6, 6, QPen(Qt::darkBlue,2),
                         QBrush(Qt::darkBlue, Qt::SolidPattern)));

  Ports.append(new Port(-30,-30));
  Ports.append(new Port( 30,-30));
  Ports.append(new Port( 30, 30));
  Ports.append(new Port(-30, 30));

  x1 = -48; y1 = -30;
  x2 =  45; y2 =  30;

  tx = x2+4;
  ty = y1+4;
  Model = "Relais";
  Name  = "S";
  SpiceModel = "SW";

  Props.append(new Property("Vt", "0.5 V", false,
		QObject::tr("threshold voltage in Volts")));
  Props.append(new Property("Vh", "0.1 V", false,
		QObject::tr("hysteresis voltage in Volts")));
  Props.append(new Property("Ron", "0", false,
		QObject::tr("resistance of \"on\" state in Ohms")));
  Props.append(new Property("Roff", "1e12", false,
		QObject::tr("resistance of \"off\" state in Ohms")));
  Props.append(new Property("Temp", "26.85", false,
		QObject::tr("simulation temperature in degree Celsius")));
}

Relais::~Relais()
{
}

Component* Relais::newOne()
{
  return new Relais();
}

QString Relais::spice_netlist(spicecompat::SpiceDialect dialect /* = spicecompat::SPICEDefault */)
{
    QString s = Name;
    QString unit;
    double Vt,Vh,fac;

    QList<int> seq; // nodes sequence
    seq<<1<<2<<0<<3;
    // output all node names
    for (int i : seq) {
        QString nam = Ports.at(i)->Connection->Name;
        if (nam=="gnd") nam = "0";
        s += " "+ nam;   // node names
    }

    QString model = " MOD_" + Name;
    s += model + " OFF\n";

    QString val = Props.at(0)->Value; // Vt
    misc::str2num(val,Vt,unit,fac);
    Vt *= fac;

    val = Props.at(1)->Value;
    misc::str2num(val,Vh,unit,fac);
    Vh *= fac;

    QString Ron = spicecompat::normalize_value(Props.at(2)->Value);
    QString Roff = spicecompat::normalize_value(Props.at(3)->Value);

    if (dialect == spicecompat::SPICEXyce) {
        s += QStringLiteral(".MODEL %1 vswitch von=%2 voff=%3 ron=%4 roff=%5 \n").arg(model).arg(Vt).arg(Vt-Vh).arg(Ron).arg(Roff);
    } else {
        s += QStringLiteral(".MODEL %1 sw vt=%2 vh=%3 ron=%4 roff=%5 \n").arg(model).arg(Vt).arg(Vh).arg(Ron).arg(Roff);
    }

    return s;
}

Element* Relais::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("Relay");
  BitmapFile = (char *) "relais";

  if(getNewOne)  return new Relais();
  return 0;
}
