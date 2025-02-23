#include "smithchartwidget.h"
#include <QDebug>
#include <QToolTip>

SmithChartWidget::SmithChartWidget(QWidget *parent) : QWidget(parent), z0(50.0), scaleFactor(1.0), panX(0.0), panY(0.0)
{
  // Default characteristic impedance
  setAttribute(Qt::WA_Hover);
  setMouseTracking(true);
}

SmithChartWidget::~SmithChartWidget()
{
}

void SmithChartWidget::setData(const QList<std::complex<double>>& impedances)
{
  impedanceData = impedances;
  update(); // Trigger a repaint
}

void SmithChartWidget::setCharacteristicImpedance(double z)
{
  z0 = z;
  update(); // Redraw the chart with the new Z0
}

void SmithChartWidget::paintEvent(QPaintEvent *event)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

         //Save transformation matrix
  painter.save();

         // Apply zoom and pan transformations
  painter.translate(width() / 2.0 + panX, height() / 2.0 + panY);
  painter.scale(scaleFactor, scaleFactor);
  painter.translate(-width() / 2.0, -height() / 2.0);

         // 1. Draw the Smith Chart grid (circles and arcs)
  drawSmithChartGrid(&painter);

         // 2. Plot the impedance data
  plotImpedanceData(&painter);

         //Restore transformation matrix
  painter.restore();
}

void SmithChartWidget::mousePressEvent(QMouseEvent *event)
{
  // Handle mouse clicks (e.g., for data point selection)
  lastMousePos = event->pos();

  std::complex<double> reflectionCoefficient = widgetToSmithChart(lastMousePos);

         // Calculate impedance from reflection coefficient
  std::complex<double> normalizedImpedance = (1.0 + reflectionCoefficient) / (1.0 - reflectionCoefficient);
  std::complex<double> impedance = normalizedImpedance * z0;
  emit impedanceSelected(impedance); // Emit the signal
}

void SmithChartWidget::drawSmithChartGrid(QPainter *painter) {
  painter->save();
  painter->setRenderHint(QPainter::Antialiasing, true);

         // Calculate the center and radius
  QPointF center(width() / 2.0, height() / 2.0);
  double radius = qMin(width(), height()) / 2.0 - 10;

         // Draw the outer circle (|Γ| = 1)
  painter->setPen(QPen(Qt::black, 2));
  painter->drawEllipse(center, radius, radius);

         // Draw the real axis
  painter->drawLine(center - QPointF(radius, 0), center + QPointF(radius, 0));

  // Draw constant resistance circles
  QVector<double> resistances = {0.2, 0.5, 1.0, 2.0, 5.0};
  painter->setPen(QPen(Qt::gray, 1));
  for (double r : resistances) {
    double x = radius * r / (1 + r);
    double y = radius / (1 + r);
    QPointF circleCenter(center.x() + x, center.y());
    painter->drawEllipse(circleCenter, y, y);

    //Paint label
    QPointF label_position(center.x() + x - y, center.y()-5);
    painter->setPen(QPen(Qt::black, 2));
    painter->drawText(label_position, QString::number(r));
    painter->setPen(QPen(Qt::gray, 1));
  }

         // Draw constant reactance arcs
  QVector<double> reactances = {0.2, 0.5, 1.0, 2.0, 5.0};
  for (double x : reactances) {
    drawReactanceArc(painter, center, radius, x);
    drawReactanceArc(painter, center, radius, -x);
  }

  painter->restore();
}

void SmithChartWidget::drawReactanceArc(QPainter *painter, const QPointF &center, double radius, double reactance) {
  painter->setPen(QPen(Qt::gray, 1));
  double x = reactance;
  double chartRadius = radius;

         // Center coordinates in normalized form
  double normalizedCenterX = 1.0;
  double normalizedCenterY = 1.0 / x;

         // Circle radius in normalized form
  double normalizedRadius = 1.0 / std::abs(x);

         // Convert to widget coordinates
  double centerX = center.x() + chartRadius * normalizedCenterX;
  double centerY = center.y() - chartRadius * normalizedCenterY;
  double arcRadius = chartRadius * normalizedRadius;

  QRectF arcRect(
      centerX - arcRadius,
      centerY - arcRadius,
      2 * arcRadius,
      2 * arcRadius
      );

         // Calculate intersection with unit circle
  double d2 = normalizedCenterX * normalizedCenterX + normalizedCenterY * normalizedCenterY;
  double d = std::sqrt(d2);

         // Calculate central angle using cosine law
  double cos_theta = (d2 + normalizedRadius * normalizedRadius - 1.0) / (2.0 * d * normalizedRadius);
  double theta = std::acos(cos_theta);

         // Convert to degrees and calculate the arc angles
  double base_angle = std::atan2(normalizedCenterY, normalizedCenterX) * 180.0 / M_PI;
  double theta_deg = theta * 180.0 / M_PI;

         // Calculate start and sweep angles
  double startAngle, sweepAngle;
  if (x > 0) {
    startAngle = (base_angle - theta_deg) - 180;
    sweepAngle = 2 * theta_deg;
  } else {
    startAngle = (base_angle + theta_deg) - 180;
    sweepAngle = -2 * theta_deg;
  }

         // Draw the arc (Qt uses 16th of a degree)
  painter->drawArc(arcRect, startAngle * 16, sweepAngle * 16);

         // Variables to store the starting and ending points
  QPointF startPoint, endPoint;

         // Calculate the points
  calculateArcPoints(arcRect, startAngle, sweepAngle, startPoint, endPoint);

         // Draw points at start and end
  painter->setPen(QPen(Qt::red, 4));
  painter->drawPoint(startPoint);

         // Calculate the direction from the center to the start point
  QPointF directionStart = startPoint - center;
  double directionLengthStart = std::sqrt(directionStart.x() * directionStart.x() + directionStart.y() * directionStart.y());

         // Normalize the direction vector
  directionStart /= directionLengthStart;

         // Move the label outside the unit circle
  double labelOffset = 1.02; // Scale factor to place the label outside the unit circle
  QPointF labelPositionStart = center + directionStart * (chartRadius * labelOffset);

         // Draw labels
  painter->setPen(QPen(Qt::black, 2));
  painter->drawText(labelPositionStart, QString::number(reactance));

         // Calculate the direction from the center to the end point
  QPointF directionEnd = endPoint - center;
  double directionLengthEnd = std::sqrt(directionEnd.x() * directionEnd.x() + directionEnd.y() * directionEnd.y());

         // Normalize the direction vector
  directionEnd /= directionLengthEnd;

         // Move the label outside the unit circle
  QPointF labelPositionEnd = center + directionEnd * (chartRadius * labelOffset);

         // Draw the reactance value near the end point
  QString label = QString::number(reactance, 'g', 3);
  painter->drawText(labelPositionEnd, label);
}

// Function to calculate the starting and ending points of the arc
void SmithChartWidget::calculateArcPoints(const QRectF& arcRect, double startAngle, double sweepAngle, QPointF& startPoint, QPointF& endPoint) {
  // Calculate the center of the ellipse
  double cx = arcRect.center().x();
  double cy = arcRect.center().y();

         // Convert angles from degrees to radians
  double startRad = qDegreesToRadians(startAngle);
  double endRad = qDegreesToRadians(startAngle + sweepAngle);

         // Calculate the starting point
  startPoint.setX(cx + (arcRect.width() / 2.0) * std::cos(startRad));
  startPoint.setY(cy - (arcRect.height() / 2.0) * std::sin(startRad)); // Subtract for Qt's coordinate system

         // Calculate the ending point
  endPoint.setX(cx + (arcRect.width() / 2.0) * std::cos(endRad));
  endPoint.setY(cy - (arcRect.height() / 2.0) * std::sin(endRad)); // Subtract for Qt's coordinate system
}

void SmithChartWidget::plotImpedanceData(QPainter *painter)
{
  painter->save();
  painter->setPen(QPen(Qt::red, 4));

  QPointF center(width() / 2.0, height() / 2.0);
  double radius = qMin(width(), height()) / 2.0 - 10;

  for (const auto& impedance : impedanceData) {
    std::complex<double> gamma = (impedance - z0) / (impedance + z0);
    double x = center.x() + radius * gamma.real();
    double y = center.y() - radius * gamma.imag();
    painter->drawPoint(QPointF(x, y));
  }

  painter->restore();
}

QPointF SmithChartWidget::smithChartToWidget(const std::complex<double>& reflectionCoefficient)
{
  double gammaReal = reflectionCoefficient.real();
  double gammaImag = reflectionCoefficient.imag();
  double gammaMagSq = gammaReal * gammaReal + gammaImag * gammaImag;

  double x = gammaReal / (1 + gammaMagSq);
  double y = gammaImag / (1 + gammaMagSq);

         // Calculate the center and radius of the Smith Chart
  QPointF center(width() / 2.0, height() / 2.0);
  double radius = qMin(width(), height()) / 2.0 - 10; // Leave some margin

         // Scale and translate the Smith Chart coordinates to widget coordinates
         // (This depends on the size of your widget and the zoom level)
  double widgetX = center.x() + x * radius;  // Example scaling
  double widgetY = center.y() - y * radius; // and translation

  return QPointF(widgetX, widgetY);
}

std::complex<double> SmithChartWidget::widgetToSmithChart(const QPointF& widgetPoint)
{
  // Calculate the center and radius of the Smith Chart
  QPointF center(width() / 2.0, height() / 2.0);
  double radius = qMin(width(), height()) / 2.0 - 10; // Leave some margin

  double x = (widgetPoint.x() - center.x()) / radius;
  double y = (center.y() - widgetPoint.y()) / radius;

  double gammaReal = x / (x * x + y * y + 1);
  double gammaImag = y / (x * x + y * y + 1);

  return std::complex<double>(gammaReal, gammaImag);
}
