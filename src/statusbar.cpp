/***************************************************************************
 *   Copyright (C) 2001 by Bernd Gehrmann                                  *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qlayout.h>
#include <qlineedit.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qfontmetrics.h>

#include <kdebug.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kparts/part.h>
#include <ktexteditor/viewcursorinterface.h>
#include <kdeversion.h>

#if defined(KDE_MAKE_VERSION)
# if KDE_VERSION >= KDE_MAKE_VERSION(3,1,0)
#include <ktexteditor/viewstatusmsginterface.h>
# endif
#endif

#include "statusbar.h"
#include "partcontroller.h"

StatusBar::StatusBar(QWidget *parent, const char *name)
    : KStatusBar(parent, name), _cursorIface(0), _activePart(0)
{
	QWidget * w = new QWidget( this );
	addWidget( w, 1, true );
	w->hide();

	_status = new QLabel( this );
	_status->setMinimumWidth(_status->fontMetrics().width("Line: XXXXX Col: XXX  OVR NORM * "));
	_status->setAlignment(QWidget::AlignCenter);
	addWidget(_status, 0, true);

	connect(PartController::getInstance(), SIGNAL(activePartChanged(KParts::Part*)),
		this, SLOT(activePartChanged(KParts::Part*)));
}


StatusBar::~StatusBar()
{}

void StatusBar::activePartChanged(KParts::Part *part)
{
	if ( _activePart && _activePart->widget() )
		disconnect( _activePart->widget(), 0, this, 0 );

	_activePart = part;
	_cursorIface = 0;
	_viewmsgIface = 0;
  
	if (part && part->widget())
	{
		if ((_viewmsgIface = dynamic_cast<KTextEditor::ViewStatusMsgInterface*>(part->widget())))
		{
			connect( part->widget(), SIGNAL( viewStatusMsg( const QString & ) ),
				this, SLOT( setStatus( const QString & ) ) );
	
			_status->show();
		}
		else if ((_cursorIface = dynamic_cast<KTextEditor::ViewCursorInterface*>(part->widget())))
		{
			connect(part->widget(), SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));
	
			_status->show();
			cursorPositionChanged();
		}
		else
		{
			// we can't produce any status data, hide the status box
			_status->hide();
		}
	}
}

void StatusBar::cursorPositionChanged()
{
  if (_cursorIface)
  {
    uint line, col;
    _cursorIface->cursorPosition(&line, &col);
    setCursorPosition(line, col);
  }
}

void StatusBar::setStatus(const QString &str)
{
	_status->setText(str);
}


void StatusBar::setCursorPosition(int line, int col)
{
	_status->setText(i18n(" Line: %1 Col: %2 ").arg(line+1).arg(col));
}

void StatusBar::addWidget ( QWidget *widget, int stretch, bool permanent)
{
	KStatusBar::addWidget(widget,stretch,permanent);

	if(widget->sizeHint().height() + 4 > height())
		setFixedHeight(widget->sizeHint().height() + 4);
}

#include "statusbar.moc"
