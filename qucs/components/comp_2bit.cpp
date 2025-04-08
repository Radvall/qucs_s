/***************************************************************************
                             comp_2bit
                            -----------
    begin                : December 2008
    copyright            : (C) 2008 by Mike Brinson
    email                : mbrin72043@yahoo.co.uk
 ***************************************************************************/

/*
 * comp_2bit.cpp - device implementations for comp_2bit module
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 */
#include "comp_2bit.h"
#include "node.h"
#include "misc.h"

comp_2bit::comp_2bit()
{
  Type = isComponent; // Analogue and digital component.
  Description = QObject::tr ("2bit comparator verilog device");

  Props.append (new Property ("TR", "6", false,
    QObject::tr ("transfer function high scaling factor")));
  Props.append (new Property ("Delay", "1 ns", false,
    QObject::tr ("output delay")
    +" ("+QObject::tr ("s")+")"));
 
  createSymbol ();
  tx = x1 + 19;
  ty = y2 + 4;
  Model = "comp_2bit";
  Name  = "Y";
}

Component * comp_2bit::newOne()
{
  comp_2bit * p = new comp_2bit();
  p->Props.front()->Value = Props.front()->Value; 
  p->recreate();
  return p;
}

Element * comp_2bit::info(QString& Name, char * &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("2Bit Comparator");
  BitmapFile = (char *) "comp_2bit";

  if(getNewOne) return new comp_2bit();
  return 0;
}

void comp_2bit::createSymbol()
{
  // Body
  Rects.append(new qucs::Rect(-40, -50, 80, 110, QPen(Qt::darkBlue,2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin)));
  Texts.append(new Text(-18,-45, "COMP", Qt::darkBlue, 12.0));

  // Left side pins and their labels
  // X0
  Ports.append(new Port(-60,-10));
  Lines.append(new qucs::Line(-60,-10,-40,-10,QPen(Qt::darkBlue,2)));
  Texts.append(new Text(-35,-18,   "0",  Qt::darkBlue, 12.0));
  // X1
  Ports.append(new Port(-60, 10));
  Lines.append(new qucs::Line(-60, 10,-40, 10,QPen(Qt::darkBlue,2)));
  Texts.append(new Text(-34, 2,   "1",  Qt::darkBlue, 12.0));

  Texts.append(new Text(-25,-12.5,   "{",  Qt::darkBlue, 16.0));
  Texts.append(new Text(-17,-9,   "X",  Qt::darkBlue, 12.0));

  // Y0
  Ports.append(new Port(-60, 30));
  Lines.append(new qucs::Line(-60, 30,-40, 30,QPen(Qt::darkBlue,2)));
  Texts.append(new Text(-35, 22,   "0",  Qt::darkBlue, 12.0));
  // Y1
  Ports.append(new Port(-60, 50));
  Lines.append(new qucs::Line(-60, 50,-40, 50,QPen(Qt::darkBlue,2)));
  Texts.append(new Text(-34, 42,   "1",  Qt::darkBlue, 12.0));

  Texts.append(new Text(-25, 27.5,   "{",  Qt::darkBlue, 16.0));
  Texts.append(new Text(-17, 31,   "Y",  Qt::darkBlue, 12.0));


  // Right side pins and their labels
  // L
  Ports.append(new Port( 60, 30));
  Lines.append(new qucs::Line( 40, 30, 60, 30,QPen(Qt::darkBlue,2)));
  Texts.append(new Text(  12, 22, "X<Y",  Qt::darkBlue, 12.0));
  // G
  Ports.append(new Port( 60, 10));
  Lines.append(new qucs::Line( 40, 10, 60, 10,QPen(Qt::darkBlue,2)));
  Texts.append(new Text(  12, 2, "X>Y", Qt::darkBlue, 12.0));
  // E
  Ports.append(new Port( 60,-10));
  Lines.append(new qucs::Line( 40,-10, 60,-10,QPen(Qt::darkBlue,2)));
  Texts.append(new Text(  12,-18, "X=Y", Qt::darkBlue, 12.0));


  x1 = -60; y1 = -54;
  x2 =  60; y2 =  64;
}

QString comp_2bit::vhdlCode( int )
{
  QString s="";

  QString td = Props.at(1)->Value;     // delay time
  if(!misc::VHDL_Delay(td, Name)) return td; // time has not VHDL format
  td += ";\n";

  QString X0   = Ports.at(0)->Connection->Name;
  QString X1   = Ports.at(1)->Connection->Name;
  QString Y0   = Ports.at(2)->Connection->Name;
  QString Y1   = Ports.at(3)->Connection->Name;
  QString L    = Ports.at(4)->Connection->Name;
  QString G    = Ports.at(5)->Connection->Name;
  QString E    = Ports.at(6)->Connection->Name;
 
  s = "\n  "+Name+":process ("+X0+", "+X1+", "+Y0+", "+Y1+")\n"+
      "  begin\n"+
      "    "+L+" <= ((not "+X1+") and "+Y1+") or (not ("+X1+" xor "+Y1+") and (not ("+X0+") and "+Y0+"))"+td+
      "    "+G+" <= ("+X1+" and (not "+Y1+"))"+" or (not ("+X1+" xor "+Y1+") and ("+X0+" and (not "+Y0+")))"+td+
      "    "+E+" <= (not ("+X1+" xor "+Y1+")) and (not ("+X0+" xor "+Y0+"))"+td+  
      "  end process;\n";
  return s;
}

QString comp_2bit::verilogCode( int )
{
  QString l="";

  QString td = Props.at(1)->Value;        // delay time
  if(!misc::Verilog_Delay(td, Name)) return td; // time does not have VHDL format

  QString X0    = Ports.at(0)->Connection->Name;
  QString X1    = Ports.at(1)->Connection->Name;
  QString Y0    = Ports.at(2)->Connection->Name;
  QString Y1    = Ports.at(3)->Connection->Name;
  QString L     = Ports.at(4)->Connection->Name;
  QString G     = Ports.at(5)->Connection->Name;
  QString E     = Ports.at(6)->Connection->Name;

  QString LR  = "L_reg"  + Name + L;
  QString GR  = "G_reg"  + Name + G;
  QString ER  = "E_reg"  + Name + E;

  l = "\n  // "+Name+" 2bit comparator\n"+
      "  assign  "+L+" = "+LR+";\n"+
      "  reg     "+LR+" = 0;\n"+
      "  assign  "+G+" = "+GR+";\n"+
      "  reg     "+GR+" = 0;\n"+
      "  assign  "+E+" = "+ER+";\n"+
      "  reg     "+ER+" = 0;\n"+
      "  always @ ("+X0+" or "+X1+" or "+Y0+" or "+Y1+")\n"+
      "  begin\n"+
      "    "+LR+" <="+td+" ((~"+X1+") && "+Y1+") || (~("+X1+" ^ "+Y1+") && (~("+X0+") && "+Y0+"));\n"+
      "    "+GR+" <="+td+" ("+X1+" && (~"+Y1+"))"+" || (~("+X1+" ^ "+Y1+") && ("+X0+" && (~"+Y0+")));\n"+
      "    "+ER+" <="+td+" (~("+X1+" ^ "+Y1+")) && (~("+X0+" ^ "+Y0+"));\n"+  
      "  end\n";
  
  return l;
}
