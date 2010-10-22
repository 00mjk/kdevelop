/***************************************************************************
 *   This file is part of KDevelop                                         *
 *   Copyright 2010 Milian Wolff <mail@milianw.de>                         *
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

#include "abstractfilemanagerplugin.h"

#include "filemanagerlistjob.h"
#include "projectmodel.h"
#include "helper.h"

#include <QFileInfo>

#include <KConfigGroup>
#include <KDebug>
#include <KMessageBox>
#include <KLocalizedString>
#include <KDirWatch>

#include <interfaces/iproject.h>
#include <interfaces/icore.h>
#include <interfaces/iruncontroller.h>
#include <interfaces/iprojectcontroller.h>
#include <QApplication>

#define ifDebug(x)

using namespace KDevelop;

//BEGIN Helper

namespace {

/**
 * Returns the parent folder item for a given item or the project root item if there is no parent.
 */
ProjectFolderItem* getParentFolder(ProjectBaseItem* item)
{
    if ( item->parent() ) {
        return static_cast<ProjectFolderItem*>(item->parent());
    } else {
        return item->project()->projectItem();
    }
}

}

//END Helper

//BEGIN Private

struct AbstractFileManagerPlugin::Private {
    explicit Private(AbstractFileManagerPlugin* qq)
        : q(qq)
    {
    }

    AbstractFileManagerPlugin* q;

    /// @p forceRecursion if true, existing folders will be re-read no matter what
    KJob* eventuallyReadFolder( ProjectFolderItem* item,
                                const bool forceRecursion = false );
    void addJobItems(FileManagerListJob* job,
                     ProjectFolderItem* baseItem,
                     const KIO::UDSEntryList& entries,
                     const bool forceRecursion);

    void deleted(const QString &path);
    void created(const QString &path);

    void projectClosing(IProject* project);
    void jobFinished(KJob* job);

    /// Stops watching the given folder for changes, only useful for local files.
    void stopWatcher(ProjectFolderItem* folder);
    /// Continues watching the given folder for changes.
    void continueWatcher(ProjectFolderItem* folder);
    /// Common renaming function.
    bool rename( ProjectBaseItem* item, const KUrl& destination);

    QMap<IProject*, KDirWatch*> m_watchers;
    QMap<IProject*, QList<KJob*> > m_projectJobs;
};

void AbstractFileManagerPlugin::Private::projectClosing(IProject* project)
{
    if ( m_projectJobs.contains(project) ) {
        // make sure the import job does not live longer than the project
        // see also addLotsOfFiles test
        foreach( KJob* job, m_projectJobs[project] ) {
            kDebug() << "killing project job:" << job;
            job->kill();
        }
        m_projectJobs.remove(project);
    }
    delete m_watchers.take(project);
}

KJob* AbstractFileManagerPlugin::Private::eventuallyReadFolder( ProjectFolderItem* item,
                                                                const bool forceRecursion )
{
    FileManagerListJob* listJob = new FileManagerListJob( item, forceRecursion );
    m_projectJobs[ item->project() ] << listJob;
    kDebug() << "adding job" << listJob << item->url() << "for project" << item->project();

    ICore::self()->runController()->registerJob( listJob );

    q->connect( listJob, SIGNAL(finished(KJob*)),
                q, SLOT(jobFinished(KJob*)) );

    q->connect( listJob, SIGNAL(entries(FileManagerListJob*, ProjectFolderItem*, KIO::UDSEntryList, bool)),
                q, SLOT(addJobItems(FileManagerListJob*, ProjectFolderItem*, KIO::UDSEntryList, bool)) );

    return listJob;
}

void AbstractFileManagerPlugin::Private::jobFinished(KJob* job)
{
    FileManagerListJob* gmlJob = qobject_cast<FileManagerListJob*>(job);
    Q_ASSERT(gmlJob);
    kDebug() << gmlJob;
    m_projectJobs[ gmlJob->item()->project() ].removeOne( job );
}

void AbstractFileManagerPlugin::Private::addJobItems(FileManagerListJob* job,
                                                     ProjectFolderItem* baseItem,
                                                     const KIO::UDSEntryList& entries,
                                                     const bool forceRecursion)
{
    if ( entries.empty() ) {
        return;
    }

    kDebug() << "reading entries of" << baseItem->url();

    // build lists of valid files and folders with relative urls to the project folder
    KUrl::List files;
    KUrl::List folders;
    foreach ( const KIO::UDSEntry& entry, entries ) {
        QString name = entry.stringValue( KIO::UDSEntry::UDS_NAME );
        if (name == "." || name == "..") {
            continue;
        }

        KUrl url = baseItem->url();
        url.addPath( name );

        if ( !q->isValid( url, entry.isDir(), baseItem->project() ) ) {
            continue;
        } else {
            if ( entry.isDir() ) {
                if( entry.isLink() ) {
                    KUrl linkedUrl = baseItem->url();
                    linkedUrl.cd(entry.stringValue( KIO::UDSEntry::UDS_LINK_DEST ));
                    // make sure we don't end in an infinite loop
                    if( linkedUrl.isParentOf( baseItem->project()->folder() ) ||
                        baseItem->project()->folder().isParentOf( linkedUrl ) ||
                        linkedUrl == baseItem->project()->folder() )
                    {
                        continue;
                    }
                }
                folders << url;
            } else {
                files << url;
            }
        }
    }

    ifDebug(kDebug() << "valid folders:" << folders;)
    ifDebug(kDebug() << "valid files:" << files;)

    // remove obsolete rows
    for ( int j = 0; j < baseItem->rowCount(); ++j ) {
        if ( baseItem->child(j)->type() == ProjectBaseItem::Folder ) {
            ProjectFolderItem* f = static_cast<ProjectFolderItem*>( baseItem->child(j) );
            // check if this is still a valid folder
            int index = folders.indexOf( f->url() );
            if ( index == -1 ) {
                // folder got removed or is now invalid
                ifDebug(kDebug() << "removing folder:" << f->url();)
                baseItem->removeRow( j );
                --j;
            } else {
                // this folder already exists in the view
                folders.removeAt( index );
                if ( forceRecursion ) {
                    //no need to add this item, but we still want to recurse into it
                    job->addSubDir( f );
                }
            }
        } else if ( baseItem->child(j)->type() == ProjectBaseItem::File ) {
            ProjectFileItem* f = static_cast<ProjectFileItem*>( baseItem->child(j) );
            // check if this is still a valid file
            int index = files.indexOf( f->url() );
            if ( index == -1 ) {
                // file got removed or is now invalid
                ifDebug(kDebug() << "removing file:" << f->url();)
                baseItem->removeRow( j );
                --j;
            } else {
                // this file already exists in the view
                files.removeAt( index );
            }
        }
    }

    // add new rows
    foreach ( const KUrl& url, files ) {
        ProjectFileItem* file = q->createFileItem( baseItem->project(), url, baseItem );
        emit q->fileAdded( file );
    }
    foreach ( const KUrl& url, folders ) {
        ProjectFolderItem* folder = q->createFolderItem( baseItem->project(), url, baseItem );
        emit q->folderAdded( folder );
        job->addSubDir( folder );
    }
}

void AbstractFileManagerPlugin::Private::created(const QString &path)
{
    kDebug() << "created:" << path;
    QFileInfo info(path);

    KUrl url = KUrl(path);
    KUrl parent = url.upUrl();

    foreach ( IProject* p, m_watchers.keys() ) {
        if ( !q->isValid(url, info.isDir(), p) ) {
            continue;
        }
        if ( !p->foldersForUrl(url).isEmpty() || !p->filesForUrl(url).isEmpty() ) {
            // exists already in this project, happens e.g. when we restart the dirwatcher
            // for files it also gets triggered for kate's backup files
            continue;
        }
        foreach ( ProjectFolderItem* parentItem, p->foldersForUrl(parent) ) {
            if ( info.isDir() ) {
                ProjectFolderItem* folder = q->createFolderItem( p, url, parentItem );
                emit q->folderAdded( folder );
                eventuallyReadFolder( folder );
            } else {
                ProjectFileItem* file = q->createFileItem( p, url, parentItem );
                emit q->fileAdded( file );
            }
        }
    }
}

void AbstractFileManagerPlugin::Private::deleted(const QString &path)
{
    if ( QFile::exists(path) ) {
        // stopDirScan...
        return;
    }
    kDebug() << "deleted:" << path;

    KUrl url = KUrl(path);
    foreach ( IProject* p, m_watchers.keys() ) {
        if ( url.equals(p->folder(), KUrl::CompareWithoutTrailingSlash) ) {
            KMessageBox::error(qApp->activeWindow(),
                               i18n("The base folder of project <b>%1</b>"
                                    " got deleted or moved outside of KDevelop.\n"
                                    "The project has to be closed.", p->name()),
                               i18n("Project Folder Deleted") );
            ICore::self()->projectController()->closeProject(p);
            continue;
        }
        foreach ( ProjectFolderItem* item, p->foldersForUrl(url) ) {
            emit q->folderRemoved(item);
            item->parent()->removeRow(item->row());
        }
        foreach ( ProjectFileItem* item, p->filesForUrl(url) ) {
            emit q->fileRemoved(item);
            item->parent()->removeRow(item->row());
        }
    }
}

bool AbstractFileManagerPlugin::Private::rename(ProjectBaseItem* item, const KUrl& destination)
{
    if ( !q->isValid(destination, true, item->project()) ) {
        int cancel = KMessageBox::warningContinueCancel( qApp->activeWindow(),
            i18n("You tried to rename '%1' to '%2', but the latter is filtered and will be hidden.\n"
                 "Do you want to continue?", item->text(), destination.fileName()),
            QString(), KStandardGuiItem::cont(), KStandardGuiItem::cancel(), "GenericManagerRenameToFiltered"
        );
        if ( cancel == KMessageBox::Cancel ) {
            return false;
        }
    }
    foreach ( ProjectFolderItem* parent, item->project()->foldersForUrl(destination.upUrl()) ) {
        if ( parent->type() == ProjectBaseItem::Folder ) {
            stopWatcher(parent);
            const KUrl source = item->url();
            bool success = renameUrl( item->project(), source, destination );
            if ( success ) {
                item->setUrl( destination );
                item->parent()->takeRow( item->row() );
                parent->appendRow( item );
                if (item->file()) {
                    emit q->fileRenamed(source, item->file());
                } else {
                    Q_ASSERT(item->folder());
                    emit q->folderRenamed(source, item->folder());
                }
            }
            continueWatcher(parent);
            return success;
        }
    }
    return false;
}

void AbstractFileManagerPlugin::Private::stopWatcher(ProjectFolderItem* folder)
{
    if ( !folder->url().isLocalFile() ) {
        return;
    }
    Q_ASSERT(m_watchers.contains(folder->project()));
    m_watchers[folder->project()]->stopDirScan(folder->url().toLocalFile());
}

void AbstractFileManagerPlugin::Private::continueWatcher(ProjectFolderItem* folder)
{
    if ( !folder->url().isLocalFile() ) {
        return;
    }
    Q_ASSERT(m_watchers.contains(folder->project()));
    m_watchers[folder->project()]->restartDirScan(folder->url().toLocalFile());
}

//END Private

//BEGIN Plugin

AbstractFileManagerPlugin::AbstractFileManagerPlugin( const KComponentData& instance,
                                                      QObject *parent,
                                                      const QVariantList & /*args*/ )
    : IPlugin( instance, parent ),
      IProjectFileManager(),
      d(new Private(this))
{
    KDEV_USE_EXTENSION_INTERFACE( IProjectFileManager )

    connect(core()->projectController(), SIGNAL(projectClosing(KDevelop::IProject*)),
            this, SLOT(projectClosing(KDevelop::IProject*)));
}

AbstractFileManagerPlugin::~AbstractFileManagerPlugin()
{
}

IProjectFileManager::Features AbstractFileManagerPlugin::features() const
{
    return Features( Folders | Files );
}

QList<ProjectFolderItem*> AbstractFileManagerPlugin::parse( ProjectFolderItem *item )
{
    // we are async, can't return anything here
    kDebug() << "note: parse will always return an empty list";
    Q_UNUSED(item);
    return QList<ProjectFolderItem*>();
}

ProjectFolderItem *AbstractFileManagerPlugin::import( IProject *project )
{
    ProjectFolderItem *projectRoot = createFolderItem( project, project->folder(), 0 );
    emit folderAdded( projectRoot );
    kDebug() << "imported new project" << project->name() << "at" << projectRoot->url();

    ///TODO: check if this works for remote files when something gets changed through another KDE app
    if ( project->folder().isLocalFile() ) {
        d->m_watchers[project] = new KDirWatch( project );

        connect(d->m_watchers[project], SIGNAL(created(QString)),
                this, SLOT(created(QString)));
        connect(d->m_watchers[project], SIGNAL(deleted(QString)),
                this, SLOT(deleted(QString)));

        d->m_watchers[project]->addDir(project->folder().toLocalFile(), KDirWatch::WatchSubDirs | KDirWatch:: WatchFiles );
    }

    return projectRoot;
}

KJob* AbstractFileManagerPlugin::createImportJob(ProjectFolderItem* item)
{
    return d->eventuallyReadFolder(item);
}

bool AbstractFileManagerPlugin::reload( ProjectFolderItem* item )
{
    kDebug() << "reloading item" << item->url();
    d->eventuallyReadFolder( item->folder(), true );
    return true;
}

ProjectFolderItem* AbstractFileManagerPlugin::addFolder( const KUrl& url,
        ProjectFolderItem * parent )
{
    kDebug() << "adding folder" << url << "to" << parent->url();
    ProjectFolderItem* created = 0;
    d->stopWatcher(parent);
    if ( createFolder(url) ) {
        created = createFolderItem( parent->project(), url, parent );
        emit folderAdded(created);
    }
    d->continueWatcher(parent);
    return created;
}


ProjectFileItem* AbstractFileManagerPlugin::addFile( const KUrl& url,
        ProjectFolderItem * parent )
{
    kDebug() << "adding file" << url << "to" << parent->url();
    ProjectFileItem* created = 0;
    d->stopWatcher(parent);
    if ( createFile(url) ) {
        created = createFileItem( parent->project(), url, parent );
        emit fileAdded(created);
    }
    d->continueWatcher(parent);
    return created;
}

bool AbstractFileManagerPlugin::renameFolder( ProjectFolderItem * folder, const KUrl& url )
{
    kDebug() << "trying to rename a folder:" << folder->url() << url;
    return d->rename(folder, url);
}

bool AbstractFileManagerPlugin::renameFile( ProjectFileItem * file, const KUrl& url )
{
    kDebug() << "trying to rename a file:" << file->url() << url;
    return d->rename(file, url);
}

bool AbstractFileManagerPlugin::removeFilesAndFolders(QList<ProjectBaseItem*> items)
{
    bool success = true;
    foreach(ProjectBaseItem* item, items)
    {
        Q_ASSERT(item->folder() || item->file());

        ProjectFolderItem* parent = getParentFolder(item);
        d->stopWatcher(parent);

        success &= removeUrl(parent->project(), item->url(), true);
        if ( success ) {
            if (item->file()) {
                emit fileRemoved(item->file());
            } else {
                Q_ASSERT(item->folder());
                emit folderRemoved(item->folder());
            }
            item->parent()->removeRow( item->row() );
        }

        d->continueWatcher(parent);
        if ( !success )
            break;
    }
    return success;
}

bool AbstractFileManagerPlugin::isValid( const KUrl& /*url*/, const bool /*isFolder*/,
                                         IProject* /*project*/ ) const
{
    return true;
}

ProjectFileItem* AbstractFileManagerPlugin::createFileItem( IProject* project, const KUrl& url,
                                                            ProjectBaseItem* parent )
{
    return new ProjectFileItem( project, url, parent );
}

ProjectFolderItem* AbstractFileManagerPlugin::createFolderItem( IProject* project, const KUrl& url,
                                                                ProjectBaseItem* parent )
{
    return new ProjectFolderItem( project, url, parent );
}

//END Plugin

#include "abstractfilemanagerplugin.moc"
