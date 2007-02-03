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
#include "document.h"

#include <kdebug.h>

#include "view.h"
#include "controller.h"

namespace Sublime {

// struct DocumentPrivate

struct DocumentPrivate {
    void removeView(QObject *obj)
    {
        views.removeAll(reinterpret_cast<Sublime::View*>(obj));
    }

    Controller *controller;
    QList<View*> views;
};



//class Document

Document::Document(const QString &title, Controller *controller)
    :QObject(controller), ViewCreator()
{
    setObjectName(title);
    d = new DocumentPrivate();
    d->controller = controller;
    d->controller->addDocument(this);
    connect(this, SIGNAL(destroyed(QObject*)), d->controller, SLOT(removeDocument(QObject*)));
}

Document::~Document()
{
    delete d;
}

Controller *Document::controller() const
{
    return d->controller;
}

View *Document::createView()
{
    View *view = newView(this);
    connect(view, SIGNAL(destroyed(QObject*)), this, SLOT(removeView(QObject*)));
    d->views.append(view);
    return view;
}

const QList<View*> &Document::views() const
{
    return d->views;
}

QString Document::title() const
{
    return objectName();
}

}

#include "document.moc"

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on
