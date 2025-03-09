#include "rectangularplotwidget.h"

RectangularPlotWidget::RectangularPlotWidget(QWidget *parent)
    : QWidget(parent), fMin(1e20), fMax(-1)
{
  // Initialize the chart and chart view
  ChartWidget = new QChart();
  ChartWidget->legend()->hide();
  chartView = new QChartView(ChartWidget, this);
  chartView->setRenderHint(QPainter::Antialiasing);

         // Initialize axes
  xAxis = new QValueAxis();
  yAxis = new QValueAxis();
  y2Axis = new QValueAxis();

  ChartWidget->addAxis(xAxis, Qt::AlignBottom);
  ChartWidget->addAxis(yAxis, Qt::AlignLeft);
  ChartWidget->addAxis(y2Axis, Qt::AlignRight);

  yAxis->setTitleText("Magnitude (dB)");
  y2Axis->setTitleText("Phase (deg)");

         // Set up the frequency units
  frequencyUnits << "Hz" << "kHz" << "MHz" << "GHz";

         // Create the main layout
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addWidget(chartView);

         // Add the axis settings layout
  mainLayout->addLayout(setupAxisSettings());
  setLayout(mainLayout);
}

RectangularPlotWidget::~RectangularPlotWidget()
{
  delete ChartWidget;
}

void RectangularPlotWidget::addTrace(const QString& name, const Trace& trace)
{
  traces[name] = trace;

  // Update frequency range if this trace has data
  if (!trace.frequencies.isEmpty()) {
    double traceMinFreq = trace.frequencies.first();
    double traceMaxFreq = trace.frequencies.last();

    // Update global min/max frequency (stored in Hz)
    if (traceMinFreq < fMin) fMin = traceMinFreq;
    if (traceMaxFreq > fMax) fMax = traceMaxFreq;

    // Get current frequency scale factor based on selected units
    double freqScale = 1.0;
    QString unit = xAxisUnits->currentText();
    if (unit == "kHz") {
      freqScale = 1e-3;  // Hz to kHz
    } else if (unit == "MHz") {
      freqScale = 1e-6;  // Hz to MHz
    } else if (unit == "GHz") {
      freqScale = 1e-9;  // Hz to GHz
    }

    // Update the axis limits with the scaled frequency values
    xAxisMin->setValue(fMin * freqScale);
    xAxisMax->setValue(fMax * freqScale);

    // Update the x-axis
    updateXAxis();
  }

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
  // Get the current scale factor based on selected units
  double freqScale = 1.0;
  QString unit = xAxisUnits->currentText();
  if (unit == "kHz") {
    freqScale = 1e-3;  // Hz to kHz
  } else if (unit == "MHz") {
    freqScale = 1e-6;  // Hz to MHz
  } else if (unit == "GHz") {
    freqScale = 1e-9;  // Hz to GHz
  }

         // Remove all existing series from the chart
  QList<QAbstractSeries*> oldSeries = ChartWidget->series();
  for (QAbstractSeries* series : oldSeries) {
    ChartWidget->removeSeries(series);
    delete series;
  }

  // Add each trace as a new series
  for (auto it = traces.constBegin(); it != traces.constEnd(); ++it) {
    const QString& name = it.key();
    const Trace& trace = it.value();

    // Create a new line series for the trace
    QLineSeries* series = new QLineSeries();
    series->setPen(trace.pen);
    series->setName(name);

    // Add data points to the series with proper frequency scaling
    for (int i = 0; i < trace.frequencies.size() && i < trace.trace.size(); ++i) {
      // Scale the frequency values according to the current frequency units
      double scaledFreq = trace.frequencies[i] * freqScale;
      series->append(scaledFreq, trace.trace[i]);
    }

    // Add the series to the chart
    ChartWidget->addSeries(series);

    // Attach to the appropriate axes
    series->attachAxis(xAxis);
    if (trace.y_axis == 2) {
      series->attachAxis(y2Axis);
    } else {
      series->attachAxis(yAxis);
    }
  }

  // Draw markers if any
  for (auto it = markers.constBegin(); it != markers.constEnd(); ++it) {
    const Marker& marker = it.value();

    // Create a vertical line series for each marker
    QLineSeries* markerSeries = new QLineSeries();
    markerSeries->setPen(marker.pen);
    markerSeries->setName(marker.id);

    // Find the y-range to cover based on current axes
    double yBottom = yAxis->min();
    double yTop = yAxis->max();

    // Scale the marker frequency according to the current units
    double scaledMarkerFreq = marker.frequency * freqScale;

    // Add a vertical line at the marker frequency
    markerSeries->append(scaledMarkerFreq, yBottom);
    markerSeries->append(scaledMarkerFreq, yTop);

    // Add the marker series to the chart
    ChartWidget->addSeries(markerSeries);
    markerSeries->attachAxis(xAxis);
    markerSeries->attachAxis(yAxis);
  }

  // Refresh the chart
  ChartWidget->update();
}

void RectangularPlotWidget::updateXAxis()
{
  double xMin = xAxisMin->value();
  double xMax = xAxisMax->value();
  double xDiv = xAxisDiv->value();

         // Set the axis range in display units
  xAxis->setRange(xMin, xMax);
  xAxis->setTickInterval(xDiv);
  xAxis->setTickCount(floor((xMax - xMin) / xDiv) + 1);
  xAxis->setTitleText("frequency (" + xAxisUnits->currentText() + ")");

         // Instead of trying to modify existing series, just redraw everything
  updatePlot();
}

void RectangularPlotWidget::updateYAxis()
{
  double yMin = yAxisMin->value();
  double yMax = yAxisMax->value();
  double yDiv = yAxisDiv->value();

  yAxis->setRange(yMin, yMax);
  yAxis->setTickInterval(yDiv);
  yAxis->setTickCount(floor((yMax - yMin) / yDiv) + 1);

  updatePlot();
}

void RectangularPlotWidget::updateY2Axis()
{
  double y2Min = y2AxisMin->value();
  double y2Max = y2AxisMax->value();
  double y2Div = y2AxisDiv->value();

  y2Axis->setRange(y2Min, y2Max);
  y2Axis->setTickInterval(y2Div);
  y2Axis->setTickCount(floor((y2Max - y2Min) / y2Div) + 1);

  updatePlot();
}


void RectangularPlotWidget::changeFreqUnits()
{
  // Get the current scale factor based on selected units
  double freqScale = 1.0;
  QString unit = xAxisUnits->currentText();
  if (unit == "kHz") {
    freqScale = 1e-3;  // Hz to kHz
  } else if (unit == "MHz") {
    freqScale = 1e-6;  // Hz to MHz
  } else if (unit == "GHz") {
    freqScale = 1e-9;  // Hz to GHz
  }

         // Block signals temporarily to avoid triggering updateXAxis() multiple times
  xAxisMin->blockSignals(true);
  xAxisMax->blockSignals(true);

  // Update the spinbox values with scaled values from the global min/max (in Hz)
  xAxisMin->setValue(fMin * freqScale);
  xAxisMax->setValue(fMax * freqScale);

  // Re-enable signals
  xAxisMin->blockSignals(false);
  xAxisMax->blockSignals(false);

  // Update the axis
  updateXAxis();
}
QGridLayout* RectangularPlotWidget::setupAxisSettings()
{
  QGridLayout *axisLayout = new QGridLayout();

         // X-axis settings
  QLabel *xAxisLabel = new QLabel("<b>x-axis</b>");
  axisLayout->addWidget(xAxisLabel, 0, 0);

  xAxisMin = new QDoubleSpinBox();
  xAxisMin->setMinimum(0.1);
  xAxisMin->setMaximum(1000000);
  xAxisMin->setValue(0);
  xAxisMin->setDecimals(1);
  xAxisMin->setSingleStep(0.1);
  connect(xAxisMin, SIGNAL(valueChanged(double)), this, SLOT(updateXAxis()));
  axisLayout->addWidget(xAxisMin, 0, 1);

  xAxisMax = new QDoubleSpinBox();
  xAxisMax->setMinimum(0.1);
  xAxisMax->setMaximum(1000000);
  xAxisMax->setValue(1000);
  xAxisMax->setDecimals(1);
  xAxisMax->setSingleStep(0.1);
  connect(xAxisMax, SIGNAL(valueChanged(double)), this, SLOT(updateXAxis()));
  axisLayout->addWidget(xAxisMax, 0, 2);

  xAxisDiv = new QDoubleSpinBox();
  xAxisDiv->setMinimum(0.1);
  xAxisDiv->setMaximum(1000000);
  xAxisDiv->setValue(100);
  xAxisDiv->setSingleStep(1);
  connect(xAxisDiv, SIGNAL(valueChanged(double)), this, SLOT(updateXAxis()));
  axisLayout->addWidget(xAxisDiv, 0, 3);

  xAxisUnits = new QComboBox();
  xAxisUnits->addItems(frequencyUnits);
  xAxisUnits->setCurrentIndex(2);
  connect(xAxisUnits, SIGNAL(currentIndexChanged(int)), this, SLOT(changeFreqUnits()));
  axisLayout->addWidget(xAxisUnits, 0, 4);

         // Left Y-axis settings
  QLabel *yAxisLabel = new QLabel("<b>y-axis</b>");
  axisLayout->addWidget(yAxisLabel, 1, 0);

  yAxisMin = new QDoubleSpinBox();
  yAxisMin->setMinimum(-1000000);
  yAxisMin->setMaximum(1000000);
  yAxisMin->setValue(-10);
  yAxisMin->setDecimals(1);
  yAxisMin->setSingleStep(1);
  connect(yAxisMin, SIGNAL(valueChanged(double)), this, SLOT(updateYAxis()));
  axisLayout->addWidget(yAxisMin, 1, 1);

  yAxisMax = new QDoubleSpinBox();
  yAxisMax->setMinimum(-1000000);
  yAxisMax->setMaximum(1000000);
  yAxisMax->setValue(10);
  yAxisMax->setDecimals(1);
  yAxisMax->setSingleStep(1);
  connect(yAxisMax, SIGNAL(valueChanged(double)), this, SLOT(updateYAxis()));
  axisLayout->addWidget(yAxisMax, 1, 2);

  yAxisDiv = new QDoubleSpinBox();
  yAxisDiv->setMinimum(0.1);
  yAxisDiv->setMaximum(1000000);
  yAxisDiv->setValue(5);
  yAxisDiv->setSingleStep(1);
  connect(yAxisDiv, SIGNAL(valueChanged(double)), this, SLOT(updateYAxis()));
  axisLayout->addWidget(yAxisDiv, 1, 3);

  yAxisUnits = new QLabel("dB");
  axisLayout->addWidget(yAxisUnits, 1, 4);

         // Right Y-axis settings
  QLabel *y2AxisLabel = new QLabel("<b>y2-axis</b>");
  axisLayout->addWidget(y2AxisLabel, 2, 0);

  y2AxisMin = new QDoubleSpinBox();
  y2AxisMin->setMinimum(-1000000);
  y2AxisMin->setMaximum(1000000);
  y2AxisMin->setValue(-180);
  y2AxisMin->setDecimals(1);
  y2AxisMin->setSingleStep(10);
  connect(y2AxisMin, SIGNAL(valueChanged(double)), this, SLOT(updateY2Axis()));
  axisLayout->addWidget(y2AxisMin, 2, 1);

  y2AxisMax = new QDoubleSpinBox();
  y2AxisMax->setMinimum(-1000000);
  y2AxisMax->setMaximum(1000000);
  y2AxisMax->setValue(180);
  y2AxisMax->setDecimals(1);
  y2AxisMax->setSingleStep(10);
  connect(y2AxisMax, SIGNAL(valueChanged(double)), this, SLOT(updateY2Axis()));
  axisLayout->addWidget(y2AxisMax, 2, 2);

  y2AxisDiv = new QDoubleSpinBox();
  y2AxisDiv->setMinimum(0.1);
  y2AxisDiv->setMaximum(1000000);
  y2AxisDiv->setValue(45);
  y2AxisDiv->setSingleStep(5);
  connect(y2AxisDiv, SIGNAL(valueChanged(double)), this, SLOT(updateY2Axis()));
  axisLayout->addWidget(y2AxisDiv, 2, 3);

  y2AxisUnits = new QLabel("deg");
  axisLayout->addWidget(y2AxisUnits, 2, 4);

  return axisLayout;
}


// These "get" functions are used by the main program to put markers and limits
double RectangularPlotWidget::getYmin(){
  return yAxisMin->value();
}

double RectangularPlotWidget::getYmax(){
  return yAxisMax->value();
}

double RectangularPlotWidget::getYdiv(){
  return yAxisDiv->value();
}

double RectangularPlotWidget::getY2min(){
  return y2AxisMin->value();
}

double RectangularPlotWidget::getY2max(){
  return y2AxisMax->value();
}

double RectangularPlotWidget::getY2div(){
  return y2AxisDiv->value();
}

double RectangularPlotWidget::getXmin(){
  return xAxisMin->value();
}

double RectangularPlotWidget::getXmax(){
  return xAxisMax->value();
}

double RectangularPlotWidget::getXdiv(){
  return xAxisDiv->value();
}
