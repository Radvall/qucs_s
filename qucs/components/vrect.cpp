/***************************************************************************
                                vrect.cpp
                               -----------
    begin                : Sat Sep 18 2004
    copyright            : (C) 2004 by Michael Margraf
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

#include "vrect.h"
#include "node.h"
#include "misc.h"
#include "extsimkernels/spicecompat.h"


vRect::vRect()
{
  Description = QObject::tr("ideal rectangle voltage source");

  Arcs.append(new qucs::Arc(-12,-12, 24, 24,     0, 16*360,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(-30,  0,-12,  0,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line( 30,  0, 12,  0,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line( 18,  5, 18, 11,QPen(Qt::red,1)));
  Lines.append(new qucs::Line( 21,  8, 15,  8,QPen(Qt::red,1)));
  Lines.append(new qucs::Line(-18,  5,-18, 11,QPen(Qt::black,1)));

  Lines.append(new qucs::Line( -5, -7, -5, -5,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line( -5, -5,  5, -5,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(  5, -5,  6,  0,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line( -5,  0,  6,  0,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line( -5,  0, -5,  5,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line( -5,  5,  5,  5,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(  5,  5,  5,  7,QPen(Qt::darkBlue,2)));

  Ports.append(new Port( 30,  0));
  Ports.append(new Port(-30,  0));

  x1 = -30; y1 = -14;
  x2 =  30; y2 =  14;

  tx = x1+4;
  ty = y2+4;
  Model = "Vrect";
  Name  = "V";
  SpiceModel = "V";

  Props.append(new Property("U", "1 V", true,
		QObject::tr("voltage of high signal")));
  Props.append(new Property("TH", "1 ms", true,
		QObject::tr("duration of high pulses")));
  Props.append(new Property("TL", "1 ms", true,
		QObject::tr("duration of low pulses")));
  Props.append(new Property("Tr", "1 ns", false,
		QObject::tr("rise time of the leading edge")));
  Props.append(new Property("Tf", "1 ns", false,
		QObject::tr("fall time of the trailing edge")));
  Props.append(new Property("Td", "0 ns", false,
		QObject::tr("initial delay time")));
  Props.append(new Property("U0", "0 V", true,
        QObject::tr("voltage of low signal (SPICE only)")));

  rotate();  // fix historical flaw
}

vRect::~vRect()
{
}

QString vRect::spice_netlist(spicecompat::SpiceDialect dialect /* = spicecompat::SPICEDefault */)
{
    Q_UNUSED(dialect);

    QString s = spicecompat::check_refdes(Name,SpiceModel);

    for (Port *p1 : Ports) {
        QString nam = p1->Connection->Name;
        if (nam=="gnd") nam = "0";
        s += " "+ nam;   // node names
    }

    QString U = spicecompat::normalize_value(Props.at(0)->Value); //VH
    QString Td = spicecompat::normalize_value(Props.at(5)->Value); // Td
    QString Tr = spicecompat::normalize_value(Props.at(3)->Value); // Tr
    QString Tf = spicecompat::normalize_value(Props.at(4)->Value); //Tf
    QString U0 = spicecompat::normalize_value(getProperty("U0")->Value); //VL
    QString TH = spicecompat::normalize_value(getProperty("TH")->Value);
    QString TL = spicecompat::normalize_value(getProperty("TL")->Value);

    s += QStringLiteral(" DC 0 PULSE( %1 %2 %3 %4 %5 %6 {(%7)+(%8)+(%9)+(%10)} )  AC 0\n")
             .arg(U0).arg(U).arg(Td).arg(Tr).arg(Tf).arg(TH).arg(TH).arg(TL).arg(Tr).arg(Tf);

    return s;
}

Component* vRect::newOne()
{
  return new vRect();
}

Element* vRect::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("Rectangle Voltage");
  BitmapFile = (char *) "vrect";

  if(getNewOne)  return new vRect();
  return 0;
}

QString vRect::netlist()
{
  QString s = Model+":"+Name;

  // output all node names
  for (Port *p1 : Ports)
    s += " "+p1->Connection->Name;   // node names

  // output all properties
  for(int i=0; i <= Props.count()-2; i++)
    if(Props.at(i)->Name != "U0")
      s += " "+Props.at(i)->Name+"=\""+Props.at(i)->Value+"\"";

  return s + '\n';
}
