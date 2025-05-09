/***************************************************************************
                               resistor.cpp
                              --------------
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
#include "resistor.h"
#include "extsimkernels/spicecompat.h"
#include "extsimkernels/verilogawriter.h"
#include "node.h"


Resistor::Resistor(bool european)
{
  Description = QObject::tr("resistor");

  Props.append(new Property("R", "1 kOhm", true,
    QObject::tr("ohmic resistance in Ohms")));
  Props.append(new Property("Temp", "26.85", false,
    QObject::tr("simulation temperature in degree Celsius (Qucsator only)")));
  Props.append(new Property("Tc1", "0.0", false,
    QObject::tr("first order temperature coefficient")));
  Props.append(new Property("Tc2", "0.0", false,
    QObject::tr("second order temperature coefficient")));
  Props.append(new Property("Tnom", "26.85", false,
    QObject::tr("temperature at which parameters were extracted (Qucsator only)")));

  // this must be the last property in the list !!!
  Props.append(new Property("Symbol", "european", false,
        QObject::tr("schematic symbol")+" [european, US]"));
  if(!european)  Props.back()->Value = "US";

  createSymbol();
  tx = x1+4;
  ty = y2+4;
  Model = "R";
  SpiceModel = "R";
  Name  = "R";
}

// -------------------------------------------------------
Component* Resistor::newOne()
{
  return new Resistor(Props.back()->Value != "US");
}

QString Resistor::spice_netlist(spicecompat::SpiceDialect dialect /* = spicecompat::SPICEDefault */)
{
    QString s = spicecompat::check_refdes(Name,SpiceModel);

    s += QStringLiteral(" %1 %2 ").arg(Ports.at(0)->Connection->Name)
            .arg(Ports.at(1)->Connection->Name); // output 2 nodes
    s.replace(" gnd ", " 0 ");

    QString Tc1 = getProperty("Tc1")->Value;
    QString Tc2 = getProperty("Tc2")->Value;

    s += QStringLiteral(" %1").arg(spicecompat::normalize_value(Props.at(0)->Value));

    if (dialect != spicecompat::CDL)
    {
        if (!Tc1.isEmpty()) {
            s += " tc1=" + Tc1;
        }

        if (!Tc2.isEmpty()) {
            s += " tc2=" + Tc2;
        }
    }

    s += QStringLiteral(" \n");

    return s;
}

QString Resistor::cdl_netlist()
{
  return spice_netlist(spicecompat::CDL);
}

QString Resistor::va_code()
{
    QString val = vacompat::normalize_value(Props.at(0)->Value);
    QString valTemp = vacompat::normalize_value(Props.at(1)->Value);
    QString plus =  Ports.at(0)->Connection->Name;
    QString minus = Ports.at(1)->Connection->Name;
    QString s = "";
    QString Vpm = vacompat::normalize_voltage(plus,minus);
    QString Ipm = vacompat::normalize_current(plus,minus,true);

    if (plus=="gnd") s += QStringLiteral("%1 <+ -(%2/( %3 ));\n").arg(Ipm).arg(Vpm).arg(val);
    else s+= QStringLiteral("%1 <+ %2/( %3 );\n").arg(Ipm).arg(Vpm).arg(val);
    s += QStringLiteral("%1 <+ white_noise( 4.0*`P_K*( %2 + 273.15) / ( %3 ), \"thermal\" );\n")
                 .arg(Ipm).arg(valTemp).arg(val);

    return s;
}

// -------------------------------------------------------
void Resistor::createSymbol()
{
  if(Props.back()->Value != "US") {
    Lines.append(new qucs::Line(-18, -9, 18, -9,QPen(Qt::darkBlue,2)));
    Lines.append(new qucs::Line( 18, -9, 18,  9,QPen(Qt::darkBlue,2)));
    Lines.append(new qucs::Line( 18,  9,-18,  9,QPen(Qt::darkBlue,2)));
    Lines.append(new qucs::Line(-18,  9,-18, -9,QPen(Qt::darkBlue,2)));
    Lines.append(new qucs::Line(-30,  0,-18,  0,QPen(Qt::darkBlue,2)));
    Lines.append(new qucs::Line( 18,  0, 30,  0,QPen(Qt::darkBlue,2)));
  }
  else {
    Lines.append(new qucs::Line(-30,  0,-18,  0,QPen(Qt::darkBlue,2)));
    Lines.append(new qucs::Line(-18,  0,-15, -7,QPen(Qt::darkBlue,2, Qt::SolidLine, Qt::RoundCap)));
    Lines.append(new qucs::Line(-15, -7, -9,  7,QPen(Qt::darkBlue,2, Qt::SolidLine, Qt::RoundCap)));
    Lines.append(new qucs::Line( -9,  7, -3, -7,QPen(Qt::darkBlue,2, Qt::SolidLine, Qt::RoundCap)));
    Lines.append(new qucs::Line( -3, -7,  3,  7,QPen(Qt::darkBlue,2, Qt::SolidLine, Qt::RoundCap)));
    Lines.append(new qucs::Line(  3,  7,  9, -7,QPen(Qt::darkBlue,2, Qt::SolidLine, Qt::RoundCap)));
    Lines.append(new qucs::Line(  9, -7, 15,  7,QPen(Qt::darkBlue,2, Qt::SolidLine, Qt::RoundCap)));
    Lines.append(new qucs::Line( 15,  7, 18,  0,QPen(Qt::darkBlue,2, Qt::SolidLine, Qt::RoundCap)));
    Lines.append(new qucs::Line( 18,  0, 30,  0,QPen(Qt::darkBlue,2)));
  }

  Ports.append(new Port(-30,  0));
  Ports.append(new Port( 30,  0));

  x1 = -30; y1 = -11;
  x2 =  30; y2 =  11;
}

// -------------------------------------------------------
Element* Resistor::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("Resistor");
  BitmapFile = (char *) "resistor";

  if(getNewOne)  return new Resistor();
  return 0;
}

// -------------------------------------------------------
Element* Resistor::info_us(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("Resistor US");
  BitmapFile = (char *) "resistor_us";

  if(getNewOne)  return new Resistor(false);
  return 0;
}
