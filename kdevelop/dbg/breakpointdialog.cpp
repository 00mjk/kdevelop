/***************************************************************************
                          breakpointdialog.cpp  -  description
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

#include "breakpointdialog.h"
#include "breakpoint.h"

#include <kapp.h>
#include <kbuttonbox.h>
#include <klineedit.h>
#include <knuminput.h>
#include <kdialog.h>

#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <klocale.h>

#include <algorithm>

/***************************************************************************/

BPDialog::BPDialog(Breakpoint* BP, QWidget *parent, const char *name) :
  KDialog(parent, name, true),  // modal
  conditional_(0),
  ignoreCount_(0)
{
  ASSERT(BP);

  QBoxLayout *topLayout = new QVBoxLayout(this, 10);
  QString title;
  if (BP->hasSourcePosition())
    title = i18n("Filename: %1 at line: %2").arg(BP->filename()).arg(BP->lineNo());
  else
    title = i18n("Breakpoint");
  QLabel *label = new QLabel(this);
  label->setText(title);
  label->setMaximumHeight(label->sizeHint().height());
  label->setMinimumSize(label->sizeHint());
  topLayout->addWidget(label, 0, 0);

  QGridLayout *grid = new QGridLayout(3, 2, 10);
  topLayout->addLayout(grid);

  QLabel *label1 = new QLabel(this);
  label1->setText(i18n("&Conditional"));
  label1->setMaximumHeight(label1->sizeHint().height());
  label1->setMinimumSize(label1->sizeHint());
  grid->addWidget(label1, 0, 0);

  conditional_ = new KLineEdit(this);
  conditional_->setText(BP->conditional());
  conditional_->setMinimumSize(conditional_->sizeHint());
  label1->setBuddy(conditional_);
  grid->addWidget(conditional_, 0, 1);

  QLabel* label2 = new QLabel(this);
  label2->setText(i18n("&Ignore count"));
  label2->setMaximumHeight(label2->sizeHint().height());
  label2->setMinimumSize(label2->sizeHint());
  grid->addWidget(label2, 1, 0);

  label2->setMaximumWidth(QMAX(label1->sizeHint().width(),
			       label2->sizeHint().width()));

  ignoreCount_ = new KIntNumInput(this);
  ignoreCount_->setValue(BP->ignoreCount());
  ignoreCount_->setMinimumSize(ignoreCount_->sizeHint());
  label2->setBuddy(ignoreCount_);
  grid->addWidget(ignoreCount_, 1, 1);

  enabled_ = new QCheckBox(  i18n("&Enable"), this);
  enabled_->setMinimumSize( enabled_->sizeHint() );
  enabled_->setChecked(BP->isEnabled());
  topLayout->addWidget( enabled_, 0, 0 );

  KButtonBox *buttonbox = new KButtonBox(this);
  QPushButton *ok = buttonbox->addButton(i18n("OK"));
  QPushButton *cancel = buttonbox->addButton(i18n("Cancel"));
  connect(ok, SIGNAL(clicked()), SLOT(accept()));
  connect(cancel, SIGNAL(clicked()), SLOT(reject()));
  ok->setDefault(true);
  buttonbox->layout();
  topLayout->addWidget(buttonbox, 0);

  topLayout->activate();
  resize(0,0);      // Force dialog to the minimum size
}

/***************************************************************************/

BPDialog::~BPDialog()
{
}

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
#include "breakpointdialog.moc"
