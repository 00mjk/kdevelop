/*
 *  Copyright (C) 2001 Matthias H�lzer-Kl�pfel <mhk@caldera.de>   
 */
 

#ifndef __KDEVPART_TIPOFDAY_H__
#define __KDEVPART_TIPOFDAY_H__


#include <kdevpart.h>


class TipOfDayPart : public KDevPart
{
  Q_OBJECT

public:
   
  TipOfDayPart(KDevApi *api, QObject *parent=0, const char *name=0);


public slots:

  void showTip();
  void showOnStart();

};


#endif
