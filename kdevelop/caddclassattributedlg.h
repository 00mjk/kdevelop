/***************************************************************************
               cclassaddattributedlg.h  -  description

                          -------------------

    begin                : Fri Mar 19 1999

    copyright            : (C) 1999 by Jonas Nordin
    email                : jonas.nordin@cenacle.se

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef CADDCLASSATTRIBUTEDLG_H
#define CADDCLASSATTRIBUTEDLG_H

#include <qdialog.h>
#include <qlineedit.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include "cproject.h"

class CAddClassAttributeDlg : public QDialog
{
  Q_OBJECT
public: // Constructor & Destructor
  CAddClassAttributeDlg( QWidget *parent=0, const char *name=0 );

public: // Public widgets

  QButtonGroup modifierGrp;
  QButtonGroup varGrp;
  QButtonGroup accessGrp;

  QLabel typeLbl;
  QLineEdit typeEdit;
  QLabel nameLbl;
  QLineEdit nameEdit;

  QRadioButton publicRb;
  QRadioButton protectedRb;
  QRadioButton privateRb;

  QCheckBox staticCb;
  QCheckBox constCb;

  QPushButton okBtn;
  QPushButton cancelBtn;

protected:
 protected slots:
  void OK();
 
private: // Private methods

  void setWidgetValues();
  void setCallbacks();

};

#endif

