/* $Id$
 *
 *  Copyright (C) 2002 Roberto Raggi (roberto@kdevelop.org)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 *
 */

/**********************************************************************
** Copyright (C) 2000 Trolltech AS.  All rights reserved.
**
** This file is part of Qt Designer.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file COPYING included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "markerwidget.h"
#include "qeditor.h"
#include "paragdata.h"

#include <qpopupmenu.h>
#include <private/qrichtext_p.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>

using namespace std;

MarkerWidget::MarkerWidget( QEditor* editor, QWidget* parent, const char* name )
    : QWidget( parent, name, WRepaintNoErase | WStaticContents | WResizeNoErase ),
      m_editor( editor )
      ,m_clickChangesBPs(true)
      ,m_changeBookmarksAllowed(false)
      ,m_changeBreakpointsAllowed(false)
      ,m_bookmarkDescr(i18n("Bookmark"))
      ,m_breakpointDescr(i18n("Breakpoint"))
{
    m_pixmapMap.insert(0x01, SmallIcon("attach"));
    m_pixmapMap.insert(0x05, SmallIcon("exec"));
    m_pixmapMap.insert(0x200, SmallIcon("stop"));
    m_pixmapMap.insert(0x400, SmallIcon("fun"));

    setFixedWidth( 20 );

    connect( m_editor->verticalScrollBar(), SIGNAL( valueChanged( int ) ),
             this, SLOT( doRepaint() ) );
    connect( m_editor, SIGNAL( textChanged() ),
             this, SLOT( doRepaint() ) );

    doRepaint();
}

MarkerWidget::~MarkerWidget()
{
}

void MarkerWidget::paintEvent( QPaintEvent* /*e*/ )
{
    m_buffer.fill();

    QTextParagraph *p = m_editor->document()->firstParagraph();
    QPainter painter( &m_buffer );
    int yOffset = m_editor->contentsY();
    while ( p ) {
        if ( !p->isVisible() ) {
            p = p->next();
            continue;
        }
        if ( p->rect().y() + p->rect().height() - yOffset < 0 ) {
            p = p->next();
            continue;
        }
        if ( p->rect().y() - yOffset > height() )
            break;

        ParagData* paragData = (ParagData*) p->extraData();
        unsigned int mark = paragData ? paragData->mark() : 0;
        if (mark) {
            unsigned int current = 0x01;
            for (; current < mark+1; current = current << 1) {
                if (mark & current) {
                    QMapIterator<int,QPixmap> it = m_pixmapMap.find(current);
                    if (it != m_pixmapMap.end()) {
                        painter.drawPixmap( 3,
                                            p->rect().y() + ( p->rect().height() - (*it).height() ) / 2 - yOffset,
                                            *it );
                    }
                }
            }
        }
        p = p->next();
    }

    painter.end();
    bitBlt( this, 0, 0, &m_buffer );
}


void MarkerWidget::resizeEvent( QResizeEvent *e )
{
    m_buffer.resize( e->size() );
    QWidget::resizeEvent( e );
}

void MarkerWidget::contextMenuEvent( QContextMenuEvent* e )
{
    QPopupMenu m( 0, "editor_breakpointsmenu" );

    int toggleBreakPoint = 0;
    int toggleBookmark = 0;
    int lmbClickChangesBPs = 0;
    int lmbClickChangesBookmarks = 0;

    QTextParagraph *p = m_editor->document()->firstParagraph();
    int yOffset = m_editor->contentsY();
    while ( p ) {
        if ( e->y() >= p->rect().y() - yOffset && e->y() <= p->rect().y() + p->rect().height() - yOffset ) {
            ParagData* data = (ParagData*) p->extraData();
            if ( data->mark() & 0x02 )
                toggleBreakPoint = m.insertItem( i18n( "Clear %1" ).arg(m_breakpointDescr) );
            else
                toggleBreakPoint = m.insertItem( i18n( "Set %1" ).arg(m_breakpointDescr) );
            m.setItemEnabled(toggleBreakPoint, m_changeBreakpointsAllowed);
            m.insertSeparator();

            if ( data->mark() & 0x01 )
                toggleBookmark = m.insertItem( i18n( "Clear %1" ).arg(m_bookmarkDescr) );
            else
                toggleBookmark = m.insertItem( i18n( "Set %1" ).arg(m_bookmarkDescr) );
            m.setItemEnabled(toggleBookmark, m_changeBookmarksAllowed);
            m.insertSeparator();

            lmbClickChangesBPs = m.insertItem( i18n( "Left mouse button click sets: %1" ).arg(m_breakpointDescr) );
            lmbClickChangesBookmarks = m.insertItem( i18n( "Left mouse button click sets: %1" ).arg(m_bookmarkDescr) );
            m.setItemChecked(lmbClickChangesBPs, m_clickChangesBPs);
            m.setItemChecked(lmbClickChangesBookmarks, !m_clickChangesBPs);

            //m.insertSeparator();
            break;
        }
        p = p->next();
    }

    int res = m.exec( e->globalPos() );
    if ( res == -1)
        return;

    ParagData* data = (ParagData*) p->extraData();

    KTextEditor::Mark mark;
    mark.line = p->paragId();

    if ( res == toggleBookmark && m_changeBookmarksAllowed ) {
        mark.type = 0x01;
        if ( data->mark() & 0x01 ) {
            data->setMark( data->mark() & ~0x01 );
            emit markChanged(mark, KTextEditor::MarkInterfaceExtension::MarkRemoved);
        }
        else {
            data->setMark( data->mark() | 0x01 );
            emit markChanged(mark, KTextEditor::MarkInterfaceExtension::MarkAdded);
        }
    } else if ( res == toggleBreakPoint && m_changeBreakpointsAllowed ) {
        mark.type = 0x02;
        if ( data->mark() & 0x02 ) {
            data->setMark( data->mark() & ~0x02 );
            emit markChanged(mark, KTextEditor::MarkInterfaceExtension::MarkRemoved);
        }
        else {
            data->setMark( data->mark() | 0x02 );
            emit markChanged(mark, KTextEditor::MarkInterfaceExtension::MarkAdded);
        }
    } else if ( res == lmbClickChangesBPs ) {
        m_clickChangesBPs = !m.isItemChecked(lmbClickChangesBPs);
    } else if ( res == lmbClickChangesBookmarks ) {
        m_clickChangesBPs = m.isItemChecked(lmbClickChangesBookmarks);
    }

    emit marksChanged();
    doRepaint();
}

void MarkerWidget::mousePressEvent( QMouseEvent * e )
{
  QTextParagraph *p = m_editor->document()->firstParagraph();
  int yOffset = m_editor->contentsY();
  ParagData* data = 0L;
  while ( p ) {
    if ( e->y() >= p->rect().y() - yOffset && e->y() <= p->rect().y() + p->rect().height() - yOffset ) {
      data = (ParagData*) p->extraData();
      break;
    }
    p = p->next();
  }

  if (e->button() == Qt::LeftButton) {
    if (!data) return;

    KTextEditor::Mark mark;
    mark.line = p->paragId();
    if (m_clickChangesBPs && m_changeBreakpointsAllowed) {
      mark.type = 0x02;
      if (data->mark() & 0x02) {
        data->setMark(data->mark() & ~0x02);
        emit markChanged(mark, KTextEditor::MarkInterfaceExtension::MarkRemoved);
      }
      else {
        data->setMark(data->mark() | 0x02);
        emit markChanged(mark, KTextEditor::MarkInterfaceExtension::MarkAdded);
      }
    }
    else if (m_changeBookmarksAllowed) {
      mark.type = 0x01;
      if (data->mark() & 0x01) {
        data->setMark(data->mark() & ~0x01);
        emit markChanged(mark, KTextEditor::MarkInterfaceExtension::MarkRemoved);
      }
      else {
        data->setMark(data->mark() | 0x01);
        emit markChanged(mark, KTextEditor::MarkInterfaceExtension::MarkAdded);
      }
    }
  }

  emit marksChanged();
  doRepaint();
}

void MarkerWidget::setPixmap(KTextEditor::MarkInterface::MarkTypes mt, const QPixmap & pm)
{
  if (mt)
    m_pixmapMap.insert(mt, pm);
}

void MarkerWidget::setDescription(KTextEditor::MarkInterface::MarkTypes mt, const QString & s)
{
  switch (mt) {
  case KTextEditor::MarkInterface::markType01: m_bookmarkDescr = s; break;
  case KTextEditor::MarkInterface::markType02: m_breakpointDescr = s; break;
  default: break;
  }
}

void MarkerWidget::setMarksUserChangable(uint markMask)
{
  m_changeBookmarksAllowed   = (markMask & KTextEditor::MarkInterface::markType01) ? true : false;
  m_changeBreakpointsAllowed = (markMask & KTextEditor::MarkInterface::markType02) ? true : false;

  doRepaint();
}

#include "markerwidget.moc"
