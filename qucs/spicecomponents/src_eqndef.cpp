#include "src_eqndef.h"
#include "extsimkernels/spicecompat.h"
#include "extsimkernels/verilogawriter.h"
#include "node.h"


Src_eqndef::Src_eqndef()
{
  Description = QObject::tr("SPICE B (V type):\nMultiple line ngspice or Xyce B specifications allowed using \"+\" continuation lines.\nLeave continuation lines blank when NOT in use.  ");
  Simulator = spicecompat::simSpice;

  Ellipses.append(new qucs::Ellips(-14,-14, 28, 28, QPen(Qt::darkRed,3)));
  Texts.append(new Text(8,-10,"Eqn",Qt::darkRed,12.0,0.0,-1.0));
  // pins
  Lines.append(new qucs::Line(-30,  0,-14,  0,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line( 30,  0, 14,  0,QPen(Qt::darkBlue,2)));
  // plus sign
  Lines.append(new qucs::Line( 18,  5, 18, 11,QPen(Qt::red,2)));
  Lines.append(new qucs::Line( 21,  8, 15,  8,QPen(Qt::red,2)));
  // minus sign
  Lines.append(new qucs::Line(-18,  5,-18, 11,QPen(Qt::black,2)));

  Ports.append(new Port( 30,  0));
  Ports.append(new Port(-30,  0));

  x1 = -30; y1 = -14;
  x2 =  30; y2 =  14;

  tx = x1+4;
  ty = y2+4;
  Model = "src_eqndef";
  SpiceModel = "B";
  Name  = "B";

  Props.append(new Property("V", "", true,"B(V) specification"));
  Props.append(new Property("Line_2", "", false,"+ continuation line 1"));
  Props.append(new Property("Line_3", "", false,"+ continuation line 2"));
  Props.append(new Property("Line_4", "", false,"+ continuation line 3"));
  Props.append(new Property("Line_5", "", false,"+ continuation line 4"));



// Props.append(new Property("V", "0", true,"Expression"));

  rotate();  // fix historical flaw
}

Src_eqndef::~Src_eqndef()
{
}

Component* Src_eqndef::newOne()
{
  return new Src_eqndef();
}

Element* Src_eqndef::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("B source (V)");
  BitmapFile = (char *) "src_eqndef";

  if(getNewOne)  return new Src_eqndef();
  return 0;
}

QString Src_eqndef::netlist()
{
    return QString();
}

QString Src_eqndef::spice_netlist(spicecompat::SpiceDialect dialect /* = spicecompat::SPICEDefault */)
{
    Q_UNUSED(dialect);

    QString s = spicecompat::check_refdes(Name,SpiceModel);
    for (Port *p1 : Ports) {
        QString nam = p1->Connection->Name;
        if (nam=="gnd") nam = "0";
        s += " "+ nam   + " ";   // node names
    }

    QString VI  = Props.at(0)-> Name;
    QString VI2 = Props.at(0)->Value;
    QString Line_2 = Props.at(1)->Value;
    QString Line_3 = Props.at(2)->Value;
    QString Line_4 = Props.at(3)->Value;
    QString Line_5 = Props.at(4)->Value;

    s += QStringLiteral(" %1 = %2 ").arg(VI).arg(VI2);
    if(  Line_2.length() > 0 )   s += QStringLiteral("\n%1").arg(Line_2);
    if(  Line_3.length() > 0 )   s += QStringLiteral("\n%1").arg(Line_3);
    if(  Line_4.length() > 0 )   s += QStringLiteral("\n%1").arg(Line_4);
    if(  Line_5.length() > 0 )   s += QStringLiteral("\n%1").arg(Line_5);
    s += "\n";

    return s;
}

QString Src_eqndef::va_code()
{
    QString s;
    QString plus = Ports.at(0)->Connection->Name;
    QString minus = Ports.at(1)->Connection->Name;
    QString Src;
    if (Props.at(0)->Name=="I") Src = vacompat::normalize_current(plus,minus,true);
    else Src = vacompat::normalize_voltage(plus,minus); // Voltage contribution is reserved for future
    // B-source may be polar
    if (plus=="gnd") s = QStringLiteral(" %1 <+ -(%2); // %3 source\n").arg(Src).arg(Props.at(0)->Value).arg(Name);
    else s = QStringLiteral(" %1 <+ %2; // %3 source\n").arg(Src).arg(Props.at(0)->Value).arg(Name);
    return s;
}
