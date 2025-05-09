/***************************************************************************
                              comp_1bit
                             -----------
    begin                : December 2008
    copyright            : (C) 2008 by Mike Brinson
    email                : mbrin72043@yahoo.co.uk
 ***************************************************************************/

/*
 * comp_1bit.cpp - device implementations for comp_1bit module
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 */
#include "comp_1bit.h"
#include "node.h"
#include "misc.h"
#include "node.h"

comp_1bit::comp_1bit()
{
  Type = isComponent; // Analogue and digital component.
  Description = QObject::tr ("1bit comparator verilog device");

  Props.append (new Property ("TR", "6", false,
    QObject::tr ("transfer function high scaling factor")));
  Props.append (new Property ("Delay", "1 ns", false,
    QObject::tr ("output delay")
    +" ("+QObject::tr ("s")+")"));
 
  createSymbol ();
  tx = x1 + 19;
  ty = y2 + 4;
  icon_dy = 8;
  Model = "comp_1bit";
  Name  = "Y";
}

Component * comp_1bit::newOne()
{
  comp_1bit * p = new comp_1bit();
  p->Props.front()->Value = Props.front()->Value; 
  p->recreate();
  return p;
}

Element * comp_1bit::info(QString& Name, char * &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("1Bit Comparator");
  BitmapFile = (char *) "comp_1bit";

  if(getNewOne) return new comp_1bit();
  return 0;
}

void comp_1bit::createSymbol()
{
  // Body
  Rects.append(new qucs::Rect(-30, -60, 60, 90, QPen(Qt::darkBlue,2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin)));
  Texts.append(new Text(-18,-55, "COMP", Qt::darkBlue, 12.0));

  // Left-side pins and their labels
  // X
  Ports.append(new Port(-50,-10));
  Lines.append(new qucs::Line(-50,-10,-30,-10,QPen(Qt::darkBlue,2)));
  Texts.append(new Text(-25,-18,   "X",  Qt::darkBlue, 12.0));
  // Y
  Ports.append(new Port(-50, 10));
  Lines.append(new qucs::Line(-50, 10,-30, 10,QPen(Qt::darkBlue,2)));
  Texts.append(new Text(-25, 2,   "Y",  Qt::darkBlue, 12.0));

  // Right-side pins and their labels
  // L
  Ports.append(new Port( 50, 20));
  Lines.append(new qucs::Line( 30, 20, 50, 20,QPen(Qt::darkBlue,2)));
  Texts.append(new Text( 0,  12, "X<Y",  Qt::darkBlue, 12.0));
  // G
  Ports.append(new Port( 50,  0));
  Lines.append(new qucs::Line( 30,  0, 50,  0,QPen(Qt::darkBlue,2)));
  Texts.append(new Text( 0,-8, "X>Y", Qt::darkBlue, 12.0));
  // E
  Ports.append(new Port( 50,-20));
  Lines.append(new qucs::Line( 30,-20, 50,-20,QPen(Qt::darkBlue,2)));
  Texts.append(new Text( 0,-28, "X=Y", Qt::darkBlue, 12.0));

  x1 = -50; y1 = -64;
  x2 =  50; y2 =  34;
}

QString comp_1bit::vhdlCode( int )
{
  QString s="";

  QString td = Props.at(1)->Value;     // delay time
  if(!misc::VHDL_Delay(td, Name)) return td; // time has not VHDL format
  td += ";\n";
 
  QString X    = Ports.at(0)->Connection->Name;
  QString Y    = Ports.at(1)->Connection->Name;
  QString L    = Ports.at(2)->Connection->Name;
  QString G    = Ports.at(3)->Connection->Name;
  QString E    = Ports.at(4)->Connection->Name;
 
  s = "\n  "+Name+":process ("+X+", "+Y+")\n"+
      "  begin\n"+
      "    "+L+" <= (not "+X+") and "+Y+td+
      "    "+G+" <= "+X+" and (not "+Y+")"+td+
      "    "+E+" <= not ("+X+" xor "+Y+")"+td+  
      "  end process;\n";
  return s;
}

QString comp_1bit::verilogCode( int )
{
  QString l="";

  QString td = Props.at(1)->Value;        // delay time
  if(!misc::Verilog_Delay(td, Name)) return td; // time does not have VHDL format

  QString X    = Ports.at(0)->Connection->Name;
  QString Y    = Ports.at(1)->Connection->Name;
  QString L    = Ports.at(2)->Connection->Name;
  QString G    = Ports.at(3)->Connection->Name;
  QString E    = Ports.at(4)->Connection->Name;

  QString LR  = "L_reg"  + Name + L;
  QString GR  = "G_reg"  + Name + G;
  QString ER  = "E_reg"  + Name + E;

  l = "\n  // "+Name+" 1bit comparator\n"+
      "  assign  "+L+" = "+LR+";\n"+
      "  reg     "+LR+" = 0;\n"+
      "  assign  "+G+" = "+GR+";\n"+
      "  reg     "+GR+" = 0;\n"+
      "  assign  "+E+" = "+ER+";\n"+
      "  reg     "+ER+" = 0;\n"+
      "  always @ ("+X+" or "+Y+")\n"+
      "  begin\n"+
      "    "+LR+" <="+td+" (~"+X+") && "+Y+";\n"+
      "    "+GR+" <="+td+" "+X+" && (~"+Y+");\n"+
      "    "+ER+" <="+td+" ~("+X+" ^ "+Y+");\n"+
      "  end\n";

  return l;
}
