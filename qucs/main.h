/***************************************************************************
                                  main.h
                                 --------
    begin                : Mon May 24  2004
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
/*!
 * \file main.h
 * \brief Definitions and declarations for the main application.
 */

#ifndef QUCS_MAIN_H
#define QUCS_MAIN_H

#include <QFont>
#include <QColor>
#include <QStringList>
#include <QDir>


class QucsApp;
class Component;
class VersionTriplet;

static const double pi = 3.1415926535897932384626433832795029;  /* pi   */

struct tQucsSettings {
  int DefaultSimulator;

  QFont font;
  QFont appFont;
  QFont textFont;
  QFont sysDefaultFont;
  float largeFontSize;
  QColor BGColor;      // background color of view area
  QString Language;

  // syntax highlighting
  QColor Comment, String, Integer, Real, Character, Type,
    Attribute, Directive, Task;

  unsigned int maxUndo;    // size of undo stack
  QString Editor;
  QString Qucsator;
  QString QucsatorDir;
  QString QucsatorVar;
  QString Qucsconv;
  QString BinDir;
  QString LangDir;
  QString LibDir;
  QString SpiceLibDir;
  QString OctaveDir;  // m-files location
  QString ExamplesDir;
  QString DocDir;

  unsigned int NodeWiring;
  QDir QucsWorkDir;

  // A dir for user projects and libraries. See also https://github.com/ra3xdh/qucs_s/issues/145
  QDir qucsWorkspaceDir;

  // This is the dir where all temporary or intermediate data should be stored.
  // Consider a data "temporary" if its used only once or it makes sense only
  // through out a single app run or a shorter period of time.
  // Don't make any assumptions about the lifetime of contents in this dir,
  // think that everything placed in here is deleted when app is terminated.
  QDir tempFilesDir;
  QDir projsDir; // current user projects subdirectory
  QDir AdmsXmlBinDir;  // dir of admsXml executable
  QDir AscoBinDir;     // dir of asco executable
  QString OpenVAFExecutable;
  QString NgspiceExecutable;  // Executables of external simulators
  QString XyceExecutable;
  QString XyceParExecutable;
  QString SpiceOpusExecutable;
  QString S4Qworkdir;
  unsigned int NProcs; // Number of processors for Xyce
  QString OctaveExecutable; // OctaveExecutable location
  QString QucsOctave; // OUCS_OCTAVE variable
  QString RFLayoutExecutable;
  bool ResolveSpicePrefix;

  // registered filename extensions with program to open the file
  QStringList FileTypes;

  // List of extensions used for spice files
  QStringList spiceExtensions;

  unsigned int numRecentDocs;
  QStringList RecentDocs;

  QStringList RecentProjects;

  bool IgnoreFutureVersion;
  bool GraphAntiAliasing;
  bool TextAntiAliasing;

  bool hasDarkTheme;
  bool fullTraceName;

  bool firstRun;
};

extern tQucsSettings QucsSettings;  // extern because nearly everywhere used
extern QucsApp *QucsMain;  // the Qucs application itself
extern QString lastDir;    // to remember last directory for several dialogs
extern QStringList qucsPathList;
extern VersionTriplet QucsVersion;

bool loadSettings();
bool saveApplSettings();
void qucsMessageOutput(QtMsgType type, const char *msg);

#endif // ifndef QUCS_MAIN_H
