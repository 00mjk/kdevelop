/*
   Copyright 2009 David Nolden <david.nolden.kdevelop@art-master.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "iassistant.h"
#include "icore.h"

#include <QAction>
#include <QThread>

using namespace KDevelop;

Q_DECLARE_METATYPE(QExplicitlySharedDataPointer<IAssistantAction>)

//BEGIN IAssistant

void IAssistant::createActions()
{
}

QAction* IAssistantAction::toQAction(QObject* parent) const
{
    Q_ASSERT(QThread::currentThread() == ICore::self()->thread() && "Actions must be created in the application main thread"
                                                    "(implement createActions() to create your actions)");

    QAction* ret = new QAction(icon(), description(), parent);
    ret->setToolTip(toolTip());

    //Add the data as a QExplicitlySharedDataPointer to the action, so this assistant stays alive at least as long as the QAction
    ret->setData(QVariant::fromValue(QExplicitlySharedDataPointer<IAssistantAction>(const_cast<IAssistantAction*>(this))));

    connect(ret, &QAction::triggered, this, &IAssistantAction::execute);
    return ret;
}

IAssistant::~IAssistant()
{
}

IAssistantAction::IAssistantAction()
    : QObject()
    , KSharedObject(*(QObject*)this)
{
}

IAssistantAction::~IAssistantAction()
{
}

QIcon IAssistantAction::icon() const
{
    return QIcon();
}

QString IAssistantAction::toolTip() const
{
    return QString();
}

//END IAssistantAction

//BEGIN AssistantLabelAction

AssistantLabelAction::AssistantLabelAction(const QString& description)
: m_description(description)
{

}

QString AssistantLabelAction::description() const
{
    return m_description;
}

void AssistantLabelAction::execute()
{
    // do nothing
}

QAction* AssistantLabelAction::toQAction(QObject* parent) const
{
    Q_UNUSED(parent);
    return nullptr;
}

//END AssistantLabelAction

//BEGIN: IAssistant

IAssistant::IAssistant()
: KSharedObject(*(QObject*)this)
{
}

QIcon IAssistant::icon() const
{
    return QIcon();
}

QString IAssistant::title() const
{
    return QString();
}

void IAssistant::doHide()
{
    emit hide();
}

QList< IAssistantAction::Ptr > IAssistant::actions() const
{
    if ( m_actions.isEmpty() ) {
        const_cast<IAssistant*>(this)->createActions();
    }
    return m_actions;
}

void IAssistant::addAction(const IAssistantAction::Ptr& action)
{
    m_actions << action;
}

void IAssistant::clearActions()
{
    m_actions.clear();
}

//END IAssistant

