/***************************************************************************
                                sp_sim.cpp
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
#include "sp_sim.h"
#include "misc.h"
#include "schematic.h"
#include "settings.h"


SP_Sim::SP_Sim()
{
  isSimulation = true;
  Description = QObject::tr("S parameter simulation");
  initSymbol(Description);
  Model = ".SP";
  Name  = "SP";
  SpiceModel = ".SP";

  // The index of the first 4 properties must not changed. Used in recreate().
  Props.append(new Property("Type", "lin", true,
    QObject::tr("sweep type")+" [lin, log, list, const]"));
  Props.append(new Property("Start", "1 MHz", true,
    QObject::tr("start frequency in Hertz")));
  Props.append(new Property("Stop", "100 MHz", true,
    QObject::tr("stop frequency in Hertz")));
  Props.append(new Property("Points", "200", true,
    QObject::tr("number of simulation steps")));
  Props.append(new Property("Noise", "no", false,
    QObject::tr("calculate noise parameters")+
    " [yes, no]"));
  Props.append(new Property("NoiseIP", "1", false,
    QObject::tr("input port for noise figure")));
  Props.append(new Property("NoiseOP", "2", false,
    QObject::tr("output port for noise figure")));
  Props.append(new Property("saveCVs", "no", false,
    QObject::tr("put characteristic values into dataset")+
    " [yes, no]"));
  Props.append(new Property("saveAll", "no", false,
    QObject::tr("save subcircuit characteristic values into dataset")+
    " [yes, no]"));
}

SP_Sim::~SP_Sim()
{
}

Component* SP_Sim::newOne()
{
  return new SP_Sim();
}

Element* SP_Sim::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("S-parameter simulation");
  BitmapFile = (char *) "sparameter";

  if(getNewOne)  return new SP_Sim();
  return 0;
}

void SP_Sim::recreate()
{
  if((Props.at(0)->Value == "list") || (Props.at(0)->Value == "const")) {
    // Call them "Symbol" to omit them in the netlist.
    Props.at(1)->Name = "Symbol";
    Props.at(1)->display = false;
    Props.at(2)->Name = "Symbol";
    Props.at(2)->display = false;
    Props.at(3)->Name = "Values";
  }
  else {
    Props.at(1)->Name = "Start";
    Props.at(2)->Name = "Stop";
    Props.at(3)->Name = "Points";
  }
}

int SP_Sim::getSPortsNumber()
{
    int p_num = 0;
    if (containingSchematic != NULL) {
        for (Component *pc : containingSchematic->a_DocComps) {
            if (pc->Model == "Pac") p_num++;
        }
        return  p_num;
    } else {
        return p_num;
    }
}

QStringList SP_Sim::getExtraVariables()
{
    switch (_settings::Get().item<int>("DefaultSimulator")) {
        case spicecompat::simNgspice:
            return getNgspiceExtraVariables();
        case spicecompat::simXyce:
            return getXyceExtraVariables();
        default:
            return QStringList();
    }
}

QStringList SP_Sim::getNgspiceExtraVariables()
{
    QStringList vars;
    bool donoise = false;
    if (getProperty("Noise")->Value == "yes") donoise = true;
    int port_number = getSPortsNumber();
    for (int i = 0; i < port_number; i++) {
        for (int j = 0; j < port_number; j++) {
            QString tail = QStringLiteral("_%1_%2").arg(i+1).arg(j+1);
            vars.append(QStringLiteral("S%1").arg(tail));
            vars.append(QStringLiteral("Y%1").arg(tail));
            vars.append(QStringLiteral("Z%1").arg(tail));
            if (donoise) {
                vars.append(QStringLiteral("Cy%1").arg(tail));
            }
        }
    }
    if (port_number == 2 && donoise) {
        vars.append("Rn");
        vars.append("NF");
        vars.append("SOpt");
        vars.append("NFmin");
    }
    return vars;
}

QStringList SP_Sim::getXyceExtraVariables()
{
    QStringList vars;
    int ports_num = getSPortsNumber();
    for (int i = 0; i < ports_num; i++) {
        for (int j = 0; j < ports_num; j++) {
            QString tail = QStringLiteral("(%1,%2)").arg(i+1).arg(j+1);
            vars.append(QStringLiteral("sdb%1").arg(tail));
            vars.append(QStringLiteral("s%1").arg(tail));
            vars.append(QStringLiteral("sp%1").arg(tail));
            vars.append(QStringLiteral("y%1").arg(tail));
            vars.append(QStringLiteral("z%1").arg(tail));
        }
    }
    return vars;
}

QString SP_Sim::getSweepString()
{
    QString s;
    QString unit;
    if (Props.at(0)->Value=="log") { // convert points number for spice compatibility
        double Np,Fstart,Fstop,fac = 1.0;
        misc::str2num(Props.at(3)->Value,Np,unit,fac); // Points number
        Np *= fac;
        misc::str2num(Props.at(1)->Value,Fstart,unit,fac);
        Fstart *= fac;
        misc::str2num(Props.at(2)->Value,Fstop,unit,fac);
        Fstop *= fac;
        double Nd = ceil(log10(Fstop/Fstart)); // number of decades
        double Npd = ceil((Np - 1)/Nd); // points per decade
        s += QStringLiteral("DEC %1 ").arg(Npd);
    } else {  // no need conversion
        s += QStringLiteral("LIN %1 ").arg(Props.at(3)->Value);
    }
    QString fstart = spicecompat::normalize_value(Props.at(1)->Value); // Start freq.
    QString fstop = spicecompat::normalize_value(Props.at(2)->Value); // Stop freq.
    s += QStringLiteral("%1 %2").arg(fstart).arg(fstop);
    return s;
}

QString SP_Sim::ngspice_netlist()
{
    QString s = "SP ";
    s += getSweepString();
    if (getProperty("Noise")->Value == "yes") s += " 1";
    s += "\n";
    return s;
}

QString SP_Sim::xyce_netlist()
{
    QString s = ".AC ";
    s += getSweepString();
    s += "\n.LIN format=touchstone sparcalc=1\n"; // enable s-param
    /*int ports_num = getSPortsNumber();
    s += ".PRINT ac format=std file=spice4qucs_sparam.prn ";
    for (int i = 0; i < ports_num; i++) {
        for (int j = 0; j < ports_num; j++) {
            s += QStringLiteral(" sdb(%1,%2) s(%1,%2) sp(%1,%2) y(%1,%2) z(%1,%2) ").arg(i+1).arg(j+1);
        }
    }
    s += "\n";*/
    return s;
}

QString SP_Sim::spice_netlist(spicecompat::SpiceDialect dialect /* = spicecompat::SPICEDefault */)
{
    if (dialect == spicecompat::SPICEXyce) {
        return xyce_netlist();
    } else {
        return ngspice_netlist();
    }
}
