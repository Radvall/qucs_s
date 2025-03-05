#include "rectangularplotwidget.h"

RectangularPlotWidget::RectangularPlotWidget(QWidget *parent)
    : QWidget(parent), m_fMin(1e20), m_fMax(-1)
{
  // Initialize the chart and chart view
  m_chart = new QChart();
  m_chartView = new QChartView(m_chart, this);
  m_chartView->setRenderHint(QPainter::Antialiasing);

         // Initialize axes
  m_xAxis = new QValueAxis();
  m_yAxis = new QValueAxis();
  m_y2Axis = new QValueAxis();

  m_chart->addAxis(m_xAxis, Qt::AlignBottom);
  m_chart->addAxis(m_yAxis, Qt::AlignLeft);
  m_chart->addAxis(m_y2Axis, Qt::AlignRight);

         // Set up the frequency units
  m_frequencyUnits << "Hz" << "kHz" << "MHz" << "GHz";

         // Create the main layout
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(m_chartView);

         // Add the axis settings layout
  mainLayout->addLayout(setupAxisSettings());
  setLayout(mainLayout);
}

RectangularPlotWidget::~RectangularPlotWidget()
{
  delete m_chart;
}

void RectangularPlotWidget::addTrace(const QString& name, const Trace& trace)
{
  traces[name] = trace;
  updatePlot();
}

void RectangularPlotWidget::removeTrace(const QString& name)
{
  traces.remove(name);
  updatePlot();
}

void RectangularPlotWidget::clearTraces()
{
  traces.clear();
  updatePlot();
}

QPen RectangularPlotWidget::getTracePen(const QString& traceName) const
{
  if (traces.contains(traceName)) {
    return traces[traceName].pen;
  }
  return QPen();
}

void RectangularPlotWidget::setTracePen(const QString& traceName, const QPen& pen)
{
  if (traces.contains(traceName)) {
    traces[traceName].pen = pen;
    updatePlot();
  }
}

QMap<QString, QPen> RectangularPlotWidget::getTracesInfo() const
{
  QMap<QString, QPen> penMap;
  for (auto it = traces.constBegin(); it != traces.constEnd(); ++it) {
    penMap.insert(it.key(), it.value().pen);
  }
  return penMap;
}

bool RectangularPlotWidget::addMarker(const QString& markerId, double frequency, const QPen& pen)
{
  if (markers.contains(markerId)) {
    return false;
  }

  bool frequencyInRange = false;
  for (auto it = traces.constBegin(); it != traces.constEnd(); ++it) {
    const Trace& trace = it.value();
    if (!trace.frequencies.isEmpty() &&
        frequency >= trace.frequencies.first() &&
        frequency <= trace.frequencies.last()) {
      frequencyInRange = true;
      break;
    }
  }

  if (!frequencyInRange) {
    return false;
  }

  Marker marker;
  marker.id = markerId;
  marker.frequency = frequency;
  marker.pen = pen;

  markers.insert(markerId, marker);
  updatePlot();
  return true;
}

bool RectangularPlotWidget::removeMarker(const QString& markerId)
{
  if (!markers.contains(markerId)) {
    return false;
  }

  markers.remove(markerId);
  updatePlot();
  return true;
}

void RectangularPlotWidget::clearMarkers()
{
  markers.clear();
  updatePlot();
}

QMap<QString, double> RectangularPlotWidget::getMarkers() const
{
  QMap<QString, double> markerFrequencies;
  for (auto it = markers.constBegin(); it != markers.constEnd(); ++it) {
    markerFrequencies.insert(it.key(), it.value().frequency);
  }
  return markerFrequencies;
}

void RectangularPlotWidget::updatePlot()
{
  m_chart->update();
}

void RectangularPlotWidget::updateXAxis()
{
  double xMin = m_xAxisMin->value();
  double xMax = m_xAxisMax->value();
  double xDiv = m_xAxisDiv->value();

  m_xAxis->setRange(xMin, xMax);
  m_xAxis->setTickInterval(xDiv);
  m_xAxis->setTickCount(floor((xMax - xMin) / xDiv) + 1);
  m_xAxis->setTitleText("frequency (" + m_xAxisUnits->currentText() + ")");

  updatePlot();
}

void RectangularPlotWidget::changeFreqUnits()
{
  double freqScale = 1.0;
  QString unit = m_xAxisUnits->currentText();
  if (unit == "kHz") {
    freqScale = 1e3;
  } else if (unit == "MHz") {
    freqScale = 1e6;
  } else if (unit == "GHz") {
    freqScale = 1e9;
  }

  m_xAxisMin->setValue(m_fMin * freqScale);
  m_xAxisMax->setValue(m_fMax * freqScale);
  updateXAxis();
}

QGridLayout* RectangularPlotWidget::setupAxisSettings()
{
  QGridLayout *axisLayout = new QGridLayout();

         // X-axis settings
  QLabel *xAxisLabel = new QLabel("X-axis:");
  axisLayout->addWidget(xAxisLabel, 0, 0);

  m_xAxisMin = new QDoubleSpinBox();
  m_xAxisMin->setMinimum(0.1);
  m_xAxisMin->setMaximum(1000000);
  m_xAxisMin->setValue(0);
  m_xAxisMin->setDecimals(1);
  m_xAxisMin->setSingleStep(0.1);
  connect(m_xAxisMin, SIGNAL(valueChanged(double)), this, SLOT(updateXAxis()));
  axisLayout->addWidget(m_xAxisMin, 0, 1);

  m_xAxisMax = new QDoubleSpinBox();
  m_xAxisMax->setMinimum(0.1);
  m_xAxisMax->setMaximum(1000000);
  m_xAxisMax->setValue(1000);
  m_xAxisMax->setDecimals(1);
  m_xAxisMax->setSingleStep(0.1);
  connect(m_xAxisMax, SIGNAL(valueChanged(double)), this, SLOT(updateXAxis()));
  axisLayout->addWidget(m_xAxisMax, 0, 2);

  m_xAxisDiv = new QDoubleSpinBox();
  m_xAxisDiv->setMinimum(0.1);
  m_xAxisDiv->setMaximum(1000000);
  m_xAxisDiv->setValue(100);
  m_xAxisDiv->setSingleStep(1);
  connect(m_xAxisDiv, SIGNAL(valueChanged(double)), this, SLOT(updateXAxis()));
  axisLayout->addWidget(m_xAxisDiv, 0, 3);

  m_xAxisUnits = new QComboBox();
  m_xAxisUnits->addItems(m_frequencyUnits);
  m_xAxisUnits->setCurrentIndex(2);
  connect(m_xAxisUnits, SIGNAL(currentIndexChanged(int)), this, SLOT(changeFreqUnits()));
  axisLayout->addWidget(m_xAxisUnits, 0, 4);

  return axisLayout;
}
