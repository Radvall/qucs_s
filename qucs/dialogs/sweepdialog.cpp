/***************************************************************************
                               sweepdialog.cpp
                              -----------------
    begin                : Sat Aug 13 2005
    copyright            : (C) 2005 by Michael Margraf
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
#include "sweepdialog.h"
#include "schematic.h"
#include "qucs.h"

#include <QGridLayout>
#include "main.h"
#include "../diagrams/graph.h"
#include "misc.h"
#include "component.h"
#include "wire.h"


#include <QLabel>
#include <QLineEdit>
#include <QValidator>
#include <QPushButton>

// SpinBoxes are used to show the calculated bias points at the given set of sweep points
mySpinBox::mySpinBox(int Min, int Max, int Step, double *Val, QWidget *Parent)
          : QSpinBox(Parent)
{
  setMinimum(Min);
  setMaximum(Max);
  setSingleStep(Step);
  Values = Val;
  ValueSize = Max;
  //editor()->
  //  setReadOnly(true);
}


#include <iostream>
using namespace std;
QString mySpinBox::textFromValue(int Val) const
{
  if (Values == NULL) return "";

  //qDebug() << "Values + Val" << *(Values+Val) << endl;
  return QString::number(*(Values+Val));
}

QValidator::State mySpinBox::validate ( QString & text, int & pos ) const
{
  if(pos>ValueSize)return QValidator::Invalid;
  if(QString::number(*(Values+pos))==text)
  return QValidator::Acceptable;
  else return QValidator::Invalid;
}


SweepDialog::SweepDialog(Schematic *Doc_,QHash<QString,double> *NodeVals)
    : QDialog(Doc_)
{
  qDebug() << "SweepDialog::SweepDialog()";

  Doc = Doc_;

  isSpice = false;
  if (QucsSettings.DefaultSimulator != spicecompat::simQucsator) {
      isSpice = true;
      if (NodeVals) pGraph = setBiasPoints(NodeVals);
      return;
  }

  pGraph = setBiasPoints();
  // if simulation has no sweeps, terminate dialog before showing it
  if(!pGraph->numAxes()) {
    reject();
    return;
  }
  if(pGraph->numAxes() <= 1)
    if(pGraph->axis(0)->count <= 1) {
      reject();
      return;
    }


  setWindowTitle(tr("Bias Points"));

  int i = 0;
  // ...........................................................
  QGridLayout *all = new QGridLayout(this);//, pGraph->cPointsX.count()+2,2,3,3);
  all->setContentsMargins(5,5,5,5);
  all->setSpacing(5);
  all->setColumnStretch(1,5);

  DataX const *pD;
  mySpinBox *Box;

  for(unsigned ii=0; (pD=pGraph->axis(ii)); ++ii) {
    all->addWidget(new QLabel(pD->Var, this), i,0);
    //cout<<"count: "<<pD->count-1<<", points: "<<*pD->Points<<endl;
    //works only for linear:
    /*double Min = pD->Points[0];
    double Max = pD->Points[pD->count-1];
    double Step = (Max-Min)/(pD->count);
    cout<<"Min: "<<Min<<", Max: "<<Max<<", Step: "<<Step<<endl;
    Box = new mySpinBox(Min, Max, Step, pD->Points, this);*/
    Box = new mySpinBox(0, pD->count-1, 1, pD->Points, this);
    Box->setValue(0);
    all->addWidget(Box, i++,1);
    connect(Box, SIGNAL(valueChanged(int)), SLOT(slotNewValue(int)));
    BoxList.append(Box);
  }

  // ...........................................................
  all->setRowStretch(i,5);
  QPushButton *ButtClose = new QPushButton(tr("Close"), this);
  all->addWidget(ButtClose, i+1,0);
  connect(ButtClose, SIGNAL(clicked()), SLOT(accept()));
  show();
}

SweepDialog::~SweepDialog()
{
  delete pGraph;

  while(!ValueList.isEmpty()) {
    delete ValueList.takeFirst();
  }
}

// ---------------------------------------------------------------
void SweepDialog::slotNewValue(int)
{
  DataX const*pD = pGraph->axis(0);

  int Factor = 1, Index = 0;
  QList<mySpinBox *>::const_iterator it;
  for(it = BoxList.constBegin(); it != BoxList.constEnd(); it++) {
    Index  += (*it)->value() * Factor;
    Factor *= pD->count;
  }
  Index *= 2;  // because of complex values

  QList<Node *>::iterator node_it;
  QList<double *>::const_iterator value_it = ValueList.begin();
  for(node_it = NodeList.begin(); node_it != NodeList.end(); node_it++) {
    qDebug() << "SweepDialog::slotNewValue:(*node_it)->Name:" << (*node_it)->Name;
    (*node_it)->Name = misc::num2str(*((*value_it)+Index));
    (*node_it)->Name += ((*node_it)->x1 & 0x10)? "A" : "V";
    value_it++;
  }

  Doc->viewport()->update();
}

// ---------------------------------------------------
Graph* SweepDialog::setBiasPoints(QHash<QString,double> *NodeVals)
{
  // When this function is entered, a simulation was performed.
  // Thus, the node names are still in "node->Name".

  qDebug() << "SweepDialog::setBiasPoints()";

  bool hasNoComp;
  Graph *pg = new Graph(NULL, ""); // HACK!
  QFileInfo Info(Doc->getDocName());
  QString DataSet = Info.absolutePath() + QDir::separator() + Doc->getDataSet();

  // Note 1:
  // Invalidate it so that "Graph::loadDatFile()" does not check for the previously loaded time.
  // This is a current hack as "Graph::loadDatFile()" does not support multi-node data loading
  // from the simulation results without refreshing (changing) or invalidating the timestamp.

  NodeList.clear();
  ValueList.clear();

  // create DC voltage for all nodes
  for(Node* pn : *Doc->a_Nodes) {
    if(pn->Name.isEmpty()) continue;

    pn->x1 = 0;
    if(pn->conn_count() < 2) {
      pn->Name = "";  // no text at open nodes
      continue;
    }
    else {
      hasNoComp = true;
      for(auto *pe : *pn)
        if(pe->Type == isWire) {
          if( ((Wire*)pe)->isHorizontal() )  pn->x1 |= 2;
        }
        else {
          if( ((Component*)pe)->Model == "GND" ) {
            hasNoComp = true;   // no text at ground symbol
            break;
          }

          if(pn->cx < pe->cx)  pn->x1 |= 1;  // to the right is no room
          hasNoComp = false;
        }
      if(hasNoComp) {  // text only were a component is connected
        pn->Name = "";
        continue;
      }
    }

    if (!isSpice) {
        pg->Var = pn->Name + ".V";
        pg->lastLoaded = QDateTime(); // Note 1 at the start of this function
        if(pg->loadDatFile(DataSet) == 2) {
          pn->Name = misc::num2str(*(pg->cPointsY)) + "V";
          NodeList.append(pn);             // remember node ...
          ValueList.append(pg->cPointsY);  // ... and all of its values
          pg->cPointsY = 0;   // do not delete it next time !
        }
        else
          pn->Name = "0V";
    } else {
        if (NodeVals->contains(pn->Name.toLower())) {
                  double volts = NodeVals->value(pn->Name.toLower());
                  pn->Name = misc::num2str(volts) + "V";
              } else pn->Name = "0V";
    }


    for(auto pe : *pn)
      if(pe->Type == isWire) {
        if( ((Wire*)pe)->Port1 != pn )  // no text at next node
          ((Wire*)pe)->Port1->Name = "";
        else  ((Wire*)pe)->Port2->Name = "";
      }
  }


  // create DC current through each probe
  for(Component* pc : *Doc->a_Components)
    if(pc->Model == "IProbe") {
      Node* pn = pc->Ports.first()->Connection;
      if(!pn->Name.isEmpty())   // preserve node voltage ?
        pn = pc->Ports.at(1)->Connection;

      pn->x1 = 0x10;   // mark current
      if (!isSpice) {
          pg->Var = pc->Name + ".I";
          pg->lastLoaded = QDateTime(); // Note 1 at the start of this function
          if(pg->loadDatFile(DataSet) == 2) {
            pn->Name = misc::num2str(*(pg->cPointsY)) + "A";
            NodeList.append(pn);             // remember node ...
            ValueList.append(pg->cPointsY);  // ... and all of its values
            pg->cPointsY = 0;   // do not delete it next time !
          }
          else
            pn->Name = "0A";
      } else {
          QString src_nam = QStringLiteral("V%1#branch").arg(pc->Name).toLower();
          if (NodeVals->contains(src_nam)) {
              pn->Name = misc::num2str(NodeVals->value(src_nam))+"A";
          } else pn->Name = "0A";
      }


      for(auto pe : *pn)
        if(pe->Type == isWire) {
          if( ((Wire*)pe)->isHorizontal() )  pn->x1 |= 2;
        }
        else {
          if(pn->cx < pe->cx)  pn->x1 |= 1;  // to the right is no room
        }
    } else if (isSpice) {
        if ((pc->Model == "S4Q_V")||(pc->Model == "Vdc")) {
            Node* pn = pc->Ports.first()->Connection;
            if(!pn->Name.isEmpty())   // preserve node voltage ?
              pn = pc->Ports.at(1)->Connection;

            pn->x1 = 0x10;   // mark current
            QString src_nam = QString(pc->Name+"#branch").toLower();
            if (NodeVals->contains(src_nam)) {
                pn->Name = misc::num2str(NodeVals->value(src_nam))+"A";
            } else pn->Name = "0A";

            for(auto pe : *pn)
              if(pe->Type == isWire) {
                if( ((Wire*)pe)->isHorizontal() )  pn->x1 |= 2;
            }
              else {
                if(pn->cx < pe->cx)  pn->x1 |= 1;  // to the right is no room
            }
        }
    }


  Doc->setShowBias(1);

  return pg;
}
