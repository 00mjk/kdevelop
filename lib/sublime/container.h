/***************************************************************************
 *   Copyright (C) 2006-2007 by Alexander Dymo  <adymo@kdevelop.org>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
#ifndef SUBLIMECONTAINER_H
#define SUBLIMECONTAINER_H

#include <QWidget>

#include <kdevexport.h>

class QStackedLayout;

namespace Sublime {

class View;

/**
@short Container for the widgets.

This container is placed inside mainwindow splitters to show widgets
for views in the area.
*/
class SUBLIME_EXPORT Container: public QWidget {
Q_OBJECT
public:
    Container(QWidget *parent = 0);
    ~Container();

    /**Adds the widget for given @p view to the container.*/
    void addWidget(View *view);
    /**Removes the widget from the container.*/
    void removeWidget(QWidget *w);
    /** @return true if widget is placed inside this container.*/
    bool hasWidget(QWidget *w);

    /**@return the number of widgets in the container.*/
    int count() const;
    /**@return the widget at given @p index.*/
    QWidget *widget(int index) const;

private:
    Q_PRIVATE_SLOT(d, void widgetActivated(int))

    struct ContainerPrivate *d;

};

}

#endif

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on
