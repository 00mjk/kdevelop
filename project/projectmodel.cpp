/* This file is part of KDevelop
    Copyright 2005 Roberto Raggi <roberto@kdevelop.org>
    Copyright 2007 Andreas Pakulat <apaku@gmx.de>
    Copyright 2007 Aleix Pol <aleixpol@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "projectmodel.h"

#include <kmimetype.h>
#include <kiconloader.h>
#include <kicon.h>
#include <kio/global.h>

#include <QApplication>
#include <QPalette>
#include <QBrush>
#include <QColor>
#include <QFileInfo>
#include <kdebug.h>

#include <interfaces/iproject.h>

namespace KDevelop
{

class ProjectBaseItemPrivate
{
public:
    ProjectBaseItemPrivate() : project(0) {}
    IProject* project;
};

class ProjectFolderItemPrivate : public ProjectBaseItemPrivate
{
public:
    KUrl m_url;
    QString m_folderName;
    bool m_isProjectRoot;
};

class ProjectBuildFolderItemPrivate : public ProjectFolderItemPrivate
{
public:
};

class ProjectFileItemPrivate : public ProjectBaseItemPrivate
{
public:
    KUrl m_url;
    QString m_fileName;
};

class ProjectTargetItemPrivate : public ProjectBaseItemPrivate
{
};

class WorkspaceItemPrivate
{
public:
    QString name;
    KSharedConfig::Ptr metadataConfig;
    QString metadataDir;
};

class ProjectModelPrivate
{
};

ProjectBaseItem::ProjectBaseItem( IProject* project, const QString &name, QStandardItem *parent )
        : QStandardItem( name ), d_ptr(new ProjectBaseItemPrivate)
{
    Q_D(ProjectBaseItem);
    d->project = project;
    setParent( parent );
    setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
}

ProjectBaseItem::ProjectBaseItem( ProjectBaseItemPrivate& dd)
    : d_ptr(&dd)
{
    setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
}

ProjectBaseItem::~ProjectBaseItem()
{
    Q_D(ProjectBaseItem);
    delete d;
}

IProject* ProjectBaseItem::project() const
{
    Q_D(const ProjectBaseItem);
    return d->project;
}

void ProjectBaseItem::setParent( QStandardItem* parent )
{
    if( parent )
        parent->setChild( parent->rowCount(), this );
}

void ProjectBaseItem::setIcon()
{
}

void ProjectBaseItem::add( ProjectBaseItem* item )
{
    appendRow( item );
}

ProjectFolderItem *ProjectBaseItem::folder() const
{
    return 0;
}

ProjectTargetItem *ProjectBaseItem::target() const
{
    return 0;
}

ProjectExecutableTargetItem *ProjectBaseItem::executable() const
{
    return 0;
}

ProjectFileItem *ProjectBaseItem::file() const
{
    return 0;
}

QList<ProjectFolderItem*> ProjectBaseItem::folderList() const
{
    QList<ProjectFolderItem*> lst;
    for ( int i = 0; i < rowCount(); ++i )
    {
        QStandardItem* item = child( i );
        if ( item->type() == Folder || item->type() == BuildFolder )
        {
            ProjectFolderItem *kdevitem = dynamic_cast<ProjectFolderItem*>( item );
            if ( kdevitem )
                lst.append( kdevitem );
        }
    }

    return lst;
}

QList<ProjectTargetItem*> ProjectBaseItem::targetList() const
{
    QList<ProjectTargetItem*> lst;
    for ( int i = 0; i < rowCount(); ++i )
    {
        QStandardItem* item = child( i );
        if ( item->type() == Target || item->type() == LibraryTarget ||
             item->type() == ExecutableTarget)
        {
            ProjectTargetItem *kdevitem = dynamic_cast<ProjectTargetItem*>( item );
            if ( kdevitem )
                lst.append( kdevitem );
        }
    }

    return lst;
}

QList<ProjectFileItem*> ProjectBaseItem::fileList() const
{
    QList<ProjectFileItem*> lst;
    for ( int i = 0; i < rowCount(); ++i )
    {
        QStandardItem* item = child( i );
        if ( item->type() == File )
        {
            ProjectFileItem *kdevitem = dynamic_cast<ProjectFileItem*>( item );
            if ( kdevitem )
                lst.append( kdevitem );
        }

    }
    return lst;
}

ProjectModel::ProjectModel( QObject *parent )
        : QStandardItemModel( parent ), d(0)
{
}

ProjectModel::~ProjectModel()
{}


ProjectBaseItem *ProjectModel::item( const QModelIndex &index ) const
{
    return dynamic_cast<ProjectBaseItem*>( itemFromIndex( index ) );
}

void ProjectModel::resetModel()
{
    reset();
}

void ProjectModel::fetchMore( const QModelIndex &parent )
{
    QStandardItem *parentItem = itemFromIndex( parent );
    if( !parentItem )
        return;
    int rowcount = parentItem->rowCount();
    for( int i=0; i<rowcount; i++ )
    {
        ProjectBaseItem *childItem = dynamic_cast<ProjectBaseItem*>(parentItem->child(i));
        if( childItem && childItem->icon().isNull() )
            childItem->setIcon();
    }
}

bool ProjectModel::canFetchMore( const QModelIndex & parent ) const
{
    QStandardItem *parentItem = itemFromIndex( parent );
    if( !parentItem )
        return false;
    return true;
}


ProjectFolderItem::ProjectFolderItem( IProject* project, const KUrl & dir, QStandardItem * parent )
        : ProjectBaseItem( *new ProjectFolderItemPrivate )
{
    Q_D(ProjectFolderItem);
    d->project = project;
    setUrl(dir);
    setParent(parent);
}

ProjectFolderItem::ProjectFolderItem( ProjectFolderItemPrivate& dd)
    : ProjectBaseItem( dd )
{
}

ProjectFolderItem::~ProjectFolderItem()
{
}

ProjectFolderItem *ProjectFolderItem::folder() const
{
    return const_cast<ProjectFolderItem*>(this);
}

int ProjectFolderItem::type() const
{
    return ProjectBaseItem::Folder;
}

const KUrl& ProjectFolderItem::url( ) const
{
    Q_D(const ProjectFolderItem);
    return d->m_url;
}

const QString& ProjectFolderItem::folderName() const
{
    Q_D(const ProjectFolderItem);
    return d->m_folderName;
}

void ProjectFolderItem::setUrl( const KUrl& url )
{
    Q_D(ProjectFolderItem);
    d->m_url = url;
    d->m_folderName = d->m_url.fileName();
    setText( d->m_folderName );
}

void ProjectFolderItem::setIcon()
{
    QStandardItem::setIcon( KIO::pixmapForUrl( url(), 0, KIconLoader::Small ) );
}

bool ProjectFolderItem::hasFileOrFolder(const QString& name) const
{
    for ( int i = 0; i < rowCount(); ++i )
    {
        QStandardItem* item = child( i );
        if ( ProjectFileItem* file = dynamic_cast<ProjectFileItem*>(item))
            if (file->fileName() == name)
                return true;

        if ( ProjectFolderItem* folder = dynamic_cast<ProjectFolderItem*>(item))
            if (folder->folderName() == name)
                return true;
    }
    return false;
}

ProjectBuildFolderItem::ProjectBuildFolderItem( ProjectBuildFolderItemPrivate& dd )
    : ProjectFolderItem( dd )
{
}

ProjectBuildFolderItem::ProjectBuildFolderItem( IProject* project, const KUrl &dir, QStandardItem *parent)
    : ProjectFolderItem( *new ProjectBuildFolderItemPrivate )
{
    Q_D(ProjectBuildFolderItem);
    d->project = project;
    setUrl( dir );
    setParent( parent );
}

int ProjectBuildFolderItem::type() const
{
    return ProjectBaseItem::BuildFolder;
}

void ProjectBuildFolderItem::setIcon()
{
    QStandardItem::setIcon( KIcon("folder-development") );
}

void ProjectFolderItem::setProjectRoot(bool isRoot)
{
	Q_D(ProjectFolderItem);
	d->m_isProjectRoot=isRoot;
	setText(project()->name());
}

bool ProjectFolderItem::isProjectRoot() const
{
	Q_D(const ProjectFolderItem);
	return d->m_isProjectRoot;
}

ProjectFileItem::ProjectFileItem( ProjectFileItemPrivate& dd)
    : ProjectBaseItem(dd)
{
    setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
}

ProjectFileItem::ProjectFileItem( IProject* project, const KUrl & file, QStandardItem * parent )
        : ProjectBaseItem( *new ProjectFileItemPrivate )
{
    Q_D(ProjectFileItem);
    d->project = project;
    setUrl( file );
    setParent( parent );
    setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
}

const KUrl & ProjectFileItem::url( ) const
{
    Q_D(const ProjectFileItem);
    return d->m_url;
}

const QString& ProjectFileItem::fileName() const
{
    Q_D(const ProjectFileItem);
    return d->m_fileName;
}

void ProjectFileItem::setUrl( const KUrl& url )
{
    Q_D(ProjectFileItem);
    d->m_url = url;
    d->m_fileName = d->m_url.fileName();
    setText( d->m_fileName );
}

int ProjectFileItem::type() const
{
    return ProjectBaseItem::File;
}

ProjectFileItem *ProjectFileItem::file() const
{
    return const_cast<ProjectFileItem*>( this );
}

void ProjectFileItem::setIcon()
{
    QStandardItem::setIcon( KIO::pixmapForUrl( url(), 0, KIconLoader::Small ) );
}

ProjectTargetItem::ProjectTargetItem( ProjectTargetItemPrivate& dd)
    : ProjectBaseItem( dd )
{
}

ProjectTargetItem::ProjectTargetItem( IProject* project, const QString &name, QStandardItem *parent )
                : ProjectBaseItem( *new ProjectTargetItemPrivate )
{
    Q_D(ProjectTargetItem);
    d->project = project;
    setText( name );
    setParent( parent );
}

int ProjectTargetItem::type() const
{
    return ProjectBaseItem::Target;
}

ProjectTargetItem *ProjectTargetItem::target() const
{
    return const_cast<ProjectTargetItem*>( this );
}

void ProjectTargetItem::setIcon()
{
    QStandardItem::setIcon( KIcon("system-run") );
}

ProjectExecutableTargetItem::ProjectExecutableTargetItem( IProject* project, const QString &name, QStandardItem *parent )
    : ProjectTargetItem(project, name, parent)
{}

ProjectExecutableTargetItem *ProjectExecutableTargetItem::executable() const
{
    return const_cast<ProjectExecutableTargetItem*>( this );
}

int ProjectExecutableTargetItem::type() const
{
    return ProjectBaseItem::ExecutableTarget;
}

ProjectLibraryTargetItem::ProjectLibraryTargetItem( IProject* project, const QString &name, QStandardItem *parent )
    : ProjectTargetItem(project, name, parent)
{}

int ProjectLibraryTargetItem::type() const
{
    return ProjectBaseItem::LibraryTarget;
}

QModelIndex ProjectModel::pathToIndex(const QStringList& tofetch_) const
{
    if(tofetch_.isEmpty())
        return QModelIndex();
    QStringList tofetch(tofetch_);
    if(tofetch.last().isEmpty())
        tofetch.takeLast();

    QModelIndex current=index(0,0, QModelIndex());

    QModelIndex ret;
    for(int a = 0; a < tofetch.size(); ++a)
    {
        const QString& currentName = tofetch[a];
        
        bool matched = false;
        QModelIndexList l = match(current, Qt::EditRole, currentName, -1, Qt::MatchExactly);
        foreach(QModelIndex idx, l) {
            //If this is not the last item, only match folders, as there may be targets and folders with the same name
            if(a == tofetch.size()-1 || item(idx)->folder()) {
                ret = idx;
                current = index(0,0, ret);
                matched = true;
                break;
            }
        }
        if(!matched) {
            ret = QModelIndex();
            break;
        }
    }
    Q_ASSERT(!ret.isValid() || data(ret).toString()==tofetch.last());
    return ret;
}

QStringList ProjectModel::pathFromIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return QStringList();

    QModelIndex idx = index;
    QStringList list;
    do {
        QString t = data(idx, Qt::EditRole).toString();
        list.prepend(t);
        QModelIndex parent = idx.parent();
        idx = parent.sibling(parent.row(), index.column());
    } while (idx.isValid());

    return list;
}


ProjectVisitor::ProjectVisitor() 
{
}


void ProjectVisitor::visit ( IProject* prj )
{
    visit( prj->projectItem() );
}


void ProjectVisitor::visit ( ProjectBuildFolderItem* folder )
{
    foreach( ProjectFileItem* item, folder->fileList() )
    {
        visit( item );
    }
    foreach( ProjectTargetItem* item, folder->targetList() )
    {
        if( item->type() == ProjectBaseItem::LibraryTarget )
        {
            visit( dynamic_cast<ProjectLibraryTargetItem*>( item ) );
        } else if( item->type() == ProjectBaseItem::ExecutableTarget )
        {
            visit( dynamic_cast<ProjectExecutableTargetItem*>( item ) );
        }
    }
    foreach( ProjectFolderItem* item, folder->folderList() )
    {
        if( item->type() == ProjectBaseItem::BuildFolder )
        {
            visit( dynamic_cast<ProjectBuildFolderItem*>( item ) );
        } else if( item->type() == ProjectBaseItem::Folder )
        {
            visit( dynamic_cast<ProjectFolderItem*>( item ) );
        }
    }
}


void ProjectVisitor::visit ( ProjectExecutableTargetItem* exec )
{
    foreach( ProjectFileItem* item, exec->fileList() )
    {
        visit( item );
    }
}


void ProjectVisitor::visit ( ProjectFolderItem* folder )
{
    foreach( ProjectFileItem* item, folder->fileList() )
    {
        visit( item );
    }
    foreach( ProjectTargetItem* item, folder->targetList() )
    {
        if( item->type() == ProjectBaseItem::LibraryTarget )
        {
            visit( dynamic_cast<ProjectLibraryTargetItem*>( item ) );
        } else if( item->type() == ProjectBaseItem::ExecutableTarget )
        {
            visit( dynamic_cast<ProjectExecutableTargetItem*>( item ) );
        }
    }
    foreach( ProjectFolderItem* item, folder->folderList() )
    {
        if( item->type() == ProjectBaseItem::BuildFolder )
        {
            visit( dynamic_cast<ProjectBuildFolderItem*>( item ) );
        } else if( item->type() == ProjectBaseItem::Folder )
        {
            visit( dynamic_cast<ProjectFolderItem*>( item ) );
        }
    }
}


void ProjectVisitor::visit ( ProjectFileItem* )
{
}

void ProjectVisitor::visit ( ProjectLibraryTargetItem* lib )
{
    foreach( ProjectFileItem* item, lib->fileList() )
    {
        visit( item );
    }
}

ProjectVisitor::~ProjectVisitor()
{
}


}
#include "projectmodel.moc"
