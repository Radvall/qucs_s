/***************************************************************************
                              jk_flipflop.cpp
                             -----------------
    begin                : Fri Jan 06 2006
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
#include "jk_flipflop.h"
#include "node.h"
#include "misc.h"
#include "node.h"


JK_FlipFlop::JK_FlipFlop()
{
  Type = isDigitalComponent;
  Description = QObject::tr("JK flip flop with asynchronous set and reset");

  Props.append(new Property("t", "0", false, QObject::tr("delay time")));

  Rects.append(new qucs::Rect(-20, -30, 40, 60, QPen(Qt::darkBlue,2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin)));

  // J
  Ports.append(new Port(-30,-20));
  Lines.append(new qucs::Line(-30,-20,-20,-20,QPen(Qt::darkBlue,2)));
  Texts.append(new Text(-17,-27.5, "J", Qt::darkBlue, 12.0));
  // K
  Ports.append(new Port(-30, 20));
  Lines.append(new qucs::Line(-30, 20,-20, 20,QPen(Qt::darkBlue,2)));
  Texts.append(new Text(-17,  12.5, "K", Qt::darkBlue, 12.0));
  // Q
  Ports.append(new Port( 30,-20));
  Lines.append(new qucs::Line( 30,-20, 20,-20,QPen(Qt::darkBlue,2)));
  Texts.append(new Text(  6,-27.5, "Q", Qt::darkBlue, 12.0));
  // nQ
  Ports.append(new Port( 30, 20));
  Lines.append(new qucs::Line( 30, 20, 20, 20,QPen(Qt::darkBlue,2)));
  Texts.append(new Text(  6, 12.5, "Q", Qt::darkBlue, 12.0));
  Texts.last()->over=true;
  // Clock
  Ports.append(new Port(-30,  0));
  Lines.append(new qucs::Line(-30,  0,-20,  0,QPen(Qt::darkBlue,2)));
  Polylines.append(new qucs::Polyline(
    std::vector<QPointF>{{-20, 4}, {-12, 0}, {-20, -4}}, QPen(Qt::darkBlue,2, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin)));
  // set
  Ports.append(new Port(  0,-40));
  Lines.append(new qucs::Line(  0,-30,  0,-40,QPen(Qt::darkBlue,2)));
  Texts.append(new Text( -3.5,-29, "S", Qt::darkBlue,  9.0));
  // reset
  Ports.append(new Port(  0, 40));
  Lines.append(new qucs::Line(  0, 30,  0, 40,QPen(Qt::darkBlue,2)));
  Texts.append(new Text( -3.5, 18, "R", Qt::darkBlue,  9.0));

  x1 = -30; y1 = -40;
  x2 =  30; y2 =  40;
  tx = x1+4;
  ty = y2+4;
  Model = "JKFF";
  Name  = "Y";
}

// -------------------------------------------------------
QString JK_FlipFlop::vhdlCode(int NumPorts)
{
  QString s = "";
  if(NumPorts <= 0) { // no truth table simulation ?
    QString td = Props.at(0)->Value;     // delay time
    if(!misc::VHDL_Delay(td, Name)) return td; // time has not VHDL format
    s += td;
  }
  s += ";\n";

  s = "  " + Name + " : process (" +
      Ports.at(5)->Connection->Name + ", " +
      Ports.at(6)->Connection->Name + ", " +
      Ports.at(4)->Connection->Name + ")\n  begin\n    if (" +
      Ports.at(6)->Connection->Name + "='1') then  " +
      Ports.at(2)->Connection->Name + " <= '0'" + s +"    elsif (" +
      Ports.at(5)->Connection->Name + "='1') then  " +
      Ports.at(2)->Connection->Name + " <= '1'" + s +"    elsif (" +
      Ports.at(4)->Connection->Name + "='1' and " +
      Ports.at(4)->Connection->Name + "'event) then\n      " +
      Ports.at(2)->Connection->Name + " <= (" +
      Ports.at(0)->Connection->Name + " and not " +
      Ports.at(2)->Connection->Name + ") or (not " +
      Ports.at(1)->Connection->Name + " and " +
      Ports.at(2)->Connection->Name + ")" + s +
      "    end if;\n  end process;\n  " +
      Ports.at(3)->Connection->Name + " <= not " +
      Ports.at(2)->Connection->Name + ";\n\n";
  return s;
}

// -------------------------------------------------------
QString JK_FlipFlop::verilogCode(int NumPorts)
{
  QString t = "";
  if(NumPorts <= 0) { // no truth table simulation ?
    QString td = Props.at(0)->Value;        // delay time
    if(!misc::Verilog_Delay(td, Name)) return td; // time has not VHDL format
    if(!td.isEmpty()) t = "   " + td + ";\n";
  }

  QString l = "";

  QString s = Ports.at(5)->Connection->Name;
  QString r = Ports.at(6)->Connection->Name;
  QString j = Ports.at(0)->Connection->Name;
  QString k = Ports.at(1)->Connection->Name;
  QString q = Ports.at(2)->Connection->Name;
  QString b = Ports.at(3)->Connection->Name;
  QString c = Ports.at(4)->Connection->Name;
  QString v = "net_reg" + Name + q;
  
  l = "\n  // " + Name + " JK-flipflop\n" +
    "  assign  " + q + " = " + v + ";\n" +
    "  assign  " + b + " = ~" + q + ";\n" +
    "  reg     " + v + " = 0;\n" +
    "  always @ (" + c + " or " + r + " or " + s + ") begin\n" + t +
    "    if (" + r + ") " + v + " <= 0;\n" +
    "    else if (" + s + ") " + v + " <= 1;\n" +
    "    else if (" + c + ")\n" + 
    "      " + v + " <= (" + j + " && ~" + q + ") || (~" +
    k + " && " + q + ");\n" +
    "  end\n\n";
  return l;
}

// -------------------------------------------------------
Component* JK_FlipFlop::newOne()
{
  return new JK_FlipFlop();
}

// -------------------------------------------------------
Element* JK_FlipFlop::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("JK-FlipFlop");
  BitmapFile = (char *) "jkflipflop";

  if(getNewOne)  return new JK_FlipFlop();
  return 0;
}
