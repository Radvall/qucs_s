/***************************************************************************
                               sp_sens_xyce.h
                                ----------
    begin                : Mon Sep 18 2017
    copyright            : (C) 2017 by Vadim Kuznetsov
    email                : ra3xdh@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SP_SENS_XYCE_H
#define SP_SENS_XYCE_H

#include "components/simulation.h"


class SpiceSENS_Xyce : public qucs::component::SimulationComponent  {
public:
  SpiceSENS_Xyce();
  ~SpiceSENS_Xyce();
  Component* newOne();
  static Element* info(QString&, char* &, bool getNewOne=false);

protected:
  QString spice_netlist(spicecompat::SpiceDialect dialect = spicecompat::SPICEDefault);
  Qt::GlobalColor color() const override { return Qt::darkGreen; }
};

#endif
