/***************************************************************************
                               inductor.h
                              ------------
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

#ifndef INDUCTOR_H
#define INDUCTOR_H

#include "component.h"


class Inductor : public Component  {
public:
  Inductor();
 ~Inductor();
  Component* newOne();
  static Element* info(QString&, char* &, bool getNewOne=false);

protected:
  QString va_code();
  void getExtraVANodes(QStringList& nodes);
  QString spice_netlist(spicecompat::SpiceDialect dialect = spicecompat::SPICEDefault);
  virtual QString cdl_netlist();
};

#endif
