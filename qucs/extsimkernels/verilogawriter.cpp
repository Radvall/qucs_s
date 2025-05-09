/*
 * verilogawriter.cpp - Subcircuit to Verilog-A module converter implementation
 *
 * Copyright (C) 2015, Vadim Kuznetsov, ra3xdh@gmail.com
 *
 * This file is part of Qucs
 *
 * Qucs is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Qucs.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include "verilogawriter.h"
#include <QPlainTextEdit>
#include "paintings/id_text.h"
#include "component.h"
#include "node.h"
#include "schematic.h"

/*!
  \file verilogawriter.cpp
  \brief Implementation of the VerilogAwriter class and vacompat namespace.
*/


/*!
 * \brief vacompat::convert_functions convert Qucs mathematical function or constant to
 *        Verilog-A equivalent
 * \param tokens[in/out] QStringList contains equation tokens
 */
void vacompat::convert_functions(QStringList &tokens)
{
    QStringList conv_list; // Put here functions need to be converted
    conv_list<<"q"<<"`P_Q"
            <<"kB"<<"`P_K"
            <<"pi"<<"`M_PI";

    for(QStringList::iterator it = tokens.begin();it != tokens.end(); it++) {
        for(int i=0;i<conv_list.count();i+=2) {
            if (conv_list.at(i)==(*it))
                (*it) = conv_list.at(i+1);
        }
    }

}

QString vacompat::normalize_voltage(QString &plus, QString &minus, bool left_side)
{
    QString s;
    if (plus=="gnd") {
        if (left_side) s = QStringLiteral("V(%1)").arg(minus);
        else s = QStringLiteral("(-V(%1))").arg(minus);
    } else if (minus=="gnd") s = QStringLiteral("V(%1)").arg(plus);
    else s = QStringLiteral("V(%1,%2)").arg(plus).arg(minus);
    return s;
}

QString vacompat::normalize_current(QString &plus, QString &minus, bool left_side)
{
    QString s;
    if (plus=="gnd") {
        if (left_side) s = QStringLiteral("I(%1)").arg(minus);
       else s = QStringLiteral("(-I(%1))").arg(minus);
    } else if (minus=="gnd") s = QStringLiteral("I(%1)").arg(plus);
    else s = QStringLiteral("I(%1,%2)").arg(plus).arg(minus);
    return s;
}


/*!
 * \brief vacompat::normalize_value Convert value from Qucs or SPICE notation
 *        to Verilog-A notation
 * \param Value[in] Componrt value to convert
 * \return Converted value
 */
QString vacompat::normalize_value(QString Value)
{
    QRegularExpression r_pattern("^[0-9]+.*Ohm$");
    QRegularExpression c_pattern("^[0-9]+.*F$");
    QRegularExpression l_pattern("^[0-9]+.*H$");
    QRegularExpression v_pattern("^[0-9]+.*V$");
    QRegularExpression hz_pattern("^[0-9]+.*Hz$");
    QRegularExpression s_pattern("^[0-9]+.*S$");

    QString s = Value.remove(' ');
    if (s.startsWith('\'')&&s.endsWith('\'')) return Value.remove('\''); // Expression detected

    if (r_pattern.match(s).hasMatch()) { // Component value
        s.remove("Ohm");
    } else if (c_pattern.match(s).hasMatch()) {
        s.remove("F");
    } else if (l_pattern.match(s).hasMatch()) {
        s.remove("H");
    } else if (v_pattern.match(s).hasMatch()) {
        s.remove("V");
    } else if (hz_pattern.match(s).hasMatch()) {
        s.remove("Hz");
    } else if (s_pattern.match(s).hasMatch()) {
        s.remove("S");
    }

    return s;
}


VerilogAwriter::VerilogAwriter()
{
}

VerilogAwriter::~VerilogAwriter()
{

}

/*!
 * \brief VerilogAwriter::prepareToVerilogA Prepare scheamtic for Verilog-A
 *        module building. Schematic must be subcircuit.
 * \param sch[in] Schematic pointer
 * \return true if schematic is subcircuit; false otherwise.
 */
bool VerilogAwriter::prepareToVerilogA(Schematic *sch)
{
    QStringList collect;
    QPlainTextEdit *err = new QPlainTextEdit;
    QTextStream stream;
    if (sch->prepareNetlist(stream,collect,err)==-10) { // Broken netlist
        delete err;
        return false;
    }
    delete err;
    sch->clearSignalsAndFileList(); // for proper build of subckts
    return true;
}

/*!
 * \brief VerilogAwriter::createVA_module Build Verilog-A module from the subcircuit.
 * \param stream[out] QTextStream where Verilog-A module should be written.
 * \param sch[in] Schematic that should be converted to Verilog-A module
 * \return true on success; false otherwise
 */
bool VerilogAwriter::createVA_module(QTextStream &stream, Schematic *sch)
{
    prepareToVerilogA(sch);

    QString s;
    stream<<"`include \"disciplines.vams\"\n";
    stream<<"`include \"constants.vams\"\n";

    QStringList ports;
    QStringList nodes;
    ports.clear();
    nodes.clear();

    for (Component* pc : sch->a_DocComps) {
        if (pc->Model=="Port") { // Find module ports
            QString s = pc->Ports.first()->Connection->Name;
            if (!ports.contains(s)) ports.append(s);
        } else {
            for (Port *pp : pc->Ports) { // Find all signals
                QString s = pp->Connection->Name;
                if (!nodes.contains(s)) nodes.append(s);
            }
            pc->getExtraVANodes(nodes);
        }
    }

    if (ports.isEmpty()) return false; // Not a subcircuit

    QFileInfo inf(sch->getDocName());
    QString base = inf.completeBaseName();
    base.remove('-').remove(' ');
    nodes.removeAll("gnd"); // Exclude ground node

    stream<<QStringLiteral("module %1(%2);\n").arg(base).arg(ports.join(", "));
    stream<<QStringLiteral("inout %1;\n").arg(ports.join(", "));
    stream<<QStringLiteral("electrical %1;\n").arg(nodes.join(", "));

    for (Painting* pi : sch->a_SymbolPaints)
      if(pi->Name == ".ID ") {
        ID_Text *pid = (ID_Text*)pi;
        for(const auto& sub_param : pid->subParameters) {
            QString s = "parameter real " + sub_param->name + ";\n";
            stream<<s;
        }
        break;
      }


    // List all variables
    for( Component* pc : sch->a_DocComps) {
        if (pc->isEquation && pc->isActive) {
            stream<<pc->getVAvariables();
        }
    }

    stream<<"analog begin \n"
            "@(initial_model)\n"
            "begin \n";
    // Output expressions
    for (Component* pc : sch->a_DocComps) {
        if (pc->isEquation && pc->isActive) {
            stream<<pc->getVAExpressions();
        }
    }

    stream<<"end\n";

    // Convert components to current equations.
    for (Component* pc : sch->a_DocComps) {
         stream<<pc->getVerilogACode();
    }

    stream<<"end\n"
            "endmodule\n";
    return true;
}
