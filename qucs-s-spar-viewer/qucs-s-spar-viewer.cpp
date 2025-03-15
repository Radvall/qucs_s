/*
 * qucs-s-spar-viewer.cpp - S-parameter viewer for Qucs-S
 *
 * copyright (C) 2024 Andres Martinez-Mera <andresmartinezmera@gmail.com>
 *
 * This is free software; you can redistribute it and/or modify
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
 * along with this package; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qucs-s-spar-viewer.h"

#include <QPixmap>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QValidator>
#include <QClipboard>
#include <QApplication>
#include <QDebug>
#include <QLineSeries>


Qucs_S_SPAR_Viewer::Qucs_S_SPAR_Viewer()
{
  QWidget *centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);
  centralWidget->setMaximumWidth(0); // Minimize central widget size

  setWindowIcon(QPixmap(":/bitmaps/big.qucs.xpm"));
  setWindowTitle("Qucs S-parameter Viewer " PACKAGE_VERSION);

  CreateMenuBar();

  // Set frequency units
  frequency_units << "Hz" << "kHz" << "MHz" << "GHz";


  // These are two maximum markers to find the lowest and the highest frequency in the data samples.
  // They are used to prevent the user from zooming out too much
  f_min = 1e20;
  f_max = -1;

  // Load default colors
  default_colors.append(QColor(Qt::red));
  default_colors.append(QColor(Qt::blue));
  default_colors.append(QColor(Qt::darkGreen));


  CreateDisplayWidgets();
  CreateRightPanel();

  // Put the following widgets on the top to make them visible to the user
  dockFiles->raise();
  dockChart->raise();

  setDockNestingEnabled(true);
  setAcceptDrops(true);//Enable drag and drop feature to open files
  loadRecentFiles();// Load "Recent Files" list
}


void Qucs_S_SPAR_Viewer::CreateMenuBar(){
  QMenu *fileMenu = new QMenu(tr("&File"));

  QAction *fileQuit = new QAction(tr("&Quit"), this);
  fileQuit->setShortcut(QKeySequence::Quit);
  connect(fileQuit, SIGNAL(triggered(bool)), SLOT(slotQuit()));

  QAction *fileOpenSession = new QAction(tr("&Open session file"), this);
  fileOpenSession->setShortcut(QKeySequence::Open);
  connect(fileOpenSession, SIGNAL(triggered(bool)), SLOT(slotLoadSession()));

  QAction *fileSaveAsSession = new QAction(tr("&Save session as ..."), this);
  fileSaveAsSession->setShortcut(QKeySequence::SaveAs);
  connect(fileSaveAsSession, SIGNAL(triggered(bool)), SLOT(slotSaveAs()));

  QAction *fileSaveSession = new QAction(tr("&Save session"), this);
  fileSaveSession->setShortcut(QKeySequence::Save);
  connect(fileSaveSession, SIGNAL(triggered(bool)), SLOT(slotSave()));

  recentFilesMenu = fileMenu->addMenu("Recent Files");
  connect(recentFilesMenu, &QMenu::aboutToShow, this, &Qucs_S_SPAR_Viewer::updateRecentFilesMenu);

  fileMenu->addAction(fileOpenSession);
  fileMenu->addAction(fileSaveSession);
  fileMenu->addAction(fileSaveAsSession);
  fileMenu->addAction(fileQuit);

  QMenu *helpMenu = new QMenu(tr("&Help"));

  QAction *helpHelp = new QAction(tr("&Help"), this);
  helpHelp->setShortcut(Qt::Key_F1);
  helpMenu->addAction(helpHelp);
  connect(helpHelp, SIGNAL(triggered(bool)), SLOT(slotHelpIntro()));

  QAction *helpAbout = new QAction(tr("&About"), this);
  helpMenu->addAction(helpAbout);
  connect(helpAbout, SIGNAL(triggered(bool)), SLOT(slotHelpAbout()));

  helpMenu->addSeparator();

  QAction * helpAboutQt = new QAction(tr("About Qt..."), this);
  helpMenu->addAction(helpAboutQt);
  connect(helpAboutQt, SIGNAL(triggered(bool)), SLOT(slotHelpAboutQt()));

  menuBar()->addMenu(fileMenu);
  menuBar()->addSeparator();
  menuBar()->addMenu(helpMenu);
}


// This function populates the left panel with the following widgets:
// - Files manager
// - Traces manager
// - Markers maanger
// - Limits manager
// - Notebook

void Qucs_S_SPAR_Viewer::CreateRightPanel(){
  // Create left panel widgets
  setFileManagementDock();
  setTraceManagementDock();
  setMarkerManagementDock();
  setLimitManagementDock();

         // Notes
  Notes_Widget = new CodeEditor();
  dockNotes = new QDockWidget("Notes", this);
  dockNotes->setWidget(Notes_Widget);

         // Disable dock closing
  dockFiles->setFeatures(dockFiles->features() & ~QDockWidget::DockWidgetClosable);
  dockTracesList->setFeatures(dockTracesList->features() & ~QDockWidget::DockWidgetClosable);
  dockMarkers->setFeatures(dockMarkers->features() & ~QDockWidget::DockWidgetClosable);
  dockLimits->setFeatures(dockLimits->features() & ~QDockWidget::DockWidgetClosable);
  dockNotes->setFeatures(dockNotes->features() & ~QDockWidget::DockWidgetClosable);

         // Add all panel docks to the right area
  addDockWidget(Qt::RightDockWidgetArea, dockFiles);
  addDockWidget(Qt::RightDockWidgetArea, dockTracesList);
  addDockWidget(Qt::RightDockWidgetArea, dockMarkers);
  addDockWidget(Qt::RightDockWidgetArea, dockLimits);
  addDockWidget(Qt::RightDockWidgetArea, dockNotes);

         // Tabify the panel docks
  tabifyDockWidget(dockFiles, dockTracesList);
  tabifyDockWidget(dockTracesList, dockMarkers);
  tabifyDockWidget(dockMarkers, dockLimits);
  tabifyDockWidget(dockLimits, dockNotes);

  // Remove the tabify between chart docks as it's already done in CreateDisplayWidgets
  // tabifyDockWidget(dockChart, dockSmithChart);

  // To prevent the gap between left and right dock areas, we need to resize the dock widgets
  // This should be called after all dock widgets are set up, perhaps in a separate method
  resizeDocks({dockChart, dockSmithChart}, {width()/2, width()/2}, Qt::Horizontal);
  resizeDocks({dockFiles, dockTracesList, dockMarkers, dockLimits, dockNotes},
              {width()/4, width()/4, width()/4, width()/4, width()/4}, Qt::Horizontal);
}


void Qucs_S_SPAR_Viewer::setFileManagementDock(){

  dockFiles = new QDockWidget("S-parameter files", this);

  QScrollArea *scrollArea_Files = new QScrollArea();
  FileList_Widget = new QWidget();
  QWidget *FilesGroup = new QWidget();

  FilesGrid = new QGridLayout(FileList_Widget);

  vLayout_Files = new QVBoxLayout(FilesGroup);

  QWidget *Buttons = new QWidget();
  QHBoxLayout *hLayout_Files_Buttons = new QHBoxLayout(Buttons);

  Button_Add_File = new QPushButton("Add file", this);
  Button_Add_File->setStyleSheet("QPushButton {background-color: green;\
                                  border-style: outset;\
                                  border-width: 2px;\
                                  border-radius: 10px;\
                                  border-color: beige;\
                                  font: bold 14px;\
                                  color: white;\
                                  min-width: 10em;\
                                  padding: 6px;\
                              }");
  connect(Button_Add_File, SIGNAL(clicked()), SLOT(addFile()));

  Delete_All_Files = new QPushButton("Delete all", this);
  Delete_All_Files->setStyleSheet("QPushButton {background-color: red;\
                                  border-style: outset;\
                                  border-width: 2px;\
                                  border-radius: 10px;\
                                  border-color: beige;\
                                  font: bold 14px;\
                                  color: white;\
                                  min-width: 10em;\
                                  padding: 6px;\
                              }");
  connect(Delete_All_Files, SIGNAL(clicked()), SLOT(removeAllFiles()));

  hLayout_Files_Buttons->addWidget(Button_Add_File);
  hLayout_Files_Buttons->addWidget(Delete_All_Files);

  scrollArea_Files->setWidget(FileList_Widget);
  scrollArea_Files->setWidgetResizable(true);
  vLayout_Files->addWidget(scrollArea_Files, Qt::AlignTop);
  vLayout_Files->addWidget(Buttons, Qt::AlignBottom);
  vLayout_Files->setStretch(0, 3);
  vLayout_Files->setStretch(1, 1);

  dockFiles->setWidget(FilesGroup);
}

void Qucs_S_SPAR_Viewer::setTraceManagementDock(){

  dockTracesList = new QDockWidget("Traces List", this);

  QWidget * TracesGroup = new QWidget();
  QVBoxLayout *Traces_VBox = new QVBoxLayout(TracesGroup);

  // Trace addition box
  QWidget * TraceSelection_Widget = new QWidget(); // Add trace

  QGridLayout * DatasetsGrid = new QGridLayout(TraceSelection_Widget);
  QLabel *dataset_label = new QLabel("<b>Dataset</b>");
  DatasetsGrid->addWidget(dataset_label, 0, 0, Qt::AlignCenter);

  QLabel *Traces_label = new QLabel("<b>Traces</b>");
  DatasetsGrid->addWidget(Traces_label, 0, 1, Qt::AlignCenter);

  QLabel *displayTypeLabel = new QLabel("<b>Display Type</b>");
  DatasetsGrid->addWidget(displayTypeLabel, 0, 2, Qt::AlignCenter);

  QCombobox_traces = new QComboBox();
  DatasetsGrid->addWidget(QCombobox_traces, 1, 1);

  Button_add_trace = new QPushButton("Add trace");
  Button_add_trace->setStyleSheet("QPushButton {background-color: green;\
                                  border-style: outset;\
                                  border-width: 2px;\
                                  border-radius: 10px;\
                                  border-color: beige;\
                                  font: bold 14px;\
                                  color: white;\
                                  min-width: 10em;\
                                  padding: 6px;\
                              }");
 connect(Button_add_trace, SIGNAL(clicked()), SLOT(addTrace())); // Connect button with the handler

  QCombobox_display_mode= new QComboBox();
  QCombobox_display_mode->addItem("dB");
  QCombobox_display_mode->addItem("Phase");
  QCombobox_display_mode->addItem("Smith");
  QCombobox_display_mode->addItem("n.u.");
  QCombobox_display_mode->setCurrentIndex(0); // Default to dB
  QCombobox_display_mode->setObjectName("DisplayTypeCombo");
  DatasetsGrid->addWidget(QCombobox_display_mode, 1, 2);

  DatasetsGrid->addWidget(Button_add_trace, 1, 3);

  QCombobox_datasets = new QComboBox();
  DatasetsGrid->addWidget(QCombobox_datasets, 1, 0);
  connect(QCombobox_datasets, SIGNAL(currentIndexChanged(int)), SLOT(updateTracesCombo())); // Each time the dataset is changed it is needed to update the traces combo.
                                                                                            // This is needed when the user has data with different number of ports.


  traceTabs = new QTabWidget(this); // Ensure 'this' is the parent

         // Create tabs for Magnitude/Phase and Smith Chart
  magnitudePhaseTab = new QWidget(traceTabs); // Parent is traceTabs
  smithTab = new QWidget(traceTabs); // Parent is traceTabs

         // Add tabs to the tab widget
  traceTabs->addTab(magnitudePhaseTab, "Magnitude/Phase");
  traceTabs->addTab(smithTab, "Smith Chart");

         // Create layouts for each tab
  magnitudePhaseLayout = new QGridLayout(magnitudePhaseTab);
  smithLayout = new QGridLayout(smithTab);

         // Set the layouts on the tabs
  magnitudePhaseTab->setLayout(magnitudePhaseLayout);
  smithTab->setLayout(smithLayout);

  // Set Magnitude tab
  QLabel * Label_Name_mag = new QLabel("<b>Name</b>");
  QLabel * Label_Color_mag = new QLabel("<b>Color</b>");
  QLabel * Label_LineStyle_mag = new QLabel("<b>Line Style</b>");
  QLabel * Label_LineWidth_mag = new QLabel("<b>Width</b>");
  QLabel * Label_Remove_mag = new QLabel("<b>Remove</b>");

  magnitudePhaseLayout->addWidget(Label_Name_mag, 0, 0, Qt::AlignCenter);
  magnitudePhaseLayout->addWidget(Label_Color_mag, 0, 1, Qt::AlignCenter);
  magnitudePhaseLayout->addWidget(Label_LineStyle_mag, 0, 2, Qt::AlignCenter);
  magnitudePhaseLayout->addWidget(Label_LineWidth_mag, 0, 3, Qt::AlignCenter);
  magnitudePhaseLayout->addWidget(Label_Remove_mag, 0, 4, Qt::AlignCenter);

  // Set Smith tab
  QLabel * Label_Name_Smith = new QLabel("<b>Name</b>");
  QLabel * Label_Color_Smith = new QLabel("<b>Color</b>");
  QLabel * Label_LineStyle_Smith = new QLabel("<b>Line Style</b>");
  QLabel * Label_LineWidth_Smith = new QLabel("<b>Width</b>");
  QLabel * Label_Remove_Smith = new QLabel("<b>Remove</b>");

  smithLayout->addWidget(Label_Name_Smith, 0, 0, Qt::AlignCenter);
  smithLayout->addWidget(Label_Color_Smith, 0, 1, Qt::AlignCenter);
  smithLayout->addWidget(Label_LineStyle_Smith, 0, 2, Qt::AlignCenter);
  smithLayout->addWidget(Label_LineWidth_Smith, 0, 3, Qt::AlignCenter);
  smithLayout->addWidget(Label_Remove_Smith, 0, 4, Qt::AlignCenter);

  setupScrollableLayout();

  Traces_VBox->addWidget(TraceSelection_Widget);
  Traces_VBox->addWidget(traceTabs);

  // Trace management
  // Titles
  TracesList_Widget = new QWidget(); // Panel with the trace settings
  QLabel * Label_Name = new QLabel("<b>Name</b>");
  QLabel * Label_Color = new QLabel("<b>Color</b>");
  QLabel * Label_LineStyle = new QLabel("<b>Line Style</b>");
  QLabel * Label_LineWidth = new QLabel("<b>Width</b>");
  QLabel * Label_Remove = new QLabel("<b>Remove</b>");

  TracesGrid = new QGridLayout(TracesList_Widget);
  TracesGrid->addWidget(Label_Name, 0, 0, Qt::AlignCenter);
  TracesGrid->addWidget(Label_Color, 0, 1, Qt::AlignCenter);
  TracesGrid->addWidget(Label_LineStyle, 0, 2, Qt::AlignCenter);
  TracesGrid->addWidget(Label_LineWidth, 0, 3, Qt::AlignCenter);
  TracesGrid->addWidget(Label_Remove, 0, 4, Qt::AlignCenter);

  dockTracesList->setWidget(TracesGroup);
}

void Qucs_S_SPAR_Viewer::setMarkerManagementDock() {
  // Markers dock
  dockMarkers = new QDockWidget("Markers", this);

  QWidget* MarkersGroup = new QWidget();
  QVBoxLayout* Markers_VBox = new QVBoxLayout(MarkersGroup);

  // Trace addition box
  QWidget* MarkerSelection_Widget = new QWidget();

  MarkersGrid = new QGridLayout(MarkerSelection_Widget);
  QLabel* Frequency_Marker_Label = new QLabel("<b>Frequency</b>");
  MarkersGrid->addWidget(Frequency_Marker_Label, 0, 0, Qt::AlignCenter);

  Button_add_marker = new QPushButton("Add marker");
  Button_add_marker->setStyleSheet("QPushButton {background-color: green;\
                                  border-style: outset;\
                                  border-width: 2px;\
                                  border-radius: 10px;\
                                  border-color: beige;\
                                  font: bold 14px;\
                                  color: white;\
                                  min-width: 10em;\
                                  padding: 6px;\
                              }");
  connect(Button_add_marker, SIGNAL(clicked()), SLOT(addMarker())); // Connect button with the handler
  MarkersGrid->addWidget(Button_add_marker, 0, 0);

  Button_Remove_All_Markers = new QPushButton("Remove all");
  Button_Remove_All_Markers->setStyleSheet("QPushButton {background-color: red;\
                                  border-style: outset;\
                                  border-width: 2px;\
                                  border-radius: 10px;\
                                  border-color: beige;\
                                  font: bold 14px;\
                                  color: white;\
                                  min-width: 10em;\
                                  padding: 6px;\
                              }");
  connect(Button_Remove_All_Markers, SIGNAL(clicked()), SLOT(removeAllMarkers())); // Connect button with the handler
  MarkersGrid->addWidget(Button_Remove_All_Markers, 0, 1);

  // Marker management
  QWidget* MarkerList_Widget = new QWidget(); // Panel with the trace settings

  QLabel* Label_Marker = new QLabel("<b>Marker</b>");
  QLabel* Label_Freq_Marker = new QLabel("<b>Frequency</b>");
  QLabel* Label_Freq_Scale_Marker = new QLabel("<b>Units</b>");
  QLabel* Label_Remove_Marker = new QLabel("<b>Remove</b>");

  MarkersGrid = new QGridLayout(MarkerList_Widget);
  MarkersGrid->addWidget(Label_Marker, 0, 0, Qt::AlignCenter);
  MarkersGrid->addWidget(Label_Freq_Marker, 0, 1, Qt::AlignCenter);
  MarkersGrid->addWidget(Label_Freq_Scale_Marker, 0, 2, Qt::AlignCenter);
  MarkersGrid->addWidget(Label_Remove_Marker, 0, 3, Qt::AlignCenter);

  QScrollArea* scrollArea_Marker = new QScrollArea();
  scrollArea_Marker->setWidget(MarkerList_Widget);
  scrollArea_Marker->setWidgetResizable(true);

  // Create tab widget to hold the two marker tables
  QTabWidget* tabWidgetMarkers = new QTabWidget();

  // Create the two tables for different marker types
  tableMarkers_Magnitude_Phase = new QTableWidget(1, 1, this);
  tableMarkers_Smith = new QTableWidget(1, 1, this);

  // Add tables to tabs
  tabWidgetMarkers->addTab(tableMarkers_Magnitude_Phase, "Magnitude/Phase");
  tabWidgetMarkers->addTab(tableMarkers_Smith, "Smith Chart");

  Markers_VBox->addWidget(MarkerSelection_Widget);
  Markers_VBox->addWidget(scrollArea_Marker);
  Markers_VBox->addWidget(tabWidgetMarkers); // Add tab widget instead of single table

  dockMarkers->setWidget(MarkersGroup);
}

void Qucs_S_SPAR_Viewer::setLimitManagementDock(){
  // Limits dock
  dockLimits = new QDockWidget("Limits", this);

  QWidget * LimitsGroup = new QWidget();
  QVBoxLayout *Limits_VBox = new QVBoxLayout(LimitsGroup);

  // Limit addition box
  QWidget * AddLimit_Widget = new QWidget(); // Add trace

  LimitsGrid = new QGridLayout(AddLimit_Widget);

  Button_add_Limit = new QPushButton("Add Limit");
  Button_add_Limit->setStyleSheet("QPushButton {background-color: green;\
                                  border-style: outset;\
                                  border-width: 2px;\
                                  border-radius: 10px;\
                                  border-color: beige;\
                                  font: bold 14px;\
                                  color: white;\
                                  min-width: 10em;\
                                  padding: 6px;\
                              }");
  connect(Button_add_Limit, SIGNAL(clicked()), SLOT(addLimit())); // Connect button with the handler
  LimitsGrid->addWidget(Button_add_Limit, 0, 0);


  Button_Remove_All_Limits = new QPushButton("Remove all");
  Button_Remove_All_Limits->setStyleSheet("QPushButton {background-color: red;\
                                  border-style: outset;\
                                  border-width: 2px;\
                                  border-radius: 10px;\
                                  border-color: beige;\
                                  font: bold 14px;\
                                  color: white;\
                                  min-width: 10em;\
                                  padding: 6px;\
                              }");
  connect(Button_Remove_All_Limits, SIGNAL(clicked()), SLOT(removeAllLimits())); // Connect button with the handler
  LimitsGrid->addWidget(Button_Remove_All_Limits, 0, 1);

  QGroupBox * LimitSettings = new QGroupBox("Settings");
  QGridLayout * LimitsSettingLayout = new QGridLayout(LimitSettings);
  QLabel * LimitsOffsetLabel = new QLabel("<b>Limits Offset</>");
  Limits_Offset = new QDoubleSpinBox();
  Limits_Offset->setValue(0);
  Limits_Offset->setSingleStep(0.1);
  Limits_Offset->setMaximum(1e4);
  Limits_Offset->setMinimum(-1e4);
  connect(Limits_Offset, SIGNAL(valueChanged(double)), SLOT(updateTraces()));
  LimitsSettingLayout->addWidget(LimitsOffsetLabel, 0, 0);
  LimitsSettingLayout->addWidget(Limits_Offset, 0, 1);

  // Limit management
  QWidget * LimitList_Widget = new QWidget(); // Panel with the trace settings

  QLabel * Label_Limit = new QLabel("<b>Limit</b>");
  QLabel * Label_Limit_Start = new QLabel("<b>Start</b>");
  QLabel * Label_Limit_Stop = new QLabel("<b>Stop</b>");
  QLabel * Label_Remove_Limit = new QLabel("<b>Remove</b>");

  LimitsGrid = new QGridLayout(LimitList_Widget);
  LimitsGrid->addWidget(Label_Limit, 0, 0, Qt::AlignCenter);
  LimitsGrid->addWidget(Label_Limit_Start, 0, 1, 1, 2, Qt::AlignCenter);
  LimitsGrid->addWidget(Label_Limit_Stop, 0, 3, 1, 2, Qt::AlignCenter);
  LimitsGrid->addWidget(Label_Remove_Limit, 0, 5, Qt::AlignCenter);

  QScrollArea *scrollArea_Limits = new QScrollArea();
  scrollArea_Limits->setWidget(LimitList_Widget);
  scrollArea_Limits->setWidgetResizable(true);

  Limits_VBox->addWidget(AddLimit_Widget);
  Limits_VBox->addWidget(LimitSettings);
  Limits_VBox->addWidget(scrollArea_Limits);

  dockLimits->setWidget(LimitsGroup);
}




void Qucs_S_SPAR_Viewer::CreateDisplayWidgets(){
  // Chart settings
  Magnitude_PhaseChart = new RectangularPlotWidget(this);
  dockChart = new QDockWidget("Magnitude / Phase", this);
  dockChart->setWidget(Magnitude_PhaseChart);
  dockChart->setAllowedAreas(Qt::AllDockWidgetAreas);
  addDockWidget(Qt::LeftDockWidgetArea, dockChart);

         // Smith Chart
  smithChart = new SmithChartWidget(this);
  smithChart->setMinimumSize(300, 300);
  dockSmithChart = new QDockWidget("Smith Chart", this);
  dockSmithChart->setWidget(smithChart);
  dockSmithChart->setAllowedAreas(Qt::AllDockWidgetAreas);
  addDockWidget(Qt::LeftDockWidgetArea, dockSmithChart);

  // Disable dock closing
  dockChart->setFeatures(dockChart->features() & ~QDockWidget::DockWidgetClosable);
  dockSmithChart->setFeatures(dockSmithChart->features() & ~QDockWidget::DockWidgetClosable);

  // Tabify the chart docks
  tabifyDockWidget(dockChart, dockSmithChart);
}

void Qucs_S_SPAR_Viewer::setupScrollAreaForLayout(QGridLayout* &layout, QWidget* parentTab, const QString &objectName)
{
  // Save the original layout and its widgets
  QGridLayout* originalLayout = layout;
  QList<QWidget*> headerWidgets;

  // Save header row (assuming row 0 is the header)
  for (int col = 0; col < 5; col++) {
    QLayoutItem* item = originalLayout->itemAtPosition(0, col);
    if (item && item->widget()) {
      QWidget* widget = item->widget();
      headerWidgets.append(widget);
      originalLayout->removeWidget(widget);
    }
  }

  // Delete the original layout (but not its widgets)
  delete originalLayout;

  // Create a scroll area
  QScrollArea* scrollArea = new QScrollArea(parentTab);
  scrollArea->setObjectName(objectName);
  scrollArea->setWidgetResizable(true);
  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  // Create a widget to hold the new layout
  QWidget* container = new QWidget();

  // Create a new grid layout for the container
  layout = new QGridLayout(container);

  // Restore header widgets to the new layout
  for (int col = 0; col < headerWidgets.size(); col++) {
    if (col < headerWidgets.size()) {
      layout->addWidget(headerWidgets[col], 0, col);
    }
  }

  // Set the widget to the scroll area
  scrollArea->setWidget(container);

  // Create a new layout for the tab to hold the scroll area
  QVBoxLayout* tabLayout = new QVBoxLayout(parentTab);
  tabLayout->addWidget(scrollArea);
  tabLayout->setContentsMargins(0, 0, 0, 0);

  // Store the scroll area reference for later access
  if (objectName == "magnitudePhaseScrollArea") {
    magnitudePhaseScrollArea = scrollArea;
  } else if (objectName == "smithScrollArea") {
    smithScrollArea = scrollArea;
  }
}

void Qucs_S_SPAR_Viewer::setupScrollableLayout()
{
  // Create scroll areas for both layouts
  setupScrollAreaForLayout(magnitudePhaseLayout, magnitudePhaseTab, "magnitudePhaseScrollArea");
  setupScrollAreaForLayout(smithLayout, smithTab, "smithScrollArea");
}

Qucs_S_SPAR_Viewer::~Qucs_S_SPAR_Viewer()
{
  QSettings settings;
  settings.setValue("recentFiles", QVariant::fromValue(recentFiles));
  delete smithChart;
}

void Qucs_S_SPAR_Viewer::slotHelpIntro()
{
  QMessageBox::about(this, tr("Qucs-S S-parameter Help"),
    tr("This is a simple viewer for S-parameter data.\n"
         "It can show several .snp files at a time in the "
         "same diagram. Trace markers can also be added "
         "so that the user can read the trace value at "
         "at an specific frequency."));
}

void Qucs_S_SPAR_Viewer::slotHelpAboutQt()
{
      QMessageBox::aboutQt(this, tr("About Qt"));
}

void Qucs_S_SPAR_Viewer::slotHelpAbout()
{
    QMessageBox::about(this, tr("About..."),
    "Qucs-S S-parameter Viewer Version " PACKAGE_VERSION+
    tr("\nCopyright (C) 2024 by")+" Andrés Martínez Mera"
    "\n"
    "\nThis is free software; see the source for copying conditions."
    "\nThere is NO warranty; not even for MERCHANTABILITY or "
    "\nFITNESS FOR A PARTICULAR PURPOSE.\n\n");
}

void Qucs_S_SPAR_Viewer::slotQuit()
{
  qApp->quit();
}


void Qucs_S_SPAR_Viewer::addFile()
{
    QFileDialog dialog(this, QStringLiteral("Select S-parameter data files (.snp)"), QDir::homePath(),
                       tr("S-Parameter Files (*.s1p *.s2p *.s3p *.s4p);;All Files (*.*)"));
    dialog.setFileMode(QFileDialog::ExistingFiles);

    QStringList fileNames;
    if (dialog.exec())
        fileNames = dialog.selectedFiles();

    addFiles(fileNames);
}

void Qucs_S_SPAR_Viewer::addFiles(QStringList fileNames)
{
    int existing_files = this->datasets.size(); // Get the number of entries in the map

    // Variables for reading the Touchstone data
    QStringList values;
    QString filename;

    if (existing_files == 0){
        // Reset limits
        this->f_max = -1;
        this->f_min = 1e30;
    }


    // Remove from the list of files those that already exist in the database
    QStringList files_dataset = datasets.keys();

    for (int i = 0; i < fileNames.length(); i++){
      filename = QFileInfo(fileNames.at(i)).fileName();
      // Check if this file already exists
      QString new_filename = filename.left(filename.lastIndexOf('.'));
      if (files_dataset.contains(new_filename)){
        // Remove it from the list of new files to load
        fileNames.removeAt(i);

        // Pop up a warning
        QMessageBox::information(
            this,
            tr("Warning"),
            tr("This file is already in the dataset.") );

      }
    }

    // Read files
    for (int i = existing_files; i < existing_files+fileNames.length(); i++)
    {
        // Create the file name label
        filename = QFileInfo(fileNames.at(i-existing_files)).fileName();

        QLabel * Filename_Label = new QLabel(filename.left(filename.lastIndexOf('.')));
        Filename_Label->setObjectName(QStringLiteral("File_") + QString::number(i));
        List_FileNames.append(Filename_Label);
        this->FilesGrid->addWidget(List_FileNames.last(), i,0,1,1);

        // Create the "Remove" button
        QToolButton * RemoveButton = new QToolButton();
        RemoveButton->setObjectName(QStringLiteral("Remove_") + QString::number(i));
        QIcon icon(":/bitmaps/trash.png"); // Use a resource path or a relative path
        RemoveButton->setIcon(icon);

        RemoveButton->setStyleSheet("QToolButton {background-color: red;\
                                        border-width: 2px;\
                                        border-radius: 10px;\
                                        border-color: beige;\
                                        font: bold 14px;\
                                        margin: auto;\
                                    }");

        List_RemoveButton.append(RemoveButton);
        this->FilesGrid->addWidget(List_RemoveButton.last(), i,1,1,1);


        connect(RemoveButton, SIGNAL(clicked()), SLOT(removeFile())); // Connect button with the handler to remove the entry.

        // Read the Touchstone file.
        // Please see https://ibis.org/touchstone_ver2.0/touchstone_ver2_0.pdf
        QMap<QString, QList<double>> file_data; // Data structure to store the file data
        QString frequency_unit, parameter, format;
        double freq_scale = 1; // Hz
        double Z0=50; // System impedance. Typically 50 Ohm

        // Get the number of ports
        QString suffix = QFileInfo(filename).suffix();
        QRegularExpression regex("(?i)[sp]");
        QStringList numberParts = suffix.split(regex);
        int number_of_ports = numberParts[1].toInt();
        file_data["n_ports"].append(number_of_ports);

        // 1) Open the file
        QFile file(fileNames.at(i-existing_files));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Cannot open the file";
            break;
        }

        // 2) Read data
        QTextStream in(&file);
        while (!in.atEnd()) {
          QString line = in.readLine();
          line = line.simplified();
          //qDebug() << line;

           if (line.isEmpty()) continue;
           if ((line.at(0).isNumber() == false) && (line.at(0) != '#')) {
               if (file_data["frequency"].size() == 0){
                   // There's still no data
                   continue;
               }else{
                   //There's already data, so it's very likely that the S-par data has ended and
                   //the following lines contain noise data. We must stop at this point.
                   break;
               }

           }
           // Check for the option line
           if (line.at(0) == '#'){

               QStringList info = line.split(" ");
               frequency_unit = info.at(1); // Specifies the unit of frequency.
                                            // Legal values are Hz, kHz, MHz, and GHz. The default value is GHz.

               frequency_unit = frequency_unit.toLower();

               if (frequency_unit == "khz"){
                   freq_scale = 1e3;
               } else {
                   if (frequency_unit == "mhz"){
                       freq_scale = 1e6;
                   } else {
                       if (frequency_unit == "ghz"){
                           freq_scale = 1e9;
                       }
                   }
               }


               parameter = info.at(2); // specifies what kind of network parameter data is contained in the file. Legal
                                       // values are:
                                       // S for Scattering parameters,
                                       // Y for Admittance parameters,
                                       // Z for Impedance parameters,
                                       // H for Hybrid-h parameters,
                                       // G for Hybrid-g parameters.
                                       // The default value is S.

               format = info.at(3);   // Specifies the format of the network parameter data pairs. Legal values are:
                                      // DB for decibel-angle (decibel = 20 × log 10|magnitude|)
                                      // MA for magnitude-angle,
                                      // RI for real-imaginary.
                                      // Angles are given in degrees. Note that this format does not apply to noise
                                      // parameters (refer to the “Noise Parameter Data” section later in this
                                      // specification). The default value is MA.

               Z0 = info.at(5).toDouble();
               file_data["Z0"].append(Z0); // Specifies the reference resistance in ohms, where n is a real, positive number of
                                           // ohms. The default reference resistance is 50 ohms. Note that this is overridden
                                           // by the [Reference] keyword, described below, for files of [Version] 2.0 and above

           continue;
           }

           // Split line by whitespace
           values.clear();
           values = line.split(' ');

           file_data["frequency"].append(values[0].toDouble()*freq_scale); // in Hz

           double S_1, S_2, S_3, S_4;
           QString s1, s2, s3, s4;
           int index = 1, data_counter = 0;

           for (int i = 1; i<=number_of_ports; i++){
               for (int j = 1; j<=number_of_ports; j++){
                   s1 = QStringLiteral("S") + QString::number(j) + QString::number(i) + QStringLiteral("_dB");
                   s2 = s1.mid(0, s1.length() - 2).append("ang");
                   s3 = s1.mid(0, s1.length() - 2).append("re");
                   s4 = s1.mid(0, s1.length() - 2).append("im");

                   S_1 = values[index].toDouble();
                   S_2 = values[index+1].toDouble();

                   convert_MA_RI_to_dB(&S_1, &S_2, &S_3, &S_4, format);

                   file_data[s1].append(S_1);//dB
                   file_data[s2].append(S_2);//ang
                   file_data[s3].append(S_3);//re
                   file_data[s4].append(S_4);//im
                   index += 2;
                   data_counter++;

                   // Check if the next values are in the new line
                   if ((index >= values.length()) && (data_counter < number_of_ports*number_of_ports)){
                       line = in.readLine();
                       line = line.simplified();
                       values = line.split(' ');
                       index = 0; // Reset index (it's a new line)
                   }
               }
           }
           if (number_of_ports == 1){
               double s11_re = file_data["S11_re"].last();
               double s11_im = file_data["S11_im"].last();
               std::complex<double> s11 (s11_re, s11_im);

               // Optional traces. They are not computed now, but only if the user wants to display them
               QStringList optional_traces;
               optional_traces.append("Re{Zin}");
               optional_traces.append("Im{Zin}");
               for (int i = 0; i < optional_traces.size(); i++) {
                 if (!file_data.contains(optional_traces[i])) {
                   // If not, create an empty list
                   file_data[optional_traces[i]] = QList<double>();
                 }
               }

           }
           if (number_of_ports == 2){
               // Optional traces. They are not computed now, but only if the user wants to display them
               QStringList optional_traces;
               optional_traces.append("delta");
               optional_traces.append("K");
               optional_traces.append("mu");
               optional_traces.append("mu_p");
               optional_traces.append("MSG");
               optional_traces.append("MAG");
               optional_traces.append("Re{Zin}");
               optional_traces.append("Im{Zin}");
               optional_traces.append("Re{Zout}");
               optional_traces.append("Im{Zout}");
               for (int i = 0; i < optional_traces.size(); i++) {
                 if (!file_data.contains(optional_traces[i])) {
                   // If not, create an empty list
                   file_data[optional_traces[i]] = QList<double>();
                 }
               }
           }
        }
        // 3) Add data to the dataset
        filename = filename.left(filename.lastIndexOf('.')); // Remove file extension
        datasets[filename] = file_data;
        file.close();

        // 4) Add new dataset to the trace selection combobox
        QCombobox_datasets->addItem(filename);
        // Update traces
        updateTracesCombo();
    }

    // Default behavior: If there's no more data loaded and a single S1P file is selected, then automatically plot S11
    if ((fileNames.length() == 1) && (fileNames.first().toLower().endsWith(".s1p")) && (datasets.size() == 1)){
        this->addTrace(filename, QStringLiteral("S11_dB"), Qt::red);
        this->addTrace(filename, QStringLiteral("S11_Smith"), Qt::darkBlue);
    }

    // Default behavior: If there's no more data loaded and a single S2P file is selected, then automatically plot S21, S11 and S22
    if ((fileNames.length() == 1) && (fileNames.first().toLower().endsWith(".s2p")) && (datasets.size() == 1)){
      this->addTrace(filename, QStringLiteral("S21_dB"), Qt::red);
        this->addTrace(filename, QStringLiteral("S11_dB"), Qt::darkBlue);
        this->addTrace(filename, QStringLiteral("S22_dB"), Qt::darkGreen);

        this->addTrace(filename, QStringLiteral("S11_Smith"), Qt::darkBlue);
        this->addTrace(filename, QStringLiteral("S22_Smith"), Qt::darkGreen);
    }

    // Default behaviour: When adding multiple S2P file, then show the S21 of all traces
    if (fileNames.length() > 1){
        bool all_s2p = true;
        for (int i = 0; i < fileNames.length(); i++){
            if (!fileNames.at(i).toLower().endsWith(".s2p")){
                all_s2p = false;
                break;
            }
        }
        if (all_s2p == true){
            QString filename;
            for (int i = 0; i < fileNames.length(); i++){
                filename = QFileInfo(fileNames.at(i)).fileName();
                filename = filename.left(filename.lastIndexOf('.'));
                // Pick a random color
                QColor trace_color = QColor(QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256));
                this->addTrace(filename, QStringLiteral("S21_dB"), trace_color);
            }
        }
    }

    // Show the trace settings widget
    dockTracesList->raise();
}

// This function is called whenever a s-par file is intended to be removed from the map of datasets
void Qucs_S_SPAR_Viewer::removeFile()
{
    QString ID = qobject_cast<QToolButton*>(sender())->objectName();
    //qDebug() << "Clicked button:" << ID;

    //Find the index of the button to remove
    int index_to_delete = -1;
    for (int i = 0; i < List_RemoveButton.size(); i++) {
        if (List_RemoveButton.at(i)->objectName() == ID) {
            index_to_delete = i;
            break;
        }
    }

    removeFile(index_to_delete);
}


void Qucs_S_SPAR_Viewer::removeFile(int index_to_delete)
{
    // Delete the label
    QLabel* labelToRemove = List_FileNames.at(index_to_delete);
    QString dataset_to_remove = labelToRemove->text();
    FilesGrid->removeWidget(labelToRemove);
    List_FileNames.removeAt(index_to_delete);
    delete labelToRemove;

    // Delete the button
    QToolButton* ButtonToRemove = List_RemoveButton.at(index_to_delete);
    FilesGrid->removeWidget(ButtonToRemove);
    List_RemoveButton.removeAt(index_to_delete);
    delete ButtonToRemove;

    // Look for the widgets associated to the trace and remove them
    QStringList List_TraceNames = traceMap.keys();
    for (int i = 0; i < List_TraceNames.size(); i++){
        QString trace_name = List_TraceNames.at(i);
        QStringList parts = {
            trace_name.section('.', 0, -2),
            trace_name.section('.', -1)
        };
        QString dataset_trace = parts[0];
        if (dataset_trace == dataset_to_remove ){
          TraceProperties& props = traceMap[trace_name];

          // Delete all widgets
          delete props.nameLabel;
          delete props.colorButton;
          delete props.deleteButton;
          delete props.LineStyleComboBox;
          delete props.width;

          // Remove from the map
          traceMap.remove(trace_name);
        }
    }


    // Delete the dataset map entry
    datasets.remove(dataset_to_remove);

    // Rebuild the dataset combobox based on the available datasets.
    QStringList new_dataset_entries = datasets.keys();

    disconnect(QCombobox_datasets, SIGNAL(currentIndexChanged(int)), this, SLOT(updateTracesCombo())); // Needed to avoid segfault
    QCombobox_datasets->clear();
    QCombobox_datasets->addItems(new_dataset_entries);
    connect(QCombobox_datasets, SIGNAL(currentIndexChanged(int)), SLOT(updateTracesCombo())); // Connect the signal again

    // Update the combobox for trace selection
    updateTracesCombo();

    // Now it is needed to readjust the widgets in the grid layout
    // Move up all widgets below the removed row
    for (int r = index_to_delete+1; r < FilesGrid->rowCount(); r++) {
        for (int c = 0; c < FilesGrid->columnCount(); c++) {
            QLayoutItem* item = FilesGrid->itemAtPosition(r, c);
            if (item) {
                int oldRow, oldCol, rowSpan, colSpan;
                FilesGrid->getItemPosition(FilesGrid->indexOf(item), &oldRow, &oldCol, &rowSpan, &colSpan);
                FilesGrid->removeItem(item);
                FilesGrid->addItem(item, oldRow - 1, oldCol, rowSpan, colSpan);
            }
        }
    }

    // Check if there are more files. If not, remove markers and limits
    if (datasets.size() == 0)
    {
        removeAllMarkers();
        removeAllLimits();
    }
}

void Qucs_S_SPAR_Viewer::removeAllFiles()
{
    int n_files = List_RemoveButton.size();
    for (int i = 0; i < n_files; i++) {
        removeFile(n_files-i-1);
    }
}


// This function is called when the user wants to remove a trace from the plot
void Qucs_S_SPAR_Viewer::removeTrace()
{
    QString ID = qobject_cast<QToolButton*>(sender())->objectName();
    //qDebug() << "Clicked button:" << ID;

    //Find the index of the button to remove
    int ntraces = getNumberOfTraces();

    TraceProperties trace_props;
    QString trace_name;
    for (int i = 0; i < ntraces; i++) {
      getTraceByPosition(i, trace_name, trace_props);

      if (trace_props.deleteButton->objectName() == ID) {
          break;
      }
    }

    removeTrace(trace_name); // Remove trace by name
}

// This function is called when the user wants to remove a trace from the plot
void Qucs_S_SPAR_Viewer::removeTrace(QStringList trace_to_remove)
{
    if (trace_to_remove.isEmpty())
        return;

    for (int i = 0; i < trace_to_remove.size(); i++)
        removeTrace(trace_to_remove.at(i));
}

void Qucs_S_SPAR_Viewer::removeTrace(const QString& trace_to_remove)
{

  if (traceMap.contains(trace_to_remove)) {
    // Get the marker properties
    TraceProperties& props = traceMap[trace_to_remove];


           // Get the appropriate layout based on the trace type
    QGridLayout *targetLayout;
    if (trace_to_remove.endsWith("Smith")) {
      // Select the layout where Smith Chart traces are shown
      targetLayout = smithLayout;
    } else {
      if (trace_to_remove.endsWith("dB") || trace_to_remove.endsWith("ang")) {
        // Select the layout where Magnitude and Phase traces are shown
        targetLayout = magnitudePhaseLayout;
      }
    }

    // Delete all widgets
    targetLayout->removeWidget(props.nameLabel);
    delete props.nameLabel;

    targetLayout->removeWidget(props.colorButton);
    delete props.colorButton;

    targetLayout->removeWidget(props.LineStyleComboBox);
    delete props.LineStyleComboBox;

    targetLayout->removeWidget(props.width);
    delete props.width;

    targetLayout->removeWidget(props.deleteButton);
    delete props.deleteButton;

    // Remove from the map
    traceMap.remove(trace_to_remove);

    // Update the corresponding chart widget
    if (trace_to_remove.endsWith("Smith")){
      // Remove from the Smith Chart widget
      QString str = trace_to_remove;
      str.chop(6); // Remove the "_Smith" suffix
      smithChart->removeTrace(str);
      return;
    } else {
      // Magnitude and phase plot

      // Update the chart limits.
      this->f_max = -1;
      this->f_min = 1e30;

      updateGridLayout(TracesGrid);
    }


  }
}


bool Qucs_S_SPAR_Viewer::removeSeriesByName(QChart* chart, const QString& name)
{
    QList<QAbstractSeries*> seriesList = chart->series();
    for (QAbstractSeries* series : seriesList) {
        if (series->name() == name) {
            chart->removeSeries(series);
            return true; // Series found and removed
        }
    }
    return false; // Series not found
}


void Qucs_S_SPAR_Viewer::convert_MA_RI_to_dB(double * S_1, double * S_2, double *S_3, double *S_4, QString format)
{
    double S_dB = *S_1, S_ang =*S_2;
    double S_re = *S_3, S_im = *S_4;
    if (format == "MA"){
        S_dB = 20*log10(*S_1);
        S_ang = *S_2;
        S_re = *S_1 * std::cos(*S_2);
        S_im = *S_1 * std::sin(*S_2);
    }else{
        if (format == "RI"){
        S_dB = 20*log10(sqrt((*S_1)*(*S_1) + (*S_2)*(*S_2)));
        S_ang = atan2(*S_2, *S_1) * 180 / M_PI;
        S_re = *S_1;
        S_im = *S_2;
        } else {
            // DB format
            double r = std::pow(10, *S_1 / 10.0);
            double theta = *S_2 * M_PI / 180.0;
            S_re = r * std::cos(theta);
            S_im = r * std::sin(theta);
        }

    }
    *S_1 = S_dB;
    *S_2 = S_ang;
    *S_3 = S_re;
    *S_4 = S_im;
}

// Gets the frequency scale unit from a String lke kHz, MHz, GHz
double Qucs_S_SPAR_Viewer::getFreqScale(QString frequency_unit)
{
    double freq_scale=1;
    if (frequency_unit == "kHz"){
        freq_scale = 1e-3;
    } else {
        if (frequency_unit == "MHz"){
            freq_scale = 1e-6;
        } else {
            if (frequency_unit == "GHz"){
                freq_scale = 1e-9;
            }
        }
    }
    return freq_scale;
}

// This function is called when the user hits the button to add a trace
void Qucs_S_SPAR_Viewer::addTrace()
{
    QString selected_dataset, selected_trace, selected_view;
    selected_dataset = this->QCombobox_datasets->currentText();
    selected_trace = this->QCombobox_traces->currentText();
    selected_view = this->QCombobox_display_mode->currentText();

    QString suffix;
    if (selected_view.compare("n.u.")){
      if (!selected_view.compare("Phase")) {
        selected_view = QString("ang");
      }
      selected_trace += QString("_") + selected_view;
    }

    int linewidth = 1;
    if (!selected_view.compare("Smith")){
      linewidth = 3;
    }

    // Color settings
    QColor trace_color;
    QPen pen;
    int num_traces = trace_list.size();
    if (num_traces >= 3){
        trace_color = QColor(QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256));
        pen.setColor(trace_color);
    }
    else {
        trace_color = this->default_colors.at(num_traces);
    }

    addTrace(selected_dataset, selected_trace, trace_color, linewidth);
}


// Read the dataset and trace Comboboxes and add a trace to the display list
void Qucs_S_SPAR_Viewer::addTrace(QString selected_dataset, QString selected_trace, QColor trace_color, int trace_width, QString trace_style)
{
  int n_trace = this->trace_list.size() + 1; // Number of displayed traces

  // Get the name of the trace to plot
  QString trace_name = selected_dataset;
  trace_name.append("."); // Separate the dataset from the trace name with a point
  trace_name.append(selected_trace);

  if (trace_list.contains(trace_name)) {
    QMessageBox::information(
        this,
        tr("Warning"),
        tr("This trace is already shown"));
    return;
  } 

  // Get the appropriate layout based on the trace type
  QGridLayout *targetLayout;
  if (selected_trace.endsWith("Smith")) {
    // Select the layout where Smith Chart traces are shown
    targetLayout = smithLayout;
  } else {
    if (selected_trace.endsWith("dB") || selected_trace.endsWith("ang")) {
    // Select the layout where Magnitude and Phase traces are shown
      targetLayout = magnitudePhaseLayout;
    }
  }

         // Add the trace to the list of displayed list and create the widgets associated to the trace properties

  // Label
  QLabel *new_trace_label = new QLabel(trace_name);
  new_trace_label->setObjectName(QStringLiteral("Trace_Name_") + trace_name);

  traceMap[trace_name].nameLabel = new_trace_label;
  targetLayout->addWidget(new_trace_label, n_trace, 0);

  // Color picker
  QPushButton *new_trace_color = new QPushButton();
  new_trace_color->setObjectName(QStringLiteral("Trace_Color_") + trace_name);
  connect(new_trace_color, SIGNAL(clicked()), SLOT(changeTraceColor()));
  QString styleSheet = QStringLiteral("QPushButton { background-color: %1; }").arg(trace_color.name());
  new_trace_color->setStyleSheet(styleSheet);
  new_trace_color->setAttribute(Qt::WA_TranslucentBackground); // Needed for Windows buttons to behave as they should


  traceMap[trace_name].colorButton = new_trace_color;
  targetLayout->addWidget(new_trace_color, n_trace, 1);

         // Line Style
  QComboBox *new_trace_linestyle = new QComboBox();
  new_trace_linestyle->setObjectName(QStringLiteral("Trace_LineStyle_") + trace_name);
  new_trace_linestyle->addItem("Solid");
  new_trace_linestyle->addItem("- - - -");
  new_trace_linestyle->addItem("·······");
  new_trace_linestyle->addItem("-·-·-·-");
  new_trace_linestyle->addItem("-··-··-");
  int index = new_trace_linestyle->findText(trace_style);
  new_trace_linestyle->setCurrentIndex(index);
  connect(new_trace_linestyle, SIGNAL(currentIndexChanged(int)), SLOT(changeTraceLineStyle()));

  traceMap[trace_name].LineStyleComboBox = new_trace_linestyle;
  targetLayout->addWidget(new_trace_linestyle, n_trace, 2);

         // Capture the pen style to correctly render the trace
  Qt::PenStyle pen_style;
  if (!trace_style.compare("Solid")) {
    pen_style = Qt::SolidLine;
  } else if (!trace_style.compare("- - - -")) {
    pen_style = Qt::DashLine;
  } else if (!trace_style.compare("·······")) {
    pen_style = Qt::DotLine;
  } else if (!trace_style.compare("-·-·-·-")) {
    pen_style = Qt::DashDotLine;
  } else if (!trace_style.compare("-··-··-")) {
    pen_style = Qt::DashDotDotLine;
  }

         // Line width
  QSpinBox *new_trace_width = new QSpinBox();
  new_trace_width->setObjectName(QStringLiteral("Trace_Width_") + trace_name);
  new_trace_width->setValue(trace_width);
  connect(new_trace_width, SIGNAL(valueChanged(int)), SLOT(changeTraceWidth()));

  traceMap[trace_name].width = new_trace_width;
  targetLayout->addWidget(new_trace_width, n_trace, 3);

         // Remove button
  QToolButton *new_trace_removebutton = new QToolButton();
  new_trace_removebutton->setObjectName(QStringLiteral("Trace_RemoveButton_") + trace_name);
  QIcon icon(":/bitmaps/trash.png"); // Use a resource path or a relative path
  new_trace_removebutton->setIcon(icon);
  new_trace_removebutton->setStyleSheet(R"(
            QToolButton {
                background-color: #FF0000;
                color: white;
                border-radius: 20px;
                margin: auto;
            }
        )");
  connect(new_trace_removebutton, SIGNAL(clicked()), SLOT(removeTrace()));

  traceMap[trace_name].deleteButton = new_trace_removebutton;
  targetLayout->addWidget(new_trace_removebutton, n_trace, 4);

  // Create the series for the trace
  QLineSeries* series = new QLineSeries();
  series->setName(trace_name);
  trace_list.append(trace_name);

         // Color settings
  QPen pen;
  pen.setColor(trace_color);
  pen.setStyle(pen_style);
  pen.setWidth(trace_width);
  series->setPen(pen); // Apply the pen to the series

  if (selected_trace.contains("dB") || selected_trace.contains("ang")){
    // Magnitude / Phase rectangular diagram
    QList<double> trace_data = datasets[selected_dataset][selected_trace];
    QList<double> frequencies = datasets[selected_dataset]["frequency"];
    double Z0 = datasets[selected_dataset]["Z0"].first();

    // Add the trace to the chart
    RectangularPlotWidget::Trace new_trace;
    new_trace.trace = trace_data;
    new_trace.frequencies = frequencies;
    new_trace.pen = pen;
    new_trace.Z0 = Z0;
    Magnitude_PhaseChart->addTrace(trace_name, new_trace);

  } else {
    if (selected_trace.contains("Smith")){
      // Smith Chart
      // Convert S-parameters to impedances
      QList<std::complex<double>> impedances;
      QList<double> frequencies = datasets[selected_dataset]["frequency"];

      QString trace_dataset = selected_trace.replace("Smith", "");

      QList<double> sii_re = datasets[selected_dataset][trace_dataset + QString("re")];
      QList<double> sii_im = datasets[selected_dataset][trace_dataset + QString("im")];

      double Z0 = datasets[selected_dataset]["Z0"].first();

      for (int i = 0; i < frequencies.size(); i++) {
        std::complex<double> sii(sii_re[i], sii_im[i]);
        std::complex<double> gamma = sii; // Reflection coefficient
        std::complex<double> impedance = Z0 * (1.0 + gamma) / (1.0 - gamma); // Convert to impedance
        impedances.push_back(impedance);
      }

      // Set the impedance data to the Smith Chart widget
      SmithChartWidget::Trace new_trace;
      new_trace.impedances = impedances;
      new_trace.frequencies = frequencies;
      new_trace.pen = pen;
      new_trace.Z0 = Z0;

      SmithChartTraces.append(new_trace);

      trace_dataset.chop(1);
      QString TraceName = selected_dataset + QString(".") + trace_dataset;
      smithChart->addTrace(TraceName, new_trace);

    }
  }
}

// This function is used for setting the available traces depending on the selected dataset
void Qucs_S_SPAR_Viewer::updateTracesCombo()
{
  QCombobox_traces->clear();
  QStringList traces;
  QString current_dataset = QCombobox_datasets->currentText();
  if (current_dataset.isEmpty()){
    return; // No datasets loaded. This happens if the user had one single file and deleted it
  }

  int n_ports = datasets[current_dataset]["n_ports"].at(0);

  for (int i = 1; i <= n_ports; i++) {
    for (int j = 1; j <= n_ports; j++) {
      traces.append(QStringLiteral("S%1%2").arg(i).arg(j)); // Magnitude (dB)
    }
  }

  if (n_ports == 1) {
    // Additional traces
    traces.append("Re{Zin}");
    traces.append("Im{Zin}");
  }

  if (n_ports == 2) {
    // Additional traces
    traces.append(QStringLiteral("|%1|").arg(QChar(0x0394)));
    traces.append("K");
    traces.append(QStringLiteral("%1%2").arg(QChar(0x03BC)).arg(QChar(0x209B)));
    traces.append(QStringLiteral("%1%2").arg(QChar(0x03BC)).arg(QChar(0x209A)));
    traces.append("MAG");
    traces.append("MSG");
    traces.append("Re{Zin}");
    traces.append("Im{Zin}");
    traces.append("Re{Zout}");
    traces.append("Im{Zout}");
  }

  QCombobox_traces->addItems(traces);
}

// This is the handler that is triggered when the user hits the button to change the color of a given trace
void Qucs_S_SPAR_Viewer::changeTraceColor()
{
    QColor color = QColorDialog::getColor(Qt::white, this, "Select Color");
            if (color.isValid()) {
                // Do something with the selected color
                // For example, set the background color of the button
                QPushButton *button = qobject_cast<QPushButton*>(sender());
                if (button) {
                  QString styleSheet = QStringLiteral("QPushButton { background-color: %1; }").arg(color.name());
                  button->setStyleSheet(styleSheet);

                    QString ID = button->objectName();

                    int ntraces = getNumberOfTraces();

                    QString trace_name;
                    TraceProperties trace_props;
                    for (int i = 0; i < ntraces; i++) {
                      getTraceByPosition(i, trace_name, trace_props);

                      if (trace_props.colorButton->objectName() == ID) {
                          break;
                      }
                    }

                    if (trace_name.endsWith("_Smith")) {
                      // Smith Chart widget

                      // Extract the base trace name (remove the "_Smith" suffix)
                      QString traceName = trace_name.left(trace_name.length() - 6);

                      QPen currentPen = smithChart->getTracePen(traceName);

                      // Update the color while preserving color and style
                      currentPen.setColor(color);

                      // Set the modified pen back to the trace
                      smithChart->setTracePen(traceName, currentPen);


                    } else {
                      // Magnitude / Phase chart
                      QPen currentPen = Magnitude_PhaseChart->getTracePen(trace_name);
                      currentPen.setColor(color);
                      Magnitude_PhaseChart->setTracePen(trace_name, currentPen);
                    }
                }
            }
}

// This is the handler that is triggered when the user hits the button to change the line style of a given trace
void Qucs_S_SPAR_Viewer::changeTraceLineStyle()
{
    QComboBox *combo = qobject_cast<QComboBox*>(sender());
    QString ID = combo->objectName();

    int ntraces = getNumberOfTraces();
    QString trace_name;
    TraceProperties trace_props;
    for (int i = 0; i < ntraces; i++) {
      getTraceByPosition(i, trace_name, trace_props);
      if (trace_props.LineStyleComboBox->objectName() == ID) {
          break;
      }
    }

    // New trace line style
    enum Qt::PenStyle PenStyle;
    switch (combo->currentIndex()) {
      case 0: // Solid
        PenStyle = Qt::SolidLine;
        break;
      case 1: // Dashed
        PenStyle = Qt::DashLine;
        break;
      case 2: // Dotted
        PenStyle = Qt::DotLine;
        break;
      case 3: // Dash Dot
        PenStyle = Qt::DashDotLine;
        break;
      case 4: // Dash Dot Dot Line
        PenStyle = Qt::DashDotDotLine;
        break;
    }


    // Get the QPen of the trace from the corresponding display widget and update the width

    if (trace_name.endsWith("_Smith")) {
      // Smith Chart

      // Extract the base trace name (remove the "_Smith" suffix)
      QString traceName = trace_name.left(trace_name.length() - 6);
      QPen currentPen = smithChart->getTracePen(traceName);

       // Update the color while preserving color and style
      currentPen.setStyle(PenStyle);

      // Set the modified pen back to the trace
      smithChart->setTracePen(traceName, currentPen);

    } else {
      // Magnitude / Phase chart
      QPen currentPen = Magnitude_PhaseChart->getTracePen(trace_name);
      currentPen.setStyle(PenStyle);
      Magnitude_PhaseChart->setTracePen(trace_name, currentPen);
    }
}

// This is the handler that is triggered when the user hits the button to change the line width of a given trace
void Qucs_S_SPAR_Viewer::changeTraceWidth() {
  QSpinBox *spinbox = qobject_cast<QSpinBox*>(sender());
  QString ID = spinbox->objectName();

  int ntraces = getNumberOfTraces();
  QString trace_name;
  TraceProperties trace_props;
  for (int i = 0; i < ntraces; i++) {
    getTraceByPosition(i, trace_name, trace_props);
    if (trace_props.width->objectName() == ID) {
      break;
    }
  }

  int TraceWidth = spinbox->value();

  // Get the QPen of the trace from the corresponding display widget and update the width
  if (trace_name.endsWith("_Smith")) {
    // Smith Chart
    QString traceName = trace_name.left(trace_name.length() - 6);
    QPen currentPen = smithChart->getTracePen(traceName);
    currentPen.setWidth(TraceWidth);
    smithChart->setTracePen(traceName, currentPen);
  } else {
    // Magnitude / Phase chart
    QPen currentPen = Magnitude_PhaseChart->getTracePen(trace_name);
    currentPen.setWidth(TraceWidth);
    Magnitude_PhaseChart->setTracePen(trace_name, currentPen);
  }
}


// Given a trace, it gives the minimum and the maximum values at both axis.
void Qucs_S_SPAR_Viewer::getMinMaxValues(QString filename, QString tracename, qreal& minX, qreal& maxX, qreal& minY, qreal& maxY) {
    // Find the minimum and the maximum in the x-axis
    QList<double> freq = datasets[filename]["frequency"];
    minX = freq.first();
    maxX = freq.last();

    // Find minimum and maximum in the y-axis
    QList<double> trace_data = datasets[filename][tracename];

    auto minIterator = std::min_element(trace_data.begin(), trace_data.end());
    auto maxIterator = std::max_element(trace_data.begin(), trace_data.end());

    minY = *minIterator;
    maxY = *maxIterator;

    if (tracename.endsWith("_ang")) {
      // Phase traces range from -180 to 180 degrees
      minY = -180;
      maxY = 180;
    } else {
      // Magnitude traces (dB)
      auto minIterator = std::min_element(trace_data.begin(), trace_data.end());
      auto maxIterator = std::max_element(trace_data.begin(), trace_data.end());
      minY = *minIterator;
      maxY = *maxIterator;
    }

}

int Qucs_S_SPAR_Viewer::findClosestIndex(const QList<double>& list, double value)
{
    return std::min_element(list.begin(), list.end(),
        [value](double a, double b) {
            return std::abs(a - value) < std::abs(b - value);
        }) - list.begin();
}


void Qucs_S_SPAR_Viewer::addMarker(double freq){

    // If there are no traces in the display, show a message and exit
    if (trace_list.size() == 0){
      QMessageBox::information(
          this,
          tr("Warning"),
          tr("The display contains no traces.") );
      return;
    }

    double f_marker;
    if (freq == -1) {
      // There's no specific frequency argument, then pick the middle point
      double f1 = Magnitude_PhaseChart->getXmin();
      double f2 = Magnitude_PhaseChart->getXmax();
      f_marker = f1 + 0.5*(f2-f1);
    } else {
      f_marker= freq;
      f_marker *= 1e-6;// Scale according to the x-axis units
    }
    QString Freq_Marker_Scale = QString("MHz");

    int n_markers = getNumberOfMarkers();
    n_markers++;

    MarkerProperties props; // Struct to hold Marker widgets

    QString new_marker_name = QStringLiteral("Mkr%1").arg(n_markers);
    QLabel * new_marker_label = new QLabel(new_marker_name);
    new_marker_label->setObjectName(new_marker_name);
    props.nameLabel = new_marker_label;

    this->MarkersGrid->addWidget(new_marker_label, n_markers, 0);

    QString SpinBox_name = QStringLiteral("Mkr_SpinBox%1").arg(n_markers);
    QDoubleSpinBox * new_marker_Spinbox = new QDoubleSpinBox();
    new_marker_Spinbox->setObjectName(SpinBox_name);
    new_marker_Spinbox->setMinimum(Magnitude_PhaseChart->getXmin());
    new_marker_Spinbox->setMaximum(Magnitude_PhaseChart->getXmax());
    new_marker_Spinbox->setDecimals(1);
    new_marker_Spinbox->setValue(f_marker);
    connect(new_marker_Spinbox, SIGNAL(valueChanged(double)), SLOT(updateMarkerTable()));
    props.freqSpinBox = new_marker_Spinbox;
    this->MarkersGrid->addWidget(new_marker_Spinbox, n_markers, 1);

    QString Combobox_name = QStringLiteral("Mkr_ComboBox%1").arg(n_markers);
    QComboBox * new_marker_Combo = new QComboBox();
    new_marker_Combo->setObjectName(Combobox_name);
    new_marker_Combo->addItems(frequency_units);
    new_marker_Combo->setCurrentIndex(Magnitude_PhaseChart->getFreqIndex());
    connect(new_marker_Combo, SIGNAL(currentIndexChanged(int)), SLOT(changeMarkerLimits()));
    props.scaleComboBox = new_marker_Combo;
    this->MarkersGrid->addWidget(new_marker_Combo, n_markers, 2);

    // Remove button
    QString DeleteButton_name = QStringLiteral("Mkr_Delete_Btn%1").arg(n_markers);
    QToolButton * new_marker_removebutton = new QToolButton();
    new_marker_removebutton->setObjectName(DeleteButton_name);
    QIcon icon(":/bitmaps/trash.png"); // Use a resource path or a relative path
    new_marker_removebutton->setIcon(icon);
    new_marker_removebutton->setStyleSheet(R"(
            QToolButton {
                background-color: #FF0000;
                color: white;
                border-radius: 20px;
            }
        )");
    connect(new_marker_removebutton, SIGNAL(clicked()), SLOT(removeMarker()));
    props.deleteButton = new_marker_removebutton;
    this->MarkersGrid->addWidget(new_marker_removebutton, n_markers, 3, Qt::AlignCenter);

    // Add marker widgets to the marker map
    markerMap[new_marker_name] = props;


    // Add new entry to the table
    QString new_freq = QStringLiteral("%1 ").arg(QString::number(f_marker, 'f', 2)) + Freq_Marker_Scale;
    QTableWidgetItem *newfreq = new QTableWidgetItem(new_freq);

    if(new_marker_name.endsWith("dB") || new_marker_name.endsWith("ang")){
      // Magnitude / phase marker
      int n_markers = tableMarkers_Magnitude_Phase->rowCount() + 1;
      tableMarkers_Magnitude_Phase->setRowCount(n_markers);
      tableMarkers_Magnitude_Phase->setRowCount(n_markers);
      tableMarkers_Magnitude_Phase->setItem(n_markers-1, 0, newfreq);
    } else {
      // Smith chart marker
      int n_markers = tableMarkers_Smith->rowCount() + 1;
      tableMarkers_Smith->setRowCount(n_markers);
      tableMarkers_Smith->setRowCount(n_markers);
      tableMarkers_Smith->setItem(n_markers-1, 0, newfreq);
    }

    changeMarkerLimits(Combobox_name);

    f_marker = getMarkerFreq(new_marker_name);

    // Define QPen
    QPen pen;
    pen.setColor(Qt::black);
    pen.setWidth(1);
    pen.setStyle(Qt::SolidLine);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCosmetic(true);

    // Add marker to the magnitude / phase plot
    Magnitude_PhaseChart->addMarker(new_marker_name, f_marker, pen);

    // Add marker to Smith Chart
    smithChart->addMarker(new_marker_name, f_marker);

}


void Qucs_S_SPAR_Viewer::updateMarkerTable(){

    //If there are no markers, remove the entries and return
  int n_markers = getNumberOfMarkers();
    if (n_markers == 0){
        tableMarkers_Magnitude_Phase->clear();
        tableMarkers_Magnitude_Phase->setColumnCount(0);
        tableMarkers_Magnitude_Phase->setRowCount(0);

        tableMarkers_Smith->clear();
        tableMarkers_Smith->setColumnCount(0);
        tableMarkers_Smith->setRowCount(0);
        return;
    }

    //Ensure that the size of the table is correct
    QList<QAbstractSeries *> seriesList; // TO DO: Get the name of all traces displayed

    // Reset headers
    QStringList header_Magnitude_Phase;
    header_Magnitude_Phase.clear();
    header_Magnitude_Phase.append("freq");

    QStringList header_Smith;
    header_Smith.clear();
    header_Smith.append("freq");

    // Build headers
    int n_traces = getNumberOfTraces();
    for (int i = 0; i < n_traces; i++) {
      // Find the name of the traces from the traces map and add them to a list.
      // Then use that list to build the header of the table
      QString trace_name;
      TraceProperties trace_props;
      getTraceByPosition(i, trace_name, trace_props);

      if (trace_name.endsWith("_dB") || trace_name.endsWith("_ang")) {
        // Magnitude / Phase
        header_Magnitude_Phase.append(trace_name);
      } else {
        // Smith
        header_Smith.append(trace_name);
      }
    }


    // Update marker data

    // Magnitude and phase table
    updateMarkerData(*tableMarkers_Magnitude_Phase, header_Magnitude_Phase);
    // Smith Chart table
    updateMarkerData(*tableMarkers_Smith, header_Smith);


    // Update markers
    QStringList marker_list = markerMap.keys();
    for (const QString &str : marker_list) {
      double marker_freq = getMarkerFreq(str);
      smithChart->updateMarkerFrequency(str, marker_freq); // Update Smith Chart markers
      Magnitude_PhaseChart->updateMarkerFrequency(str, marker_freq); // Update magnitude / phase markers
    }


}


// Fill the different marker tables
void Qucs_S_SPAR_Viewer::updateMarkerData(QTableWidget& table, QStringList header){

  QPointF P;
  qreal targetX;
  QString new_val;
  QString freq_marker;

  int n_markers = getNumberOfMarkers();
  int n_traces = header.size();
  table.setColumnCount(n_traces);
  table.setRowCount(n_markers);
  table.setHorizontalHeaderLabels(header);

  for (int c = 0; c<n_traces; c++){//Traces
    for (int r = 0; r<n_markers; r++){//Marker
      QString markerName;
      MarkerProperties mkr_props;
      getMarkerByPosition(r, markerName, mkr_props); // Get the whole marker given the position

             // Compose the marker text
      freq_marker = QStringLiteral("%1 ").arg(QString::number(mkr_props.freqSpinBox->value(), 'f', 1)) + mkr_props.scaleComboBox->currentText();

      if (c==0){
        // First column
        QTableWidgetItem *new_item = new QTableWidgetItem(freq_marker);
        table.setItem(r, c, new_item);
        continue;
      }
      targetX = getFreqFromText(freq_marker);


      QString trace_name = header.at(c);

      // Look into dataset for the trace data
      QStringList parts = {
          trace_name.section('.', 0, -2),
          trace_name.section('.', -1)
      };
      QString file = parts[0];
      QString trace = parts[1];

      // Find data on the dataset
      if (trace.endsWith("dB") || trace.endsWith("_ang")) {
        // Go directly to the dataset for data
        P = findClosestPoint(datasets[file]["frequency"], datasets[file][trace], targetX);
        new_val = QStringLiteral("%1").arg(QString::number(P.y(), 'f', 2));
      } else {
        if (trace.endsWith("Smith")){
          // Get R + j X
          QString sxx_re = trace;
          QString sxx_im = trace;
          sxx_re.replace("Smith", "re");
          sxx_im.replace("Smith", "im");


          QPointF sij_real = findClosestPoint(datasets[file]["frequency"], datasets[file][sxx_re], targetX);
          QPointF sij_imag = findClosestPoint(datasets[file]["frequency"], datasets[file][sxx_im], targetX);
          double Z0 = datasets[file]["Z0"].at(0);

          double S_real = sij_real.y();
          double S_imag = sij_imag.y();

          // Calculate VSWR
          double magnitude_Gamma = sqrt(S_real * S_real + S_imag * S_imag);
          double SWR = (1.0 + magnitude_Gamma) / (1.0 - magnitude_Gamma);

          // Calculate complex impedance
          std::complex<double> Gamma(S_real, S_imag);
          std::complex<double> Z = Z0 * (1.0 + Gamma) / (1.0 - Gamma);

          double imag_part = Z.imag();

          if (imag_part < 0) {
            new_val = QStringLiteral("Z=%1-j%2 Ω\nSWR = %3").arg(QString::number(Z.real(), 'f', 1)).arg(QString::number(Z.imag(), 'f', 1)).arg(QString::number(SWR, 'f', 2));
          } else {
            if (imag_part > 0) {
              new_val = QStringLiteral("Z=%1+j%2 Ω\nSWR = %3").arg(QString::number(Z.real(), 'f', 1)).arg(QString::number(Z.imag(), 'f', 1)).arg(QString::number(SWR, 'f', 2));
            } else {
              // Z is pure real
              new_val = QStringLiteral("Z=%1 Ω\nSWR = %3").arg(QString::number(Z.real(), 'f', 1)).arg(QString::number(SWR, 'f', 2));
            }
          }
        }

      }

      QTableWidgetItem *new_item = new QTableWidgetItem(new_val);
      table.setItem(r, c, new_item);
    }
  }
}

// Find the closest x-axis value in a series given a x value (not necesarily in the grid)
QPointF Qucs_S_SPAR_Viewer::findClosestPoint(const QList<double>& xValues, const QList<double>& yValues, double targetX)
{
  if (xValues.isEmpty() || yValues.isEmpty() || xValues.size() != yValues.size()) {
    return QPointF(); // Return invalid point if lists are empty or have different sizes
  }

         // Initialize with the first point
  QPointF closestPoint(xValues.first(), yValues.first());
  double minDistance = qAbs(targetX - closestPoint.x());

         // Iterate through all points to find the closest one
  for (int i = 0; i < xValues.size(); ++i) {
    double distance = qAbs(targetX - xValues[i]);
    if (distance < minDistance) {
      minDistance = distance;
      closestPoint = QPointF(xValues[i], yValues[i]);
    }
  }

  return closestPoint;
}


double Qucs_S_SPAR_Viewer::getFreqFromText(QString freq)
{
    // Remove any whitespace from the string
    freq = freq.simplified();

    // Regular expression to match the number and unit
    QRegularExpression re("(\\d+(?:\\.\\d+)?)(\\s*)(Hz|kHz|MHz|GHz)");
    re.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(freq);

    if (match.hasMatch()) {
        double value = match.captured(1).toDouble();
        QString unit = match.captured(3).toLower();

        // Convert to Hz based on the unit
        if (unit == "khz") {
            return value * 1e3;
        } else if (unit == "mhz") {
            return value * 1e6;
        } else if (unit == "ghz") {
            return value * 1e9;
        } else {
            // Assume Hz if no unit or Hz is specified
            return value;
        }
    }

    // Return -1 if the input doesn't match the expected format
    return -1;
}


// This function is called when the user wants to remove a marker from the plot
void Qucs_S_SPAR_Viewer::removeMarker()
{
    QString ID = qobject_cast<QToolButton*>(sender())->objectName();
    //qDebug() << "Clicked button:" << ID;

    //Find the index of the button to remove
    int nmarkers = getNumberOfMarkers();

    MarkerProperties mkr_prop;
    QString mkr_name;
    for (int i = 0; i < nmarkers; i++) {
      getMarkerByPosition(i, mkr_name, mkr_prop);
      if (mkr_prop.deleteButton->objectName() == ID) {
          break;
      }
    }
    removeMarker(mkr_name); // Remove marker by name
}


void Qucs_S_SPAR_Viewer::removeMarker(const QString& markerName) {
  if (markerMap.contains(markerName)) {
    // Get the marker properties
    MarkerProperties& props = markerMap[markerName];

    // Delete all widgets
    delete props.nameLabel;
    delete props.freqSpinBox;
    delete props.scaleComboBox;
    delete props.deleteButton;

    // Remove from the map
    markerMap.remove(markerName);

    // Remove markers from the display widgets
    Magnitude_PhaseChart->removeMarker(markerName);
    smithChart->removeMarker(markerName);

    updateMarkerTable();
    updateMarkerNames();
    updateGridLayout(MarkersGrid);
  }
}

// Removes all markers on a row
void Qucs_S_SPAR_Viewer::removeAllMarkers()
{
    int n_markers = getNumberOfMarkers();
    for (int i = 0; i < n_markers; i++) {
      QString marker_to_remove = QString("Mkr%1").arg(n_markers-i-1);
      removeMarker(marker_to_remove);
    }
}

// After removing a marker, the names of the other markers must be updated
void Qucs_S_SPAR_Viewer::updateMarkerNames()
{
  int n_markers = getNumberOfMarkers();
  for (int i = 0; i < n_markers; i++) {
    MarkerProperties mkr_props;
    QString mkr_name;

    getMarkerByPosition(i, mkr_name, mkr_props);

    QLabel * MarkerLabel = mkr_props.nameLabel;
    MarkerLabel->setText(QStringLiteral("Mkr%1").arg(i+1));
  }
}

// After removing a marker, the names of the other markers must be updated
void Qucs_S_SPAR_Viewer::updateLimitNames()
{
  int n_limits = getNumberOfMarkers();
  for (int i = 0; i < n_limits; i++) {
    QLabel * LimitLabel = List_LimitNames[i];
    LimitLabel->setText(QStringLiteral("Limit %1").arg(i+1));
  }
}

// This function is called when the user wants to remove a limit from the plot
void Qucs_S_SPAR_Viewer::removeLimit()
{
  QString ID = qobject_cast<QToolButton*>(sender())->objectName();
  //qDebug() << "Clicked button:" << ID;

         //Find the index of the button to remove
  int index_to_delete = -1;
  for (int i = 0; i < List_Button_Delete_Limit.size(); i++) {
    if (List_Button_Delete_Limit.at(i)->objectName() == ID) {
      index_to_delete = i;
      break;
    }
  }
  removeLimit(index_to_delete);
}

void Qucs_S_SPAR_Viewer::removeLimit(int index_to_delete)
{
  // Delete the label
  QLabel* labelToRemove = List_LimitNames.at(index_to_delete);
  LimitsGrid->removeWidget(labelToRemove);
  List_LimitNames.removeAt(index_to_delete);
  delete labelToRemove;

  // Delete the fstart SpinBox
  QDoubleSpinBox * SpinBox_fstart_ToRemove = List_Limit_Start_Freq.at(index_to_delete);
  LimitsGrid->removeWidget(SpinBox_fstart_ToRemove);
  List_Limit_Start_Freq.removeAt(index_to_delete);
  delete SpinBox_fstart_ToRemove;

  // Delete the fstop SpinBox
  QDoubleSpinBox * SpinBox_fstop_ToRemove = List_Limit_Stop_Freq.at(index_to_delete);
  LimitsGrid->removeWidget(SpinBox_fstop_ToRemove);
  List_Limit_Stop_Freq.removeAt(index_to_delete);
  delete SpinBox_fstop_ToRemove;

  // Delete the start value SpinBox
  QDoubleSpinBox * SpinBox_val_start_ToRemove = List_Limit_Start_Value.at(index_to_delete);
  LimitsGrid->removeWidget(SpinBox_val_start_ToRemove);
  List_Limit_Start_Value.removeAt(index_to_delete);
  delete SpinBox_val_start_ToRemove;

  // Delete the stop value SpinBox
  QDoubleSpinBox * SpinBox_val_stop_ToRemove = List_Limit_Stop_Value.at(index_to_delete);
  LimitsGrid->removeWidget(SpinBox_val_stop_ToRemove);
  List_Limit_Stop_Value.removeAt(index_to_delete);
  delete SpinBox_val_stop_ToRemove;

  //Delete frequency scale combo. fstart
  QComboBox* Combo_fstart_ToRemove = List_Limit_Start_Freq_Scale.at(index_to_delete);
  LimitsGrid->removeWidget(Combo_fstart_ToRemove);
  List_Limit_Start_Freq_Scale.removeAt(index_to_delete);
  delete Combo_fstart_ToRemove;

  //Delete frequency scale combo. fstop
  QComboBox* Combo_fstop_ToRemove = List_Limit_Stop_Freq_Scale.at(index_to_delete);
  LimitsGrid->removeWidget(Combo_fstop_ToRemove);
  List_Limit_Stop_Freq_Scale.removeAt(index_to_delete);
  delete Combo_fstop_ToRemove;

  // Delete the "delete" button
  QToolButton* ButtonToRemove = List_Button_Delete_Limit.at(index_to_delete);
  LimitsGrid->removeWidget(ButtonToRemove);
  List_Button_Delete_Limit.removeAt(index_to_delete);
  delete ButtonToRemove;

  // Delete the "coupled" button
  QPushButton* ButtonCoupledToRemove = List_Couple_Value.at(index_to_delete);
  LimitsGrid->removeWidget(ButtonCoupledToRemove);
  List_Couple_Value.removeAt(index_to_delete);
  delete ButtonCoupledToRemove;

  // Delete the separator
  QFrame* SeparatorToRemove = List_Separators.at(index_to_delete);
  LimitsGrid->removeWidget(SeparatorToRemove);
  List_Separators.removeAt(index_to_delete);
  delete SeparatorToRemove;

  updateGridLayout(LimitsGrid);
  updateLimitNames();
}

void Qucs_S_SPAR_Viewer::removeAllLimits()
{
  int n_limits = List_LimitNames.size();
  for (int i = 0; i < n_limits; i++) {
    removeLimit(n_limits-i-1);
  }
}

// If the combobox associated to a marker changes, the limits of the marker must be updated too
void Qucs_S_SPAR_Viewer::changeMarkerLimits()
{
    QString ID = qobject_cast<QComboBox*>(sender())->objectName();
    //qDebug() << "Clicked button:" << ID;
    changeMarkerLimits(ID);

}

// If the combobox associated to a marker changes, the limits of the marker must be updated too
void Qucs_S_SPAR_Viewer::changeMarkerLimits(QString ID)
{
    //Find the index of the marker
    int index = -1;
    int nmarkers = getNumberOfMarkers();

    // Inspects all the markers' combobox and find that've been triggered
    for (int i = 0; i < nmarkers; i++) {
      MarkerProperties mkr_props;
      QString mkr_name;

      getMarkerByPosition(i, mkr_name, mkr_props);

      if (mkr_props.scaleComboBox->objectName() == ID) {
          index = i;
          break;
      }
    }

    // The lower and upper limits are given by the axis settings
    double f_upper = Magnitude_PhaseChart->getXmax();
    double f_lower = Magnitude_PhaseChart->getXmin();
    double f_scale = 1e-6;

    f_upper /=f_scale;
    f_lower /=f_scale;

    // Get markers properties
    MarkerProperties mkr_props;
    QString mkr_name;

    getMarkerByPosition(index, mkr_name, mkr_props);

    // Now we have to normalize this with respect to the marker's combo
    QString new_scale = mkr_props.scaleComboBox->currentText();
    double f_scale_combo = getFreqScale(new_scale);
    f_upper *= f_scale_combo;
    f_lower *= f_scale_combo;

    mkr_props.freqSpinBox->setMinimum(f_lower);
    mkr_props.freqSpinBox->setMaximum(f_upper);

    // Update minimum step
    double diff = f_upper - f_lower;
    if (diff < 1){
         mkr_props.freqSpinBox->setSingleStep(0.01);
    }else{
        if (diff < 10){
             mkr_props.freqSpinBox->setSingleStep(0.1);
        }else{
            if (diff < 100){
                 mkr_props.freqSpinBox->setSingleStep(1);
            }else{
                 mkr_props.freqSpinBox->setSingleStep(10);
            }

        }
    }

    updateMarkerTable();
}



void Qucs_S_SPAR_Viewer::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void Qucs_S_SPAR_Viewer::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    QStringList fileList;

    for (const QUrl &url : qAsConst(urls)) {
        if (url.isLocalFile()) {
            fileList << url.toLocalFile();
        }
    }

    if (!fileList.isEmpty()) {
      // Check if this is a session file
      if (fileList.size() == 1){
        if (fileList.first().endsWith(".spar", Qt::CaseInsensitive)) {// Then open it as a session settings file.
          // Remove traces and the dataset from the current session before loading the session file
          removeAllFiles();
          loadSession(fileList.first());
          this->activateWindow();
          return;
        }
      }

      addFiles(fileList);
      this->activateWindow();
    }
}

void Qucs_S_SPAR_Viewer::addLimit(double f_limit1, QString f_limit1_unit, double f_limit2, QString f_limit2_unit, double y_limit1, double y_limit2, bool coupled)
{
  // If there are no traces in the display, show a message and exit
  if (trace_list.size() == 0){
    QMessageBox::information(
        this,
        tr("Warning"),
        tr("The display contains no traces.") );
    return;
  }

  if (f_limit1 == -1) {
    // There's no specific data passed. Then get it from the widgets
    double f1 = Magnitude_PhaseChart->getXmin();
    double f2 = Magnitude_PhaseChart->getXmax();
    f_limit1 = f1 + 0.25*(f2-f1);
    f_limit2 = f1 + 0.75*(f2-f1);

    double y1 = Magnitude_PhaseChart->getYmin();
    double y2 = Magnitude_PhaseChart->getYmax();

    y_limit1 = y1 + (y2-y1)/2;
    y_limit2 = y_limit1;

  }

  int n_limits = List_LimitNames.size();
  n_limits++;
  int limit_index = 3*n_limits-2;

  QString tooltip_message;

  QString new_limit_name = QStringLiteral("Limit %1").arg(n_limits);
  QLabel * new_limit_label = new QLabel(new_limit_name);
  new_limit_label->setObjectName(new_limit_name);
  List_LimitNames.append(new_limit_label);
  this->LimitsGrid->addWidget(new_limit_label, limit_index, 0);

  QString SpinBox_fstart_name = QStringLiteral("Lmt_Freq_Start_SpinBox_%1").arg(new_limit_name);
  QDoubleSpinBox * new_limit_fstart_Spinbox = new QDoubleSpinBox();
  new_limit_fstart_Spinbox->setObjectName(SpinBox_fstart_name);
  new_limit_fstart_Spinbox->setMinimum(Magnitude_PhaseChart->getXmin());
  new_limit_fstart_Spinbox->setMaximum(Magnitude_PhaseChart->getXmax());
  new_limit_fstart_Spinbox->setSingleStep(Magnitude_PhaseChart->getXdiv()/5);
  new_limit_fstart_Spinbox->setValue(f_limit1);
  connect(new_limit_fstart_Spinbox, SIGNAL(valueChanged(double)), SLOT(updateTraces()));
  List_Limit_Start_Freq.append(new_limit_fstart_Spinbox);
  this->LimitsGrid->addWidget(new_limit_fstart_Spinbox, limit_index, 1);

  QString Combobox_start_name = QStringLiteral("Lmt_Start_ComboBox_%1").arg(new_limit_name);
  QComboBox * new_start_limit_Combo = new QComboBox();
  new_start_limit_Combo->setObjectName(Combobox_start_name);
  new_start_limit_Combo->addItems(frequency_units);
  if (f_limit1_unit.isEmpty()){
    new_start_limit_Combo->setCurrentIndex(1);
  } else {
    int index = new_start_limit_Combo->findText(f_limit1_unit);
    new_start_limit_Combo->setCurrentIndex(index);
  }
  connect(new_start_limit_Combo, SIGNAL(currentIndexChanged(int)), SLOT(updateTraces()));
  List_Limit_Start_Freq_Scale.append(new_start_limit_Combo);
  this->LimitsGrid->addWidget(new_start_limit_Combo, limit_index, 2);

  QString SpinBox_fstop_name = QStringLiteral("Lmt_Freq_Stop_SpinBox_%1").arg(new_limit_name);
  QDoubleSpinBox * new_limit_fstop_Spinbox = new QDoubleSpinBox();
  new_limit_fstop_Spinbox->setObjectName(SpinBox_fstop_name);
  new_limit_fstop_Spinbox->setMinimum(Magnitude_PhaseChart->getXmin());
  new_limit_fstop_Spinbox->setMaximum(Magnitude_PhaseChart->getXmax());
  new_limit_fstop_Spinbox->setSingleStep(Magnitude_PhaseChart->getXdiv()/5);
  new_limit_fstop_Spinbox->setValue(f_limit2);
  connect(new_limit_fstop_Spinbox, SIGNAL(valueChanged(double)), SLOT(updateTraces()));
  List_Limit_Stop_Freq.append(new_limit_fstop_Spinbox);
  this->LimitsGrid->addWidget(new_limit_fstop_Spinbox, limit_index, 3);

  QString Combobox_stop_name = QStringLiteral("Lmt_Stop_ComboBox_%1").arg(new_limit_name);
  QComboBox * new_stop_limit_Combo = new QComboBox();
  new_stop_limit_Combo->setObjectName(Combobox_stop_name);
  new_stop_limit_Combo->addItems(frequency_units);
  if (f_limit2_unit.isEmpty()){
    new_stop_limit_Combo->setCurrentIndex(1);
  } else {
    int index = new_stop_limit_Combo->findText(f_limit2_unit);
    new_stop_limit_Combo->setCurrentIndex(index);
  }

  connect(new_stop_limit_Combo, SIGNAL(currentIndexChanged(int)), SLOT(updateTraces()));
  List_Limit_Stop_Freq_Scale.append(new_stop_limit_Combo);
  this->LimitsGrid->addWidget(new_stop_limit_Combo, limit_index, 4);

  // Remove button
  QString DeleteButton_name = QStringLiteral("Lmt_Delete_Btn_%1").arg(new_limit_name);
  QToolButton * new_limit_removebutton = new QToolButton();
  new_limit_removebutton->setObjectName(DeleteButton_name);
  tooltip_message = QStringLiteral("Remove this limit");
  new_limit_removebutton->setToolTip(tooltip_message);
  QIcon icon(":/bitmaps/trash.png");
  new_limit_removebutton->setIcon(icon);
  new_limit_removebutton->setStyleSheet(R"(
            QToolButton {
                background-color: #FF0000;
                color: white;
                border-radius: 20px;
            }
        )");
  connect(new_limit_removebutton, SIGNAL(clicked()), SLOT(removeLimit()));
  List_Button_Delete_Limit.append(new_limit_removebutton);
  this->LimitsGrid->addWidget(new_limit_removebutton, limit_index, 5, Qt::AlignCenter);

  QString SpinBox_val_start_name = QStringLiteral("Lmt_Val_Start_SpinBox_%1").arg(new_limit_name);
  QDoubleSpinBox * new_limit_val_start_Spinbox = new QDoubleSpinBox();
  new_limit_val_start_Spinbox->setObjectName(SpinBox_val_start_name);
  new_limit_val_start_Spinbox->setMaximum(Magnitude_PhaseChart->getYmin());
  new_limit_val_start_Spinbox->setMaximum(Magnitude_PhaseChart->getYmax());
  new_limit_val_start_Spinbox->setValue(y_limit1);
  new_limit_val_start_Spinbox->setSingleStep(Magnitude_PhaseChart->getYdiv()/5);
  connect(new_limit_val_start_Spinbox, SIGNAL(valueChanged(double)), SLOT(updateLimits()));
  List_Limit_Start_Value.append(new_limit_val_start_Spinbox);
  this->LimitsGrid->addWidget(new_limit_val_start_Spinbox, limit_index+1, 1);

  // Coupled spinbox value
  QString CoupleButton_name = QStringLiteral("Lmt_Couple_Btn_%1").arg(new_limit_name);
  QPushButton * new_limit_CoupleButton = new QPushButton("<--->");
  new_limit_CoupleButton->setObjectName(CoupleButton_name);
  new_limit_CoupleButton->setChecked(coupled);
  tooltip_message = QStringLiteral("Couple start and stop values");
  new_limit_CoupleButton->setToolTip(tooltip_message);
  connect(new_limit_CoupleButton, SIGNAL(clicked(bool)), SLOT(coupleSpinBoxes()));
  List_Couple_Value.append(new_limit_CoupleButton);
  this->LimitsGrid->addWidget(new_limit_CoupleButton, limit_index+1, 2);

  QString SpinBox_val_stop_name = QStringLiteral("Lmt_Val_Stop_SpinBox_%1").arg(new_limit_name);
  QDoubleSpinBox * new_limit_val_stop_Spinbox = new QDoubleSpinBox();
  new_limit_val_stop_Spinbox->setObjectName(SpinBox_val_stop_name);
  new_limit_val_stop_Spinbox->setMaximum(Magnitude_PhaseChart->getYmax());
  new_limit_val_stop_Spinbox->setValue(y_limit2);
  new_limit_val_stop_Spinbox->setSingleStep(Magnitude_PhaseChart->getYdiv()/5);
  connect(new_limit_val_stop_Spinbox, SIGNAL(valueChanged(double)), SLOT(updateLimits()));
  List_Limit_Stop_Value.append(new_limit_val_stop_Spinbox);
  this->LimitsGrid->addWidget(new_limit_val_stop_Spinbox, limit_index+1, 3);

  if (coupled){
    new_limit_CoupleButton->setText("<--->");
  } else {
    new_limit_CoupleButton->setText("<-X->");
  }
  new_limit_CoupleButton->click();

  QString Separator_name = QStringLiteral("Lmt_Separator_%1").arg(new_limit_name);
  QFrame * new_Separator = new QFrame();
  new_Separator->setObjectName(Separator_name);
  new_Separator->setFrameShape(QFrame::HLine);
  new_Separator->setFrameShadow(QFrame::Sunken);
  List_Separators.append(new_Separator);
  this->LimitsGrid->addWidget(new_Separator, limit_index+2, 0, 1, 6);


}

void Qucs_S_SPAR_Viewer::coupleSpinBoxes(){

  QPushButton* button = qobject_cast<QPushButton*>(sender());
  // Get the button ID, from it we can get the index and then lock the upper limit spinbox
  QString name_button = button->objectName();
  // Get the limit name
  int lastUnderscoreIndex = name_button.lastIndexOf('_');
  QString limit_name = name_button.mid(lastUnderscoreIndex + 1);

  // Get a list with the limit names
  QList<QString> labelNames;
  for (const QLabel *label : qAsConst(List_LimitNames)) {
    labelNames.append(label->text());
  }

  int index = labelNames.indexOf(limit_name);

  QDoubleSpinBox * upper_limit_spinbox = List_Limit_Stop_Value.at(index);

  if (button->text() == "<--->"){
    button->setText("<-X->");
    QString tooltip_message = QStringLiteral("Uncouple start and stop values");
    button->setToolTip(tooltip_message);
    QDoubleSpinBox * lower_limit_spinbox = List_Limit_Start_Value.at(index);
    upper_limit_spinbox->setValue(lower_limit_spinbox->value());
    upper_limit_spinbox->setDisabled(true);
  }else{
    button->setText("<--->");
    upper_limit_spinbox->setEnabled(true);
  }
}

// This function is called when a limit widget is changed. It is needed in case some value-coupling
// button is activated
void Qucs_S_SPAR_Viewer::updateLimits()
{
  // First check if some value-coupling button is activated. If not, simply call updateTraces()
  int n_limits = List_Couple_Value.size();
  for (int i = 0; i < n_limits; i++) {
    QPushButton* button = List_Couple_Value.at(i);
    if (button->text() == "<-X->"){
      // The control is locked. Set the stop value equal to the start value
      QDoubleSpinBox* start = List_Limit_Start_Value.at(i);
      QDoubleSpinBox* stop = List_Limit_Stop_Value.at(i);
      double val_start = start->value();
      stop->setValue(val_start);
    }
  }
}


void Qucs_S_SPAR_Viewer::slotSave()
{
  if (savepath.isEmpty()){
    slotSaveAs();
    return;
  }
  save();
}

void Qucs_S_SPAR_Viewer::slotSaveAs()
{
  if (datasets.isEmpty()){
    // Nothing to save
    QMessageBox::information(
        this,
        tr("Error"),
        tr("Nothing to save: No data was loaded.") );
    return;
  }

  // Get the path to save
  savepath = QFileDialog::getSaveFileName(this,
                                              tr("Save session"),
                                              QDir::homePath() + "/ViewerSession.spar",
                                              tr("Qucs-S snp viewer session (*.spar);"));

  // If the user decides not to enter a path, then return.
  if (savepath.isEmpty()){
    return;
  }
  save();
}

bool Qucs_S_SPAR_Viewer::save()
{
  if (datasets.isEmpty()){
    // Nothing to save
    QMessageBox::information(
        this,
        tr("Error"),
        tr("Nothing to save: No data was loaded.") );
    return false;
  }
  QFile file(savepath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    return false;
  }

  QXmlStreamWriter xmlWriter(&file);
  xmlWriter.setAutoFormatting(true);
  xmlWriter.writeStartDocument();
  xmlWriter.writeStartElement("DATA");//Top level

  // ----------------------------------------------------------------
  // Save the markers
  int nmarkers = getNumberOfMarkers();
  if (nmarkers != 0){
    xmlWriter.writeStartElement("MARKERS");
    double freq;
    for (int i = 0; i < nmarkers; i++)
    {
      MarkerProperties mkr_props;
      QString mkr_name;

      getMarkerByPosition(i, mkr_name, mkr_props);

      freq = mkr_props.freqSpinBox->value();
      QString scale = mkr_props.scaleComboBox->currentText();
      freq /= getFreqScale(scale);
      xmlWriter.writeTextElement("Mkr", QString::number(freq));
    }
    xmlWriter.writeEndElement(); // Markers
  }
  // ----------------------------------------------------------------
  // Save the limits
  if (List_Limit_Start_Freq.size() != 0){
      double freq, value;
      bool Coupled_Limits;
      xmlWriter.writeStartElement("LIMITS");

      // Offset
      value = Limits_Offset->value();
      xmlWriter.writeTextElement("offset", QString::number(value));

      for (int i = 0; i < List_Limit_Start_Freq.size(); i++)
      {
        xmlWriter.writeStartElement("Limit");

        // fstart
        freq = List_Limit_Start_Freq[i]->value();
        xmlWriter.writeTextElement("fstart", QString::number(freq));

        // fstart unit
        QString fstart_unit = List_Limit_Start_Freq_Scale[i]->currentText();
        xmlWriter.writeTextElement("fstart_unit", fstart_unit);

        // Start value
        value = List_Limit_Start_Value[i]->value();
        xmlWriter.writeTextElement("val_start", QString::number(value));

        // fstop
        freq = List_Limit_Stop_Freq[i]->value();
        xmlWriter.writeTextElement("fstop", QString::number(freq));

        // fstop unit
        QString fstop_unit = List_Limit_Stop_Freq_Scale[i]->currentText();
        xmlWriter.writeTextElement("fstop_unit", fstop_unit);

        // Stop value
        value = List_Limit_Stop_Value[i]->value();
        xmlWriter.writeTextElement("val_stop", QString::number(value));

        // Couple values
        Coupled_Limits = (List_Couple_Value[i]->text() == "<-X->");
        xmlWriter.writeTextElement("couple_values", QString::number(Coupled_Limits));

        xmlWriter.writeEndElement(); // Limit
      }
      xmlWriter.writeEndElement(); // Limits
  }
  // ----------------------------------------------------------------
  // Save the traces displayed and their properties
  int ntraces = getNumberOfTraces();
  if (ntraces != 0){
    xmlWriter.writeStartElement("DISPLAYED_TRACES");
    QString trace_name, color, style;
    int width;
    for (int i = 0; i < ntraces; i++){

      QString trace_name;
      TraceProperties trace_props;
      getTraceByPosition(i, trace_name, trace_props);

      xmlWriter.writeStartElement("trace");
      // Trace name
      xmlWriter.writeTextElement("trace_name", trace_name);

      // Trace width
      width = trace_props.width->value();
      xmlWriter.writeTextElement("trace_width", QString::number(width));

      // Trace color
      color = trace_props.colorButton->palette().color(QPalette::Button).name();
      xmlWriter.writeTextElement("trace_color", color);

      // Trace style
      style = trace_props.LineStyleComboBox->currentText();
      xmlWriter.writeTextElement("trace_style", style);
      xmlWriter.writeEndElement(); // Trace

    }
    xmlWriter.writeEndElement(); // Displayed traces
  }
  // ----------------------------------------------------------------
  // Save the axes settings
  xmlWriter.writeStartElement("AXES");
  xmlWriter.writeTextElement("x-axis-min", QString::number(Magnitude_PhaseChart->getXmin()));
  xmlWriter.writeTextElement("x-axis-max", QString::number(Magnitude_PhaseChart->getXmax()));
  xmlWriter.writeTextElement("x-axis-div", QString::number(Magnitude_PhaseChart->getXdiv()));
  //xmlWriter.writeTextElement("x-axis-scale", QCombobox_x_axis_units->currentText());
  xmlWriter.writeTextElement("y-axis-min", QString::number(Magnitude_PhaseChart->getYmin()));
  xmlWriter.writeTextElement("y-axis-max", QString::number(Magnitude_PhaseChart->getYmax()));
  xmlWriter.writeTextElement("y-axis-div", QString::number(Magnitude_PhaseChart->getYdiv()));
  xmlWriter.writeTextElement("lock_status", QString::number(lock_axis));

  xmlWriter.writeEndElement(); // Axes

  // ----------------------------------------------------------------
  // Save notes
  xmlWriter.writeStartElement("NOTES");
  xmlWriter.writeTextElement("note", Notes_Widget->getText());
  xmlWriter.writeEndElement();

  // ----------------------------------------------------------------
  // Save the datasets
  // Only S-parameter data is saved. This is done to minimize the size of the session file.
  xmlWriter.writeStartElement("DATASETS");
  for (auto outerIt = datasets.constBegin(); outerIt != datasets.constEnd(); ++outerIt)
  {
    xmlWriter.writeStartElement("file");
    xmlWriter.writeAttribute("file_name", outerIt.key());

    const QMap<QString, QList<double>>& innerMap = outerIt.value();
    for (auto innerIt = innerMap.constBegin(); innerIt != innerMap.constEnd(); ++innerIt)
    {
      QString trace_name = innerIt.key();
      QStringList blacklist;
      blacklist.append("K");
      blacklist.append("mu");
      blacklist.append("mu_p");
      blacklist.append("delta");
      blacklist.append("MAG");
      blacklist.append("MSG");
      blacklist.append("Re{Zin}");
      blacklist.append("Im{Zin}");
      blacklist.append("Re{Zout}");
      blacklist.append("Im{Zout}");

      if (blacklist.contains(trace_name)){
        // Zin, Zout, K, mu, etc. traces are discarded
        continue;
      }

      // Only S (Re/Im ang dB/ang) traces here
      xmlWriter.writeStartElement("trace");
      xmlWriter.writeAttribute("trace_name", trace_name);

      const QList<double>& values = innerIt.value();
      for (const double& value : values)
      {
        xmlWriter.writeTextElement("value", QString::number(value));
      }

      xmlWriter.writeEndElement(); // inner-item
    }

    xmlWriter.writeEndElement(); // outer-item
  }

  xmlWriter.writeEndElement(); // Datasets
  // ----------------------------------------------------------------

  xmlWriter.writeEndElement(); // Top level
  xmlWriter.writeEndDocument();

  file.close();
  return true;
}


void Qucs_S_SPAR_Viewer::slotLoadSession()
{
  QString fileName = QFileDialog::getOpenFileName(this,
                                                  tr("Open S-parameter Viewer Session"),
                                                  QDir::homePath(),
                                                  tr("Qucs-S snp viewer session (*.spar);"));

  loadSession(fileName);
}


void Qucs_S_SPAR_Viewer::loadSession(QString session_file)
{
  QFile file(session_file);

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "Error opening file:" << file.errorString();
    return;
  }

  savepath = session_file;

  addRecentFile(session_file);// Add it to the "Recent Files" list

  QXmlStreamReader xml(&file);

  // Trace properties
  QList<int> trace_width;
  QList<QString> trace_name, trace_color, trace_style;

  // Limit data
  QList<double> Limit_Start_Freq, Limit_Start_Val, Limit_Stop_Freq, Limit_Stop_Val;
  QList<int> Limit_Couple_Values;
  QList<QString> Limit_Start_Freq_Unit, Limit_Stop_Freq_Unit;

  // Markers
  QList<double> Markers;

  // Clear current dataset
  datasets.clear();

  while (!xml.atEnd() && !xml.hasError()) {
    // Read next element
    QXmlStreamReader::TokenType token = xml.readNext();

    //qDebug() << xml.name().toString();
    if (token == QXmlStreamReader::StartElement) {
      if (xml.name() == QStringLiteral("trace")) {
        while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == QStringLiteral("trace"))) {
          xml.readNext();
          if (xml.tokenType() == QXmlStreamReader::StartElement) {
            if (xml.name() == QStringLiteral("trace_name")) {
              trace_name.append(xml.readElementText());
            } else if (xml.name() == QStringLiteral("trace_width")) {
              trace_width.append(xml.readElementText().toInt());
            } else if (xml.name() == QStringLiteral("trace_color")) {
              trace_color.append(xml.readElementText());
            } else if (xml.name() == QStringLiteral("trace_style")) {
              trace_style.append(xml.readElementText());
            }
          }
        }
      } else if (xml.name() == QStringLiteral("Limit")) {
        while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == QStringLiteral("Limit"))) {
          xml.readNext();
          if (xml.tokenType() == QXmlStreamReader::StartElement) {
            if (xml.name() == QStringLiteral("fstart")) {
              Limit_Start_Freq.append(xml.readElementText().toDouble());
            } else if (xml.name() == QStringLiteral("val_start")) {
              Limit_Start_Val.append(xml.readElementText().toDouble());
            } else if (xml.name() == QStringLiteral("fstop")) {
              Limit_Stop_Freq.append(xml.readElementText().toDouble());
            } else if (xml.name() == QStringLiteral("val_stop")) {
              Limit_Stop_Val.append(xml.readElementText().toDouble());
            } else if (xml.name() == QStringLiteral("fstart_unit")) {
              Limit_Start_Freq_Unit.append(xml.readElementText());
            } else if (xml.name() == QStringLiteral("fstop_unit")) {
              Limit_Stop_Freq_Unit.append(xml.readElementText());
            } else if (xml.name() == QStringLiteral("couple_values")) {
              Limit_Couple_Values.append(xml.readElementText().toInt());
            }else if (xml.name() == QStringLiteral("offset")) {
              Limits_Offset->setValue(xml.readElementText().toDouble());
            }
          }
        }
      } else if (xml.name() == QStringLiteral("MARKERS")){
        while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == QStringLiteral("MARKERS"))) {
          xml.readNext();
          if (xml.tokenType() == QXmlStreamReader::StartElement) {
            double value = xml.readElementText().toDouble();
            Markers.append(value);
          }
        }
      } else if (xml.name() == QStringLiteral("file")) {
        // Load datasets
        QString fileName, traceName;
        while (!xml.atEnd() && !xml.hasError())
        {
          if (xml.tokenType() == QXmlStreamReader::StartElement)
          {
            if (xml.name() == QStringLiteral("file"))
            {
              fileName = xml.attributes().value("file_name").toString();
              //qDebug() << "File name:" << fileName;
            }
            else if (xml.name() == QStringLiteral("trace"))
            {
              traceName = xml.attributes().value("trace_name").toString();
              //qDebug() << "Trace name:" << traceName;
            }
            else if (xml.name() == QStringLiteral("value"))
            {
              QString value = xml.readElementText();
              //qDebug() << "Value:" << value;
              datasets[fileName][traceName].append(value.toDouble());
            }
          }
          xml.readNext();
        }
      } else if (xml.name().toString().contains("note")){
          QString note = xml.readElementText();
          Notes_Widget->loadText(note);
      }
    }
  }


  if (xml.hasError()) {
    qDebug() << "Error parsing XML: " << xml.errorString();
  }

  // Close the file
  file.close();


  // Update dataset and trace selection comboboxes
  QStringList files = datasets.keys();
  for (int i = 0; i < files.size(); i++){
    QString file = files.at(i);
    QCombobox_datasets->addItem(file);

    // Add file management widgets
    // Label
    QLabel * Filename_Label = new QLabel(file);
    Filename_Label->setObjectName(QStringLiteral("File_") + QString::number(i));
    List_FileNames.append(Filename_Label);
    this->FilesGrid->addWidget(List_FileNames.last(), i, 0, 1, 1);

    // Create the "Remove" button
    QToolButton * RemoveButton = new QToolButton();
    RemoveButton->setObjectName(QStringLiteral("Remove_") + QString::number(i));
    QIcon icon(":/bitmaps/trash.png"); // Use a resource path or a relative path
    RemoveButton->setIcon(icon);

    RemoveButton->setStyleSheet("QToolButton {background-color: red;\
                                    border-width: 2px;\
                                    border-radius: 10px;\
                                    border-color: beige;\
                                    font: bold 14px;\
                                }");
    List_RemoveButton.append(RemoveButton);
    this->FilesGrid->addWidget(List_RemoveButton.last(), i, 1, 1, 1);
    connect(RemoveButton, SIGNAL(clicked()), SLOT(removeFile())); // Connect button with the handler to remove the entry.

  }
  updateTracesCombo();// Update traces

  // Add traces to the display
  for (int i = 0; i < trace_name.size(); i++){
    QStringList parts = {
        trace_name[i].section('.', 0, -2),
        trace_name[i].section('.', -1)
    };
    addTrace(parts[0], parts[1], trace_color.at(i), trace_width.at(i), trace_style.at(i));
  }

  // Add markers
  for (int i = 0; i < Markers.size(); i++){
    addMarker(Markers.at(i));
  }

  // Add limits
  for (int i = 0; i < Limit_Start_Freq.size(); i++){
    addLimit(Limit_Start_Freq.at(i), Limit_Start_Freq_Unit.at(i), Limit_Stop_Freq.at(i), Limit_Stop_Freq_Unit.at(i), Limit_Start_Val.at(i), Limit_Stop_Val.at(i), Limit_Couple_Values.at(i));
  }

  // Show the trace settings widget
  dockTracesList->raise();

  return;
}

void Qucs_S_SPAR_Viewer::updateGridLayout(QGridLayout* layout)
{
  // Store widget information
  struct WidgetInfo {
    QWidget* widget;
    int row, column, rowSpan, columnSpan;
    Qt::Alignment alignment;
  };
  QVector<WidgetInfo> widgetInfos;

  // Collect information about remaining widgets
  for (int i = 0; i < layout->count(); ++i) {
    QLayoutItem* item = layout->itemAt(i);
    QWidget* widget = item->widget();
    if (widget) {
      int row, column, rowSpan, columnSpan;
      layout->getItemPosition(i, &row, &column, &rowSpan, &columnSpan);
      widgetInfos.push_back({widget, row, column, rowSpan, columnSpan, item->alignment()});
    }
  }

  // Clear the layout
  while (layout->count() > 0) {
    QLayoutItem* item = layout->takeAt(0);
    delete item;
  }

  // Re-add widgets with updated positions
  int row = 0;
  for (const auto& info : widgetInfos) {
    int newColumn = info.column;

    if (info.columnSpan == layout->columnCount()){// Separator widget
      row++;
    }

    layout->addWidget(info.widget, row, newColumn, info.rowSpan, info.columnSpan, info.alignment);

    if (info.columnSpan == layout->columnCount()){
      row++;
    }

    if (newColumn == layout->columnCount()-1) {
      row++;
    }
  }

}

// Add session file to the recent files list
void Qucs_S_SPAR_Viewer::addRecentFile(const QString& filePath) {
  recentFiles.insert(recentFiles.begin(), filePath);
  recentFiles.erase(std::unique(recentFiles.begin(), recentFiles.end()), recentFiles.end());
  if (recentFiles.size() > 10) {
    recentFiles.resize(10);
  }
}

// This function updates teh "Recent Files" list whenever the user hovers the mouse over the menu
void Qucs_S_SPAR_Viewer::updateRecentFilesMenu() {
  recentFilesMenu->clear();
  for (const auto& filePath : recentFiles) {
    QAction* action = recentFilesMenu->addAction(filePath);
    connect(action, &QAction::triggered, this, [this, filePath]() {
      loadSession(filePath);
    });
  }
  recentFilesMenu->addSeparator();
  recentFilesMenu->addAction("Clear Recent Files", this, &Qucs_S_SPAR_Viewer::clearRecentFiles);
}

void Qucs_S_SPAR_Viewer::clearRecentFiles() {
  recentFiles.clear();
}

// Save "Recent Files" list. This is called when the program is about to close
void Qucs_S_SPAR_Viewer::saveRecentFiles() {
  QSettings settings;
  settings.setValue("recentFiles", QVariant::fromValue(recentFiles));
}

// Load "Recent Files" list. This is called when the program starts up
void Qucs_S_SPAR_Viewer::loadRecentFiles() {
  QSettings settings;
  recentFiles = settings.value("recentFiles").value<std::vector<QString>>();
}

// This function is called when the user wants to see a trace which can be calculated from the S-parameters
void Qucs_S_SPAR_Viewer::calculate_Sparameter_trace(QString file, QString metric){


  std::complex<double> s11, s12, s21, s22, s11_conj, s22_conj;
  double Z0 = datasets[file]["Rn"].last();

  for (int i = 0; i < datasets[file]["S11_re"].size(); i++) {
    // S-parameter data (n.u.)
    double s11_re =  datasets[file]["S11_re"][i];
    double s11_im =  datasets[file]["S11_im"][i];
    s11 = std::complex<double>(s11_re, s11_im);
    s11_conj = std::complex<double>(s11_re, -s11_im);

    if ( datasets[file]["n_ports"].last() == 2) {
      double s12_re =  datasets[file]["S12_re"][i];
      double s12_im =  datasets[file]["S12_im"][i];
      double s21_re =  datasets[file]["S21_re"][i];
      double s21_im =  datasets[file]["S21_im"][i];
      double s22_re =  datasets[file]["S22_re"][i];
      double s22_im =  datasets[file]["S22_im"][i];
      s12 = std::complex<double> (s12_re, s12_im);
      s21 = std::complex<double> (s21_re, s21_im);
      s22 = std::complex<double> (s22_re, s22_im);
      s22_conj = std::complex<double> (s22_re, -s22_im);
    }

    double delta = abs(s11*s22 - s12*s21); // Determinant of the S matrix

    if (!metric.compare("delta")) {
      datasets[file]["delta"].append(delta);
    } else {
      if (!metric.compare("K")) {
        double K = (1 - abs(s11)*abs(s11) - abs(s22)*abs(s22) + delta*delta) / (2*abs(s12*s21)); // Rollet factor.
        datasets[file]["K"].append(K);
      } else {
        if (!metric.compare("mu")) {
          double mu = (1 - abs(s11)*abs(s11)) / (abs(s22-delta*s11_conj) + abs(s12*s21));
          datasets[file]["mu"].append(mu);
        } else {
          if (!metric.compare("mu_p")) {
            double mu_p = (1 - abs(s22)*abs(s22)) / (abs(s11-delta*s22_conj) + abs(s12*s21));
            datasets[file]["mu_p"].append(mu_p);
          } else {
            if (!metric.compare("MSG")) {
              double MSG = abs(s21) / abs(s12);
              MSG = 10*log10(MSG);
              datasets[file]["MSG"].append(MSG);
            } else {
              if (!metric.compare("MSG")) {
                double K = (1 - abs(s11)*abs(s11) - abs(s22)*abs(s22) + delta*delta) / (2*abs(s12*s21)); // Rollet factor.
                double MSG = abs(s21) / abs(s12);
                double MAG = MSG * (K - std::sqrt(K * K - 1));
                MAG = 10*log10(abs(MAG));
                datasets[file]["MAG"].append(MAG);
              } else {
                if (metric.contains("Zin")) {
                  std::complex<double> Zin = std::complex<double>(Z0) * (1.0 + s11) / (1.0 - s11);
                  datasets[file]["Re{Zin}"].append(Zin.real());
                  datasets[file]["Im{Zin}"].append(Zin.imag());
                } else {
                  if (metric.contains("Zout")) {
                    std::complex<double> Zout = std::complex<double>(Z0) * (1.0 + s22) / (1.0 - s22);
                    datasets[file]["Re{Zout}"].append(Zout.real());
                    datasets[file]["Im{Zout}"].append(Zout.imag());
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}



// Gets the marker frequency based on the marker name
double Qucs_S_SPAR_Viewer::getMarkerFreq(QString markerName){
  // Check if marker exists
  if (!markerMap.contains(markerName)) {
    qWarning() << "Marker" << markerName << "not found!";
    return 0.0;
  }

  // Get the marker properties
  const MarkerProperties& props = markerMap[markerName];

  // Get the base frequency from the spin box
  double baseFrequency = props.freqSpinBox->value();

  // Get the scale factor from the combo box
  QString scaleText = props.scaleComboBox->currentText();
  double scaleFactor = 1.0;

  // Convert scale text to actual multiplication factor
  if (scaleText == "Hz") {
    scaleFactor = 1.0;
  } else if (scaleText == "kHz") {
    scaleFactor = 1e3;
  } else if (scaleText == "MHz") {
    scaleFactor = 1e6;
  } else if (scaleText == "GHz") {
    scaleFactor = 1e9;
  } else if (scaleText == "THz") {
    scaleFactor = 1e12;
  }

  // Apply scale factor to base frequency
  return baseFrequency * scaleFactor;
}

// Get the marker given the position of the entry
bool Qucs_S_SPAR_Viewer::getMarkerByPosition(int position, QString& outMarkerName, MarkerProperties& outProperties) {
  // Check if position is valid
  if (position < 0 || position >= markerMap.size()) {
    qWarning() << "Invalid position:" << position;
    return false;
  }

  // Get an iterator to the beginning of the map
  auto it = markerMap.begin();

  // Advance the iterator by 'position' steps
  std::advance(it, position);

  // Get the marker name and properties
  outMarkerName = it.key();
  outProperties = it.value();

  return true;
}


// Get the trace given the position of the entry
bool Qucs_S_SPAR_Viewer::getTraceByPosition(int position, QString& outTraceName, TraceProperties& outProperties) {
  // Check if position is valid
  if (position < 0 || position >= traceMap.size()) {
    qWarning() << "Invalid position:" << position;
    return false;
  }

         // Get an iterator to the beginning of the map
  auto it = traceMap.begin();

         // Advance the iterator by 'position' steps
  std::advance(it, position);

         // Get the marker name and properties
  outTraceName = it.key();
  outProperties = it.value();

  return true;
}


// Returns the total number of markers
int Qucs_S_SPAR_Viewer::getNumberOfMarkers(){
  return markerMap.keys().size();
}

// Returns the total number of traces
int Qucs_S_SPAR_Viewer::getNumberOfTraces(){
  return traceMap.keys().size();
}
