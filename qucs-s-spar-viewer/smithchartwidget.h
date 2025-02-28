#ifndef SMITHCHARTWIDGET_H
#define SMITHCHARTWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QMap>
#include <complex>
#include <QPen>
#include <QSet>

class Qucs_S_SPAR_Viewer; // Forward declaration

class SmithChartWidget : public QWidget {
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

  SmithChartWidget(QWidget *parent = nullptr);
  ~SmithChartWidget() override;

  void addTrace(const QString& name, const Trace& trace);
  void removeTrace(const QString&);
  void clearTraces();
  void setCharacteristicImpedance(double z0);
  double characteristicImpedance() const { return z0; }
  QPen getTracePen(const QString& traceName) const;
  void setTracePen(const QString& traceName, const QPen& pen);
  QMap<QString, QPen> getTracesInfo() const;

  // Modified marker functionality with string ID
  bool addMarker(const QString& markerId, double frequency, const QPen& pen = QPen(Qt::red, 2));
  bool removeMarker(const QString& markerId);
  void clearMarkers();
  QMap<QString, double> getMarkers() const;

signals:
  void impedanceSelected(const std::complex<double>& impedance);

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;

private:
  void drawSmithChartGrid(QPainter *painter);
  void drawReactanceArc(QPainter *painter, const QPointF &center, double radius, double reactance);
  void plotImpedanceData(QPainter *painter);
  void drawMarkers(QPainter *painter);
  QPointF smithChartToWidget(const std::complex<double>& reflectionCoefficient);
  std::complex<double> widgetToSmithChart(const QPointF& widgetPoint);
  std::complex<double> interpolateImpedance(const QList<double>& frequencies,
                                            const QList<std::complex<double>>& impedances,
                                            double targetFreq);

  void calculateArcPoints(const QRectF& arcRect, double startAngle, double sweepAngle, QPointF& startPoint, QPointF& endPoint);

private:
  QMap<QString, Trace> traces;  // Changed from QList to QMap
  QMap<QString, Marker> markers; // Store markers by ID instead of frequency
  double z0; // Characteristic impedance
  QPointF lastMousePos;

         // Add a scale factor for zoom functionality
  double scaleFactor;
  double panX;
  double panY;
};

#endif // SMITHCHARTWIDGET_H
