/*
    This file is part of the KDevelop Okteta module, part of the KDE project.

    Copyright 2010 Friedrich W. H. Kossebau <kossebau@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library. If not, see <http://www.gnu.org/licenses/>.
*/

#include "oktetatoolviewfactory.h"

// plugin
#include "kastentoolviewwidget.h"
// Kasten
#include <abstracttoolview.h>
#include <abstracttool.h>


namespace KDevelop
{

OktetaToolViewFactory::OktetaToolViewFactory( Kasten::AbstractToolView* toolView, const QString& id )
  : IToolViewFactory(),
    mToolView( toolView ),
    mId( id )
{
}

QWidget* OktetaToolViewFactory::create( QWidget* parent )
{
    return new KastenToolViewWidget( mToolView, parent );
}

Qt::DockWidgetArea OktetaToolViewFactory::defaultPosition()
{
    return Qt::LeftDockWidgetArea;
}

QString OktetaToolViewFactory::id() const
{
    return "org.kde.okteta." + mId;
}

OktetaToolViewFactory::~OktetaToolViewFactory()
{
    Kasten::AbstractTool* tool = mToolView->tool();
    delete mToolView;
    delete tool;
}

}
