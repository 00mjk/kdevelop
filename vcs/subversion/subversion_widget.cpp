/**
	 Copyright (C) 2003-2005 Mickael Marchand <marchand@kde.org>

	 This program is free software; you can redistribute it and/or
	 modify it under the terms of the GNU General Public
	 License as published by the Free Software Foundation; either
	 version 2 of the License, or (at your option) any later version.

	 This program is distributed in the hope that it will be useful,
	 but WITHOUT ANY WARRANTY; without even the implied warranty of
	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	 General Public License for more details.

	 You should have received a copy of the GNU General Public License
	 along with this program; see the file COPYING.  If not, write to
	 the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
	 Boston, MA 02111-1307, USA.
	 */

#include <kparts/part.h>
#include <kdevcore.h>
#include <kdebug.h>

#include "subversion_part.h"
#include "subversion_widget.h"

subversionWidget::subversionWidget(subversionPart *part, QWidget *parent, const char* name)
 : QTextEdit(parent, name)
{
	m_part = part;
	setReadOnly( true );
#if QT_VERSION >= 0x030100
	setTextFormat( Qt::LogText );
#endif
}

subversionWidget::~subversionWidget()
{}

SvnIntSortListItem::SvnIntSortListItem( QListView* parent )
	:QListViewItem(parent)
{}
SvnIntSortListItem::~SvnIntSortListItem()
{}

int SvnIntSortListItem::compare( QListViewItem *item, int col, bool ascending ) const
{
	
	unsigned int myVal = this->text(col).toUInt();
	unsigned int yourVal = item->text(col).toUInt();
	if( myVal < yourVal ) return -1;
	if( myVal == yourVal ) return 0;
	if( myVal > yourVal ) return 1;
}


#include "subversion_widget.moc"
