#ifndef RECTANGULARPLOTWIDGET_H
#define RECTANGULARPLOTWIDGET_H

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QMap>
#include <QPen>
#include <QGraphicsTextItem>
#include <QCheckBox>  // Added for checkbox
#include <complex>
#include <limits>

class RectangularPlotWidget : public QWidget
{
  Q_OBJECT

public:
  struct Trace {
    QList<double> trace;
    QList<double> frequencies;
    QString units;
    QPen pen;
    double Z0;
    int y_axis;
    QString y_axis_title;
  };

  struct Marker {
    QString id;
    double frequency;
    QPen pen;
  };

  struct Limit {
    double f1; // Start frequency
    double f2; // End frequency
    double y1; // Start y
    double y2; // End y
    int y_axis; // 0: Left y-axis; 1: Right y-axis
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

  double getYmax();
  double getYmin();
  double getYdiv();

  double getY2max();
  double getY2min();
  double getY2div();

  double getXmax();
  double getXmin();
  double getXdiv();
  double getXscale();
  QString getXunits();
  int getFreqIndex();

  void change_Y_axis_title(QString title);
  void change_Y_axis_units(QString title);
  void change_Y2_axis_title(QString title);
  void change_Y2_axis_units(QString title);
  void change_X_axis_title(QString title);
  void change_X_axis_label(QString title);

  QLabel *xAxisLabel;

  bool addMarker(const QString& markerId, double frequency, const QPen& pen = QPen(Qt::red, 2));
  bool removeMarker(const QString& markerId);
  bool updateMarkerFrequency(const QString& markerId, double newFrequency);
  void clearMarkers();
  QMap<QString, double> getMarkers() const;


  bool addLimit(const QString& LimitId, const Limit & limit);
  void removeLimit(const QString& LimitID);
  void clearLimits();
  QMap<QString, Limit> getLimits() const;
  bool updateLimit(const QString& limitId, const Limit& limit);

  QChart *chart() const { return ChartWidget; }

private slots:
  void updateXAxis();
  void updateYAxis();
  void updateY2Axis();
  void changeFreqUnits();
  void toggleShowValues(bool show);  // New slot for checkbox

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

  QDoubleSpinBox *yAxisMin;
  QDoubleSpinBox *yAxisMax;
  QDoubleSpinBox *yAxisDiv;
  QLabel *yAxisUnits;

  QDoubleSpinBox *y2AxisMin;
  QDoubleSpinBox *y2AxisMax;
  QDoubleSpinBox *y2AxisDiv;
  QLabel *y2AxisUnits;

  QCheckBox *showValuesCheckbox;  // New checkbox for showing values
  bool showTraceValues;           // Flag to control value display

  QStringList frequencyUnits;
  double fMin;
  double fMax;

  QMap<QString, Trace> traces;
  QMap<QString, Marker> markers;
  QMap<QString, Limit> limits;

  // Lists to keep track of graphics items
  QList<QGraphicsTextItem*> markerLabels;
  QList<QGraphicsTextItem*> intersectionLabels;

  QGridLayout* setupAxisSettings();
  void updatePlot();
  void clearGraphicsItems();

  int getYAxisTraceCount() const;
  int getY2AxisTraceCount() const;
};

#endif // RECTANGULARPLOTWIDGET_H
