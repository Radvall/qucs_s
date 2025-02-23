#ifndef SMITHCHARTWIDGET_H
#define SMITHCHARTWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QList>
#include <complex>
#include <QPen>

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

  SmithChartWidget(QWidget *parent = nullptr);
  ~SmithChartWidget() override;

  void addTrace(const Trace& trace);
  void clearTraces(); // New method to remove all traces
  void setCharacteristicImpedance(double z0);
  double characteristicImpedance() const { return z0; }

signals:
  void impedanceSelected(const std::complex<double>& impedance); // Signal for impedance selection

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;

private:
  void drawSmithChartGrid(QPainter *painter);
  void drawReactanceArc(QPainter *painter, const QPointF &center, double radius, double reactance);
  void plotImpedanceData(QPainter *painter);
  QPointF smithChartToWidget(const std::complex<double>& reflectionCoefficient);
  std::complex<double> widgetToSmithChart(const QPointF& widgetPoint);

  void calculateArcPoints(const QRectF& arcRect, double startAngle, double sweepAngle, QPointF& startPoint, QPointF& endPoint);

private:
  QList<Trace> traces;
  double z0; // Characteristic impedance
  QPointF lastMousePos;

         // Add a scale factor for zoom functionality
  double scaleFactor;
  double panX;
  double panY;
};

#endif // SMITHCHARTWIDGET_H
