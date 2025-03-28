#ifndef S4Q_IEQNDEF_H
#define S4Q_IEQNDEF_H

#include "components/component.h"

class S4Q_Ieqndef : public Component {
public:
  S4Q_Ieqndef();
  ~S4Q_Ieqndef();
  Component* newOne();
  static Element* info(QString&, char* &, bool getNewOne=false);
protected:
  QString netlist();
  QString spice_netlist(spicecompat::SpiceDialect dialect = spicecompat::SPICEDefault);
  QString va_code();
};

#endif // S4Q_IEQNDEF_H
