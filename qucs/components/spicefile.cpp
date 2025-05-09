/***************************************************************************
                              spicefile.cpp
                             ---------------
    begin                : Tue Dez 28 2004
    copyright            : (C) 2004 by Michael Margraf
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

#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <QRegularExpression>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QDebug>
#include <QStatusBar>

#include "spicefile.h"
#include "main.h"
#include "node.h"
#include "qucs.h"
#include "misc.h"
#include "extsimkernels/spicecompat.h"


SpiceFile::SpiceFile()
{
  Description = QObject::tr("SPICE netlist file");
  // Property descriptions not needed, but must not be empty !
  Props.append(new Property("File", "", true, QStringLiteral("x")));
  Props.append(new Property("Ports", "", false, QStringLiteral("x")));
  Props.append(new Property("Sim", "yes", false, QStringLiteral("x")));
  Props.append(new Property("Preprocessor", "none", false, QStringLiteral("x")));
  Props.append(new Property("Params", "", false, QStringLiteral("x")));
  withSim = false;

  Model = "SPICE";
  SpiceModel = "X";
  Name  = "X";
  changed = false;

  // Do NOT call createSymbol() here. But create port to let it rotate.
  Ports.append(new Port(0, 0));
}

// -------------------------------------------------------
Component* SpiceFile::newOne()
{
  SpiceFile *p = new SpiceFile();
  p->recreate();   // createSymbol() is NOT called in constructor !!!
  return p;
}

// -------------------------------------------------------
Element* SpiceFile::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("SPICE netlist");
  BitmapFile = (char *) "spicefile";

  if(getNewOne) {
    SpiceFile *p = new SpiceFile();
    p->recreate();   // createSymbol() is NOT called in constructor !!!
    return p;
  }
  return 0;
}

// -------------------------------------------------------
void SpiceFile::createSymbol()
{
  QFont f = QucsSettings.font; // get the basic font
  // symbol text is smaller (10 pt default)
  f.setPointSize(10);
  // use the screen-compatible metric
  QFontMetrics  smallmetrics(f, 0);   // get size of text
  int fHeight = smallmetrics.lineSpacing();

  int No = 0;
  QString tmp, PortNames = Props.at(1)->Value;
  if(!PortNames.isEmpty())  No = PortNames.count(',') + 1;
  
  // draw symbol outline
  #define HALFWIDTH  17
  int h = 30*((No-1)/2) + 15;
  Lines.append(new qucs::Line(-HALFWIDTH, -h, HALFWIDTH, -h,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line( HALFWIDTH, -h, HALFWIDTH,  h,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(-HALFWIDTH,  h, HALFWIDTH,  h,QPen(Qt::darkBlue,2)));
  Lines.append(new qucs::Line(-HALFWIDTH, -h,-HALFWIDTH,  h,QPen(Qt::darkBlue,2)));

  int w, i = fHeight/2;
  if(withSim) {
    i = fHeight - 2;
    tmp = QObject::tr("sim");
    w = smallmetrics.boundingRect(tmp).width();
    Texts.append(new Text(w/-2, 0, tmp, Qt::red));
  }
  tmp = QObject::tr("spice");
  w = smallmetrics.boundingRect(tmp).width();
  Texts.append(new Text(w/-2, -i, tmp));

  i = 0;
  int y = 15-h;
  while(i<No) { // add ports lines and numbers
    Lines.append(new qucs::Line(-30,  y,-HALFWIDTH,  y,QPen(Qt::darkBlue,2)));
    Ports.append(new Port(-30,  y));
    tmp = PortNames.section(',', i, i).mid(4);
    w = smallmetrics.boundingRect(tmp).width();
    Texts.append(new Text(-20-w, y-fHeight-2, tmp)); // text right-aligned
    i++;

    if(i == No) break; // if odd number of ports there will be one port less on the right side
    Lines.append(new qucs::Line(HALFWIDTH,  y, 30,  y,QPen(Qt::darkBlue,2)));
    Ports.append(new Port( 30,  y));
    tmp = PortNames.section(',', i, i).mid(4);
    Texts.append(new Text( 20, y-fHeight-2, tmp)); // text left-aligned
    y += 60;
    i++;
  }

  if(No > 0) {
    Lines.append(new qucs::Line( 0, h, 0,h+15,QPen(Qt::darkBlue,2)));
    Texts.append(new Text( 4, h,"Ref"));
    Ports.append(new Port( 0, h+15));    // 'Ref' port
  }

  x1 = -30; y1 = -h-2;
  x2 =  30; y2 =  h+15;

  // compute component name text position - normal size font
  QFontMetrics  metrics(QucsSettings.font, 0);   // use the screen-compatible metric
  fHeight = metrics.lineSpacing();
  tx = x1+4;
  ty = y1 - fHeight - 4;
  if(Props.first()->display) ty -= fHeight;
  changed = true;
}

// ---------------------------------------------------
QString SpiceFile::netlist()
{
  if(Props.at(1)->Value.isEmpty())
    return QString();  // no ports, no subcircuit instance

  QString s = "Sub:"+Name;   // SPICE netlist is subcircuit
  for (Port *pp : Ports)
    s += " "+pp->Connection->Name;   // output all node names

  QString f = misc::properFileName(Props.first()->Value);
  s += " Type=\""+misc::properName(f)+"\"\n";
  return s;
}

// -------------------------------------------------------
QString SpiceFile::getSubcircuitFile()
{
  return misc::properAbsFileName(Props.front()->Value, containingSchematic);
}

// -------------------------------------------------------------------------
bool SpiceFile::createSubNetlist(QTextStream *stream)
{
  // check file name
  QString FileName = Props.first()->Value;
  if(FileName.isEmpty()) {
    ErrText += QObject::tr("ERROR: No file name in SPICE component \"%1\".").
      arg(Name);
    return false;
  }

  // check input and output file
  QFile SpiceFile, ConvFile;
  FileName = getSubcircuitFile();
  SpiceFile.setFileName(FileName);
  if(!SpiceFile.open(QIODevice::ReadOnly)) {
    ErrText += QObject::tr("ERROR: Cannot open SPICE file \"%1\".").
      arg(FileName);
    return false;
  }
  SpiceFile.close();
  QString ConvName = SpiceFile.fileName() + ".lst";
  ConvFile.setFileName(ConvName);
  QFileInfo Info(ConvName);

  // re-create converted file if necessary
  if(changed || !ConvFile.exists() ||
     (lastLoaded.isValid() && lastLoaded < Info.lastModified())) {
    if(!ConvFile.open(QIODevice::WriteOnly)) {
      ErrText += QObject::tr("ERROR: Cannot save converted SPICE file \"%1\".").
                          arg(FileName + ".lst");
      return false;
    }
    outstream = stream;
    filstream = new QTextStream(&ConvFile);
    QString SpiceName = SpiceFile.fileName();
    bool ret = recreateSubNetlist(&SpiceName, &FileName);
    ConvFile.close();
    delete filstream;
    return ret;
  }

  // load old file and stuff into stream
  if(!ConvFile.open(QIODevice::ReadOnly)) {
    ErrText += QObject::tr("ERROR: Cannot open converted SPICE file \"%1\".").
                        arg(FileName + ".lst");
    return false;
  }
  QByteArray FileContent = ConvFile.readAll();
  ConvFile.close();
  //? stream->writeRawBytes(FileContent.data(), FileContent.size());
  (*stream) << FileContent.data();
  return true;
}


bool SpiceFile::createSpiceSubckt(QTextStream *stream)
{
    (*stream)<<"\n";
    QFile sub_file(getSubcircuitFile());
    if (sub_file.open(QIODevice::ReadOnly|QFile::Text)) {
        QTextStream ts(&sub_file);
        (*stream)<<ts.readAll().remove(QChar(0x1A));
        sub_file.close();
    }
    (*stream)<<"\n";
    return true;
}

// -------------------------------------------------------------------------
bool SpiceFile::recreateSubNetlist(QString *SpiceFile, QString *FileName)
{
  // initialize collectors
  ErrText = "";
  NetText = "";
  SimText = "";
  NetLine = "";

  // evaluate properties
  if(Props.at(1)->Value != "")
    makeSubcircuit = true;
  else
    makeSubcircuit = false;
  if(Props.at(2)->Value == "yes")
    insertSim = true;
  else
    insertSim = false;

  // preprocessor run if necessary
  QString preprocessor = Props.at(3)->Value;
  if (preprocessor != "none") {
    bool piping = true;
    QStringList script;
#if defined(_WIN32) || defined(__MINGW32__)
    QString interpreter = "tinyperl.exe";
#else
    QString interpreter = "perl";
#endif
    if (preprocessor == "ps2sp") {
      script << "ps2sp";
    } else if (preprocessor == "spicepp") {
      script << "spicepp.pl";
    } else if (preprocessor == "spiceprm") {
      script << "spiceprm";
      piping = false;
    }
    SpicePrep = new QProcess(this);
    script << interpreter;
    script << script;
    script << *SpiceFile;

    QFile PrepFile;
    QString PrepName = *SpiceFile + ".pre";

    if (!piping) {
      script << PrepName;
      connect(SpicePrep, SIGNAL(readyReadStandardOutput()), SLOT(slotSkipOut()));
      connect(SpicePrep, SIGNAL(readyReadStandardError()), SLOT(slotGetPrepErr()));
    } else {
      connect(SpicePrep, SIGNAL(readyReadStandardOutput()), SLOT(slotGetPrepOut()));
      connect(SpicePrep, SIGNAL(readyReadStandardError()), SLOT(slotGetPrepErr()));
    }

    QMessageBox *MBox = 
      new QMessageBox(QMessageBox::NoIcon,
                      QObject::tr("Info"),
                      QObject::tr("Preprocessing SPICE file \"%1\".").arg(*SpiceFile),
                      QMessageBox::Abort);
    MBox->setAttribute(Qt::WA_DeleteOnClose);
    connect(SpicePrep, SIGNAL(finished(int)), MBox, SLOT(close()));

    if (piping) {
      PrepFile.setFileName(PrepName);
      if(!PrepFile.open(QIODevice::WriteOnly)) {
        ErrText +=
          QObject::tr("ERROR: Cannot save preprocessed SPICE file \"%1\".").
          arg(PrepName);
        return false;
      }
      prestream = new QTextStream(&PrepFile);
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PATH", env.value("PATH") );
    SpicePrep->setProcessEnvironment(env);
    QString cmd = script.at(0);
    QStringList script_args = script;
    script_args.removeAt(0);
    SpicePrep->start(cmd,script_args);
    //QucsHelp->setCommunication(0);

    if(SpicePrep->state()!=QProcess::Running&&
            SpicePrep->state()!=QProcess::Starting) {
      ErrText += QObject::tr("ERROR: Cannot execute \"%1\".").
              arg(interpreter + " " + script.join(" ") + "\".");
      if (piping) {
        PrepFile.close();
        delete prestream;
      }
      return false;
    }
    //SpicePrep->closeStdin();

    MBox->exec();
    delete SpicePrep;
    if (piping) {
      PrepFile.close();
      delete prestream;
    }
    *SpiceFile = PrepName;
  }

  // begin command line construction
  QString prog;
  QStringList com;
  //prog =  QucsSettings.BinDir + "qucsconv"  + executableSuffix;
  prog =  QucsSettings.Qucsconv;

  if(makeSubcircuit) com << "-g" << "_ref";
  com << "-if" << "spice" << "-of" << "qucs";
  com << "-i" << *SpiceFile;

  // begin netlist text creation
  if(makeSubcircuit) {
    QString f = misc::properFileName(*FileName);
    NetText += "\n.Def:" + misc::properName(f) + " ";
    QString PortNames = Props.at(1)->Value;
    PortNames.replace(',', ' ');
    NetText += PortNames;
    if(makeSubcircuit) NetText += " _ref";
  }
  NetText += "\n";

  // startup SPICE conversion process
  QucsConv = new QProcess(this);
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert("PATH", env.value("PATH") );
  QucsConv->setProcessEnvironment(env);

  qDebug() << "SpiceFile::recreateSubNetlist :Command:" << prog << com.join(" ");
//  QucsConv->start(com.join(" "));
  QucsConv->start(prog, com);

  /// these slots might write into NetText, ErrText, outstream, filstream
  connect(QucsConv, SIGNAL(readyReadStandardOutput()), SLOT(slotGetNetlist()));
  connect(QucsConv, SIGNAL(readyReadStandardError()), SLOT(slotGetError()));
  connect(QucsConv, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(slotExited()));

  if(QucsConv->state()!=QProcess::Running&&
          QucsConv->state()!=QProcess::Starting) {
    ErrText += QObject::tr("COMP ERROR: Cannot start QucsConv!");
    return false;
  }
  (*outstream) << NetText;
  (*filstream) << NetText;

  // only interact with the GUI if it was launched
  if (QucsMain != nullptr) {
    QucsMain->statusBar()->showMessage(tr("Converting SPICE file \"%1\".").arg(*SpiceFile), 2000);
  }
  else
    qDebug() << QObject::tr("Converting SPICE file \"%1\".").arg(*SpiceFile);

  // finish
  QucsConv->waitForFinished();
  delete QucsConv;
  lastLoaded = QDateTime::currentDateTime();
  return true;
}

// -------------------------------------------------------------------------
void SpiceFile::slotSkipErr()
{
  SpicePrep->readAllStandardError();
}

// -------------------------------------------------------------------------
void SpiceFile::slotSkipOut()
{
    SpicePrep->readAllStandardOutput();
}

// -------------------------------------------------------------------------
void SpiceFile::slotGetPrepErr()
{
  ErrText += QString(SpicePrep->readAllStandardError());
}

// -------------------------------------------------------------------------
void SpiceFile::slotGetPrepOut()
{
  (*prestream) << QString(SpicePrep->readAllStandardOutput());
}

// -------------------------------------------------------------------------
void SpiceFile::slotGetError()
{
  ErrText += QString(QucsConv->readAllStandardError());
}

// -------------------------------------------------------------------------
void SpiceFile::slotGetNetlist()
{
  int i;
  QString s;
  NetLine += QString(QucsConv->readAllStandardOutput());

  while((i = NetLine.indexOf('\n')) >= 0) {
    s = NetLine.left(i);
    NetLine.remove(0, i+1);
    s = s.trimmed();
    if(s.size()>0&&s.at(0) == '#') {
      continue;
    } else if(s.isEmpty()) {
      continue;
    } else if(s.size()>0&&s.at(0) == '.') {
      if(s.left(5) != ".Def:") {
        if(insertSim) SimText += s + "\n";
        continue;
      }
    }
    if(makeSubcircuit) {
      (*outstream) << "  ";
      (*filstream) << "  ";
    }
    (*outstream) << s << "\n";
    (*filstream) << s << "\n";
  }
}

// -------------------------------------------------------------------------
void SpiceFile::slotExited()
{
  if (QucsConv->exitStatus() != QProcess::NormalExit) {
    NetText = "";
  }
  else {
    if(makeSubcircuit) {
      (*outstream) << ".Def:End\n\n";
      (*filstream) << ".Def:End\n\n";
    }
    if(!SimText.isEmpty()) {
      (*outstream) << SimText;
      (*filstream) << SimText;
    }
  }
}

QString SpiceFile::spice_netlist(spicecompat::SpiceDialect dialect /* = spicecompat::SPICEDefault */)
{
    Q_UNUSED(dialect);

    QStringList ports_lst = Props.at(1)->Value.split(",");
    for (auto & it : ports_lst) {
        if (it.startsWith("_net")) it.remove(0,4);
    }
    QStringList nod_lst;
    QString compname = spicecompat::getSubcktName(getSubcircuitFile());
    spicecompat::getPins(getSubcircuitFile(),compname,nod_lst);

    QList<int> seq;
    seq.clear();
    for(int i=0;i<nod_lst.count();i++) {
        int idx = ports_lst.indexOf(nod_lst.at(i));
        if (idx >= 0)
            seq.append(ports_lst.indexOf(nod_lst.at(i)));
    }

    QString s = spicecompat::check_refdes(Name,SpiceModel);
    //foreach(Port *p1, Ports) {
    for (int i : seq) {
        s += " "+Ports.at(i)->Connection->Name;   // node names
    }

    s += " " + spicecompat::getSubcktName(getSubcircuitFile());
    s += " " + getProperty("Params")->Value;
    s += "\n";
    return s;
}

QString SpiceFile::cdl_netlist()
{
    return spice_netlist(spicecompat::CDL);
}
