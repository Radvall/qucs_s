/***************************************************************************
                                jfet.cpp
                               ----------
    begin                : Fri Jun 4 2004
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

#include "jfet.h"
#include "node.h"
#include "extsimkernels/spicecompat.h"


JFET::JFET() {
    Description = QObject::tr("junction field-effect transistor");

    // this must be the first property in the list !!!
    Props.append(new Property("Type", "nfet", true,
                              QObject::tr("polarity") + " [nfet, pfet]"));
    Props.append(new Property("Vt0", "-2.0 V", true,
                              QObject::tr("threshold voltage")));
    Props.append(new Property("Beta", "1e-4", true,
                              QObject::tr("transconductance parameter")));
    Props.append(new Property("Lambda", "0.0", true,
                              QObject::tr("channel-length modulation parameter")));
    Props.append(new Property("Rd", "0.0", false,
                              QObject::tr("parasitic drain resistance")));
    Props.append(new Property("Rs", "0.0", false,
                              QObject::tr("parasitic source resistance")));
    Props.append(new Property("Is", "1e-14", false,
                              QObject::tr("gate-junction saturation current")));
    Props.append(new Property("N", "1.0", false,
                              QObject::tr("gate-junction emission coefficient")));
    Props.append(new Property("Isr", "1e-14", false,
                              QObject::tr("gate-junction recombination current parameter")));
    Props.append(new Property("Nr", "2.0", false,
                              QObject::tr("Isr emission coefficient")));
    Props.append(new Property("Cgs", "0.0", false,
                              QObject::tr("zero-bias gate-source junction capacitance")));
    Props.append(new Property("Cgd", "0.0", false,
                              QObject::tr("zero-bias gate-drain junction capacitance")));
    Props.append(new Property("Pb", "1.0", false,
                              QObject::tr("gate-junction potential")));
    Props.append(new Property("Fc", "0.5", false,
                              QObject::tr("forward-bias junction capacitance coefficient")));
    Props.append(new Property("M", "0.5", false,
                              QObject::tr("gate P-N grading coefficient")));
    Props.append(new Property("Kf", "0.0", false,
                              QObject::tr("flicker noise coefficient")));
    Props.append(new Property("Af", "1.0", false,
                              QObject::tr("flicker noise exponent")));
    Props.append(new Property("Ffe", "1.0", false,
                              QObject::tr("flicker noise frequency exponent")));
    Props.append(new Property("Temp", "26.85", false,
                              QObject::tr("simulation temperature in degree Celsius")));
    Props.append(new Property("Xti", "3.0", false,
                              QObject::tr("saturation current temperature exponent")));
    Props.append(new Property("Vt0tc", "0.0", false,
                              QObject::tr("Vt0 temperature coefficient")));
    Props.append(new Property("Betatce", "0.0", false,
                              QObject::tr("Beta exponential temperature coefficient")));
    Props.append(new Property("Tnom", "26.85", false,
                              QObject::tr("temperature at which parameters were extracted")));
    Props.append(new Property("Area", "1.0", false,
                              QObject::tr("default area for JFET")));
    Props.append(new Property("UseGlobTemp", "yes", false,
                              QObject::tr("Use global SPICE temperature")+" [yes,no]"));

    createSymbol();
    tx = x2 + 4;
    ty = y1 + 4;
    Model = "JFET";
    Name = "T";
    SpiceModel = "J";
    SpiceModelcards.append("NJF");
    SpiceModelcards.append("PJF");
}

// -------------------------------------------------------
Component* JFET::newOne()
{
  JFET* p = new JFET();
  p->Props.front()->Value = Props.front()->Value;
  p->recreate();
  return p;
}

QString JFET::spice_netlist(spicecompat::SpiceDialect dialect /* = spicecompat::SPICEDefault */)
{
    QString s = spicecompat::check_refdes(Name,SpiceModel);
    QList<int> pin_seq;
    pin_seq<<1<<0<<2; // Pin sequence: DGS
    // output all node names
    for (int pin : pin_seq) {
        QString nam = Ports.at(pin)->Connection->Name;
        if (nam=="gnd") nam = "0";
        s += " "+ nam;   // node names
    }


    QStringList spice_incompat,spice_tr;
    if (dialect == spicecompat::SPICEXyce) {
        spice_incompat<<"Type"<<"Area"<<"Temp"<<"Ffe"<<"N"
                     <<"Isr"<<"Nr"<<"M"<<"Xti"<<"Betatce"<<"Vt0tc"<<"UseGLobTemp";
                                  // spice-incompatible parameters
        spice_tr<<"Vt0"<<"VtO"; // parameters that need conversion of names
    } else {
        spice_incompat<<"Type"<<"Area"<<"Temp"<<"Ffe"<<"N"<<"Isr"<<"Nr"<<"M"<<"Xti"<<"Betatce"<<"UseGlobTemp";
                                  // spice-incompatible parameters
        spice_tr<<"Vt0tc"<<"Tcv"; // parameters that need conversion of names
    }


    QString par_str = form_spice_param_list(spice_incompat,spice_tr);

    QString jfet_type = getProperty("Type")->Value.at(0).toUpper();

    if (getProperty("UseGlobTemp")->Value == "yes") {
      s += QStringLiteral(" JMOD_%1 %2\n").arg(Name).arg(getProperty("Area")->Value);
    } else {
      s += QStringLiteral(" JMOD_%1 %2 TEMP=%3\n").arg(Name).arg(getProperty("Area")->Value)
      .arg(getProperty("Temp")->Value);
    }

    if (dialect != spicecompat::CDL)
    {
        s += QStringLiteral(".MODEL JMOD_%1 %2JF (%3)\n").arg(Name).arg(jfet_type).arg(par_str);
    }

    return s;
}

QString JFET::cdl_netlist()
{
    return spice_netlist(spicecompat::CDL);
}

// -------------------------------------------------------
Element* JFET::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("n-JFET");
  BitmapFile = (char *) "nfet";

  if(getNewOne)  return new JFET();
  return 0;
}

// -------------------------------------------------------
Element* JFET::info_p(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("p-JFET");
  BitmapFile = (char *) "pfet";

  if(getNewOne) {
    JFET* p = new JFET();
    p->Props.front()->Value = "pfet";
    p->recreate();
    return p;
  }
  return 0;
}

// -------------------------------------------------------
void JFET::createSymbol()
{
  Lines.append(new qucs::Line(-10,-15,-10, 15,QPen(Qt::darkBlue,3)));
  Lines.append(new qucs::Line(-30,  0,-10,  0,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(-10,-10,  0,-10,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(  0,-10,  0,-30,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(-10, 10,  0, 10,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(  0, 10,  0, 30,QPen(Qt::darkBlue,2)));

  Lines.append(new qucs::Line( -4, 24,  4, 20,QPen(Qt::darkBlue,2)));

  if(Props.front()->Value == "nfet") {
    Lines.append(new qucs::Line(-16, -5,-11,  0,QPen(Qt::darkBlue,2)));
    Lines.append(new qucs::Line(-16,  5,-11,  0,QPen(Qt::darkBlue,2)));
  }
  else {
    Lines.append(new qucs::Line(-18,  0,-13, -5,QPen(Qt::darkBlue,2)));
    Lines.append(new qucs::Line(-18,  0,-13,  5,QPen(Qt::darkBlue,2)));
  }

  Ports.append(new Port(-30,  0));
  Ports.append(new Port(  0,-30));
  Ports.append(new Port(  0, 30));

  x1 = -30; y1 = -30;
  x2 =   4; y2 =  30;
}
