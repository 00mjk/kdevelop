/***************************************************************************
*   Copyright (C) 2003, 2006 by KDevelop Authors                          *
*   kdevelop-devel@kde.org                                                *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "kdevkonsoleview.h"

#include <QDir>
#include <QLabel>
#include <QFrame>
#include <QLayout>
#include <QVBoxLayout>

#include <kurl.h>
#include <kicon.h>
#include <kdebug.h>
#include <klocale.h>
#include <klibloader.h>

#include <kparts/part.h>

#include <kdevcore.h>
#include <project/kdevprojectcontroller.h>
#include <kdevdocumentcontroller.h>

KDevKonsoleView::KDevKonsoleView( QWidget *parent )
        : QWidget( parent ),
        m_part( 0 )
{
    setObjectName( i18n( "Konsole" ) );

    setWhatsThis( i18n( "<b>Konsole</b><p>"
            "This window contains an embedded console." ) );
    setWindowIcon( KIcon( "konsole" ) );
    setWindowTitle( i18n( "Konsole" ) );

    m_vbox = new QVBoxLayout( this );
    m_vbox->setMargin( 0 );
    m_vbox->setSpacing( 0 );

    init();

    //TODO Make this configurable in the future,
    // but by default the konsole shouldn't
    // automatically switch directories on you.

//     connect( KDevelop::Core::documentController(), SIGNAL( documentActivated( KDevDocument* ) ),
//              this, SLOT( documentActivated( KDevDocument* ) ) );

    connect( KDevelop::Core::projectController(), SIGNAL( projectOpened() ),
             this, SLOT( projectOpened() ) );

    connect( KDevelop::Core::projectController(), SIGNAL( projectClosed() ),
             this, SLOT( projectClosed() ) );
}

KDevKonsoleView::~KDevKonsoleView()
{
    delete m_part;
}

void KDevKonsoleView::init()
{
    Q_ASSERT( m_part == 0 );

    if ( KLibFactory * factory = KLibLoader::self() ->factory( "libkonsolepart" ) )
    {
        if ( ( m_part = qobject_cast<KParts::ReadOnlyPart*>( factory->create( this ) ) ) )
        {
            m_part->widget() ->setFocusPolicy( Qt::WheelFocus );
            m_part->widget() ->setFocus();

            if ( QFrame * frame = qobject_cast<QFrame*>( m_part->widget() ) )
                frame->setFrameStyle( QFrame::Panel | QFrame::Sunken );

            m_vbox->addWidget( m_part->widget() );
            connect( m_part, SIGNAL( destroyed() ), this, SLOT( partDestroyed() ) );

            setFocusProxy( m_part->widget() );

            m_part->widget() ->show();
        }
    }
    else
    {
        m_vbox->addWidget(
            new QLabel( i18n( "Konsole not available or libkonsolepart not in path." ),
                        this ) );
    }
}

void KDevKonsoleView::partDestroyed()
{
    m_part = 0;
    init();
}

void KDevKonsoleView::projectOpened()
{
    setDirectory( KDevelop::Core::projectController()->globalFile() );
}

void KDevKonsoleView::projectClosed()
{
    setDirectory( KDevelop::Core::projectController()->projectsDirectory() );
}

void KDevKonsoleView::documentActivated( KDevelop::Document *document )
{
    setDirectory( document->url() );
}

void KDevKonsoleView::setDirectory( const KUrl &url )
{
    if ( !url.isValid() || !url.isLocalFile() )
        return ;

    if ( m_part && url != m_part->url() )
        m_part->openUrl( url );
}

#include "kdevkonsoleview.moc"

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on
