/***************************************************************************
                              schematic.cpp
                             ---------------
    begin                : Sat Mar 3 2006
    copyright            : (C) 2006 by Michael Margraf
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

#include <algorithm>
#include <cassert>
#include <limits.h>
#include <stdlib.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFileInfo>
#include <QLineEdit>
#include <QListWidget>
#include <QMouseEvent>
#include <QPaintDevice>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QPrinter>
#include <QRect>
#include <QTextStream>
#include <QUrl>
#include <QWheelEvent>
#include <qt3_compat/qt_compat.h>
#include <QRegularExpression>

#include "components/vafile.h"
#include "components/verilogfile.h"
#include "components/vhdlfile.h"
#include "diagrams/diagrams.h"
#include "main.h"
#include "mouseactions.h"
#include "node.h"
#include "paintings/paintings.h"
#include "qucs.h"
#include "schematic.h"
#include "settings.h"
#include "textdoc.h"

#include "misc.h"

// just dummies for empty lists
Q3PtrList<Wire> SymbolWires;
Q3PtrList<Node> SymbolNodes;
Q3PtrList<Diagram> SymbolDiags;
Q3PtrList<Component> SymbolComps;

/**
    If \c point does not lie within \c rect then returns a new
    rectangle made by enlarging the source rectangle to include
    the \c point. Otherwise returns a rectangle of the same size.
*/
static QRect includePoint(QRect rect, QPoint point) {
  if (rect.contains(point)) {
    return rect;
  } else {
    return rect.united(QRect{point, point});
  }
}

Schematic::Schematic(QucsApp *App_, const QString &Name_) :
    QucsDoc(App_, Name_),
    a_Wires(&a_DocWires),
    a_DocWires(),
    a_Nodes(&a_DocNodes),
    a_DocNodes(),
    a_Diagrams(&a_DocDiags),
    a_DocDiags(),
    a_Paintings(&a_DocPaints),
    a_DocPaints(),
    a_Components(&a_DocComps),
    a_DocComps(),
    a_SymbolPaints(),
    a_PostedPaintEvents(),
    a_symbolMode(false),
    a_isSymbolOnly(false),
    a_GridX(10),
    a_GridY(10),
    a_ViewX1(0),
    a_ViewY1(0),
    a_ViewX2(1),
    a_ViewY2(1),
    a_showFrame(0), // don't show
    a_Frame_Text0(tr("Title")),
    a_Frame_Text1(tr("Drawn By:")),
    a_Frame_Text2(tr("Date:")),
    a_Frame_Text3(tr("Revision:")),
    a_tmpScale(1.0),
    a_tmpViewX1(-200),
    a_tmpViewY1(-200),
    a_tmpViewX2(200),
    a_tmpViewY2(200),
    a_tmpUsedX1(-200),
    a_tmpUsedY1(-200),
    a_tmpUsedX2(200),
    a_tmpUsedY2(200),
    a_undoActionIdx(0),
    // The 'i' means state for being unchanged.
    a_undoAction((QVector<QString*>() << new QString(" i\n</>\n</>\n</>\n</>\n"))),
    a_undoSymbolIdx(0),
    // The 'i' means state for being unchanged.
    a_undoSymbol((QVector<QString*>() << new QString(" i\n</>\n</>\n</>\n</>\n"))),
    a_UsedX1(INT_MAX),
    a_UsedY1(INT_MAX),
    a_UsedX2(INT_MAX),
    a_UsedY2(INT_MAX),
    a_previousCursorPosition(),
    a_dragIsOkay(false),
    a_FileInfo(),
    a_Signals(),
    a_PortTypes(),
    a_isAnalog(false),
    a_isVerilog(false),
    a_creatingLib(false)
{
    setFont(QucsSettings.font);
    a_GridColor = _settings::Get().item<QString>("GridColor");

    a_tmpPosX = a_tmpPosY = -100;

    a_DocComps.setAutoDelete(true);
    a_DocWires.setAutoDelete(true);
    a_DocNodes.setAutoDelete(true);
    a_DocDiags.setAutoDelete(true);
    a_DocPaints.setAutoDelete(true);
    a_SymbolPaints.setAutoDelete(true);

    setVScrollBarMode(Q3ScrollView::AlwaysOn);
    setHScrollBarMode(Q3ScrollView::AlwaysOn);
    misc::setWidgetBackgroundColor(viewport(), QucsSettings.BGColor);
    viewport()->setMouseTracking(true);
    viewport()->setAcceptDrops(true); // enable drag'n drop

    // to repair some strange  scrolling artefacts
    connect(this, SIGNAL(horizontalSliderReleased()), viewport(), SLOT(update()));
    connect(this, SIGNAL(verticalSliderReleased()), viewport(), SLOT(update()));
    if (App_) {
        connect(this,SIGNAL(signalCursorPosChanged(int, int, QString)),App_,SLOT(printCursorPosition(int, int, QString)));

        connect(this, SIGNAL(horizontalSliderPressed()), App_, SLOT(slotHideEdit()));
        connect(this, SIGNAL(verticalSliderPressed()), App_, SLOT(slotHideEdit()));
        connect(this, SIGNAL(signalUndoState(bool)), App_, SLOT(slotUpdateUndo(bool)));
        connect(this, SIGNAL(signalRedoState(bool)), App_, SLOT(slotUpdateRedo(bool)));
        connect(this, SIGNAL(signalFileChanged(bool)), App_, SLOT(slotFileChanged(bool)));
    }
}

Schematic::~Schematic() {}

// ---------------------------------------------------
bool Schematic::createSubcircuitSymbol()
{
    // If the number of ports is not equal, remove or add some.
    unsigned int countPort = adjustPortNumbers();

    // If a symbol does not yet exist, create one.
    if (a_SymbolPaints.count() != countPort)
        return false;

    int h = 30 * ((countPort - 1) / 2) + 10;
    a_SymbolPaints.prepend(new ID_Text(-20, h + 4));

    a_SymbolPaints.append(new GraphicLine(-20, -h, 40, 0, QPen(Qt::darkBlue, 2)));
    a_SymbolPaints.append(new GraphicLine(20, -h, 0, 2 * h, QPen(Qt::darkBlue, 2)));
    a_SymbolPaints.append(new GraphicLine(-20, h, 40, 0, QPen(Qt::darkBlue, 2)));
    a_SymbolPaints.append(new GraphicLine(-20, -h, 0, 2 * h, QPen(Qt::darkBlue, 2)));

    unsigned int i = 0, y = 10 - h;
    while (i < countPort) {
        i++;
        a_SymbolPaints.append(new GraphicLine(-30, y, 10, 0, QPen(Qt::darkBlue, 2)));
        a_SymbolPaints.at(i)->setCenter(-30, y);

        if (i == countPort)
            break;
        i++;
        a_SymbolPaints.append(new GraphicLine(20, y, 10, 0, QPen(Qt::darkBlue, 2)));
        a_SymbolPaints.at(i)->setCenter(30, y);
        y += 60;
    }
    return true;
}

// ---------------------------------------------------
void Schematic::becomeCurrent(bool update)
{
    emit signalCursorPosChanged(0, 0, "");

    // update appropriate menu entry
    if (a_symbolMode) {
        a_App->symEdit->setText(tr("Edit Schematic"));
        a_App->symEdit->setStatusTip(tr("Edits the schematic"));
        a_App->symEdit->setWhatsThis(tr("Edit Schematic\n\nEdits the schematic"));
    } else {
        a_App->symEdit->setText(tr("Edit Circuit Symbol"));
        a_App->symEdit->setStatusTip(tr("Edits the symbol for this schematic"));
        a_App->symEdit->setWhatsThis(
            tr("Edit Circuit Symbol\n\nEdits the symbol for this schematic"));
    }

    if (a_symbolMode) {
        a_Nodes = &SymbolNodes;
        a_Wires = &SymbolWires;
        a_Diagrams = &SymbolDiags;
        a_Paintings = &a_SymbolPaints;
        a_Components = &SymbolComps;

        // "Schematic" is used to edit usual schematic files (containing
        // a schematic and a subcircuit symbol) and *.sym files (which
        // contain *only* a symbol definition). If we're dealing with
        // symbol file, then there is no need to create a subcircuit
        // symbol, a symbol is already there.
        if (!a_DocName.endsWith(".sym") && createSubcircuitSymbol()) {
            updateAllBoundingRect();
            setChanged(true, true);
        }

        emit signalUndoState(a_undoSymbolIdx != 0);
        emit signalRedoState(a_undoSymbolIdx != a_undoSymbol.size() - 1);
    } else {
        a_Nodes = &a_DocNodes;
        a_Wires = &a_DocWires;
        a_Diagrams = &a_DocDiags;
        a_Paintings = &a_DocPaints;
        a_Components = &a_DocComps;

        emit signalUndoState(a_undoActionIdx != 0);
        emit signalRedoState(a_undoActionIdx != a_undoAction.size() - 1);
        if (update)
            reloadGraphs(); // load recent simulation data
    }
}

// ---------------------------------------------------
void Schematic::setName(const QString &Name_)
{
    a_DocName = Name_;
    QFileInfo Info(a_DocName);
    QString base = Info.completeBaseName();
    QString ext = Info.suffix();
    a_DataSet = base + ".dat";
    a_Script = base + ".m";
    if (ext != "dpl")
        a_DataDisplay = base + ".dpl";
    else
        a_DataDisplay = base + ".sch";
}

// ---------------------------------------------------
// Sets the document to be changed or not to be changed.
void Schematic::setChanged(bool c, bool fillStack, char Op)
{
    if ((!a_DocChanged) && c)
        emit signalFileChanged(true);
    else if (a_DocChanged && (!c))
        emit signalFileChanged(false);
    a_DocChanged = c;

    a_showBias = -1; // schematic changed => bias points may be invalid

    if (!fillStack)
        return;

    // ................................................
    if (a_symbolMode) { // for symbol edit mode
        while (a_undoSymbol.size() > a_undoSymbolIdx + 1) {
            delete a_undoSymbol.last();
            a_undoSymbol.pop_back();
        }

        a_undoSymbol.append(new QString(createSymbolUndoString(Op)));
        a_undoSymbolIdx++;

        emit signalUndoState(true);
        emit signalRedoState(false);

        while (static_cast<unsigned int>(a_undoSymbol.size())
               > QucsSettings.maxUndo) { // "while..." because
            delete a_undoSymbol.first();
            a_undoSymbol.pop_front();
            a_undoSymbolIdx--;
        }
        return;
    }

    // ................................................
    // for schematic edit mode
    while (a_undoAction.size() > a_undoActionIdx + 1) {
        delete a_undoAction.last();
        a_undoAction.pop_back();
    }

    if (Op == 'm') { // only one for move marker
        if (a_undoAction.at(a_undoActionIdx)->at(0) == Op) {
            delete a_undoAction.last();
            a_undoAction.pop_back();
            a_undoActionIdx--;
        }
    }

    a_undoAction.append(new QString(createUndoString(Op)));
    a_undoActionIdx++;

    emit signalUndoState(true);
    emit signalRedoState(false);

    while (static_cast<unsigned int>(a_undoAction.size())
           > QucsSettings.maxUndo) { // "while..." because
        delete a_undoAction.first();   // "maxUndo" could be decreased meanwhile
        a_undoAction.pop_front();
        a_undoActionIdx--;
    }
    return;
}

// -----------------------------------------------------------
bool Schematic::sizeOfFrame(int &xall, int &yall)
{
    // Values exclude border of 1.5cm at each side.
    switch (a_showFrame) {
    case 1:
        xall = 1020;
        yall = 765;
        break; // DIN A5 landscape
    case 2:
        xall = 765;
        yall = 1020;
        break; // DIN A5 portrait
    case 3:
        xall = 1530;
        yall = 1020;
        break; // DIN A4 landscape
    case 4:
        xall = 1020;
        yall = 1530;
        break; // DIN A4 portrait
    case 5:
        xall = 2295;
        yall = 1530;
        break; // DIN A3 landscape
    case 6:
        xall = 1530;
        yall = 2295;
        break; // DIN A3 portrait
    case 7:
        xall = 1414;
        yall = 1054;
        break; // letter landscape
    case 8:
        xall = 1054;
        yall = 1414;
        break; // letter portrait
    default:
        return false;
    }

    return true;
}

void Schematic::paintFrame(QPainter* painter) {
    // dimensions:  X cm / 2.54 * 144
    int frame_width, frame_height;
    if (!sizeOfFrame(frame_width, frame_height))
        return;

    painter->save();
    painter->setPen(QPen(Qt::darkGray, 1));

    // Width of stripe along frame border in column and row labels are placed
    const int frame_margin = painter->fontMetrics().lineSpacing() + 4;

    // Outer rect
    painter->drawRect(0, 0, frame_width, frame_height);
    // a bit smaller than outer rect
    painter->drawRect(frame_margin, frame_margin, frame_width - 2 * frame_margin, frame_height - 2 * frame_margin);

    // Column labels
    {
      const int h_step = frame_width / ((frame_width + 127) / 255);
      uint column_number = 1;

      for (int x = h_step; x <= frame_width; x += h_step) {
        painter->drawLine(x, 0, x, frame_margin);
        painter->drawLine(x, frame_height - frame_margin, x, frame_height);

        auto cn = QString::number(column_number);
        auto tx = x - h_step / 2 + 5;
        painter->drawText(tx, 3, 1, 1, Qt::TextDontClip, cn);
        painter->drawText(tx, frame_height - frame_margin + 3, 1, 1, Qt::TextDontClip, cn);

        column_number++;
      }
    }

    // Row labels
    {
      const int v_step = frame_height / ((frame_height + 127) / 255);
      char row_letter = 'A';

      for (int y = v_step; y <= frame_height; y += v_step) {
        painter->drawLine(0, y, frame_margin, y);
        painter->drawLine(frame_width - frame_margin, y, frame_width, y);

        auto rl = QString::fromLatin1(&row_letter, 1);
        auto ty = y - v_step/2 + 5;
        painter->drawText(5, ty, rl);
        painter->drawText(frame_width - frame_margin + 5, ty, rl);

        row_letter++;
      }
    }

    // draw text box with text
    int x1_ = frame_width - 340 - frame_margin;
    int y1_ = frame_height - 3 - frame_margin;
    int x2_ = frame_width - frame_margin - 3;
    int y2_ = frame_height - frame_margin - 3;

    const int d = 6;
    const double z = 200.0;
    y1_ -= painter->fontMetrics().lineSpacing() + d;
    painter->drawLine(x1_, y1_, x2_, y1_);
    painter->drawText(x1_ + d, y1_ + (d >> 1), 1, 1, Qt::TextDontClip, a_Frame_Text2);
    painter->drawLine(x1_ + z, y1_, x1_ + z, y1_ + painter->fontMetrics().lineSpacing() + d);
    painter->drawText(x1_ + d + z, y1_ + (d >> 1), 1, 1, Qt::TextDontClip, a_Frame_Text3);
    y1_ -= painter->fontMetrics().lineSpacing() + d;
    painter->drawLine(x1_, y1_, x2_, y1_);
    painter->drawText(x1_ + d, y1_ + (d >> 1), 1, 1, Qt::TextDontClip, a_Frame_Text1);
    y1_ -= (a_Frame_Text0.count('\n') + 1) * painter->fontMetrics().lineSpacing() + d;
    painter->drawRect(x2_, y2_, x1_ - x2_ - 1, y1_ - y2_ - 1);
    painter->drawText(x1_ + d, y1_ + (d >> 1), 1, 1, Qt::TextDontClip, a_Frame_Text0);

    painter->restore();
}

// -----------------------------------------------------------
// Is called when the content (schematic or data display) has to be drawn.
void Schematic::drawContents(QPainter *p, int, int, int, int)
{
    QTransform trf{p->transform()};
    trf
        .scale(a_Scale, a_Scale)
        .translate(-a_ViewX1, -a_ViewY1);
    p->setTransform(trf);

    auto renderHints = p->renderHints();
    renderHints
        .setFlag(QPainter::Antialiasing)
        .setFlag(QPainter::TextAntialiasing)
        .setFlag(QPainter::SmoothPixmapTransform);
    p->setRenderHints(renderHints);

    p->setFont(QucsSettings.font);
    drawGrid(p);

    if (!a_symbolMode)
        paintFrame(p);

    drawElements(p);
    if (a_showBias > 0) {
        drawDcBiasPoints(p);
    }

    drawPostPaintEvents(p);
}

void Schematic::drawElements(QPainter* painter) {
    for (auto* component : *a_Components) {
        component->paint(painter);
    }

    for (auto* wire : *a_Wires) {
        wire->paint(painter);
        if (wire->Label) {
            wire->Label->paint(painter); // separate because of paintSelected
        }
    }

    for (auto* node : *a_Nodes) {
        node->paint(painter);
        if (node->Label) {
            node->Label->paint(painter); // separate because of paintSelected
        }
    }

    for (auto* diagram : *a_Diagrams) {
        diagram->paint(painter);
    }

    for (auto* painting : *a_Paintings) {
        painting->paint(painter);
    }
}

void Schematic::drawDcBiasPoints(QPainter* painter) {
    painter->save();
    int x, y, z;

    const int xOffset = 10;
    const int yOffset = 10;

    for (auto* pn : *a_Nodes) {
        if (pn->Name.isEmpty())
            continue;

        QString value = misc::formatValue(pn->Name, 4);

        x = pn->cx;
        y = pn->cy + 4;
        z = pn->x1;

        QRect textRect = painter->fontMetrics().boundingRect(value);
        int rectWidth = textRect.width() + 6;
        int rectHeight = textRect.height() + 4;

        if (z & 0x10) {
            x += xOffset;
            y -= yOffset;
        } else {
            x -= xOffset;
            y -= yOffset;
        }

        int rectX = x - rectWidth / 2;
        int rectY = y - rectHeight / 2;

        painter->setBrush(QBrush(QColor(230,230,230)));
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect( QRectF(rectX, rectY, rectWidth, rectHeight),15,15,Qt::RelativeSize);

        painter->setPen(z & 0x10 ? Qt::darkGreen : Qt::blue);
        painter->drawText(x - textRect.width() / 2, y + textRect.height() / 4, value);
    }
    painter->restore();
}



void Schematic::drawPostPaintEvents(QPainter* painter) {
    painter->save();
    /*
   * The following events used to be drawn from mouseactions.cpp, but since Qt4
   * Paint actions can only be called from within the paint event, so they
   * are put into a QList (PostedPaintEvents) and processed here
   */
    for (auto p : a_PostedPaintEvents) {
        // QPainter painter2(viewport()); for if(p.PaintOnViewport)
        QPen pen(Qt::black);
        painter->setPen(Qt::black);
        switch (p.pe) {
        case _NotRop:
            painter->setCompositionMode(QPainter::RasterOp_SourceAndNotDestination);
            break;
        case _Rect:
            painter->drawRect(p.x1, p.y1, p.x2, p.y2);
            break;
        case _SelectionRect:
            pen.setCosmetic(true);
            pen.setStyle(Qt::DashLine);
            pen.setColor(QColor(50, 50, 50, 100));
            painter->setPen(pen);
            painter->fillRect(p.x1, p.y1, p.x2, p.y2, QColor(200, 220, 240, 100));
            painter->drawRect(p.x1, p.y1, p.x2, p.y2);
            break;
        case _Line: {
            painter->save();
            QPen lp{painter->pen()};
            lp.setWidth(p.a == 0 ? 1 : p.a);
            painter->setPen(lp);
            painter->drawLine(p.x1, p.y1, p.x2, p.y2);
            painter->restore();
            break;
        }
        case _Ellipse:
            painter->drawEllipse(p.x1, p.y1, p.x2, p.y2);
            break;
        case _Arc:
            painter->drawArc(p.x1, p.y1, p.x2, p.y2, p.a, p.b);
            break;
        case _DotLine:
            painter->setPen(Qt::DotLine);
            painter->drawLine(p.x1, p.y1, p.x2, p.y2);
            break;
        case _DotRect:
            painter->setPen(Qt::DotLine);
            painter->drawRect(p.x1, p.y1, p.x2, p.y2);
            break;
        case _Translate:; //painter2.translate(p.x1, p.y1);
        case _Scale:; //painter2.scale(p.x1,p.y1);
            break;
        }
    }
    a_PostedPaintEvents.clear();
    painter->restore();
}

void Schematic::PostPaintEvent(
    PE pe, int x1, int y1, int x2, int y2, int a, int b, bool PaintOnViewport)
{
    PostedPaintEvent p = {pe, x1, y1, x2, y2, a, b, PaintOnViewport};
    a_PostedPaintEvents.push_back(p);
    viewport()->update();
    update();
}

// ---------------------------------------------------
void Schematic::contentsMouseMoveEvent(QMouseEvent *Event)
{
    const QPoint modelPos = contentsToModel(Event->pos());
    auto xpos = modelPos.x();
    auto ypos = modelPos.y();
    QString text = "";

    auto doubleToString = [](bool condition, double number) {
      return condition ? misc::num2str(number) : misc::StringNiceNum(number);
    };

    if (a_Diagrams == nullptr) return; // fix for crash on document closing; appears time to time

    for (Diagram* diagram = a_Diagrams->last(); diagram != nullptr; diagram = a_Diagrams->prev()) {
        // BUG: Obtaining the diagram type by name is marked as a bug elsewhere (to be solved separately).
        // TODO: Currently only rectangular diagrams are supported.
        if (diagram->getSelected(xpos, ypos) && diagram->Name == "Rect") {
            bool hasY1, hasY2 = false;
            for (auto graph: diagram->Graphs) {
                hasY1 |= graph->yAxisNo == 0;
                hasY2 |= graph->yAxisNo == 1;
            }

            QPointF mouseClickPoint = QPointF(xpos - diagram->cx, diagram->cy - ypos);
            MappedPoint mp = diagram->pointToValue(mouseClickPoint);

            auto _x = doubleToString(diagram->engineeringNotation, mp.x);
            text = "X=" + _x;
            if (hasY1) {
                text.append("; Y1=");
                auto _y1 = doubleToString(diagram->engineeringNotation, mp.y1);
                text.append(_y1);
            }
            if (hasY2) {
                text.append("; Y2=");
                auto _y2 = doubleToString(diagram->engineeringNotation, mp.y2);
                text.append(_y2);
            }
            break;
        }
    }

    emit signalCursorPosChanged(xpos, ypos, text);

    // Perform "pan with mouse"
    if (Event->buttons() & Qt::MiddleButton) {
        const QPoint currentCursorPosition = contentsToViewport(Event->pos());

        const int dx = currentCursorPosition.x() - a_previousCursorPosition.x();
        if (dx < 0) {
            scrollRight(std::abs(dx));
        } else if (dx > 0) {
            scrollLeft(dx);
        }

        const int dy = currentCursorPosition.y() - a_previousCursorPosition.y();
        if (dy < 0) {
            scrollDown(std::abs(dy));
        } else if (dy > 0) {
            scrollUp(dy);
        }

        a_previousCursorPosition = currentCursorPosition;
    }

    if (a_App->MouseMoveAction)
        (a_App->view->*(a_App->MouseMoveAction))(this, Event);
}

// -----------------------------------------------------------
void Schematic::contentsMousePressEvent(QMouseEvent *Event)
{
    a_App->editText->setHidden(true); // disable text edit of component property
    this->setFocus();
    if (a_App->MouseReleaseAction == &MouseActions::MReleasePaste)
        return;

    const QPoint inModel = contentsToModel(Event->pos());

    if (Event->button() == Qt::RightButton)
        if (a_App->MousePressAction != &MouseActions::MPressElement)
            if (a_App->MousePressAction != &MouseActions::MPressWire2) {
                // show menu on right mouse button
                a_App->view->rightPressMenu(this, Event, inModel.x(), inModel.y());
                if (a_App->MouseReleaseAction)
                    // Is not called automatically because menu has focus.
                    (a_App->view->*(a_App->MouseReleaseAction))(this, Event);
                return;
            }

    // Begin "pan with mouse" action. Panning starts if *only*
    // the middle button is pressed.
    if (Event->button() == Qt::MiddleButton) {
        a_previousCursorPosition = contentsToViewport(Event->pos());
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (a_App->MousePressAction)
        (a_App->view->*(a_App->MousePressAction))(this, Event, inModel.x(), inModel.y());
}

// -----------------------------------------------------------
void Schematic::contentsMouseReleaseEvent(QMouseEvent *Event)
{
    // End "pan with mouse" action.
    if (Event->button() == Qt::MiddleButton) {
        unsetCursor();
        return;
    }

    if (a_App->MouseReleaseAction)
        (a_App->view->*(a_App->MouseReleaseAction))(this, Event);
}

// -----------------------------------------------------------
void Schematic::contentsMouseDoubleClickEvent(QMouseEvent *Event)
{
    if (a_App->MouseDoubleClickAction)
        (a_App->view->*(a_App->MouseDoubleClickAction))(this, Event);
}

void Schematic::print(QPrinter*, QPainter* painter, bool printAll,
                      bool fitToPage, QMargins margins) {
    painter->save();

    const QRectF pageSize{0, 0, static_cast<double>(painter->device()->width()),
                          static_cast<double>(painter->device()->height())};

    QRect printedArea = printAll ? allBoundingRect() : currentSelection().bounds;

    if (printAll && a_showFrame) {
        int frame_width, frame_height;
        sizeOfFrame(frame_width, frame_height);
        printedArea |= QRect{0, 0, frame_width, frame_height};
    }

    printedArea = printedArea.marginsAdded(margins);

    double scale = 1.0;
    if (fitToPage) {
        scale = std::min(pageSize.width() / printedArea.width(),
                         pageSize.height() / printedArea.height());
    } else {
        QFontInfo printerFontInfo{QFont{QucsSettings.font, painter->device()}};
        QFontInfo schematicFontInfo{QucsSettings.font};

        scale = static_cast<double>(printerFontInfo.pixelSize()) /
                static_cast<double>(schematicFontInfo.pixelSize());
    }
    painter->scale(scale, scale);

    painter->translate(-printedArea.left(), -printedArea.top());

    // put picture in center
    {
        auto w = pageSize.width() / scale;
        if (printedArea.width() <= w) {
            auto d = (w - printedArea.width()) / 2;
            painter->translate(d, 0);
        }

        auto h = pageSize.height() / scale;
        if (printedArea.height() <= h) {
            auto d = (h - printedArea.height()) / 2;
            painter->translate(0, d);
        }
    }

    // User chose a font with size in points while looking at the font
    // on screen. The same font size in *points* equals to different
    // amount of pixels when shown on screen or printed on paper,
    // because underlying painting devices have different resolutions.
    // To preserve the ratio of text sizes and elements' sizes
    // font size has to be set in pixels here. This makes lines, squares,
    // circles, etc. and size of font be measured in same units and scale
    // them equally.
    auto f = QucsSettings.font;
    QFontInfo fi{f};
    f.setPixelSize(fi.pixelSize());
    painter->setFont(f);

    paintSchToViewpainter(painter, printAll);

    painter->restore();
}

namespace {
// helper to be used in Schematic::paintSchToViewpainter
template <typename T> void draw_preserve_selection(T* elem, QPainter* p) {
    bool selected = elem->isSelected;
    elem->isSelected = false;
    elem->paint(p);
    elem->isSelected = selected;
}
} // namespace

void Schematic::paintSchToViewpainter(QPainter* painter, bool printAll) {
    if (printAll && a_showFrame && !a_symbolMode) {
        paintFrame(painter);
    }

    const auto should_draw = [=](Element* drawable) {
      return printAll || drawable->isSelected;
    };

    for (auto* component : *a_Components) {
        if (should_draw(component)) {
            draw_preserve_selection(component, painter);
        }
    }

    for (auto* wire : *a_Wires) {
        if (should_draw(wire)) {
            draw_preserve_selection(wire, painter);
        }

        if (auto* label = wire->Label) {
            if (should_draw(label)) {
                draw_preserve_selection(label, painter);
            }
        }
    }

    for (auto* node : *a_Nodes) {
        for (auto* connected : *node) {
            if (should_draw(connected)) {
                draw_preserve_selection(node, painter);
                break;
            }
        }

        if (auto* label = node->Label) {
            if (should_draw(label)) {
                draw_preserve_selection(label, painter);
            }
        }
    }

    for (auto* painting : *a_Paintings) {
        if (should_draw(painting)) {
            draw_preserve_selection(painting, painter);
        }
    }

    for (auto* diagram : *a_Diagrams) {
        if (!should_draw(diagram)) {
            continue;
        }

        // if graph or marker is selected, deselect during printing
        for (Graph* pg : diagram->Graphs) {
            if (pg->isSelected) {
                pg->Type |= 1; // remember selection
            }
            pg->isSelected = false;
            for (Marker* pm : pg->Markers) {
                if (pm->isSelected) {
                    pm->Type |= 1; // remember selection
                }
                pm->isSelected = false;
            }
        }
        draw_preserve_selection(diagram, painter);

        // revert selection of graphs and markers
        for (Graph* pg : diagram->Graphs) {
            if (pg->Type & 1) {
                pg->isSelected = true;
            }
            pg->Type &= -2;
            for (Marker* pm : pg->Markers) {
                if (pm->Type & 1) {
                    pm->isSelected = true;
                }
                pm->Type &= -2;
            }
        }
    }

    if (a_showBias > 0) { // show DC bias points in schematic ?
        drawDcBiasPoints(painter);
    }
}

void Schematic::zoomAroundPoint(double offeredScaleChange, QPoint coords, bool viewportRelative=true)
{
    const double desiredScale = a_Scale * offeredScaleChange;
    const auto viewportCoords =
        viewportRelative ? coords : coords - QPoint{contentsX(), contentsY()};
    const auto focusPoint = viewportToModel(viewportCoords);
    const auto model = includePoint(modelRect(), focusPoint);

    renderModel(desiredScale, model, focusPoint, viewportCoords);
}

// -----------------------------------------------------------
float Schematic::zoomBy(float s)
{
    // Change scale and keep the point displayed in the center
    // of the viewport at same place after scaling.

    const double newScale = a_Scale * s;
    const auto vpCenter = viewportRect().center();
    const auto centerPoint = viewportToModel(vpCenter);
    const auto model = includePoint(modelRect(), centerPoint);

    return renderModel(newScale, model, centerPoint, vpCenter);
}

// ---------------------------------------------------
void Schematic::showAll()
{
    sizeOfAll(a_UsedX1, a_UsedY1, a_UsedX2, a_UsedY2);
    if (a_UsedX1 == 0)
        if (a_UsedX2 == 0)
            if (a_UsedY1 == 0)
                if (a_UsedY2 == 0) {
                    a_UsedX1 = a_UsedY1 = INT_MAX;
                    a_UsedX2 = a_UsedY2 = INT_MIN;
                    return;
                }

    // Reshape model plane to cut off unused parts
    constexpr int margin = 40;
    QRect newModelBounds = modelRect();
    newModelBounds.setLeft(a_UsedX1 - margin);
    newModelBounds.setTop(a_UsedY1 - margin);
    newModelBounds.setRight(a_UsedX2 + margin);
    newModelBounds.setBottom(a_UsedY2 + margin);

    // The shape of the model plane may not fit the shape of the viewport,
    // so we looking for a scale value which enables to fit the whole model
    // into the viewport
    const double xScale = static_cast<double>(viewport()->width()) /
                          static_cast<double>(newModelBounds.width());
    const double yScale = static_cast<double>(viewport()->height()) /
                          static_cast<double>(newModelBounds.height());
    const double newScale = std::min(xScale, yScale);

    renderModel(newScale, newModelBounds, newModelBounds.center(), viewportRect().center());
}

// ------------------------------------------------------
void Schematic::zoomToSelection() {
    sizeOfAll(a_UsedX1, a_UsedY1, a_UsedX2, a_UsedY2);
    if (a_UsedX1 == 0)
        if (a_UsedX2 == 0)
            if (a_UsedY1 == 0)
                if (a_UsedY2 == 0) {
                    a_UsedX1 = a_UsedY1 = INT_MAX;
                    a_UsedX2 = a_UsedY2 = INT_MIN;

                    // No elements present – nothing can be selected; quit
                    return;
                }

    const QRect selectedBoundingRect{ currentSelection().bounds };

    // Working with raw coordinates is clumsy, abstract them out
    const QRect usedBoundingRect{a_UsedX1, a_UsedY1, a_UsedX2 - a_UsedX1, a_UsedY2 - a_UsedY1};

    if (selectedBoundingRect.width() == 0 || selectedBoundingRect.height() == 0) {
        // If nothing is selected, then what should be shown? Probably it's best
        // to do nothing.
        return;
    }

    // While we here, lets reshape model plane to cut off unused parts
    constexpr int margin = 40;
    QRect modelBounds = modelRect();
    modelBounds.setLeft(usedBoundingRect.left() - margin);
    modelBounds.setTop(usedBoundingRect.top() - margin);
    modelBounds.setRight(usedBoundingRect.right() + margin);
    modelBounds.setBottom(usedBoundingRect.bottom() + margin);

    // Find out the scale at which selected area's longest side would fit
    // into the viewport
    const double xScale = static_cast<double>(viewport()->width()) /
                          static_cast<double>(selectedBoundingRect.width());
    const double yScale = static_cast<double>(viewport()->height()) /
                          static_cast<double>(selectedBoundingRect.height());
    const double newScale = std::min(xScale, yScale);

    renderModel(newScale, modelBounds, selectedBoundingRect.center(), viewportRect().center());
}

// ---------------------------------------------------
void Schematic::showNoZoom()
{
    constexpr double noScale = 1.0;
    const QPoint vpCenter = viewportRect().center();
    const QPoint displayedInCenter = viewportToModel(vpCenter);

    sizeOfAll(a_UsedX1, a_UsedY1, a_UsedX2, a_UsedY2);
    if (a_UsedX1 == 0)
        if (a_UsedX2 == 0)
            if (a_UsedY1 == 0)
                if (a_UsedY2 == 0) {
                    a_UsedX1 = a_UsedY1 = INT_MAX;
                    a_UsedX2 = a_UsedY2 = INT_MIN;
                    // If there is no elements in schematic, then just set scale 1.0
                    // at the place we currently in.
                    renderModel(noScale, includePoint(modelRect(), displayedInCenter), displayedInCenter, vpCenter);
                    return;
                }

    // Working with raw coordinates is clumsy. Wrap them in useful abstraction.
    const QRect usedBoundingRect{a_UsedX1, a_UsedY1, a_UsedX2 - a_UsedX1, a_UsedY2 - a_UsedY1};

    // Trim unused model space
    constexpr int margin = 40;
    QRect newModelBounds = modelRect();
    newModelBounds.setLeft(usedBoundingRect.left() - margin);
    newModelBounds.setTop(usedBoundingRect.top() - margin);
    newModelBounds.setRight(usedBoundingRect.right() + margin);
    newModelBounds.setBottom(usedBoundingRect.bottom() + margin);

    // If a part of "used" area is currently displayed in the center of the
    // viewport, then keep it in the same place after scaling. Otherwise focus
    // on the center of the used area after scale change.
    if (usedBoundingRect.contains(displayedInCenter)) {
        renderModel(noScale, newModelBounds, displayedInCenter, vpCenter);
    } else {
        renderModel(noScale, newModelBounds, usedBoundingRect.center(), vpCenter);
    }
 }

 // If the model plane is smaller than rectangle described by points (x1, y1)
 // and (x2, y2) than extend the model plane size.
 void Schematic::enlargeView(int x1, int y1, int x2, int y2) {
    // Set 'Used' area size to the of the given rectangle
    if (x1 < a_UsedX1)
        a_UsedX1 = x1;
    if (y1 < a_UsedY1)
        a_UsedY1 = y1;
    if (x2 > a_UsedX2)
        a_UsedX2 = x2;
    if (y2 > a_UsedY2)
        a_UsedY2 = y2;

    // Construct the desired model plane
    constexpr int margin = 40;
    QRect newModel = modelRect();
    if (x1 < a_ViewX1)
        newModel.setLeft(x1 - margin);
    if (y1 < a_ViewY1)
        newModel.setTop(y1 - margin);
    if (x2 > a_ViewX2)
        newModel.setRight(x2 + margin);
    if (y2 > a_ViewY2)
        newModel.setBottom(y2 + margin);

    const auto vpCenter = viewportRect().center();
    const auto displayedInCenter = viewportToModel(vpCenter);
    newModel = includePoint(newModel, displayedInCenter);
    renderModel(a_Scale, newModel, displayedInCenter, vpCenter);
 }

 QPoint Schematic::setOnGrid(const QPoint& p) {
   QPoint snappedToGrid{p.x(), p.y()};
   setOnGrid(snappedToGrid.rx(), snappedToGrid.ry());
   return snappedToGrid;
 }

// ---------------------------------------------------
// Sets an arbitrary coordinate onto the next grid coordinate.
void Schematic::setOnGrid(int &x, int &y)
{
    if (x < 0)
        x -= (a_GridX >> 1) - 1;
    else
        x += a_GridX >> 1;
    x -= x % a_GridX;

    if (y < 0)
        y -= (a_GridY >> 1) - 1;
    else
        y += a_GridY >> 1;
    y -= y % a_GridY;
}

void Schematic::drawGrid(QPainter* painter) {
    if (!a_GridOn)
        return;

    painter->save();
    // Painter might have been scaled somewhere upstream in call stack,
    // and we don't grid points to change their size or thickness
    // depending on zoom level. Thus we remove any transformations
    // and draw on "raw" painter, controlling all offsets manually
    painter->setTransform(QTransform{});

    // A grid drawn with pen of 1.0 width reportedly looks good both
    // on standard and HiDPI displays.
    // See here for details https://github.com/ra3xdh/qucs_s/pull/524
    painter->setPen(QPen{ a_GridColor, 1.0 });

    {
        // Draw small cross at origin of coordinates
        const QPoint origin = modelToViewport(QPoint{0, 0});
        painter->drawLine(origin.x() - 3, origin.y(), origin.x() + 4, origin.y());  // horizontal stick
        painter->drawLine(origin.x(), origin.y() - 3, origin.x(), origin.y() + 4);  // vertical stick
    }

    // Grid is drawn as a set of nodes, each node looks like a point and a node
    // is located at every horizontal and vertical step:
    // .  .  .  .  .
    // .  .  .  .  .
    // .  .  .  .  .
    // .  .  .  .  .
    //
    // To find out where to start drawing grid nodes, we find a point
    // which is currently shown at the top left corner of the viewport
    // and then find a grid-node nearest to this point. We then convert these
    // grid-node coordinates back to viewport-coordinates. This gives us
    // coordinates of a point somewhere around the top-left corner of the
    // viewport. This point corresponds to a grid-node. The same is done to a
    // bottom-right corner. Two resulting points decsribe the area where
    // grid-nodes should be drawn — where to start and where to finish drawing
    // these nodes.

    QPoint topLeft = viewportToModel(viewportRect().topLeft());
    const QPoint gridTopLeft = modelToViewport(setOnGrid(topLeft));

    QPoint bottomRight = viewportToModel(viewportRect().bottomRight());
    const QPoint gridBottomRight = modelToViewport(setOnGrid(bottomRight));

    // This is the minimal distance between drawn grid-nodes. No matter how
    // a user scales the view, any two adjacent nodes must have at least this
    // amount of "free space" between them.
    constexpr double minimalVisibleGridStep = 8.0;

    // In some scales drawing a point for every step may lead to a very dense
    // grid without much space between nodes. But we want to have some minimal
    // distance between them. In such cases nodes shouldn't be drawn for every
    // grid-step, but instead for every two grid-steps, or every three, and so on.
    //
    // To find out how frequently grid nodes should be drawn, we start from single
    // grid-step and grow it until its "size in scale" gets larger than the minimal
    // distance between points.

    double horizontalStep{ a_GridX * a_Scale };
    for (int n = 2; horizontalStep < minimalVisibleGridStep; n++) {
        horizontalStep = n * a_GridX * a_Scale;
    }

    double verticalStep{ a_GridY * a_Scale };
    for (int n = 2; verticalStep < minimalVisibleGridStep; n++) {
        verticalStep = n * a_GridY * a_Scale;
    }

    // Finally draw the grid-nodes
    for (double x = gridTopLeft.x(); x <= gridBottomRight.x(); x += horizontalStep) {
        for (double y = gridTopLeft.y(); y <= gridBottomRight.y(); y += verticalStep) {
            painter->drawPoint(std::round(x), std::round(y));
        }
    }

    painter->restore();
}

void Schematic::relativeRotation(int &newX, int &newY, int comX, int comY, int oldX, int oldY)
{
    // Shift coordinate system to center of mass
    // Rotate
    // Shift coordinate system back to origin
    newX = (oldY-comY)+comX;
    newY = -(oldX-comX)+comY;
}

void Schematic::updateAllBoundingRect() {
    sizeOfAll(a_UsedX1, a_UsedY1, a_UsedX2, a_UsedY2);
}

QRect Schematic::allBoundingRect() {
    updateAllBoundingRect();
    return QRect{a_UsedX1, a_UsedY1, (a_UsedX2 - a_UsedX1), (a_UsedY2 - a_UsedY1)};
}

// ---------------------------------------------------
void Schematic::sizeOfAll(int &xmin, int &ymin, int &xmax, int &ymax)
{
    xmin = INT_MAX;
    ymin = INT_MAX;
    xmax = INT_MIN;
    ymax = INT_MIN;

    if (a_Components->isEmpty() && a_Wires->isEmpty() && a_Diagrams->isEmpty() && a_Paintings->isEmpty()) {
        xmin = xmax = 0;
        ymin = ymax = 0;
        return;
    }

    int x1, y1, x2, y2;
    // find boundings of all components
    for (auto* pc : *a_Components) {
        pc->entireBounds(x1, y1, x2, y2);
        xmin = std::min(x1, xmin);
        xmax = std::max(x2, xmax);
        ymin = std::min(y1, ymin);
        ymax = std::max(y2, ymax);
    }

    // find boundings of all wires
    for (auto* pw : *a_Wires) {
        xmin = std::min(pw->x1, xmin);
        xmax = std::max(pw->x2, xmax);
        ymin = std::min(pw->y1, ymin);
        ymax = std::max(pw->y2, ymax);

        if (auto* pl = pw->Label; pl) { // check position of wire label
            pl->getLabelBounding(x1, y1, x2, y2);
            xmin = std::min(x1, xmin);
            xmax = std::max(x2, xmax);
            ymin = std::min(y1, ymin);
            ymax = std::max(y2, ymax);
        }
    }

    // find boundings of all node labels
    for (auto* pn : *a_Nodes) {
        if (auto* pl = pn->Label; pl) { // check position of node label
            pl->getLabelBounding(x1, y1, x2, y2);
            xmin = std::min(x1, xmin);
            xmax = std::max(x2, xmax);
            ymin = std::min(y1, ymin);
            ymax = std::max(y2, ymax);
        }
    }

    // find boundings of all diagrams
    for (auto* pd : *a_Diagrams) {
        pd->Bounding(x1, y1, x2, y2);
        xmin = std::min(x1, xmin);
        xmax = std::max(x2, xmax);
        ymin = std::min(y1, ymin);
        ymax = std::max(y2, ymax);

        for (auto* pg : pd->Graphs)
            // test all markers of diagram
            for (auto* pm : pg->Markers) {
                pm->Bounding(x1, y1, x2, y2);
                xmin = std::min(x1, xmin);
                xmax = std::max(x2, xmax);
                ymin = std::min(y1, ymin);
                ymax = std::max(y2, ymax);
            }
    }

    // find boundings of all Paintings
    for (auto* pp : *a_Paintings) {
        pp->Bounding(x1, y1, x2, y2);
        xmin = std::min(x1, xmin);
        xmax = std::max(x2, xmax);
        ymin = std::min(y1, ymin);
        ymax = std::max(y2, ymax);
    }
}

Schematic::Selection Schematic::currentSelection() const {
    int xmin = INT_MAX;
    int ymin = INT_MAX;
    int xmax = INT_MIN;
    int ymax = INT_MIN;

    bool isAnySelected = false;

    if (a_Components->isEmpty() && a_Wires->isEmpty() && a_Diagrams->isEmpty() &&
        a_Paintings->isEmpty()) {
        return {};
    }

    Selection selection;

    int x1, y1, x2, y2;
    // find boundings of all components
    for (auto* pc : *a_Components) {
        if (!pc->isSelected) {
            continue;
        }
        isAnySelected = true;
        selection.components.push_back(pc);
        pc->entireBounds(x1, y1, x2, y2);
        xmin = std::min(x1, xmin);
        xmax = std::max(x2, xmax);
        ymin = std::min(y1, ymin);
        ymax = std::max(y2, ymax);
    }

    // find boundings of all wires
    for (auto* pw : *a_Wires) {
        if (pw->isSelected) {
            isAnySelected = true;
            selection.wires.push_back(pw);
            xmin = std::min(pw->x1, xmin);
            xmax = std::max(pw->x2, xmax);
            ymin = std::min(pw->y1, ymin);
            ymax = std::max(pw->y2, ymax);
        }

        if (pw->Label != nullptr && pw->Label->isSelected) { // check position of wire label
            isAnySelected = true;
            selection.labels.push_back(pw->Label);
            pw->Label->getLabelBounding(x1, y1, x2, y2);
            xmin = std::min(x1, xmin);
            xmax = std::max(x2, xmax);
            ymin = std::min(y1, ymin);
            ymax = std::max(y2, ymax);
        }
    }

    // find boundings of all node labels
    for (auto* pn : *a_Nodes) {
        if (std::all_of(pn->begin(), pn->end(), [](auto* e) { return e->isSelected; })) {
            selection.nodes.push_back(pn);
        }

        if (pn->Label != nullptr && pn->Label->isSelected) { // check position of node label
            selection.labels.push_back(pn->Label);
            isAnySelected = true;
            pn->Label->getLabelBounding(x1, y1, x2, y2);
            xmin = std::min(x1, xmin);
            xmax = std::max(x2, xmax);
            ymin = std::min(y1, ymin);
            ymax = std::max(y2, ymax);
        }
    }

    // find boundings of all diagrams
    for (auto* pd : *a_Diagrams) {
        if (pd->isSelected) {
            selection.diagrams.push_back(pd);
            isAnySelected = true;
            pd->Bounding(x1, y1, x2, y2);
            xmin = std::min(x1, xmin);
            xmax = std::max(x2, xmax);
            ymin = std::min(y1, ymin);
            ymax = std::max(y2, ymax);
        }

        for (Graph* pg : pd->Graphs) {
            // test all markers of diagram
            for (Marker* pm : pg->Markers) {
                if (!pm->isSelected) continue;
                isAnySelected = true;
                selection.markers.push_back(pm);
                pm->Bounding(x1, y1, x2, y2);
                xmin = std::min(x1, xmin);
                xmax = std::max(x2, xmax);
                ymin = std::min(y1, ymin);
                ymax = std::max(y2, ymax);
            }
        }
    }

    // find boundings of all Paintings
    for (auto* pp : *a_Paintings) {
        if (!pp->isSelected) {
            continue;
        }
        selection.paintings.push_back(pp);
        isAnySelected = true;
        pp->Bounding(x1, y1, x2, y2);
        xmin = std::min(x1, xmin);
        xmax = std::max(x2, xmax);
        ymin = std::min(y1, ymin);
        ymax = std::max(y2, ymax);
    }

    if (!isAnySelected) {
        return {};
    }

    selection.bounds = QRect{xmin, ymin, xmax - xmin, ymax - ymin};
    return selection;
}

// ---------------------------------------------------
// Rotates all selected components around their midpoint.
bool Schematic::rotateElements()
{
    a_Wires->setAutoDelete(false);
    a_Components->setAutoDelete(false);

    // To rotate a selected area its necessary to work with half steps
    int x1 = INT_MAX, y1 = INT_MAX;
    int x2 = INT_MIN, y2 = INT_MIN;
    QList<Element *> ElementCache;
    copyLabels(x1, y1, x2, y2, &ElementCache); // must be first of all !
    copyComponents(x1, y1, x2, y2, &ElementCache);
    copyWires(x1, y1, x2, y2, &ElementCache);
    copyPaintings(x1, y1, x2, y2, &ElementCache);
    if (y1 == INT_MAX)
    {
        return false; // no element selected
    }
    int comX = (x1 + ((x2-x1) / 2)); // center of mass
    int comY = (y1 + ((y2-y1) / 2)); 
    int newPosX = INT_MIN; 
    int newPosY = INT_MIN;

    a_Wires->setAutoDelete(true);
    a_Components->setAutoDelete(true);
    setOnGrid(comX, comY);

    Wire *pw;
    Painting *pp;
    Component *pc;
    WireLabel *pl;
    // re-insert elements
    for (Element *pe : ElementCache)
        switch (pe->Type) {
        case isComponent:
        case isAnalogComponent:
        case isDigitalComponent:
            pc = (Component *) pe;
            pc->rotate(); //rotate component !before! rotating its center
            relativeRotation(newPosX, newPosY, comX, comY, pc->cx, pc->cy);
            pc->setCenter(newPosX, newPosY);
            insertRawComponent(pc);
            break;

        case isWire:
            pw = (Wire *) pe;
            relativeRotation(newPosX, newPosY, comX, comY, pw->x1, pw->y1);
            pw->x1 = newPosX;
            pw->y1 = newPosY;
            relativeRotation(newPosX, newPosY, comX, comY, pw->x2, pw->y2);
            pw->x2 = newPosX;
            pw->y2 = newPosY;

            pl = pw->Label;
            if (pl) {
                x2 = pl->cx;
                relativeRotation(newPosX, newPosY, comX, comY, pl->cx, pl->cy);
                pl->cx = newPosX;
                pl->cy = newPosY;
                if (pl->Type == isHWireLabel)
                    pl->Type = isVWireLabel;
                else
                    pl->Type = isHWireLabel;
            }
            insertWire(pw);
            break;

        case isHWireLabel:
        case isVWireLabel:
            pl = (WireLabel *) pe;
            relativeRotation(newPosX, newPosY, comX, comY, pl->x1, pl->y1);
            pl->x1 = newPosX;
            pl->y1 = newPosY;
            break;
        case isNodeLabel:
            pl = (WireLabel *) pe;
            if (pl->pOwner == 0) {
                relativeRotation(newPosX, newPosY, comX, comY, pl->x1, pl->y1);
                pl->x1 = newPosX;
                pl->y1 = newPosY;
            }
            relativeRotation(newPosX, newPosY, comX, comY, pl->cx, pl->cy);
            pl->cx = newPosX;
            pl->cy = newPosY;
            insertNodeLabel(pl);
            break;

        case isPainting:
            pp = (Painting *) pe;
            pp->rotate(x1, y1); // rotate around the center x1 y1
            a_Paintings->append(pp);
            break;
        default:;
        }

    ElementCache.clear();

    setChanged(true, true);
    return true;
}

// ---------------------------------------------------
// Mirrors all selected components.
// First copy them to 'ElementCache', then mirror and insert again.
bool Schematic::mirrorXComponents()
{
    a_Wires->setAutoDelete(false);
    a_Components->setAutoDelete(false);

    int x1, y1, x2, y2;
    QList<Element *> ElementCache;
    if (!copyComps2WiresPaints(x1, y1, x2, y2, &ElementCache))
        return false;
    a_Wires->setAutoDelete(true);
    a_Components->setAutoDelete(true);

    y1 = (y1 + y2) >> 1; // axis for mirroring
    setOnGrid(y2, y1);
    y1 <<= 1;

    Wire *pw;
    Painting *pp;
    Component *pc;
    WireLabel *pl;
    // re-insert elements
    for (Element *pe : ElementCache)
        switch (pe->Type) {
        case isComponent:
        case isAnalogComponent:
        case isDigitalComponent:
            pc = (Component *) pe;
            pc->mirrorX(); // mirror component !before! mirroring its center
            pc->setCenter(pc->cx, y1 - pc->cy);
            insertRawComponent(pc);
            break;
        case isWire:
            pw = (Wire *) pe;
            pw->y1 = y1 - pw->y1;
            pw->y2 = y1 - pw->y2;
            pl = pw->Label;
            if (pl)
                pl->cy = y1 - pl->cy;
            insertWire(pw);
            break;
        case isHWireLabel:
        case isVWireLabel:
            pl = (WireLabel *) pe;
            pl->y1 = y1 - pl->y1;
            break;
        case isNodeLabel:
            pl = (WireLabel *) pe;
            if (pl->pOwner == 0)
                pl->y1 = y1 - pl->y1;
            pl->cy = y1 - pl->cy;
            insertNodeLabel(pl);
            break;
        case isPainting:
            pp = (Painting *) pe;
            pp->getCenter(x2, y2);
            pp->mirrorX(); // mirror painting !before! mirroring its center
            pp->setCenter(x2, y1 - y2);
            a_Paintings->append(pp);
            break;
        default:;
        }

    ElementCache.clear();
    setChanged(true, true);
    return true;
}

// ---------------------------------------------------
// Mirrors all selected components. First copy them to 'ElementCache', then mirror and insert again.
bool Schematic::mirrorYComponents()
{
    a_Wires->setAutoDelete(false);
    a_Components->setAutoDelete(false);

    int x1, y1, x2, y2;
    QList<Element *> ElementCache;
    if (!copyComps2WiresPaints(x1, y1, x2, y2, &ElementCache))
        return false;
    a_Wires->setAutoDelete(true);
    a_Components->setAutoDelete(true);

    x1 = (x1 + x2) >> 1; // axis for mirroring
    setOnGrid(x1, x2);
    x1 <<= 1;

    Wire *pw;
    Painting *pp;
    Component *pc;
    WireLabel *pl;
    // re-insert elements
    for (Element *pe : ElementCache)
        switch (pe->Type) {
        case isComponent:
        case isAnalogComponent:
        case isDigitalComponent:
            pc = (Component *) pe;
            pc->mirrorY(); // mirror component !before! mirroring its center
            pc->setCenter(x1 - pc->cx, pc->cy);
            insertRawComponent(pc);
            break;
        case isWire:
            pw = (Wire *) pe;
            pw->x1 = x1 - pw->x1;
            pw->x2 = x1 - pw->x2;
            pl = pw->Label;
            if (pl)
                pl->cx = x1 - pl->cx;
            insertWire(pw);
            break;
        case isHWireLabel:
        case isVWireLabel:
            pl = (WireLabel *) pe;
            pl->x1 = x1 - pl->x1;
            break;
        case isNodeLabel:
            pl = (WireLabel *) pe;
            if (pl->pOwner == 0)
                pl->x1 = x1 - pl->x1;
            pl->cx = x1 - pl->cx;
            insertNodeLabel(pl);
            break;
        case isPainting:
            pp = (Painting *) pe;
            pp->getCenter(x2, y2);
            pp->mirrorY(); // mirror painting !before! mirroring its center
            pp->setCenter(x1 - x2, y2);
            a_Paintings->append(pp);
            break;
        default:;
        }

    ElementCache.clear();
    setChanged(true, true);
    return true;
}

// ---------------------------------------------------
// Updates the graph data of all diagrams (load from data files).
void Schematic::reloadGraphs()
{
    QFileInfo Info(a_DocName);
    for (Diagram *pd = a_Diagrams->first(); pd != 0; pd = a_Diagrams->next())
        pd->loadGraphData(Info.path() + QDir::separator() + a_DataSet);
}

// Copy function,
void Schematic::copy()
{
    QString s = createClipboardFile();
    QClipboard *cb = QApplication::clipboard(); // get system clipboard
    if (!s.isEmpty()) {
        cb->setText(s, QClipboard::Clipboard);
    }
}

// ---------------------------------------------------
// Cut function, copy followed by deletion
void Schematic::cut()
{
    copy();
    deleteElements(); //delete selected elements
    viewport()->update();
}

// ---------------------------------------------------
// Performs paste function from clipboard
bool Schematic::paste(QTextStream *stream, QList<Element*> *pe)
{
    return pasteFromClipboard(stream, pe);
}

// ---------------------------------------------------
// Loads this Qucs document.
bool Schematic::load()
{
    a_DocComps.clear();
    a_DocWires.clear();
    a_DocNodes.clear();
    a_DocDiags.clear();
    a_DocPaints.clear();
    a_SymbolPaints.clear();

    if (!loadDocument())
        return false;
    a_lastSaved = QDateTime::currentDateTime();

    while (!a_undoAction.isEmpty()) {
        delete a_undoAction.last();
        a_undoAction.pop_back();
    }
    a_undoActionIdx = 0;
    while (!a_undoSymbol.isEmpty()) {
        delete a_undoSymbol.last();
        a_undoSymbol.pop_back();
    }
    a_symbolMode = true;
    setChanged(false, true); // "not changed" state, but put on undo stack
    a_undoSymbolIdx = 0;
    a_undoSymbol.at(a_undoSymbolIdx)->replace(1, 1, 'i');
    a_symbolMode = false;
    setChanged(false, true); // "not changed" state, but put on undo stack
    a_undoActionIdx = 0;
    a_undoAction.at(a_undoActionIdx)->replace(1, 1, 'i');

    // The undo stack of the circuit symbol is initialized when first
    // entering its edit mode.

    // have to call this to avoid crash at sizeOfAll
    becomeCurrent(false);

    showAll();
    a_tmpViewX1 = a_tmpViewY1 = -200; // was used as temporary cache
    return true;
}

// ---------------------------------------------------
// Saves this Qucs document. Returns the number of subcircuit ports.
int Schematic::save()
{
    int result = 0;
    // When saving *only* a symbol, there is no corresponding schematic:
    // and thus ports in symbol don't have corresponding ports in schematic.
    // There is just nothing to adjust.
    //
    // In other cases we want to delete any dangling ports from symbol
    // and invoke "adjustPortNumbers" for it.
    if (!a_isSymbolOnly) {
        result = adjustPortNumbers(); // same port number for schematic and symbol
    } else {
        orderSymbolPorts();
    }
    if (saveDocument() < 0)
        return -1;

    QFileInfo Info(a_DocName);
    a_lastSaved = Info.lastModified();

    if (result >= 0) {
        setChanged(false);

        QVector<QString *>::iterator it;
        for (it = a_undoAction.begin(); it != a_undoAction.end(); it++) {
            (*it)->replace(1, 1, ' '); //at(1) = ' '; state of being changed
        }
        //(1) = 'i';   // state of being unchanged
        a_undoAction.at(a_undoActionIdx)->replace(1, 1, 'i');

        for (it = a_undoSymbol.begin(); it != a_undoSymbol.end(); it++) {
            (*it)->replace(1, 1, ' '); //at(1) = ' '; state of being changed
        }
        //at(1) = 'i';   // state of being unchanged
        a_undoSymbol.at(a_undoSymbolIdx)->replace(1, 1, 'i');
    }
    // update the subcircuit file lookup hashes
    //QucsMain->updateSchNameHash();
    //QucsMain->updateSpiceNameHash();

    return result;
}

// ---------------------------------------------------
// If the port number of the schematic and of the symbol are not
// equal add or remove some in the symbol.
int Schematic::adjustPortNumbers()
{
    int x1, x2, y1, y2;
    // get size of whole symbol to know where to place new ports
    if (a_symbolMode)
        sizeOfAll(x1, y1, x2, y2);
    else {
        a_Components = &SymbolComps;
        a_Wires = &SymbolWires;
        a_Nodes = &SymbolNodes;
        a_Diagrams = &SymbolDiags;
        a_Paintings = &a_SymbolPaints;
        sizeOfAll(x1, y1, x2, y2);
        a_Components = &a_DocComps;
        a_Wires = &a_DocWires;
        a_Nodes = &a_DocNodes;
        a_Diagrams = &a_DocDiags;
        a_Paintings = &a_DocPaints;
    }
    x1 += 40;
    y2 += 20;
    setOnGrid(x1, y2);

    Painting *pp;
    // delete all port names in symbol
    for (pp = a_SymbolPaints.first(); pp != 0; pp = a_SymbolPaints.next())
        if (pp->Name == ".PortSym ")
            ((PortSymbol *) pp)->nameStr = "";

    QString Str;
    int countPort = 0;

    QFileInfo Info(a_DataDisplay);
    QString Suffix = Info.suffix();

    // handle VHDL file symbol
    if (Suffix == "vhd" || Suffix == "vhdl") {
        QStringList::iterator it;
        QStringList Names, GNames, GTypes, GDefs;
        int Number;

        // get ports from VHDL file
        QFileInfo Info(a_DocName);
        QString Name = Info.path() + QDir::separator() + a_DataDisplay;

        // obtain VHDL information either from open text document or the
        // file directly
        VHDL_File_Info VInfo;
        TextDoc *d = (TextDoc *) a_App->findDoc(Name);
        if (d)
            VInfo = VHDL_File_Info(d->document()->toPlainText());
        else
            VInfo = VHDL_File_Info(Name, true);

        if (!VInfo.PortNames.isEmpty())
            Names = VInfo.PortNames.split(",", Qt::SkipEmptyParts);

        for (pp = a_SymbolPaints.first(); pp != 0; pp = a_SymbolPaints.next())
            if (pp->Name == ".ID ") {
                ID_Text *id = (ID_Text *) pp;
                id->Prefix = VInfo.EntityName.toUpper();
                id->Parameter.clear();
                if (!VInfo.GenNames.isEmpty())
                    GNames = VInfo.GenNames.split(",", Qt::SkipEmptyParts);
                if (!VInfo.GenTypes.isEmpty())
                    GTypes = VInfo.GenTypes.split(",", Qt::SkipEmptyParts);
                if (!VInfo.GenDefs.isEmpty())
                    GDefs = VInfo.GenDefs.split(",", Qt::SkipEmptyParts);
                ;
                for (Number = 1, it = GNames.begin(); it != GNames.end(); ++it) {
                    id->Parameter.append(
                        new SubParameter(true,
                                         *it + "=" + GDefs[Number - 1],
                                         tr("generic") + " " + QString::number(Number),
                                         GTypes[Number - 1]));
                    Number++;
                }
            }

        for (Number = 1, it = Names.begin(); it != Names.end(); ++it, Number++) {
            countPort++;

            Str = QString::number(Number);
            // search for matching port symbol
            for (pp = a_SymbolPaints.first(); pp != 0; pp = a_SymbolPaints.next())
                if (pp->Name == ".PortSym ")
                    if (((PortSymbol *) pp)->numberStr == Str)
                        break;

            if (pp)
                ((PortSymbol *) pp)->nameStr = *it;
            else {
                a_SymbolPaints.append(new PortSymbol(x1, y2, Str, *it));
                y2 += 40;
            }
        }
    }
    // handle Verilog-HDL file symbol
    else if (Suffix == "v") {
        QStringList::iterator it;
        QStringList Names;
        int Number;

        // get ports from Verilog-HDL file
        QFileInfo Info(a_DocName);
        QString Name = Info.path() + QDir::separator() + a_DataDisplay;

        // obtain Verilog-HDL information either from open text document or the
        // file directly
        Verilog_File_Info VInfo;
        TextDoc *d = (TextDoc *) a_App->findDoc(Name);
        if (d)
            VInfo = Verilog_File_Info(d->document()->toPlainText());
        else
            VInfo = Verilog_File_Info(Name, true);
        if (!VInfo.PortNames.isEmpty())
            Names = VInfo.PortNames.split(",", Qt::SkipEmptyParts);

        for (pp = a_SymbolPaints.first(); pp != 0; pp = a_SymbolPaints.next())
            if (pp->Name == ".ID ") {
                ID_Text *id = (ID_Text *) pp;
                id->Prefix = VInfo.ModuleName.toUpper();
                id->Parameter.clear();
            }

        for (Number = 1, it = Names.begin(); it != Names.end(); ++it, Number++) {
            countPort++;

            Str = QString::number(Number);
            // search for matching port symbol
            for (pp = a_SymbolPaints.first(); pp != 0; pp = a_SymbolPaints.next())
                if (pp->Name == ".PortSym ")
                    if (((PortSymbol *) pp)->numberStr == Str)
                        break;

            if (pp)
                ((PortSymbol *) pp)->nameStr = *it;
            else {
                a_SymbolPaints.append(new PortSymbol(x1, y2, Str, *it));
                y2 += 40;
            }
        }
    }
    // handle Verilog-A file symbol
    else if (Suffix == "va") {
        QStringList::iterator it;
        QStringList Names;
        int Number;

        // get ports from Verilog-A file
        QFileInfo Info(a_DocName);
        QString Name = Info.path() + QDir::separator() + a_DataDisplay;

        // obtain Verilog-A information either from open text document or the
        // file directly
        VerilogA_File_Info VInfo;
        TextDoc *d = (TextDoc *) a_App->findDoc(Name);
        if (d)
            VInfo = VerilogA_File_Info(d->toPlainText());
        else
            VInfo = VerilogA_File_Info(Name, true);

        if (!VInfo.PortNames.isEmpty())
            Names = VInfo.PortNames.split(",", Qt::SkipEmptyParts);

        for (pp = a_SymbolPaints.first(); pp != 0; pp = a_SymbolPaints.next())
            if (pp->Name == ".ID ") {
                ID_Text *id = (ID_Text *) pp;
                id->Prefix = VInfo.ModuleName.toUpper();
                id->Parameter.clear();
            }

        for (Number = 1, it = Names.begin(); it != Names.end(); ++it, Number++) {
            countPort++;

            Str = QString::number(Number);
            // search for matching port symbol
            for (pp = a_SymbolPaints.first(); pp != 0; pp = a_SymbolPaints.next())
                if (pp->Name == ".PortSym ")
                    if (((PortSymbol *) pp)->numberStr == Str)
                        break;

            if (pp)
                ((PortSymbol *) pp)->nameStr = *it;
            else {
                a_SymbolPaints.append(new PortSymbol(x1, y2, Str, *it));
                y2 += 40;
            }
        }
    }
    // handle schematic symbol
    else {
        // go through all components in a schematic
        for (Component *pc = a_DocComps.first(); pc != 0; pc = a_DocComps.next()) {
            if (pc->Model == "Port") {
                countPort++;

                Str = pc->Props.front()->Value;
                // search for matching port symbol
                for (pp = a_SymbolPaints.first(); pp != 0; pp = a_SymbolPaints.next()) {
                    if (pp->Name == ".PortSym ") {
                        if (((PortSymbol *) pp)->numberStr == Str)
                            break;
                    }
                }

                if (pp) {
                    ((PortSymbol *) pp)->nameStr = pc->Name;
                } else {
                    a_SymbolPaints.append(new PortSymbol(x1, y2, Str, pc->Name));
                    y2 += 40;
                }
            }
        }
    }

    // delete not accounted port symbols
    for (pp = a_SymbolPaints.first(); pp != 0;) {
        if (pp->Name == ".PortSym ")
            if (((PortSymbol *) pp)->nameStr.isEmpty()) {
                a_SymbolPaints.remove();
                pp = a_SymbolPaints.current();
                continue;
            }
        pp = a_SymbolPaints.next();
    }

    return countPort;
}

int Schematic::orderSymbolPorts()
{
  Painting *pp;
  int countPorts = 0;
  QSet<int> port_numbers, existing_numbers, free_numbers;
  int max_port_number = 0;
  for (pp = a_SymbolPaints.first(); pp != 0; pp = a_SymbolPaints.next()) {
    if (pp->Name == ".PortSym ") {
      countPorts++;
      QString numstr = ((PortSymbol *) pp)->numberStr;
      if (numstr != "0") {
        if (numstr.toInt() > max_port_number) {
          max_port_number = numstr.toInt();
        }
        existing_numbers.insert(numstr.toInt());
      }
    }
  }

  max_port_number = std::max(countPorts,max_port_number);
  for (int i = 1; i <= max_port_number; i++) {
    port_numbers.insert(i);
  }

  free_numbers = port_numbers - existing_numbers;

  // Assign new numbers only if port number is empty; Preserve ports order.
  for (pp = a_SymbolPaints.first(); pp != 0; pp = a_SymbolPaints.next()) {
    if (pp->Name == ".PortSym ") {
      QString numstr = ((PortSymbol *) pp)->numberStr;
      if (numstr == "0") {
        int free_num = *free_numbers.constBegin();
        free_numbers.remove(free_num);
        ((PortSymbol *) pp)->numberStr = QString::number(free_num);
      }
    }
  }

  return countPorts;
}

// ---------------------------------------------------
bool Schematic::undo()
{
    if (a_symbolMode) {
        if (a_undoSymbolIdx == 0) {
            return false;
        }

        rebuildSymbol(a_undoSymbol.at(--a_undoSymbolIdx));

        emit signalUndoState(a_undoSymbolIdx != 0);
        emit signalRedoState(a_undoSymbolIdx != a_undoSymbol.size() - 1);

        if (a_undoSymbol.at(a_undoSymbolIdx)->at(1) == 'i'
            && a_undoAction.at(a_undoActionIdx)->at(1) == 'i') {
            setChanged(false, false);
            return true;
        }

        setChanged(true, false);
        return true;
    }

    // ...... for schematic edit mode .......
    if (a_undoActionIdx == 0) {
        return false;
    }

    rebuild(a_undoAction.at(--a_undoActionIdx));
    reloadGraphs(); // load recent simulation data

    emit signalUndoState(a_undoActionIdx != 0);
    emit signalRedoState(a_undoActionIdx != a_undoAction.size() - 1);

    if (a_undoAction.at(a_undoActionIdx)->at(1) == 'i') {
        if (a_undoSymbol.isEmpty()) {
            setChanged(false, false);
            return true;
        } else if (a_undoSymbol.at(a_undoSymbolIdx)->at(1) == 'i') {
            setChanged(false, false);
            return true;
        }
    }

    setChanged(true, false);
    return true;
}

// ---------------------------------------------------
bool Schematic::redo()
{
    if (a_symbolMode) {
        if (a_undoSymbolIdx == a_undoSymbol.size() - 1) {
            return false;
        }

        rebuildSymbol(a_undoSymbol.at(++a_undoSymbolIdx));
        adjustPortNumbers(); // set port names

        emit signalUndoState(a_undoSymbolIdx != 0);
        emit signalRedoState(a_undoSymbolIdx != a_undoSymbol.size() - 1);

        if (a_undoSymbol.at(a_undoSymbolIdx)->at(1) == 'i'
            && a_undoAction.at(a_undoActionIdx)->at(1) == 'i') {
            setChanged(false, false);
            return true;
        }

        setChanged(true, false);
        return true;
    }

    //
    // ...... for schematic edit mode .......
    if (a_undoActionIdx == a_undoAction.size() - 1) {
        return false;
    }

    rebuild(a_undoAction.at(++a_undoActionIdx));
    reloadGraphs(); // load recent simulation data

    emit signalUndoState(a_undoActionIdx != 0);
    emit signalRedoState(a_undoActionIdx != a_undoAction.size() - 1);

    if (a_undoAction.at(a_undoActionIdx)->at(1) == 'i') {
        if (a_undoSymbol.isEmpty()) {
            setChanged(false, false);
            return true;
        } else if (a_undoSymbol.at(a_undoSymbolIdx)->at(1) == 'i') {
            setChanged(false, false);
            return true;
        }
    }

    setChanged(true, false);
    return true;
}

// ---------------------------------------------------
// Sets selected elements on grid.
bool Schematic::elementsOnGrid()
{
    int x, y, No;
    bool count = false;
    WireLabel *pl, *pLabel;
    Q3PtrList<WireLabel> LabelCache;

    // test all components
    a_Components->setAutoDelete(false);
    for (Component *pc = a_Components->last(); pc != nullptr; pc = a_Components->prev())
        if (pc->isSelected) {
            // rescue non-selected node labels
            for (Port *pp : pc->Ports)
                if (pp->Connection->Label)
                    if (pp->Connection->conn_count() < 2) {
                        LabelCache.append(pp->Connection->Label);
                        pp->Connection->Label->pOwner = 0;
                        pp->Connection->Label = 0;
                    }

            x = pc->cx;
            y = pc->cy;
            No = a_Components->at();
            deleteComp(pc);
            setOnGrid(pc->cx, pc->cy);
            insertRawComponent(pc);
            a_Components->at(No); // restore current list position
            pc->isSelected = false;
            count = true;

            x -= pc->cx;
            y -= pc->cy; // re-insert node labels and correct position
            for (pl = LabelCache.first(); pl != 0; pl = LabelCache.next()) {
                pl->cx -= x;
                pl->cy -= y;
                insertNodeLabel(pl);
            }
            LabelCache.clear();
        }
    a_Components->setAutoDelete(true);

    a_Wires->setAutoDelete(false);
    // test all wires and wire labels
    for (Wire *pw = a_Wires->last(); pw != 0; pw = a_Wires->prev()) {
        pl = pw->Label;
        pw->Label = nullptr;

        if (pw->isSelected) {
            // rescue non-selected node label
            pLabel = nullptr;
            if (pw->Port1->Label) {
                if (pw->Port1->conn_count() < 2) {
                    pLabel = pw->Port1->Label;
                    pw->Port1->Label = nullptr;
                }
            } else if (pw->Port2->Label) {
                if (pw->Port2->conn_count() < 2) {
                    pLabel = pw->Port2->Label;
                    pw->Port2->Label = nullptr;
                }
            }

            No = a_Wires->at();
            deleteWire(pw);
            setOnGrid(pw->x1, pw->y1);
            setOnGrid(pw->x2, pw->y2);
            insertWire(pw);
            a_Wires->at(No); // restore current list position
            pw->isSelected = false;
            count = true;
            if (pl)
                setOnGrid(pl->cx, pl->cy);

            if (pLabel) {
                setOnGrid(pLabel->cx, pLabel->cy);
                insertNodeLabel(pLabel);
            }
        }

        if (pl) {
            pw->Label = pl;
            if (pl->isSelected) {
                setOnGrid(pl->x1, pl->y1);
                pl->isSelected = false;
                count = true;
            }
        }
    }
    a_Wires->setAutoDelete(true);

    // test all node labels
    for (Node *pn = a_Nodes->first(); pn != 0; pn = a_Nodes->next())
        if (pn->Label)
            if (pn->Label->isSelected) {
                setOnGrid(pn->Label->x1, pn->Label->y1);
                pn->Label->isSelected = false;
                count = true;
            }

    // test all diagrams
    for (Diagram *pd = a_Diagrams->last(); pd != 0; pd = a_Diagrams->prev()) {
        if (pd->isSelected) {
            setOnGrid(pd->cx, pd->cy);
            pd->isSelected = false;
            count = true;
        }

        for (Graph *pg : pd->Graphs)
            // test markers of diagram
            for (Marker *pm : pg->Markers)
                if (pm->isSelected) {
                    x = pm->x1 + pd->cx;
                    y = pm->y1 + pd->cy;
                    setOnGrid(x, y);
                    pm->x1 = x - pd->cx;
                    pm->y1 = y - pd->cy;
                    pm->isSelected = false;
                    count = true;
                }
    }

    // test all paintings
    for (Painting *pa = a_Paintings->last(); pa != 0; pa = a_Paintings->prev())
        if (pa->isSelected) {
            setOnGrid(pa->cx, pa->cy);
            pa->isSelected = false;
            count = true;
        }

    if (count)
        setChanged(true, true);
    return count;
}

// ---------------------------------------------------
void Schematic::switchPaintMode()
{
    a_symbolMode = !a_symbolMode; // change mode

    int tmp, t2;
    float temp;
    temp = a_Scale;
    a_Scale = a_tmpScale;
    a_tmpScale = temp;
    tmp = contentsX();
    t2 = contentsY();
    setContentsPos(a_tmpPosX, a_tmpPosY);
    a_tmpPosX = tmp;
    a_tmpPosY = t2;
    tmp = a_ViewX1;
    a_ViewX1 = a_tmpViewX1;
    a_tmpViewX1 = tmp;
    tmp = a_ViewY1;
    a_ViewY1 = a_tmpViewY1;
    a_tmpViewY1 = tmp;
    tmp = a_ViewX2;
    a_ViewX2 = a_tmpViewX2;
    a_tmpViewX2 = tmp;
    tmp = a_ViewY2;
    a_ViewY2 = a_tmpViewY2;
    a_tmpViewY2 = tmp;
    tmp = a_UsedX1;
    a_UsedX1 = a_tmpUsedX1;
    a_tmpUsedX1 = tmp;
    tmp = a_UsedY1;
    a_UsedY1 = a_tmpUsedY1;
    a_tmpUsedY1 = tmp;
    tmp = a_UsedX2;
    a_UsedX2 = a_tmpUsedX2;
    a_tmpUsedX2 = tmp;
    tmp = a_UsedY2;
    a_UsedY2 = a_tmpUsedY2;
    a_tmpUsedY2 = tmp;
}

// *********************************************************************
// **********                                                 **********
// **********      Function for serving mouse wheel moving    **********
// **********                                                 **********
// *********************************************************************
void Schematic::contentsWheelEvent(QWheelEvent *Event)
{
    a_App->editText->setHidden(true); // disable edit of component property

    // A mouse wheel angle delta of a single step is typically 120,
    // but other devices may produce various values. For example,
    // angle values produced by a touchpad depend on how fast user
    // moves their fingers.
    //
    // When used for scrolling the view here angle delta is divided by
    // some number ("2" at the moment). There is nothing special about
    // this number, its sole purpose is to reduce a scroll-step
    // to a reasonable size.

    // Mouse may have a special wheel for horizontal scrolling
    const int horizontalWheelAngleDelta = Event->angleDelta().x();
    const int verticalWheelAngleDelta = Event->angleDelta().y();

    // Scroll horizontally
    // Horizontal scroll is performed either by a special wheel
    // or by usual mouse wheel with Shift pressed down.
    if ((Event->modifiers() & Qt::ShiftModifier) || horizontalWheelAngleDelta) {
        int delta = (horizontalWheelAngleDelta ? horizontalWheelAngleDelta : verticalWheelAngleDelta) / 2;
        if (delta > 0) {
            scrollLeft(delta);
        } else {
            scrollRight(-delta);
        }
    }
    // Zoom in or out
    else if (Event->modifiers() & Qt::ControlModifier) {
        // zoom factor scaled according to the wheel delta, to accommodate
        //  values different from 60 (slower or faster zoom)
        double scaleCoef = pow(1.1, verticalWheelAngleDelta / 60.0);
        const QPoint pointer{
            static_cast<int>(Event->position().x()),
            static_cast<int>(Event->position().y())};
        zoomAroundPoint(scaleCoef, pointer);
    }
    // Scroll vertically
    else {
        int delta = verticalWheelAngleDelta / 2;
        if (delta > 0) {
            scrollUp(delta);
        } else {
            scrollDown(-delta);
        }
    }
    Event->accept(); // QScrollView must not handle this event
}

// Scrolls the visible area upwards and enlarges or reduces the view
// area accordingly.
void Schematic::scrollUp(int step)
{
    assert(step >= 0);

    // Y-axis is directed "from top to bottom": the higher a point is
    // located, the smaller its y-coordinate and vice versa. Keep this in mind
    // while reading the code below.

    const int stepInModel = static_cast<int>(std::round(step/a_Scale));
    const QPoint viewportTopLeft = viewportRect().topLeft();

    // A point currently displayed in top left corner
    QPoint mtl = viewportToModel(viewportTopLeft);
    // A point that should be displayed in top left corner after scrolling
    mtl.setY(mtl.y() - stepInModel);

    QRect modelBounds = modelRect();

    // If the "should-be-displayed" point is located higher than model upper bound,
    // then extend the model
    modelBounds.setTop(std::min(mtl.y(), modelBounds.top()));

    // Cut off a bit of unused model space from its bottom side.
    const auto b = modelBounds.bottom() - stepInModel;
    if (b > a_UsedY2) {
        modelBounds.setBottom(b);
    }

    renderModel(a_Scale, modelBounds, mtl, viewportTopLeft);
}

// Scrolls the visible area downwards and enlarges or reduces the view
// area accordingly.
void Schematic::scrollDown(int step)
{
    assert(step >= 0);

    // Y-axis is directed "from top to bottom": the lower a point is
    // located, the bigger its y-coordinate and vice versa. Keep this in mind
    // while reading the code below.

    const int stepInModel = static_cast<int>(std::round(step/a_Scale));
    const QPoint viewportBottomLeft = viewportRect().bottomLeft();

    // A point currently displayed in bottom left corner
    QPoint mbl = viewportToModel(viewportBottomLeft);
    // A point that should be displayed in bottom left corner after scrolling
    mbl.setY(mbl.y() + stepInModel);

    QRect modelBounds = modelRect();

    // If the "should-be-displayed" point is lower than model bottom bound,
    // then extend the model
    modelBounds.setBottom(std::max(mbl.y(), modelBounds.bottom()));

    // Cut off a bit of unused model space from its top side.
    const auto t = modelBounds.top() + stepInModel;
    if (t < a_UsedY1) {
        modelBounds.setTop(t);
    }

    // Render model in its new size and position point in the top left corner of viewport
    renderModel(a_Scale, modelBounds, mbl, viewportBottomLeft);
}

// Scrolls the visible area to the left and enlarges or reduces the view
// area accordingly.
void Schematic::scrollLeft(int step)
{
    assert(step >= 0);

    // X-axis is directed "from left to right": the more to the left a point is
    // located, the smaller its x-coordinate and vice versa. Keep this in mind
    // while reading the code below.

    const int stepInModel = static_cast<int>(std::round(step/a_Scale));
    const QPoint viewportTopLeft = viewportRect().topLeft();

    // A point currently displayed in top left corner
    QPoint mtl = viewportToModel(viewportTopLeft);
    // A point that should be displayed in top left corner after scrolling
    mtl.setX(mtl.x() - stepInModel);

    QRect modelBounds = modelRect();

    // If the "should-be-displayed" point is to the left of model left bound,
    // then extend the model
    modelBounds.setLeft(std::min(mtl.x(), modelBounds.left()));

    // Cut off a bit of unused model space from its right side.
    const auto r = modelBounds.right() - stepInModel;
    if (r > a_UsedX2) {
        modelBounds.setRight(r);
    }

    renderModel(a_Scale, modelBounds, mtl, viewportTopLeft);
}

// Scrolls the visible area to the right and enlarges or reduces the
// view area accordingly.
void Schematic::scrollRight(int step)
{
    assert(step >= 0);

    // X-axis is directed "from left to right": the more to the right a point is
    // located, the bigger its x-coordinate and vice versa. Keep this in mind
    // while reading the code below.

    const int stepInModel = static_cast<int>(std::round(step/a_Scale));
    const QPoint viewportTopRight = viewportRect().topRight();

    // A point currently displayed in top right corner
    QPoint mtr = viewportToModel(viewportTopRight);
    // A point that should be displayed in top right corner after scrolling
    mtr.setX(mtr.x() + stepInModel);

    QRect modelBounds = modelRect();

    // If the "should-be-displayed" point is to the right of the model right bound,
    // then extend the model
    modelBounds.setRight(std::max(mtr.x(), modelBounds.right()));

    // Cut off a bit of unused model space from its left side.
    const auto l = modelBounds.left() + stepInModel;
    if (l < a_UsedX1) {
        modelBounds.setLeft(l);
    }

    renderModel(a_Scale, modelBounds, mtr, viewportTopRight);
}

// -----------------------------------------------------------
// Is called if the scroll arrow of the ScrollBar is pressed.
void Schematic::slotScrollUp()
{
    a_App->editText->setHidden(true); // disable edit of component property
    scrollUp(verticalScrollBar()->singleStep());
}

// -----------------------------------------------------------
// Is called if the scroll arrow of the ScrollBar is pressed.
void Schematic::slotScrollDown()
{
    a_App->editText->setHidden(true); // disable edit of component property
    scrollDown(verticalScrollBar()->singleStep());
}

// -----------------------------------------------------------
// Is called if the scroll arrow of the ScrollBar is pressed.
void Schematic::slotScrollLeft()
{
    a_App->editText->setHidden(true); // disable edit of component property
    scrollLeft(horizontalScrollBar()->singleStep());
}

// -----------------------------------------------------------
// Is called if the scroll arrow of the ScrollBar is pressed.
void Schematic::slotScrollRight()
{
    a_App->editText->setHidden(true); // disable edit of component property
    scrollRight(horizontalScrollBar()->singleStep());
}

// *********************************************************************
// **********                                                 **********
// **********        Function for serving drag'n drop         **********
// **********                                                 **********
// *********************************************************************

// Is called if an object is dropped (after drag'n drop).
void Schematic::contentsDropEvent(QDropEvent *Event)
{
    if (a_dragIsOkay) {
        QList<QUrl> urls = Event->mimeData()->urls();
        if (urls.isEmpty()) {
            return;
        }

        // do not close untitled document to avoid segfault
        QucsDoc *d = QucsMain->getDoc(0);
        bool changed = d->getDocChanged();
        d->setDocChanged(true);

        // URI:  file:/home/linuxuser/Desktop/example.sch
        for (QUrl url : urls) {
            a_App->gotoPage(QDir::toNativeSeparators(url.toLocalFile()));
        }

        d->setDocChanged(changed);
        return;
    }

    auto ev_pos = Event->position();
    QPoint inModel = contentsToModel(ev_pos.toPoint());
    //QMouseEvent e(QEvent::MouseButtonPress, ev_pos, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);

    QMouseEvent e(QEvent::MouseButtonPress, ev_pos, mapToGlobal(ev_pos), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);

    a_App->view->MPressElement(this, &e, inModel.x(), inModel.y());

    delete a_App->view->selElem;
    a_App->view->selElem = nullptr; // no component selected

    if (formerAction) {
        formerAction->setChecked(true);
    } else {
        QucsMain->select->setChecked(true); // restore old action
    }
}

// ---------------------------------------------------
void Schematic::contentsDragEnterEvent(QDragEnterEvent *Event)
{
    //FIXME: the function of drag library component seems not working?
    formerAction = nullptr;
    a_dragIsOkay = false;

    // file dragged in ?
    if (Event->mimeData()->hasUrls()) {
        a_dragIsOkay = true;
        Event->accept();
        return;
    }

    // drag library component
    if (Event->mimeData()->hasText()) {
        QString s = Event->mimeData()->text();
        if (s.left(15) == "QucsComponent:<") {
            s = s.mid(14);
            a_App->view->selElem = getComponentFromName(s);
            if (a_App->view->selElem) {
                Event->accept();
                return;
            }
        }
        Event->ignore();
        return;
    }

    //  if(Event->format(1) == 0) {  // only one MIME type ?

    // drag component from listview
    //if(Event->provides("application/x-qabstractitemmodeldatalist")) {
    if (Event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        QListWidgetItem *Item = a_App->CompComps->currentItem();
        if (Item) {
            formerAction = a_App->activeAction;
            a_App->slotSelectComponent(Item); // also sets drawn=false
            a_App->MouseMoveAction = 0;
            a_App->MousePressAction = 0;

            Event->accept();
            return;
        }
    }
    //  }

    Event->ignore();
}

// ---------------------------------------------------
void Schematic::contentsDragLeaveEvent(QDragLeaveEvent *)
{
    if (formerAction)
        formerAction->setChecked(true); // restore old action
}

void Schematic::contentsNativeGestureZoomEvent( QNativeGestureEvent* Event) {
  a_App->editText->setHidden(true); // disable edit of component property

  const auto factor = 1.0 + Event->value();
  const auto pointer = mapFromGlobal(Event->globalPosition().toPoint());
  zoomAroundPoint(factor,pointer);
}

// ---------------------------------------------------
void Schematic::contentsDragMoveEvent(QDragMoveEvent *Event)
{
    if (!a_dragIsOkay) {
        if (a_App->view->selElem == 0) {
            Event->ignore();
            return;
        }

        auto ev_pos = Event->position();
        /*QMouseEvent e(QEvent::MouseMove,
                      Event->position(),
                      Qt::NoButton,
                      Qt::NoButton,
                      Qt::NoModifier);*/
        QMouseEvent e(QEvent::MouseButtonPress, ev_pos, mapToGlobal(ev_pos), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        a_App->view->MMoveElement(this, &e);
    }

    Event->accept();
}

bool Schematic::checkDplAndDatNames()
{
    QFileInfo Info(a_DocName);
    if (!a_DocName.isEmpty() && a_DataSet.size() > 4 && a_DataDisplay.size() > 4) {
        QString base = Info.completeBaseName();
        QString base_dat = a_DataSet;
        base_dat.chop(4);
        QString base_dpl = a_DataDisplay;
        base_dpl.chop(4);
        if (base != base_dat || base != base_dpl) {
            QString msg = QObject::tr(
                "The schematic name and dataset/display file name is not matching! "
                "This may happen if schematic was copied using the file manager "
                "instead of using File->SaveAs. Correct dataset and display names "
                "automatically?\n\n");
            msg += QString(QObject::tr("Schematic file: ")) + base + ".sch\n";
            msg += QString(QObject::tr("Dataset file: ")) + a_DataSet + "\n";
            msg += QString(QObject::tr("Display file: ")) + a_DataDisplay + "\n";
            auto r = QMessageBox::information(this,
                                              QObject::tr("Open document"),
                                              msg,
                                              QMessageBox::Yes,
                                              QMessageBox::No);
            if (r == QMessageBox::Yes) {
                a_DataSet = base + ".dat";
                a_DataDisplay = base + ".dpl";
                return true;
            }
        }
    } else {
        return false;
    }
    return false;
}
