/***************************************************************************
                               diode.h
                              ---------
    begin                : Sat Aug 23 2003
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

#ifndef DIODE_H
#define DIODE_H

#include "component.h"


class Diode : public MultiViewComponent  {
public:
  Diode();
 ~Diode() {};
  Component* newOne();
  static Element* info(QString&, char* &, bool getNewOne=false);

protected:
  QString spice_netlist(spicecompat::SpiceDialect dialect = spicecompat::SPICEDefault);
  virtual QString cdl_netlist();
  void createSymbol();
};

#endif
