/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Thu Aug 28 18:17:41 CEST 2003
    copyright            : (C) 2005 by Michael Margraf
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <QApplication>
#include <QString>
#include <QTranslator>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDir>
#include <QFont>
#include <QSettings>

#include "qucsfilter.h"
#include "../qucs/extsimkernels/spicecompat.h"

struct tQucsSettings QucsSettings;



// #########################################################################
// Loads the settings file and stores the settings.
bool loadSettings()
{
    QSettings settings("qucs","qucs_s");
    settings.beginGroup("QucsFilter");
    if(settings.contains("x"))QucsSettings.x=settings.value("x").toInt();
    if(settings.contains("y"))QucsSettings.y=settings.value("y").toInt();
    settings.endGroup();
    if(settings.contains("Language"))QucsSettings.Language=settings.value("Language").toString();
    if(settings.contains("DefaultSimulator"))
        QucsSettings.DefaultSimulator = settings.value("DefaultSimulator").toInt();
    else QucsSettings.DefaultSimulator = spicecompat::simNotSpecified;

  return true;
}


// #########################################################################
// Saves the settings in the settings file.
bool saveApplSettings(QucsFilter *qucs)
{
    QSettings settings ("qucs","qucs_s");
    settings.beginGroup("QucsFilter");
    settings.setValue("x", qucs->x());
    settings.setValue("y", qucs->y());
    settings.endGroup();
  return true;

}



// #########################################################################
// ##########                                                     ##########
// ##########                  Program Start                      ##########
// ##########                                                     ##########
// #########################################################################

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);

  // apply default settings
  QucsSettings.x = 200;
  QucsSettings.y = 100;

  // is application relocated?
  QDir QucsDir;
  QString QucsApplicationPath = QCoreApplication::applicationDirPath();
#ifdef __APPLE__
  QucsDir = QDir(QucsApplicationPath.section("/bin",0,0));
#else
  QucsDir = QDir(QucsApplicationPath);
  QucsDir.cdUp();
#endif
  QucsSettings.LangDir = QucsDir.canonicalPath() + "/share/" QUCS_NAME "/lang/";

  loadSettings();

  QTranslator tor( 0 );
  QString lang = QucsSettings.Language;
  if(lang.isEmpty())
    lang = QString(QLocale::system().name());
  static_cast<void>(tor.load( QStringLiteral("qucs_") + lang, QucsSettings.LangDir));
  a.installTranslator( &tor );

  QucsFilter *qucs = new QucsFilter();
  qucs->raise();
  qucs->move(QucsSettings.x, QucsSettings.y);  // position before "show" !!!
  qucs->show();
  int result = a.exec();
  saveApplSettings(qucs);
  return result;
}
