/***************************************************************************
                         iAmpMod.h  -  description
                   --------------------------------------
    begin                    : Fri Mar 9 2007
    copyright              : (C) 2007 by Gunther Kraut
    email                     : gn.kraut@t-online.de
    spice4qucs code added  Wed. 24 March 2015
    copyright              : (C) 2015 by Mike Brinson
    email                    : mbrin72043@yahoo.co.uk

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 #ifndef IAMPMOD_H
#define IAMPMOD_H

#include "components/component.h"

class iAmpMod : public Component {
public:
  iAmpMod();
  ~iAmpMod();
  Component* newOne();
  static Element* info(QString&, char* &, bool getNewOne=false);
protected:
  QString netlist();
  QString spice_netlist(spicecompat::SpiceDialect dialect = spicecompat::SPICEDefault);
};

#endif // IAMPMOD_H
