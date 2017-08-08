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
#include "view.h"

#include <QWidget>

#include "document.h"
#include "tooldocument.h"

namespace Sublime {

class View;
class Document;

class ViewPrivate
{
public:
    ViewPrivate();
    Document *doc;
    QWidget *widget;
    void unsetWidget();
    View::WidgetOwnership ws;
};

ViewPrivate::ViewPrivate()
    :doc(nullptr), widget(nullptr)
{
}

void ViewPrivate::unsetWidget()
{
    widget = nullptr;
}

View::View(Document *doc, WidgetOwnership ws )
    :QObject(doc), d(new ViewPrivate)
{
    d->doc = doc;
    d->ws = ws;
}

View::~View()
{
    if (d->widget && d->ws == View::TakeOwnership ) {
        d->widget->hide();
        d->widget->setParent(nullptr);
        d->widget->deleteLater();
    }
}

Document *View::document() const
{
    return d->doc;
}

QWidget *View::widget(QWidget *parent)
{
    if (!d->widget)
    {
        d->widget = createWidget(parent);
        connect(d->widget, &QWidget::destroyed, this, [&] { d->unsetWidget(); });
    }
    return d->widget;
}

QWidget *View::createWidget(QWidget *parent)
{
    return d->doc->createViewWidget(parent);
}

bool View::hasWidget() const
{
    return d->widget != nullptr;
}

void View::requestRaise()
{
    emit raise(this);
}

QString View::viewState() const
{
    return QString();
}

void View::setState(const QString & state)
{
    Q_UNUSED(state);
}

QList<QAction*> View::toolBarActions() const
{
    ToolDocument* tooldoc = dynamic_cast<ToolDocument*>( document() );
    if( tooldoc )
    {
        return tooldoc->factory()->toolBarActions( d->widget );
    }
    return QList<QAction*>();
}

QList< QAction* > View::contextMenuActions() const
{
    ToolDocument* tooldoc = dynamic_cast<ToolDocument*>( document() );
    if( tooldoc )
    {
        return tooldoc->factory()->contextMenuActions( d->widget );
    }
    return QList<QAction*>();
}

QString View::viewStatus() const
{
    return QString();
}

void View::notifyPositionChanged(int newPositionInArea)
{
    emit positionChanged(this, newPositionInArea);
}

}

#include "moc_view.cpp"
