/***************************************************************************
                                ground.cpp
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

#include "ground.h"


Ground::Ground()
{
  Type = isComponent;   // both analog and digital
  Description = QObject::tr("ground (reference potential)");

  Lines.append(new qucs::Line(  0,  0,  0, 10,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(-11, 10, 11, 10,QPen(Qt::darkBlue,3)));
  Lines.append(new qucs::Line( -7, 16,  7, 16,QPen(Qt::darkBlue,3)));
  Lines.append(new qucs::Line( -3, 22,  3, 22,QPen(Qt::darkBlue,3)));

  Ports.append(new Port(  0,  0));

  x1 = -12; y1 =  0;
  x2 =  12; y2 = 25;

  tx = 0;
  ty = 0;
  icon_dy = -2;
  Model = "GND";
  Name  = "";
  SpiceModel = "*";
}

Ground::~Ground()
{
}

Component* Ground::newOne()
{
  return new Ground();
}

// -------------------------------------------------------
Element* Ground::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("Ground");
  BitmapFile = (char *) "gnd";

  if(getNewOne)  return new Ground();
  return 0;
}

// -------------------------------------------------------
QString Ground::netlist()
{
  return QString();
}

QString Ground::spice_netlist(spicecompat::SpiceDialect dialect /* = spicecompat::SPICEDefault */)
{
    Q_UNUSED(dialect);

    return QString();
}
