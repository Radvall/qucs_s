/***************************************************************************
                               textdoc.cpp
                              -------------
Copyright (C) 2006 by Michael Margraf <michael.margraf@alumni.tu-berlin.de>
Copyright (C) 2014 by Guilherme Brondani Torri <guitorri@gmail.com>

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
#include <QAction>
#include <QMessageBox>
#include <QTextStream>
#include <qpalette.h>

#include "main.h"
#include "misc.h"
#include "qucs.h"
#include "textdoc.h"
#include "syntax.h"
#include "components/vhdlfile.h"
#include "components/verilogfile.h"
#include "components/vafile.h"

/*!
 * \file textdoc.cpp
 * \brief Implementation of the TextDoc class.
 */

/*!
 * \brief TextDoc::TextDoc Text document constructor
 * \param App_ is the parent object
 * \param Name_ is the initial text document name
 */
TextDoc::TextDoc(QucsApp *App_, const QString& Name_) : QPlainTextEdit(), QucsDoc(App_, Name_)
{
  setFont(QucsSettings.textFont);

  simulation = true;
  Library = "";
  Libraries = "";
  SetChanged = false;
  devtype = DEV_DEF;

  a_tmpPosX = a_tmpPosY = 1;  // set to 1 to trigger line highlighting
  setLanguage (Name_);

  viewport()->setFocus();

  setWordWrapMode(QTextOption::NoWrap);
  misc::setWidgetBackgroundColor(viewport(),QucsSettings.BGColor);
  // Set black text if light background
  QPalette p = palette();
  if(QucsSettings.BGColor.value() > 127) {
    p.setColor(QPalette::Text, Qt::black);
    setPalette(p);
  }
  connect(this, SIGNAL(textChanged()), SLOT(slotSetChanged()));
  connect(this, SIGNAL(cursorPositionChanged()),
          SLOT(slotCursorPosChanged()));
  if (App_) {
    connect(this, SIGNAL(signalCursorPosChanged(int, int, QString)),
        App_, SLOT(printCursorPosition(int, int, QString)));
    connect(this, SIGNAL(signalUndoState(bool)),
        App_, SLOT(slotUpdateUndo(bool)));
    connect(this, SIGNAL(signalRedoState(bool)),
        App_, SLOT(slotUpdateRedo(bool)));
    connect(this, SIGNAL(signalFileChanged(bool)),
        App_, SLOT(slotFileChanged(bool)));
  }

  syntaxHighlight = new SyntaxHighlighter(this);
  syntaxHighlight->setLanguage(language);
  syntaxHighlight->setDocument(document());

  connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));
  highlightCurrentLine();
}

/*!
 * \brief TextDoc::~TextDoc Text document destructor
 */
TextDoc::~TextDoc()
{
  delete syntaxHighlight;
}

/*!
 * \brief TextDoc::setLanguage(const QString&)
 * \param FileName Text document file name
 * Extract the file name suffix and assign a language_type to it.
 */
void TextDoc::setLanguage (const QString& FileName)
{
  QFileInfo Info (FileName);
  QString ext = Info.suffix();
  if (ext == "vhd" || ext == "vhdl")
    setLanguage (LANG_VHDL);
  else if (ext == "v")
    setLanguage (LANG_VERILOG);
  else if (ext == "va")
    setLanguage (LANG_VERILOGA);
  else if (ext == "m" || ext == "oct")
    setLanguage (LANG_OCTAVE);
  else
    setLanguage (LANG_NONE);
}

/*!
 * \brief TextDoc::setLanguage(int)
 * \param lang is a language_type
 * Assign value to text document object language variable
 */
void TextDoc::setLanguage (int lang)
{
  language = lang;
}

/*!
 * \brief TextDoc::saveSettings saves the text document settings .cfg
 * \return true/false if settings file opened with success
 */
bool TextDoc::saveSettings (void)
{
  QFile file (a_DocName + ".cfg");
  if (!file.open (QIODevice::WriteOnly))
    return false;

  QTextStream stream (&file);
  stream << "Textfile settings file, Qucs " PACKAGE_VERSION "\n"
    << "Simulation=" << simulation << "\n"
    << "Duration=" << a_SimTime << "\n"
    << "Module=" << (!simulation) << "\n"
    << "Library=" << Library << "\n"
    << "Libraries=" << Libraries << "\n"
    << "ShortDesc=" << ShortDesc << "\n"
    << "LongDesc=" << LongDesc << "\n"
    << "Icon=" << Icon << "\n"
    << "Recreate=" << recreate << "\n"
    << "DeviceType=" << devtype << "\n";

  file.close ();
  SetChanged = false;
  return true;
}

/*!
 * \brief TextDoc::loadSettings loads the text document settings
 * \return true/false if settings file opened with success
 */
bool TextDoc::loadSettings (void)
{
  QFile file (a_DocName + ".cfg");
  if (!file.open (QIODevice::ReadOnly))
    return false;

  QTextStream stream (&file);
  QString Line, Setting;

  bool ok;
  while (!stream.atEnd ()) {
    Line = stream.readLine ();
    Setting = Line.section ('=', 0, 0);
    Line = Line.section ('=', 1).trimmed ();
    if (Setting == "Simulation") {
      simulation = Line.toInt (&ok);
    } else if (Setting == "Duration") {
      a_SimTime = Line;
    } else if (Setting == "Module") {
    } else if (Setting == "Library") {
      Library = Line;
    } else if (Setting == "Libraries") {
      Libraries = Line;
    } else if (Setting == "ShortDesc") {
      ShortDesc = Line;
    } else if (Setting == "LongDesc") {
      LongDesc = Line;
    } else if (Setting == "Icon") {
      Icon = Line;
    } else if (Setting == "Recreate") {
      recreate = Line.toInt (&ok);
    } else if (Setting == "DeviceType") {
      devtype = Line.toInt (&ok);
    }
  }

  file.close ();
  return true;
}

/*!
 * \brief TextDoc::setName sets the text file name on its tab
 * \param Name_ text file name to be set
 */
void TextDoc::setName (const QString& Name_)
{
  a_DocName = Name_;
  setLanguage (a_DocName);

  QFileInfo Info (a_DocName);

  a_DataSet = Info.baseName () + ".dat";
  a_DataDisplay = Info.baseName () + ".dpl";
  if(Info.suffix() == "m" || Info.suffix() == "oct")
    a_SimTime = "1";
}

/*!
 * \brief TextDoc::becomeCurrent sets text document as current
 *
 * \detail Make sure the menu options are adjusted.
 */
void TextDoc::becomeCurrent (bool)
{
  slotCursorPosChanged();
  viewport()->setFocus ();

  emit signalUndoState(document()->isUndoAvailable());
  emit signalRedoState(document()->isRedoAvailable());

  // update appropriate menu entries
  a_App->symEdit->setText (tr("Edit Text Symbol"));
  a_App->symEdit->setStatusTip (tr("Edits the symbol for this text document"));
  a_App->symEdit->setWhatsThis (
    tr("Edit Text Symbol\n\nEdits the symbol for this text document"));

  if (language == LANG_VHDL) {
    a_App->insEntity->setText (tr("VHDL entity"));
    a_App->insEntity->setStatusTip (tr("Inserts skeleton of VHDL entity"));
    a_App->insEntity->setWhatsThis (
      tr("VHDL entity\n\nInserts the skeleton of a VHDL entity"));
  }
  else if (language == LANG_VERILOG || language == LANG_VERILOGA) {
    a_App->insEntity->setText (tr("Verilog module"));
    a_App->insEntity->setStatusTip (tr("Inserts skeleton of Verilog module"));
    a_App->insEntity->setWhatsThis (
      tr("Verilog module\n\nInserts the skeleton of a Verilog module"));
    a_App->buildModule->setEnabled(true);
  }
  else if (language == LANG_OCTAVE) {
    a_App->insEntity->setText (tr("Octave function"));
    a_App->insEntity->setStatusTip (tr("Inserts skeleton of Octave function"));
    a_App->insEntity->setWhatsThis (
      tr("Octave function\n\nInserts the skeleton of a Octave function"));
  }
  a_App->simulate->setEnabled (true);
  a_App->editActivate->setEnabled (true);
}

bool TextDoc::baseSearch(const QString &str, bool CaseSensitive, bool wordOnly, bool backward)
{
  QTextDocument::FindFlags flag = QTextDocument::FindFlags();
  bool finded;

  if (CaseSensitive) {
    flag = QTextDocument::FindCaseSensitively;
  }
  if (backward) {
    flag = flag | QTextDocument::FindBackward;
  }
  if (wordOnly) {
    flag = flag | QTextDocument::FindWholeWords;
  }

  finded = find(str, flag);
  if (!finded) {
    if (!backward) {
      moveCursor(QTextCursor::Start);
    } else {
      moveCursor(QTextCursor::End);
    }
    finded = find(str, flag);
    if (!finded) {
      QMessageBox::information(this,
          tr("Find..."), tr("Cannot find target: %1").arg(str),
          QMessageBox::Ok | QMessageBox::Default | QMessageBox::Escape);
    }
  }
  return finded;
}

// implement search function
// if target not find, auto revert to head, find again
// if still not find, pop out message
void TextDoc::search(const QString &str, bool CaseSensitive, bool wordOnly, bool backward)
{
  baseSearch(str, CaseSensitive, wordOnly, backward);
}

// implement replace function
void TextDoc::replace(const QString &str, const QString &str2, bool needConfirmed,
                      bool CaseSensitive, bool wordOnly, bool backward)
{
  bool finded = baseSearch(str, CaseSensitive, wordOnly, backward);
  int i;

  if(finded) {
    i = QMessageBox::Yes;
    if (needConfirmed) {
      i = QMessageBox::information(this,
          tr("Replace..."), tr("Replace occurrence ?"),
          QMessageBox::Yes|QMessageBox::No);
    }
    if(i == QMessageBox::Yes) {
      insertPlainText(str2);
    }
  }
}


/*!
 * \brief TextDoc::slotCursorPosChanged update status bar with line:column
 */
void TextDoc::slotCursorPosChanged()
{
  QTextCursor pos = textCursor();
  int x = pos.blockNumber();
  int y = pos.columnNumber();
  emit signalCursorPosChanged(x+1, y+1, "");
  a_tmpPosX = x;
  a_tmpPosY = y;
}

/*!
 * \brief TextDoc::slotSetChanged togles tab icon to indicate unsaved changes
 */
void TextDoc::slotSetChanged()
{
  if((document()->isModified() && !a_DocChanged) || SetChanged) {
    a_DocChanged = true;
  }
  else if((!document()->isModified() && a_DocChanged)) {
    a_DocChanged = false;
  }
  emit signalFileChanged(a_DocChanged);
  emit signalUndoState(document()->isUndoAvailable());
  emit signalRedoState(document()->isRedoAvailable());
}

/*!
 * \brief TextDoc::createStandardContextMenu creates the standard context menu
 * \param pos
 * \return
 *
 *  \todo \fixme is this working?
 */
QMenu *TextDoc::createStandardContextMenu()
{
  QMenu *popup = QPlainTextEdit::createStandardContextMenu();

   if (language != LANG_OCTAVE) {
       ((QWidget *) popup)->addAction(a_App->fileSettings);
   }
   return popup;
}

/*!
 * \brief TextDoc::load loads a text document
 * \return true/false if the document was opened with success
 */
bool TextDoc::load ()
{
  QFile file (a_DocName);
  if (!file.open (QIODevice::ReadOnly))
    return false;
  setLanguage (a_DocName);

  QTextStream stream (&file);
  insertPlainText(stream.readAll());
  document()->setModified(false);
  slotSetChanged ();
  file.close ();
  a_lastSaved = QDateTime::currentDateTime ();
  loadSettings ();
  a_SimOpenDpl = simulation ? true : false;
  refreshLanguage();
  return true;
}


/*!
 * \brief TextDoc::save saves the current document and it settings
 * \return true/false if the document was opened with success
 */
int TextDoc::save ()
{
  saveSettings ();

  QFile file (a_DocName);
  if (!file.open (QIODevice::WriteOnly))
    return -1;
  setLanguage (a_DocName);

  QTextStream stream (&file);
  stream << toPlainText();
  document()->setModified (false);
  slotSetChanged ();
  file.close ();

  QFileInfo Info (a_DocName);
  a_lastSaved = Info.lastModified ();

  /// clear highlighted lines on save \see MessageDock::slotCursor()
  QList<QTextEdit::ExtraSelection> extraSelections;
  this->setExtraSelections(extraSelections);
  refreshLanguage();

  return 0;
}

/*!
 * \brief Zooms the document in and out. Note, the zoom amount is fixed by Qt and the
 *        amount passed is ignored.
 */
double TextDoc::zoomBy(double zoom)
{
  // qucs_actions defines zooming in as > 1.
  if (zoom > 1.0) {
    zoomIn();
  }

  else {
    zoomOut();
  }

  return zoom;
}

/*!
 * \brief Resets the font scaling to default.
 */
void TextDoc::showNoZoom()
{
  // Get a copy of this editor's existing font and apply the default font size to it.
  QFont tempFont = font();
  tempFont.setPointSize(QucsSettings.font.pointSize());
  setFont(tempFont);
}

/*!
 * \brief TextDoc::loadSimulationTime set a_SimTime member variable
 * \param Time string  with simulation time
 * \return true if a_SimTime is set
 */
bool TextDoc::loadSimulationTime(QString& Time)
{
  if(!a_SimTime.isEmpty()) {
    Time = a_SimTime;
    return true;
  }
  return false;
}

/*!
 * \brief TextDoc::commentSelected toggles the comment of selected text
 * See also QucsApp::slotEditActivate
 */
void TextDoc::commentSelected ()
{
  QTextCursor cursor = this->textCursor();

  if(!cursor.hasSelection())
      return; // No selection available

  // get range of selection
  int start = cursor.selectionStart();
  int end = cursor.selectionEnd();

  cursor.setPosition(start);
  int firstLine = cursor.blockNumber();
  cursor.setPosition(end, QTextCursor::KeepAnchor);
  int lastLine = cursor.blockNumber();

  // use comment string indicator depending on language
  QString co;

  switch (language) {
  case LANG_VHDL:
    co = "--";
    break;
  case LANG_VERILOG:
  case LANG_VERILOGA:
    co = "//";
    break;
  case LANG_OCTAVE:
    co = "%";
    break;
  default:
    co = "";
    break;
  }

  QStringList newlines;
  for (int i=firstLine; i<=lastLine; i++) {
      QString line = document()->findBlockByLineNumber(i).text();
      if (line.startsWith(co)){
          // uncomment
          line.remove(0,co.length());
          newlines << line;
      }
      else {
          // comment
          line = line.insert(0, co);
          newlines << line;
      }
  }
  insertPlainText(newlines.join("\n"));
}

/*!
 * \brief TextDoc::insertSkeleton adds a basic skeleton for type of text file
 */
void TextDoc::insertSkeleton ()
{
  if (language == LANG_VHDL)
    appendPlainText("entity  is\n  port ( : in bit);\nend;\n"
      "architecture  of  is\n  signal : bit;\nbegin\n\nend;\n\n");
  else if (language == LANG_VERILOG)
    appendPlainText ("module  ( );\ninput ;\noutput ;\nbegin\n\nend\n"
      "endmodule\n\n");
  else if (language == LANG_OCTAVE)
    appendPlainText ("function  =  ( )\n"
      "endfunction\n\n");
}

/*!
 * \brief TextDoc::getModuleName parse the module name ou of the text file contents
 * \return the module name
 */
QString TextDoc::getModuleName (void)
{
  switch (language) {
  case LANG_VHDL:
    {
      VHDL_File_Info VInfo (toPlainText());
      return VInfo.EntityName;
    }
  case LANG_VERILOG:
    {
      Verilog_File_Info VInfo (toPlainText());
      return VInfo.ModuleName;
    }
  case LANG_VERILOGA:
    {
      VerilogA_File_Info VInfo (toPlainText());
      return VInfo.ModuleName;
    }
  case LANG_OCTAVE:
    {
      QFileInfo Info (a_DocName);
      return Info.baseName ();
    }
  default:
    return "";
  }
}

/*!
 * \brief Handle mouse wheel events
 *        Used to 'zoom' i.e., increase / reduce the font size.
 */
void TextDoc::wheelEvent(QWheelEvent* event)
{
  if (event->modifiers() & Qt::CTRL) {
    if (event->angleDelta().y() > 0) {
      zoomIn();
    }

    else {
      zoomOut();
    }
  }

  else {
    QPlainTextEdit::wheelEvent(event);
  }
}

/*!
 * \brief TextDoc::highlightCurrentLine mark the current line
 */
void TextDoc::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::blue).lighter(195);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void TextDoc::refreshLanguage()
{
    this->setLanguage(a_DocName);
    syntaxHighlight->setLanguage(language);
    syntaxHighlight->setDocument(document());
}
