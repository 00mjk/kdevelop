/***************************************************************************
                          breakpointdialog.h  -  description                              
                             -------------------                                         
    begin                : Mon Sep 20 1999                                           
    copyright            : (C) 1999 by John Birch                         
    email                : jb.nz@writeme.com                                     
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/


#ifndef BREAKPOINTDIALOG_H
#define BREAKPOINTDIALOG_H

#include <kdialog.h>
#include <knuminput.h>

#include <qcheckbox.h>
#include <qlineedit.h>

class Breakpoint;

/**
  *@author John Birch
  */

class BPDialog : public KDialog  {
   Q_OBJECT

public:
  BPDialog(Breakpoint* breakpoint, QWidget *parent=0, const char *name=0);
  ~BPDialog();
  
  QString getConditional()      { return conditional_->text(); }
  int getIgnoreCount()          { return ignoreCount_->value(); }
  bool isEnabled()              { return enabled_->isChecked(); }

private:
  QCheckBox*    enabled_;
  QLineEdit*    conditional_;
  KIntNumInput* ignoreCount_;
};

#endif
