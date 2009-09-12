/***************************************************************************
 *   Copyright 2007 Robert Gruber <rgruber@users.sourceforge.net>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "snippetrepository.h"
#include "globals.h"

#include <QTimer>
#include <QFSFileEngine>
#include <QRegExp>

#include <kicon.h>
#include <kdebug.h>

#include "snippetstore.h"
#include "snippet.h"

#include <KMessageBox>
#include <KLocalizedString>
#include <QApplication>

SnippetRepository::SnippetRepository(const QString& name, const QString& location)
 : QStandardItem(name)
{
    setIcon( KIcon("folder") );
    setLocation( location );

    // Tell the new repository to load it's snippets
    QTimer::singleShot(0, this, SLOT(slotSyncRepository()));
}

SnippetRepository::~SnippetRepository()
{
    // remove all our children from both the model and our internal data structures
    removeRows( 0, rowCount() );

    if (index().isValid()) {
        // in case this item is still a member of a model, remove itself
        QStandardItem::parent()->removeRows( row(), 1 );
    }

    snippets_.clear();
    subrepos_.clear();
}

void SnippetRepository::slotSyncRepository()
{
    // as we are going to reget all snippets from this base directory
    // we first need to remove all snippets and sub-repos
    removeRows( 0, rowCount() );
    snippets_.clear();
    subrepos_.clear();

    // now reget all snippets and sub-repos
    QFSFileEngine engine( location_ );

    QStringList filter;
    filter << "*";

    QStringList list;
    list = engine.entryList( QDir::Dirs|QDir::Files, filter);

    // Ignore all files starting with a dot.
    // On Linux this includes the "." and ".." entries of each directory.
    QRegExp noHiddenFiles("^[^.]");
    QStringList l = list.filter(noHiddenFiles);

    // Now run through all found entries ...
    foreach (const QString &file, l) {
        QFileInfo info(location_+QDir::separator()+file);

        if (info.isHidden()) {
            // On Windows hidden files do not start with a dot.
            // so we sort them out here
            continue;
        }

        if (info.isDir()) {
            // ... and create either a new repository ...
            SnippetStore::instance()->createNewRepository( this, file, location_+QDir::separator()+file );
        } else {
            // ... or a new snippet
            new Snippet(file, this);
        }
    }
}

void SnippetRepository::addSnippet( Snippet* snippet )
{
    appendRow( snippet );
    snippets_.append( snippet );
}

void SnippetRepository::changeLocation(const QString& newLocation, const QString& newName)
{
    // In case the location didn't change, we might still need to update the name
    if (getLocation() == QDir::cleanPath(newLocation)) {
        if (!newName.isEmpty())
            setText( newName );
        return;
    }

    //if the location changed, try to move the repository
    QString oldLocation = getLocation();
    QDir dir(oldLocation);
    if (dir.rename( oldLocation, newLocation )) {
        // if moving was ok, set the location, tooltip and the name
        setLocation( newLocation );
        if (!newName.isEmpty())
            setText( newName );

        // as the snippets and subrepos now changed their
        // location on the disk, resync everything in this repo
        slotSyncRepository();
    } else {
        KMessageBox::error(
            QApplication::activeWindow(),
            i18n("Could not move repository from \"%1\" to \"%2\".", oldLocation, newLocation)
        );
    }
}

void SnippetRepository::removeDirectory()
{
    QDir dir( getLocation() );
    if (dir.rmdir( getLocation() )) {
        QStandardItem::parent()->removeRows( row(), 1 );
    } else {
        KMessageBox::error(
            QApplication::activeWindow(),
            i18n("Could not remove repository \"%1\".", getLocation())
        );
    }
}

void SnippetRepository::createSubRepo(const QString& subrepo)
{
    // First create the directory ...
    QDir dir( location_ );
    dir.mkdir( subrepo );

    // ... then sync this repository in order to see the directory in the model
    slotSyncRepository();
}

#include "snippetrepository.moc"
