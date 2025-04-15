/***************************************************************************
                         core.cpp  -  description
                   --------------------------------------
    begin                  : Sun 22 Nov 2015
    copyright              : (C) by Mike Brinson (mbrin72043@yahoo.co.uk),
                           :  Vadim Kuznetsov (ra3xdh@gmail.com)

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ja_core.h"
#include "node.h"
#include "extsimkernels/spicecompat.h"

#include "main.h"

#include <QObject>
#include <QString>


JA_core::JA_core()
{
  Description = QObject::tr("Magnetic core");
  Simulator = spicecompat::simSpice;

  Lines.append(new qucs::Line(-40,   0,  -30,     0,QPen(Qt::darkRed,2)));  // L1
  Lines.append(new qucs::Line(-30, -15,  30,    -15,QPen(Qt::darkRed,3)));      // L2
  Lines.append(new qucs::Line( 30, -15,  30,     15,QPen(Qt::darkRed,3)));      // L3
  Lines.append(new qucs::Line( 30,  15, -30,     15,QPen(Qt::darkRed,3)));      // L4
  Lines.append(new qucs::Line(-30,  15, -30,    -15,QPen(Qt::darkRed,3)));  // L5

  Lines.append(new qucs::Line(-25,   -5,  25,   -5,QPen(Qt::black,3)));      // L7
  Lines.append(new qucs::Line(-25,    0,  25,    0,QPen(Qt::black,3)));      // L8
  Lines.append(new qucs::Line(-25,    5,  25,    5,QPen(Qt::black,3)));      // L9

  Lines.append(new qucs::Line(40,     0, 30,     0,QPen(Qt::darkRed,2)));  // L16


  Texts.append(new Text(-50,-25,"H",Qt::black,18));
  Texts.append(new Text(35,-25,"B",Qt::black,18));
  //Lines.append(new qucs::Line( -40,  -30, -40,  -20,QPen(Qt::red,3)));        // +
  //Lines.append(new qucs::Line( -45,  -25, -35,  -25,QPen(Qt::red,3)));
  //Lines.append(new qucs::Line(  45,  -25,  35,  -25,QPen(Qt::black,3)));      // -

  Ports.append(new Port( -40,    0));  // Pin
  Ports.append(new Port(  40,    0));  // Pout

  x1 = -45; y1 = -30;
  x2 =  45; y2 =  30;

  tx = x1+5;
  ty = y2+5;
  Model = "CORE";
  SpiceModel = "X";
  Name  = "CORE";

  Props.append(new Property("A", "26.0", true,QObject::tr("Domains wall density (A/m)")));
  Props.append(new Property("K", "27.0", true,QObject::tr("Domains pinning (A/m)")));
  Props.append(new Property("C", "0.05", true,QObject::tr("Magnetization reversibility")));
  Props.append(new Property("Ms", "395e3", true,QObject::tr("Saturation magnetization (A/m)")));
  Props.append(new Property("alpha", "1e-4", true,QObject::tr("Interdomain coupling")));
  Props.append(new Property("PATH", "1.0", true,QObject::tr("Magnetic path length (m)")));
  Props.append(new Property("AREA", "1.0", true,QObject::tr("Core cross section area (m^2)")));
  Props.append(new Property("GAP", "0.0", true,QObject::tr("Air gap length (m)")));
  Props.append(new Property("CoreType", "generic", true,"[generic,ring,E-type]"));
  Props.append(new Property("D1", "1.0", false,"Dimension 1"));
  Props.append(new Property("D2", "1.0", false,"Dimension 2"));
  Props.append(new Property("D3", "1.0", false,"Dimension 3"));
  Props.append(new Property("D4", "1.0", false,"Dimension 4"));
  Props.append(new Property("D5", "1.0", false,"Dimension 5"));
  Props.append(new Property("D6", "1.0", false,"Dimension 6"));

}

JA_core::~JA_core()
{
}

Component* JA_core::newOne()
{
  return new JA_core();
}

Element* JA_core::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("core");
  BitmapFile = (char *) "core";

  if(getNewOne)  return new JA_core();
  return 0;
}

QString JA_core::netlist()
{
    return QString();
}

QString JA_core::spice_netlist(spicecompat::SpiceDialect dialect /* = spicecompat::SPICEDefault */)
{
  Q_UNUSED(dialect);

  QString h_node = Ports.at(0)->Connection->Name;
  QString b_node = Ports.at(1)->Connection->Name;

  QString s = "X_" + Name;
  s += QString(" %1 %2 ja_core ").arg(h_node).arg(b_node);
  QString A = spicecompat::normalize_value(getProperty("A")->Value);
  QString K = spicecompat::normalize_value(getProperty("K")->Value);
  QString C = spicecompat::normalize_value(getProperty("C")->Value);
  QString alpha = spicecompat::normalize_value(getProperty("alpha")->Value);
  QString MS = spicecompat::normalize_value(getProperty("Ms")->Value);
  QString AREA = spicecompat::normalize_value(getProperty("AREA")->Value);
  QString PATH = spicecompat::normalize_value(getProperty("PATH")->Value);
  QString GAP = spicecompat::normalize_value(getProperty("GAP")->Value);

  s += QString("A=%1 K=%2 MS=%3 C=%4 alpha=%5 AREA=%6 PATH=%7 GAP=%8\n")
           .arg(A).arg(K).arg(MS).arg(C).arg(alpha).arg(AREA).arg(PATH).arg(GAP);

  return s;
}

QString JA_core::getSpiceLibrary()
{
  QString f = QucsSettings.LibDir + QDir::toNativeSeparators("/magnetics/core.cir");
  QString s = QString (".INCLUDE \"%1\"\n").arg(f);
  return s;
}
