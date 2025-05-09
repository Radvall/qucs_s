/*
 * pad4bit.cpp - device implementations for pad4bit module
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 */
#include "pad4bit.h"
#include "node.h"

pad4bit::pad4bit()
{
  Type = isComponent; // Analogue and digital component.
  Description = QObject::tr ("4bit pattern generator verilog device");

  Props.append (new Property ("Number", "0", false,
    QObject::tr ("pad output value")));

  createSymbol ();
  tx = x1 + 4;
  ty = y2 + 4;
  icon_dx = 6;
  Model = "pad4bit";
  Name  = "Y";
}

Component * pad4bit::newOne()
{
  pad4bit * p = new pad4bit();
  p->Props.front()->Value = Props.front()->Value; 
  p->recreate();
  return p;
}

Element * pad4bit::info(QString& Name, char * &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("4Bit Pattern");
  BitmapFile = (char *) "pad4bit";

  if(getNewOne) return new pad4bit();
  return 0;
}

void pad4bit::createSymbol()
{
  // Body
  Rects.append(new qucs::Rect(-60, -50, 90, 100, QPen(Qt::darkGreen,2,Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin)));

  // Pins
  // A
  Ports.append(new Port(40,-30));
  Lines.append(new qucs::Line( 40,-30, 30,-30,QPen(Qt::darkGreen,2)));
  // B
  Ports.append(new Port(40,-10));
  Lines.append(new qucs::Line( 40,-10, 30,-10,QPen(Qt::darkGreen,2)));
  // C
  Ports.append(new Port(40, 10));
  Lines.append(new qucs::Line( 40, 10, 30, 10,QPen(Qt::darkGreen,2)));
  // D
  Ports.append(new Port(40, 30));
  Lines.append(new qucs::Line( 40, 30, 30, 30,QPen(Qt::darkGreen,2)));
 
  // 1st row
  Texts.append(new Text(-50,-38, "0", Qt::darkGreen, 12.0));
  Texts.append(new Text(-30,-38, "1", Qt::darkGreen, 12.0));
  Texts.append(new Text(-10,-38, "2", Qt::darkGreen, 12.0));
  Texts.append(new Text(10,-38, "3", Qt::darkGreen, 12.0));
  // 2nd row
  Texts.append(new Text(-50,-18, "4", Qt::darkGreen, 12.0));
  Texts.append(new Text(-30,-18, "5", Qt::darkGreen, 12.0));
  Texts.append(new Text(-10,-18, "6", Qt::darkGreen, 12.0));
  Texts.append(new Text( 10,-18, "7", Qt::darkGreen, 12.0));
  // 3rd row
  Texts.append(new Text(-50,2, "8", Qt::darkGreen, 12.0));
  Texts.append(new Text(-30,2, "9", Qt::darkGreen, 12.0));
  Texts.append(new Text(-15,2, "10", Qt::darkGreen, 12.0));
  Texts.append(new Text(  5,2, "11", Qt::darkGreen, 12.0));
  // 4th row
  Texts.append(new Text(-55,22, "12", Qt::darkGreen, 12.0));
  Texts.append(new Text(-35,22, "13", Qt::darkGreen, 12.0));
  Texts.append(new Text(-15,22, "14", Qt::darkGreen, 12.0));
  Texts.append(new Text(  5,22, "15", Qt::darkGreen, 12.0));
 

  x1 = -64; y1 = -54;
  x2 =  40; y2 =  54;
}

QString pad4bit::vhdlCode( int )
{
  QString v = Props.at(0)->Value;
  QString s1, s2, s3, s ="";

  QString A    = Ports.at(0)->Connection->Name;
  QString B    = Ports.at(1)->Connection->Name;
  QString C    = Ports.at(2)->Connection->Name;
  QString D    = Ports.at(3)->Connection->Name;

  s1 = "\n  "+Name+":process\n"+
       "  variable n_" + Name + " : integer := " + v + ";\n" +
       "  begin\n";
  s2 = "    case n_" + Name + " is\n" +
       "      when  0 => "+A+" <= '0'; "+B+" <= '0'; "+C+" <= '0'; "+D+" <= '0';\n"+
       "      when  1 => "+A+" <= '0'; "+B+" <= '0'; "+C+" <= '0'; "+D+" <= '1';\n"+
       "      when  2 => "+A+" <= '0'; "+B+" <= '0'; "+C+" <= '1'; "+D+" <= '0';\n"+
       "      when  3 => "+A+" <= '0'; "+B+" <= '0'; "+C+" <= '1'; "+D+" <= '1';\n"+
       "      when  4 => "+A+" <= '0'; "+B+" <= '1'; "+C+" <= '0'; "+D+" <= '0';\n"+
       "      when  5 => "+A+" <= '0'; "+B+" <= '1'; "+C+" <= '0'; "+D+" <= '1';\n"+
       "      when  6 => "+A+" <= '0'; "+B+" <= '1'; "+C+" <= '1'; "+D+" <= '0';\n"+
       "      when  7 => "+A+" <= '0'; "+B+" <= '1'; "+C+" <= '1'; "+D+" <= '1';\n"+
       "      when  8 => "+A+" <= '1'; "+B+" <= '0'; "+C+" <= '0'; "+D+" <= '0';\n"+
       "      when  9 => "+A+" <= '1'; "+B+" <= '0'; "+C+" <= '0'; "+D+" <= '1';\n"+
       "      when 10 => "+A+" <= '1'; "+B+" <= '0'; "+C+" <= '1'; "+D+" <= '0';\n"+
       "      when 11 => "+A+" <= '1'; "+B+" <= '0'; "+C+" <= '1'; "+D+" <= '1';\n"+
       "      when 12 => "+A+" <= '1'; "+B+" <= '1'; "+C+" <= '0'; "+D+" <= '0';\n"+
       "      when 13 => "+A+" <= '1'; "+B+" <= '1'; "+C+" <= '0'; "+D+" <= '1';\n"+
       "      when 14 => "+A+" <= '1'; "+B+" <= '1'; "+C+" <= '1'; "+D+" <= '0';\n"+
       "      when 15 => "+A+" <= '1'; "+B+" <= '1'; "+C+" <= '1'; "+D+" <= '1';\n"+
       "      when others => null;\n"
       "    end case;\n";
  s3 = "    wait for 1 ns;\n"
       "  end process;\n";
  s = s1+s2+s3;
  return s;
}

QString pad4bit::verilogCode( int )
{
  QString v = Props.at(0)->Value;

  QString l = "";
  QString l1, l2, l3;

  QString A   = Ports.at(0)->Connection->Name;
  QString B   = Ports.at(1)->Connection->Name;
  QString C   = Ports.at(2)->Connection->Name;
  QString D   = Ports.at(3)->Connection->Name;

  QString AR  = "A_reg"  + Name + A;
  QString BR  = "B_reg"  + Name + B;
  QString CR  = "C_reg"  + Name + C;
  QString DR  = "C_reg"  + Name + D;
  
  l1 = "\n  // "+Name+" 4bit pattern generator\n"+
       "  assign  "+A+" = "+AR+";\n"+
       "  reg     "+AR+" = 0;\n"+
       "  assign  "+B+" = "+BR+";\n"+
       "  reg     "+BR+" = 0;\n"+
       "  assign  "+C+" = "+CR+";\n"+
       "  reg     "+CR+" = 0;\n"+
       "  assign  "+D+" = "+DR+";\n"+
       "  reg     "+DR+" = 0;\n"+
       "  initial\n";
  l2 = "  begin\n"
       "    case ("+v+")\n"+
       "       0 : begin "+AR+" = 0; "+BR+" = 0; "+CR+" = 0; "+DR+" = 0; end\n"+
       "       1 : begin "+AR+" = 0; "+BR+" = 0; "+CR+" = 0; "+DR+" = 1; end\n"+
       "       2 : begin "+AR+" = 0; "+BR+" = 0; "+CR+" = 1; "+DR+" = 0; end\n"+
       "       3 : begin "+AR+" = 0; "+BR+" = 0; "+CR+" = 1; "+DR+" = 1; end\n"+
       "       4 : begin "+AR+" = 0; "+BR+" = 1; "+CR+" = 0; "+DR+" = 0; end\n"+
       "       5 : begin "+AR+" = 0; "+BR+" = 1; "+CR+" = 0; "+DR+" = 1; end\n"+
       "       6 : begin "+AR+" = 0; "+BR+" = 1; "+CR+" = 1; "+DR+" = 0; end\n"+
       "       7 : begin "+AR+" = 0; "+BR+" = 1; "+CR+" = 1; "+DR+" = 1; end\n"+
       "       8 : begin "+AR+" = 1; "+BR+" = 0; "+CR+" = 0; "+DR+" = 0; end\n"+
       "       9 : begin "+AR+" = 1; "+BR+" = 0; "+CR+" = 0; "+DR+" = 1; end\n"+
       "      10 : begin "+AR+" = 1; "+BR+" = 0; "+CR+" = 1; "+DR+" = 0; end\n"+
       "      11 : begin "+AR+" = 1; "+BR+" = 0; "+CR+" = 1; "+DR+" = 1; end\n"+
       "      12 : begin "+AR+" = 1; "+BR+" = 1; "+CR+" = 0; "+DR+" = 0; end\n"+
       "      13 : begin "+AR+" = 1; "+BR+" = 1; "+CR+" = 0; "+DR+" = 1; end\n"+
       "      14 : begin "+AR+" = 1; "+BR+" = 1; "+CR+" = 1; "+DR+" = 0; end\n"+
       "      15 : begin "+AR+" = 1; "+BR+" = 1; "+CR+" = 1; "+DR+" = 1; end\n"+
       "    endcase\n";
  l3 = "  end\n";
  l = l1+l2+l3;
  return l;
}
