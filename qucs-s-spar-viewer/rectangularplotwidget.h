#ifndef RECTANGULARPLOTWIDGET_H
#define RECTANGULARPLOTWIDGET_H

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QMap>
#include <QPen>
#include <complex>
#include <limits>  // For std::numeric_limits

class RectangularPlotWidget : public QWidget
{
  Q_OBJECT

public:
  struct Trace {
    QList<double> trace;
    QList<double> frequencies;
    QPen pen;
    double Z0;
    int y_axis;
  };

  struct Marker {
    QString id;
    double frequency;
    QPen pen;
  };

  explicit RectangularPlotWidget(QWidget *parent = nullptr);
  ~RectangularPlotWidget();

  void addTrace(const QString& name, const Trace& trace);
  void removeTrace(const QString& name);
  void clearTraces();
  QPen getTracePen(const QString& traceName) const;
  void setTracePen(const QString& traceName, const QPen& pen);
  QMap<QString, QPen> getTracesInfo() const;

         // Used to set markers and limits
  double getYmax();
  double getYmin();
  double getYdiv();

  double getY2max();
  double getY2min();
  double getY2div();

  double getXmax();
  double getXmin();
  double getXdiv();

  bool addMarker(const QString& markerId, double frequency, const QPen& pen = QPen(Qt::red, 2));
  bool removeMarker(const QString& markerId);
  void clearMarkers();
  QMap<QString, double> getMarkers() const;

  QChart *chart() const { return ChartWidget; }

private slots:
  void updateXAxis();
  void updateYAxis();
  void updateY2Axis();
  void changeFreqUnits();

private:
  QChart *ChartWidget;
  QChartView *chartView;
  QValueAxis *xAxis;
  QValueAxis *yAxis;
  QValueAxis *y2Axis;

  QDoubleSpinBox *xAxisMin;
  QDoubleSpinBox *xAxisMax;
  QDoubleSpinBox *xAxisDiv;
  QComboBox *xAxisUnits;

  // Controls for left y-axis
  QDoubleSpinBox *yAxisMin;
  QDoubleSpinBox *yAxisMax;
  QDoubleSpinBox *yAxisDiv;
  QLabel *yAxisUnits;

  // Controls for right y-axis
  QDoubleSpinBox *y2AxisMin;
  QDoubleSpinBox *y2AxisMax;
  QDoubleSpinBox *y2AxisDiv;
  QLabel *y2AxisUnits;

  QStringList frequencyUnits;
  double fMin;
  double fMax;

  QMap<QString, Trace> traces;
  QMap<QString, Marker> markers;

  QGridLayout* setupAxisSettings();
  void updatePlot();

  // Helper methods for auto-scaling
  int getYAxisTraceCount() const;
  int getY2AxisTraceCount() const;
};

#endif // RECTANGULARPLOTWIDGET_H
