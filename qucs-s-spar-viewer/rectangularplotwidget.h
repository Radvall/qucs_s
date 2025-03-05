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

class RectangularPlotWidget : public QWidget
{
  Q_OBJECT

public:
  struct Trace {
    QList<std::complex<double>> impedances;
    QList<double> frequencies;
    QPen pen;
    double Z0;
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

  bool addMarker(const QString& markerId, double frequency, const QPen& pen = QPen(Qt::red, 2));
  bool removeMarker(const QString& markerId);
  void clearMarkers();
  QMap<QString, double> getMarkers() const;

  QChart *chart() const { return m_chart; }

private slots:
  void updateXAxis();
  void changeFreqUnits();

private:
  QChart *m_chart;
  QChartView *m_chartView;
  QValueAxis *m_xAxis;
  QValueAxis *m_yAxis;
  QValueAxis *m_y2Axis;

  QDoubleSpinBox *m_xAxisMin;
  QDoubleSpinBox *m_xAxisMax;
  QDoubleSpinBox *m_xAxisDiv;
  QComboBox *m_xAxisUnits;

  QStringList m_frequencyUnits;
  double m_fMin;
  double m_fMax;

  QMap<QString, Trace> traces;
  QMap<QString, Marker> markers;

  QGridLayout* setupAxisSettings();
  void updatePlot();
};

#endif // RECTANGULARPLOTWIDGET_H
