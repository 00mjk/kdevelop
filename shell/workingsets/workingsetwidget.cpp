/*
    Copyright David Nolden  <david.nolden.kdevelop@art-master.de>
    Copyright 2010 Milian Wolff <mail@milianw.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "workingsetwidget.h"

#include <KDebug>

#include <sublime/area.h>

#include "mainwindow.h"

#include "workingsetcontroller.h"
#include "workingset.h"
#include "workingsettoolbutton.h"

using namespace KDevelop;

WorkingSet* getSet(const QString& id)
{
    return Core::self()->workingSetControllerInternal()->getWorkingSet(id);
}

WorkingSetWidget::WorkingSetWidget(MainWindow* parent, Sublime::Area* area)
    : WorkingSetToolButton(0, getSet(area->workingSet()), parent), m_area(area)
{
    //Queued connect so the change is already applied to the area when we start processing
    connect(m_area, SIGNAL(changingWorkingSet(Sublime::Area*, QString, QString)), this,
            SLOT(changingWorkingSet(Sublime::Area*, QString, QString)), Qt::QueuedConnection);

    setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored));
}

void WorkingSetWidget::setVisible( bool visible )
{
    // never show empty working sets
    // TODO: I overloaded this only because hide() in the ctor does not work, other ideas?
    QWidget::setVisible( !visible || !workingSet()->isEmpty() );
}

void WorkingSetWidget::changingWorkingSet( Sublime::Area* area, const QString& /*from*/, const QString& newSet)
{
    kDebug() << "re-creating widget" << m_area;

    Q_ASSERT(area == m_area);

    setWorkingSet(getSet(newSet));
    setVisible(!workingSet()->isEmpty());
}

#include "workingsetwidget.moc"
