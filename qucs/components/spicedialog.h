/***************************************************************************
                              spicedialog.h
                             ---------------
    begin                : Tue May 3 2005
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

#ifndef SPICEDIALOG_H
#define SPICEDIALOG_H

#include <QDialog>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

class Schematic;
class SpiceFile;
class QLineEdit;
class QPushButton;
class QListWidget;
class QListWidgetItem;
class QCheckBox;
class QVBoxLayout;
class QProcess;
class QRegExpValidator;
class QComboBox;
class QucsApp;
class QTextStream;

class SpiceDialog : public QDialog {
   Q_OBJECT
public:
  SpiceDialog(QucsApp*, SpiceFile*, Schematic*);
 ~SpiceDialog();

private slots:
  void slotButtOK();
  void slotButtCancel();
  void slotButtApply();
  void slotButtBrowse();
  void slotButtEdit();

  void slotButtAdd();
  void slotButtRemove();
  void slotAddPort(QListWidgetItem *);
  void slotRemovePort(QListWidgetItem *);

  void slotGetNetlist();
  void slotGetError();

  void slotSkipOut();
  void slotSkipErr();
  void slotGetPrepOut();
  void slotGetPrepErr();
  void slotPrepChanged(int);

protected slots:
    void reject();

private:
  bool loadSpiceNetList(const QString&);

  QVBoxLayout *all;   // the mother of all widgets
  QRegularExpressionValidator *Validator, *ValRestrict;
  QRegularExpression    Expr;
  QListWidget *NodesList, *PortsList;
  QCheckBox   *FileCheck, *SimCheck, *ParamCheck;
  QLineEdit   *FileEdit, *CompNameEdit, *ParamsEdit;
  QPushButton *ButtBrowse, *ButtEdit, *ButtAdd, *ButtRemove,
              *ButtOK, *ButtApply, *ButtCancel;
  QComboBox   *PrepCombo;
  SpiceFile   *Comp;
  Schematic   *Doc;
  bool        changed;
  int         currentPrep;

  QTextStream *prestream;
  QProcess *QucsConv, *SpicePrep;
  QString Line, Error;  // to store the text read from QucsConv
  int textStatus; // to store with text data QucsConv will sent next

  QucsApp* App;
};

#endif
