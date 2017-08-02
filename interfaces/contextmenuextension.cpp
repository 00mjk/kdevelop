/***************************************************************************
 *   This file is part of KDevelop                                         *
 *   Copyright 2008 Andreas Pakulat <apaku@gmx.de>                         *
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

#include "contextmenuextension.h"

#include "qtcompat_p.h"

#include <QMap>

#include <QMenu>
#include <QAction>

#include <KLocalizedString>

namespace KDevelop
{

const QString ContextMenuExtension::FileGroup     = QStringLiteral("FileGroup");
const QString ContextMenuExtension::RefactorGroup = QStringLiteral("RefactorGroup");
const QString ContextMenuExtension::BuildGroup    = QStringLiteral("BuildGroup");
const QString ContextMenuExtension::RunGroup      = QStringLiteral("RunGroup");
const QString ContextMenuExtension::DebugGroup    = QStringLiteral("DebugGroup");
const QString ContextMenuExtension::EditGroup     = QStringLiteral("EditGroup");
const QString ContextMenuExtension::VcsGroup      = QStringLiteral("VcsGroup");
const QString ContextMenuExtension::ProjectGroup  = QStringLiteral("ProjectGroup");
const QString ContextMenuExtension::OpenEmbeddedGroup  = QStringLiteral("OpenEmbeddedGroup");
const QString ContextMenuExtension::OpenExternalGroup  = QStringLiteral("OpenExternalGroup");
const QString ContextMenuExtension::AnalyzeFileGroup = QStringLiteral("AnalyzeFileGroup");
const QString ContextMenuExtension::AnalyzeProjectGroup = QStringLiteral("AnalyzeProjectGroup");
const QString ContextMenuExtension::NavigationGroup = QStringLiteral("NavigationGroup");
const QString ContextMenuExtension::ExtensionGroup  = QStringLiteral("ExtensionGroup");

class ContextMenuExtensionPrivate
{
public:
    QMap<QString,QList<QAction*> > extensions;
};

ContextMenuExtension::ContextMenuExtension()
    : d(new ContextMenuExtensionPrivate)
{
}

ContextMenuExtension::~ContextMenuExtension()
{
    delete d;
}


ContextMenuExtension::ContextMenuExtension( const ContextMenuExtension& rhs )
    : d( new ContextMenuExtensionPrivate )
{
    d->extensions = rhs.d->extensions;
}

ContextMenuExtension& ContextMenuExtension::operator=( const ContextMenuExtension& rhs )
{
    if( this == &rhs )
        return *this;

    d->extensions = rhs.d->extensions;
    return *this;
}

QList<QAction*> ContextMenuExtension::actions( const QString& group ) const
{
    return d->extensions.value( group, QList<QAction*>() );
}

void ContextMenuExtension::addAction( const QString& group, QAction* action )
{
    if( !d->extensions.contains( group ) )
    {
        d->extensions.insert( group, QList<QAction*>() << action );
    } else
    {
        d->extensions[group].append( action );
    }
}

static
void populateMenuWithGroup(
    QMenu* menu,
    const QList<ContextMenuExtension>& extensions,
    const QString& groupName,
    const QString& groupDisplayName = QString(),
    bool forceAddMenu = false,
    bool addSeparator = true)
{
    QList<QAction*> groupActions;
    for (const ContextMenuExtension& extension : extensions) {
        groupActions += extension.actions(groupName);
    }
    // remove NULL QActions, if any. Those can end up in groupActions if plugins
    // like the debugger plugins are not loaded.
    groupActions.removeAll(nullptr);

    if (groupActions.isEmpty()) {
        return;
    }

    QMenu* groupMenu = menu;
    if ((groupActions.count() > 1 && !groupDisplayName.isEmpty()) ||
        (!groupDisplayName.isEmpty() && forceAddMenu)) {
        groupMenu = menu->addMenu(groupDisplayName);
    }

    for (QAction* action : groupActions) {
        groupMenu->addAction(action);
    }

    if (addSeparator) {
        menu->addSeparator();
    }
}

void ContextMenuExtension::populateMenu(QMenu* menu, const QList<ContextMenuExtension>& extensions)
{
    populateMenuWithGroup(menu, extensions, BuildGroup);
    populateMenuWithGroup(menu, extensions, FileGroup);
    populateMenuWithGroup(menu, extensions, EditGroup);
    populateMenuWithGroup(menu, extensions, DebugGroup, i18n("Debug"));
    populateMenuWithGroup(menu, extensions, RefactorGroup, i18n("Refactor"));
    populateMenuWithGroup(menu, extensions, NavigationGroup);
    populateMenuWithGroup(menu, extensions, AnalyzeFileGroup, i18n("Analyze Current File With"), true, false);
    populateMenuWithGroup(menu, extensions, AnalyzeProjectGroup, i18n("Analyze Current Project With"), true);
    populateMenuWithGroup(menu, extensions, VcsGroup);
    populateMenuWithGroup(menu, extensions, ExtensionGroup);
}

}
