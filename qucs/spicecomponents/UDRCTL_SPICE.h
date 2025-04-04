/*
 * UDRCTL_SPICE.h - device definitions for unuform distributed RC Line module
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 */

#ifndef UDRCTL_SPICE_H
#define UDRCTL_SPICE_H

#include "components/component.h"

class UDRCTL_SPICE : public Component {
public:
    UDRCTL_SPICE();
    ~UDRCTL_SPICE();
    Component* newOne();
    static Element* info(QString&, char* &, bool getNewOne=false);

protected:
    QString netlist();
    QString spice_netlist(spicecompat::SpiceDialect dialect = spicecompat::SPICEDefault);
};

#endif /* UDRCTL_SPICE_H */
