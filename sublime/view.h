/***************************************************************************
 *   Copyright 2006-2007 Alexander Dymo  <adymo@kdevelop.org>       *
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
#ifndef SUBLIMEVIEW_H
#define SUBLIMEVIEW_H

#include <QtCore/QObject>

#include <sublimeexport.h>

namespace Sublime {

class Document;

/**
@short View - the wrapper to the widget that knows about its document

Views are the convenient way to manage a widget. It is specifically designed to be
light and fast. Use @ref Document::createView() to get the new view for the document
and call @ref View::widget() to create and get the actual widget.

It is not possible to create a view by hand. You need either subclass it or use a Document.*/
class SUBLIME_EXPORT View: public QObject {
    Q_OBJECT
public:
    ~View();

    /**@return the document for this view.*/
    Document *document() const;
    /**@return widget for this view (creates it if it's not yet created).*/
    virtual QWidget *widget(QWidget *parent = 0);
    /**@return true if this view has an initialized widget.*/
    bool hasWidget() const;

protected:
    View(Document *doc);

private:
    Q_PRIVATE_SLOT(d, void unsetWidget())

    //copy is not allowed, create a new view from the document instead
    View(const View &v);
    struct ViewPrivate *const d;

    friend class Document;
};

}

#endif

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on
