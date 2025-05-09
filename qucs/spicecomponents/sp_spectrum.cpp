/***************************************************************************
                               sp_fourier.cpp
                               ------------
    begin                : Sun May 17 2015
    copyright            : (C) 2015 by Vadim Kuznetsov
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
#include "sp_spectrum.h"
#include "misc.h"
#include "extsimkernels/spicecompat.h"


SpiceFFT::SpiceFFT()
{
  isSimulation = true;
  Description = QObject::tr("Spectrum analysis");
  Simulator = spicecompat::simNgspice | spicecompat::simSpiceOpus;
  initSymbol(Description);
  Model = ".FFT";
  Name  = "FFT";
  SpiceModel = ".FFT";

  // The index of the first 4 properties must not changed. Used in recreate().
  Props.append(new Property("BW","1MHz",true,"Bandwidth"));
  Props.append(new Property("dF","10kHz",true,"Frequency step"));
  Props.append(new Property("Window","hanning", true, "Window type "
                                     "[none,rectangular,bartlet,blackman,hanning,hamming,gaussian,flattop]"));
  Props.append(new Property("Order","2",false,"Order of the Gaussian window"));
  Props.append(new Property("Tstart","0",false,"Transient starting time"));
  Props.append(new Property("initialDC", "yes", false,
              QObject::tr("perform initial DC (set \"no\" to activate UIC)")+" [yes, no]"));
}

SpiceFFT::~SpiceFFT()
{
}

Component* SpiceFFT::newOne()
{
  return new SpiceFFT();
}

Element* SpiceFFT::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("Spectrum analysis");
  BitmapFile = (char *) "sp_fft";

  if(getNewOne)  return new SpiceFFT();
  return 0;
}

QString SpiceFFT::spice_netlist(spicecompat::SpiceDialect dialect /* = spicecompat::SPICEDefault */)
{
    Q_UNUSED(dialect);

    QString s;
    QString unit;
    double num,fac;
    misc::str2num(getProperty("BW")->Value,num,unit,fac);
    double bw = num*fac;
    misc::str2num(getProperty("dF")->Value,num,unit,fac);
    double df = num*fac;
    misc::str2num(getProperty("Tstart")->Value,num,unit,fac);
    double tstart = num*fac;
    double tstop = 1.0/df + tstart;
    double tstep = 1.0/(2*bw);
    QString win = getProperty("Window")->Value;
    s =  QStringLiteral("tran %1 %2 %3").arg(tstep).arg(tstop).arg(tstart);
    if (dialect != spicecompat::SPICEXyce) {
      if (getProperty("initialDC")->Value == "no") {
        s += " UIC";
      }
    }
    s += "\n";
    s += QStringLiteral("set specwindow=%1\n").arg(win);
    if (win == "gaussian") {
        s += QStringLiteral("set specwindoworder=%1\n")
                .arg(getProperty("Order")->Value);
    }

    return s;
}
