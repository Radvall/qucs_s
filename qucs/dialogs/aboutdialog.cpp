/*
 * aboutdialog.cpp - customary about dialog showing various info
 *
 * Copyright (C) 2015-2016, Qucs team (see AUTHORS file)
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

/*!
 * \file aboutdialog.cpp
 * \brief Implementation of the About dialog
 */

#include <array>
#include <algorithm>
#include <random>

#include <iostream>
#include <string>

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "aboutdialog.h"

#include <QObject>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QLabel>
#include <QPushButton>
#include <QApplication>
#include <QDebug>


AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
  qucs_sDevs = {{
     "Vadim Kuznetsov - " + tr("project maintainer, simulator interface and GUI design"),
     "Mike Brinson - " + tr("component models, documentation"),
     "Tom Russo - " + tr("Xyce integration"),
     "Tom Hajjar - " + tr("Testing, examples"),
     "Sergey Krasilnikov - " + tr("Qt6 support, general improvements"),
     "Sergey Ryzhov - " + tr("Digital simulation, general improvements"),
     "Andrey Kalmykov - " + tr("Schematic rendering engine, refactoring"),
     "Andr&#xe9;s Mart&#xed;nez Mera - " + tr("RF design tools"),
     "Muhammet Şükrü Demir - " + tr("CI setup, build system, MacOS support"),
     "Hampton Morgan - " + tr("Documentation"),
     "Iwbnwif Yiw - " + tr("Refactoring, general improvements"),
     "Thomas Zecha - " + tr("Microelectronics PDK support"),
     "Maria Dubinina - " + tr("testing, general bugfixes")
  }};
  currAuths = {{
    "Guilherme Brondani Torri - " + tr("GUI programmer, Verilog-A dynamic loader"),
    "Mike Brinson - " + tr("testing, modelling and documentation, tutorial contributor"),
    "Richard Crozier - " + tr("testing, modelling, Octave."),
    "Bastien Roucaries - " + tr("bondwire and rectangular waveguide model implementation"),
    "Frans Schreuder - " + tr("GUI programmer, release"),
    "Vadim Kuznetsov - " + tr("filter synthesis (qucs-activefilter), SPICE integration (NGSPICE, Xyce)"),
    "Claudio Girardi - " + tr("testing, general fixes"),
    "Felix Salfelder - " + tr("refactoring, modularity"),
    "Andr&#xe9;s Mart&#xed;nez Mera - " + tr("RF design tools")
  }};

  prevDevs = {{
      "Michael Margraf - " + tr("founder of the project, GUI programmer"),
      "Stefan Jahn - " + tr("Programmer of simulator"),
      "Jens Flucke - " + tr("webpages and translator"),
      "Raimund Jacob - " + tr("tester and applyer of Stefan's patches, author of documentation"),
      "Vincent Habchi - " + tr("coplanar line and filter synthesis code, documentation contributor"),
      "Toyoyuki Ishikawa - " + tr("some filter synthesis code and attenuator synthesis"),
      "Gopala Krishna A - " + tr("GUI programmer, Qt4 porter"),
      "Helene Parruitte - " + tr("programmer of the Verilog-AMS interface"),
      "Gunther Kraut - " + tr("equation solver contributions, exponential sources, author of documentation"),
      "Andrea Zonca - " + tr("temperature model for rectangular waveguide"),
      "Clemens Novak - " + tr("GUI programmer"),
      "You-Tang Lee (YodaLee) - " + tr("GUI programmer, Qt4 porter")
  }};

  trAuths = {{
    tr("German by") + " Stefan Jahn",
    tr("Polish by") + " Dariusz Pienkowski",
    tr("Romanian by") + " Radu Circa",
    tr("French by") + " Vincent Habchi, F5RCS",
    tr("Portuguese by") + " Luciano Franca, Helio de Sousa, Guilherme Brondani Torri",
    tr("Spanish by") + " Jose L. Redrejo Rodriguez",
    tr("Japanese by") + " Toyoyuki Ishikawa",
    tr("Italian by") + " Giorgio Luparia, Claudio Girardi",
    tr("Hebrew by") + " Dotan Nahum",
    tr("Swedish by") + " Markus Gothe, Peter Landgren",
    tr("Turkish by") + " Muhammet Şükrü Demir",
    tr("Hungarian by") + " Jozsef Bus",
    tr("Russian by") + " Igor Gorbounov and Anton Midyukov",
    tr("Czech by") + " Marek Straka,Martin Stejskal",
    tr("Catalan by") + " Antoni Subirats",
    tr("Ukrainian by") + " Dystryk",
    tr("Arabic by") + " Chabane Noureddine",
    tr("Kazakh by") + " Erbol Keshubaev",
    tr("Chinese by") + " HGC"
  }};

  std::shuffle(currAuths.begin(), currAuths.end(), rng);

  QLabel *lbl;

  setWindowTitle(tr("About Qucs"));

  all = new QVBoxLayout(this);
  //all->setContentsMargins(0,0,0,0);
  //all->setSpacing(0);

  QLabel *iconLabel = new QLabel();
  iconLabel->setPixmap(QPixmap(QStringLiteral(":/bitmaps/hicolor/scalable/apps/qucs.svg")));

  QWidget *hbox = new QWidget();
  QHBoxLayout *hl = new QHBoxLayout(hbox);
  hl->setContentsMargins(0,0,0,0);

  hl->addWidget(iconLabel);
  all->addWidget(hbox);

  QWidget *vbox = new QWidget();
  QVBoxLayout *vl = new QVBoxLayout(vbox);
  //vl->setContentsMargins(0,0,0,0);
  hl->addWidget(vbox);

  QString versionText;
  versionText = tr("Version")+" "+PACKAGE_VERSION+"\n";

  vl->addWidget(new QLabel("<span style='font-size:x-large; font-weight:bold;'>Quite Universal Circuit Simulator</span>"));
  lbl = new QLabel(versionText);
  lbl->setAlignment(Qt::AlignHCenter);
  vl->addWidget(lbl);
  vl->addWidget(new QLabel(tr("Copyright (C)")+" 2017-2025 Qucs-S Team\n"+
               tr("Copyright (C)")+" 2011-2016 Qucs Team\n"+
			   tr("Copyright (C)")+" 2003-2009 Michael Margraf"));

  lbl = new QLabel("\nThis is free software; see the source for copying conditions."
		   "\nThere is NO warranty; not even for MERCHANTABILITY or "
		   "\nFITNESS FOR A PARTICULAR PURPOSE.\n");
  lbl->setAlignment(Qt::AlignHCenter);
  all->addWidget(lbl);

  QTabWidget *t = new QTabWidget();
  all->addWidget(t);
  connect(t, SIGNAL(currentChanged(int)), this, SLOT(currentChangedSlot(int)));

  authorsBrowser = new QTextBrowser;
  // the Ctrl-Wheel event we would like to filter is handled by the viewport
  authorsBrowser->viewport()->installEventFilter(this);
  trBrowser = new QTextBrowser;
  trBrowser->viewport()->installEventFilter(this);

  QString supportText;
  // link to home page, help mailing list, IRC ?
  supportText = tr("Home Page") + " : <a href='https://ra3xdh.github.io/'>https://ra3xdh.github.io/</a><br/>"+
    tr("Documentation start page") + " : <a href='https://qucs-s-help.readthedocs.io/'>https://qucs-s-help.readthedocs.io/</a><br/>" +
    tr("Bugtracker page") + " : <a href='https://github.com/ra3xdh/qucs_s/issues'>https://github.com/ra3xdh/qucs_s/issues</a><br/>" +
    tr("Forum") + " : <a href='https://github.com/ra3xdh/qucs_s/discussions'>https://github.com/ra3xdh/qucs_s/discussions</a><br/>";

  QTextBrowser *supportBrowser = new QTextBrowser;
  supportBrowser->viewport()->installEventFilter(this);
  supportBrowser->setOpenExternalLinks(true);
  supportBrowser->setHtml(supportText);

  QString licenseText;
  licenseText = "Qucs-S is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.<br/><br/>This software is distributed in the hope that it will be useful,but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details..<br/><br/> You should have received a copy of the GNU General Public License along with Qucs, see the file COPYING. If not see <a href='http://www.gnu.org/licenses/'>http://www.gnu.org/licenses/</a> or write to the Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,Boston, MA 02110-1301, USA.";

  QTextBrowser *licenseBrowser = new QTextBrowser;
  licenseBrowser->viewport()->installEventFilter(this);
  licenseBrowser->setOpenExternalLinks(true);
  licenseBrowser->setHtml(licenseText);

  t->addTab(authorsBrowser, tr("Authors"));
  t->addTab(trBrowser, tr("Translations"));
  t->addTab(supportBrowser, tr("Support"));
  t->addTab(licenseBrowser, tr("License"));

  QWidget *hbBtn = new QWidget();
  QHBoxLayout *hlBtn = new QHBoxLayout(hbBtn);
  hlBtn->setContentsMargins(0,0,0,0);
  all->addWidget(hbBtn);

  QPushButton *okButton = new QPushButton(tr("&OK"), parent);
  okButton->setFocus();
  connect(okButton, SIGNAL(clicked()), this, SLOT(close()));
  hlBtn->addStretch();
  hlBtn->addWidget(okButton);

  setAuthorsText();
  setTrText();
  prevTab = 0; // first Tab is selected by default
}

void AboutDialog::currentChangedSlot(int index) {
  if (prevTab == 0) { // deselected tab with current and previous authors
    // shuffle them
    std::shuffle(currAuths.begin(), currAuths.end(), rng);
    std::shuffle(prevDevs.begin(), prevDevs.end(), rng);
    setAuthorsText();
  } else if (prevTab == 1) {// deselected tab with translators
    std::shuffle(trAuths.begin(), trAuths.end(), rng);
    setTrText();
  }

  prevTab = index;
}

void AboutDialog::setAuthorsText() {

  QString authorsText;
  authorsText = tr("Qucs-S project team:");
  authorsText += "<ul>";
  for(const auto& tStr : qucs_sDevs) {
    authorsText += ("<li>" + tStr + "</li>");
  }
  authorsText += "</ul>";

  authorsText += tr("Based on Qucs project developed by:") + "<ul>";

  for(const auto& tStr : currAuths) {
    authorsText += ("<li>" + tStr + "</li>");
  }
  authorsText += "</ul>";
  authorsText += tr("Previous Developers") + "<ul>";
  for(const auto& tStr : prevDevs) {
    authorsText += ("<li>" + tStr + "</li>");
  }
  authorsText += "</ul>";

  authorsBrowser->setHtml(authorsText);
}

void AboutDialog::setTrText() {
  QString trText;
  trText = tr("GUI translations :") + "<ul>";
  for(const auto& tStr : trAuths) {
    trText += ("<li>" + tStr + "</li>");
  }
  trText += "</ul>";

  trBrowser->setHtml(trText);
}

// event filter to remove the Ctrl-Wheel (text zoom) event
bool AboutDialog::eventFilter(QObject *obj, QEvent *event) {
  if ((event->type() == QEvent::Wheel) &&
      (QApplication::keyboardModifiers() & Qt::ControlModifier )) {
    return true; // eat Ctrl-Wheel event
  } else {
    // pass the event on to the parent class
    return QDialog::eventFilter(obj, event);
  }
}


