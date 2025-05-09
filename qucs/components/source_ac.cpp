/***************************************************************************
                               source_ac.cpp
                              ---------------
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

#include "source_ac.h"
#include "node.h"
#include "extsimkernels/spicecompat.h"

#include <cmath>

Source_ac::Source_ac()
{
  Description = QObject::tr("ac power source");

  Lines.append(new qucs::Line(-22,-11, 22,-11,QPen(Qt::darkGray,0)));
  Lines.append(new qucs::Line(-22, 11, 22, 11,QPen(Qt::darkGray,0)));
  Lines.append(new qucs::Line(-22,-11,-22, 11,QPen(Qt::darkGray,0)));
  Lines.append(new qucs::Line( 22,-11, 22, 11,QPen(Qt::darkGray,0)));

  Arcs.append(new qucs::Arc(-19, -9, 18, 18,     0, 16*360,QPen(Qt::darkBlue,2)));
  Arcs.append(new qucs::Arc(-13, -6,  6,  6,16*270, 16*180,QPen(Qt::darkBlue,2)));
  Arcs.append(new qucs::Arc(-13,  0,  6,  6, 16*90, 16*180,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(-30,  0,-19,  0,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line( 30,  0, 19,  0,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line( -1,  0,  3,  0,QPen(Qt::darkBlue,2)));

  Lines.append(new qucs::Line(  3, -5, 19, -5,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(  3,  5, 19,  5,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(  3, -5,  3,  5,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line( 19, -5, 19,  5,QPen(Qt::darkBlue,2)));

  Lines.append(new qucs::Line( 25,  5, 25, 11,QPen(Qt::red,1)));
  Lines.append(new qucs::Line( 28,  8, 22,  8,QPen(Qt::red,1)));
  Lines.append(new qucs::Line(-25,  5,-25, 11,QPen(Qt::black,1)));

  Ports.append(new Port( 30,  0));
  Ports.append(new Port(-30,  0));

  x1 = -30; y1 = -14;
  x2 =  30; y2 =  14;

  tx = x1+4;
  ty = y2+4;
  Model = "Pac";
  SpiceModel = "P";
  Name  = "P";

  // This property must be the first one !
  Props.append(new Property("Num", "1", true,
		QObject::tr("number of the port")));
  Props.append(new Property("Z", "50 Ohm", true,
		QObject::tr("port impedance")));
  Props.append(new Property("P", "0 dBm", false,
		QObject::tr("(available) ac power in dBm")));
  Props.append(new Property("f", "1 MHz", false,
		QObject::tr("frequency in Hertz")));
  Props.append(new Property("Temp", "26.85", false,
	QObject::tr("simulation temperature in degree Celsius")));
  Props.append(new Property("EnableTran", "true", false,
    QObject::tr("enable transient model as sine source [true,false]")));

  rotate();  // fix historical flaw
}

Source_ac::~Source_ac()
{
}

Component* Source_ac::newOne()
{
  return new Source_ac();
}

Element* Source_ac::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("Power Source");
  BitmapFile = (char *) "source";

  if(getNewOne)  return new Source_ac();
  return 0;
}


QString Source_ac::ngspice_netlist()
{
    QString s = QStringLiteral("V%1").arg(Name);
    for (Port *p1 : Ports) {
        QString nam = p1->Connection->Name;
        if (nam=="gnd") nam = "0";
        s += " "+ nam;   // node names
    }

    double z0 = spicecompat::normalize_value(getProperty("Z")->Value).toDouble();
    double p = spicecompat::normalize_value(getProperty("P")->Value).toDouble();
    double vrms = sqrt(z0/1000.0)*pow(10, p/20.0);
    double vamp = 2.0*vrms*sqrt(2.0);
    QString f = spicecompat::normalize_value(getProperty("f")->Value);

    bool en_tran = true;
    if (getProperty("EnableTran")->Value == "true") {
        en_tran = true;
    } else {
        en_tran = false;
    }

    s += QStringLiteral(" dc 0 ac %1").arg(vamp);
    if (en_tran) {
        s += QStringLiteral(" SIN(0 %1 %2)").arg(vamp).arg(f);
    }
    s += QStringLiteral(" portnum %1").arg(getProperty("Num")->Value);
    s += QStringLiteral(" z0 %1").arg(z0);
    s += "\n";
    return s;
}

QString Source_ac::xyce_netlist()
{
    QString s = spicecompat::check_refdes(Name,SpiceModel);
    for (Port *p1 : Ports) {
        QString nam = p1->Connection->Name;
        if (nam=="gnd") nam = "0";
        s += " "+ nam;   // node names
    }
    s += QStringLiteral(" port=%1 ").arg(getProperty("Num")->Value);
    QString s_z0 = spicecompat::normalize_value(getProperty("Z")->Value);
    double z0 = s_z0.toDouble();
    QString s_p = spicecompat::normalize_value(getProperty("P")->Value);
    double p = s_p.toDouble();
    double vrms = sqrt(z0/1000.0)*pow(10, p/20.0);
    double vamp = 2.0*vrms*sqrt(2.0);

    bool en_tran = true;
    if (getProperty("EnableTran")->Value == "true") {
        en_tran = true;
    } else {
        en_tran = false;
    }

    s += QStringLiteral(" z0=%1 ").arg(s_z0);
    QString f = spicecompat::normalize_value(getProperty("f")->Value);
    s += QStringLiteral(" AC %1 ").arg(vamp);
    if (en_tran) {
        s += QStringLiteral(" SIN 0 %1 %2").arg(vamp).arg(f);
    }
    s += "\n";
    return s;
}

QString Source_ac::spice_netlist(spicecompat::SpiceDialect dialect /* = spicecompat::SPICEDefault */)
{
    if (dialect == spicecompat::SPICEXyce) {
        return xyce_netlist();
    } else {
        return ngspice_netlist();
    }
}

QString Source_ac::netlist()
{
    QString s = Model+":"+Name;

    // output all node names
    for (Port *p1 : Ports)
      s += " "+p1->Connection->Name;   // node names

    // output all properties
    for(int i=0; i <= Props.count()-2; i++)
      if(Props.at(i)->Name != "EnableTran")
        s += " "+Props.at(i)->Name+"=\""+Props.at(i)->Value+"\"";

    return s + '\n';
}
