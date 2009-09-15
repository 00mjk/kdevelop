/***************************************************************************
 *   Copyright 2007 Robert Gruber <rgruber@users.sourceforge.net>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "snippetstore.h"


#include "snippetrepository.h"

#include <KMessageBox>
#include <KLocalizedString>
#include <QApplication>

SnippetStore* SnippetStore::self_ = NULL;


SnippetStore::SnippetStore()
{
}

SnippetStore::~SnippetStore()
{
    invisibleRootItem()->removeRows( 0, invisibleRootItem()->rowCount() );
    repos_.clear();
    self_ = NULL;
}

SnippetStore* SnippetStore::instance()
{
    if (!self_) {
        self_ = new SnippetStore();
    }

    return self_;
}

void SnippetStore::createNewRepository(SnippetRepository* parent, const QString& name, const QString& dir)
{
    {
        QDir location(dir);
        if ( dir.isEmpty() || (!location.exists() && !location.mkpath(dir)) ) {
            KMessageBox::error(
                QApplication::activeWindow(),
                i18n("Could not create the repository folder \"%1\".", dir)
            );
            return;
        }
    }

    if (!parent) {
        // Check if the directory is not already in the SnippetStore
        // We allow each toplevel repository only once
        foreach(SnippetRepository* repo, repos_) {
            if (repo->getLocation() == QDir::cleanPath(dir) ) {
                return;
            }
        }
    }

    //Add the given directory to the SnippetStore
    SnippetRepository *item = new SnippetRepository(name, dir);
    if (parent) {
        parent->addSubRepo( item );
        connect(item, SIGNAL(repositoryRemoved(SnippetRepository*)),
                parent, SLOT(slotSubRepositoryRemoved(SnippetRepository*)));
    } else {
        repos_.append( item );
        appendRow( item );
        connect(item, SIGNAL(repositoryRemoved(SnippetRepository*)),
                this, SLOT(remove(SnippetRepository*)));
    }
}

Qt::ItemFlags SnippetStore::flags(const QModelIndex & index) const
{
    Q_UNUSED(index)

    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    return flags;
}

void SnippetStore::remove(SnippetRepository* repo)
{
    int idx = repos_.indexOf( repo );

    if (idx >= 0) {
        // Only remove the given item if it's really a tolevel repo
        invisibleRootItem()->removeRows( repo->row(), 1 );
        repos_.removeAt( idx );
    }
}

void SnippetStore::load(KConfigGroup config)
{
    if (config.hasGroup("repos")) {
        KConfigGroup repoGroup = config.group("repos");
        QStringList keys = repoGroup.keyList();
        foreach(QString key, keys) {
            createNewRepository(0, key, repoGroup.readEntry(key));
        }
    }
}

void SnippetStore::save(KConfigGroup config)
{
    // remove all old repos that might have changed
    config.group("repos").deleteGroup();
    config.sync();

    foreach(SnippetRepository* repo, repos_) {
        config.group("repos").writeEntry(repo->text(), repo->getLocation());
    }
}

#include "snippetstore.moc"
