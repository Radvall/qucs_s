/***************************************************************************
                          painting.cpp  -  description
                             -------------------
    begin                : Sat Nov 22 2003
    copyright            : (C) 2003 by Michael Margraf
    email                : michael.margraf@alumni.tu-berlin.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "painting.h"


QString Painting::save()
{
  return QString();
}

QString Painting::saveCpp()
{
  return QString();
}

QString Painting::saveJSON()
{
  return QString();
}

QString Painting::toPenString (int pen) {
  switch (pen) {
  case Qt::NoPen: return "Qt::NoPen";
  case Qt::SolidLine: return "Qt::SolidLine";
  case Qt::DashLine: return "Qt::DashLine";
  case Qt::DotLine: return "Qt::DotLine";
  case Qt::DashDotLine: return "Qt::DashDotLine";
  case Qt::DashDotDotLine: return "Qt::DashDotDotLine";
  case Qt::MPenStyle: return "Qt::MPenStyle";
  }
  return "Qt::NoPen";
}

QString Painting::toBrushString (int brush) {
  switch (brush) {
  case Qt::NoBrush: return "Qt::NoBrush";
  case Qt::SolidPattern: return "Qt::SolidPattern";
  case Qt::Dense1Pattern: return "Qt::Dense1Pattern";
  case Qt::Dense2Pattern: return "Qt::Dense2Pattern";
  case Qt::Dense3Pattern: return "Qt::Dense3Pattern";
  case Qt::Dense4Pattern: return "Qt::Dense4Pattern";
  case Qt::Dense5Pattern: return "Qt::Dense5Pattern";
  case Qt::Dense6Pattern: return "Qt::Dense6Pattern";
  case Qt::Dense7Pattern: return "Qt::Dense7Pattern";
  case Qt::HorPattern: return "Qt::HorPattern";
  case Qt::VerPattern: return "Qt::VerPattern";
  case Qt::CrossPattern: return "Qt::CrossPattern";
  case Qt::BDiagPattern: return "Qt::BDiagPattern";
  case Qt::FDiagPattern: return "Qt::FDiagPattern";
  case Qt::DiagCrossPattern: return "Qt::DiagCrossPattern";
  case Qt::TexturePattern : return "Qt::CustomPattern ";
  }
  return "Qt::NoBrush";
}

bool Painting::moveCenterTo(int x, int y) noexcept
{
  return moveCenter(x - cx, y - cy);
}

bool Painting::moveCenter(int dx, int dy) noexcept
{
  cx += dx;
  cy += dy;
  x1 += dx;
  x2 += dx;
  y1 += dy;
  y2 += dy;
  afterMove(dx, dy);
  return dx != 0 || dy != 0;
}

void Painting::updateCenter() noexcept
{
  cx = x1 + (x2 - x1) / 2;
  cy = y1 + (y2 - y1) / 2;
}

QRect Painting::boundingRect() const noexcept
{
  return QRect{QPoint{x1, y1}, QSize{x2 - x1, y2 - y1}}.normalized();
}
