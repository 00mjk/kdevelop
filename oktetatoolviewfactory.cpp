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
#include <Kasten/AbstractToolViewFactory>
#include <Kasten/AbstractToolFactory>
#include <Kasten/AbstractToolView>
// KDE
#include <KIcon>


namespace KDevelop
{

OktetaToolViewFactory::OktetaToolViewFactory( Kasten::AbstractToolViewFactory* toolViewFactory,
                                              Kasten::AbstractToolFactory* toolFactory,
                                              const QString& iconName,
                                              const QString& id, Qt::DockWidgetArea defaultPosition )
  : IToolViewFactory(),
    mToolViewFactory( toolViewFactory ),
    mToolFactory( toolFactory ),
    mIconName( iconName ),
    mId( "org.kde.okteta." + id ),
    mDefaultPosition( defaultPosition )
{
}

QString OktetaToolViewFactory::id() const { return mId; }
Qt::DockWidgetArea OktetaToolViewFactory::defaultPosition() { return mDefaultPosition; }

QWidget* OktetaToolViewFactory::create( QWidget* parent )
{
    Kasten::AbstractTool* tool = mToolFactory->create();
    Kasten::AbstractToolView* toolView = mToolViewFactory->create( tool );

    QWidget* widget = new KastenToolViewWidget( toolView, parent );
    widget->setWindowIcon( KIcon(mIconName) );
    return widget;
}


OktetaToolViewFactory::~OktetaToolViewFactory()
{
    delete mToolViewFactory;
    delete mToolFactory;
}

}
