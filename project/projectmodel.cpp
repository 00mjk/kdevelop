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

#include <QApplication>
#include <QPalette>
#include <QBrush>
#include <QColor>
#include <QFileInfo>
#include <kdebug.h>

#include <interfaces/iproject.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/icore.h>
#include "interfaces/iprojectfilemanager.h"
#include <language/duchain/indexedstring.h>
#include <KLocalizedString>
#include <KMessageBox>
#include <kio/udsentry.h>
#include <kio/netaccess.h>

namespace KDevelop
{

QStringList removeProjectBasePath( const QStringList& fullpath, KDevelop::ProjectBaseItem* item )
{
    QStringList result = fullpath;
    if( item )
    {
        KDevelop::ProjectModel* model = KDevelop::ICore::self()->projectController()->projectModel();
        QStringList basePath = model->pathFromIndex( model->indexFromItem( item ) );
        if( basePath.count() >= fullpath.count() )
        {
            return QStringList();
        }
        for( int i = 0; i < basePath.count(); i++ )
        {
            result.takeFirst();
        }
    }
    return result;
}

QStringList joinProjectBasePath( const QStringList& partialpath, KDevelop::ProjectBaseItem* item )
{
    QStringList basePath;
    if( item )
    {
        KDevelop::ProjectModel* model = KDevelop::ICore::self()->projectController()->projectModel();
        basePath = model->pathFromIndex( model->indexFromItem( item ) );
    }
    return basePath + partialpath;
}

class ProjectModelPrivate
{
public:
    ProjectModelPrivate( ProjectModel* model ): model( model )
    {
    }
    ProjectBaseItem* rootItem;
    ProjectModel* model;
    ProjectBaseItem* itemFromIndex( const QModelIndex& idx ) {
        if( !idx.isValid() ) {
            return rootItem;
        }
        if( idx.model() != model ) {
            return 0;
        }
        return model->itemFromIndex( idx );
    }

};

class ProjectBaseItemPrivate
{
public:
    ProjectBaseItemPrivate() : project(0), parent(0), row(-1), model(0) {}
    IProject* project;
    ProjectBaseItem* parent;
    int row;
    QList<ProjectBaseItem*> childs;
    QString text;
    ProjectBaseItem::ProjectItemType type;
    Qt::ItemFlags flags;
    ProjectModel* model;
    KUrl m_url;
};


ProjectBaseItem::ProjectBaseItem( IProject* project, const QString &name, ProjectBaseItem *parent )
        : d_ptr(new ProjectBaseItemPrivate)
{
    Q_D(ProjectBaseItem);
    d->project = project;
    d->text = name;
    if( parent ) {
        parent->appendRow( this );
    }
}

ProjectBaseItem::~ProjectBaseItem()
{
    Q_D(ProjectBaseItem);
    if( parent() ) {
        parent()->takeRow( d->row );
    } else if( model() ) {
        model()->takeRow( d->row );
    }
    delete d;
}

ProjectBaseItem* ProjectBaseItem::child( int row ) const
{
    Q_D(const ProjectBaseItem);
    if( row < 0 || row >= d->childs.length() ) {
        return 0;
    }
    return d->childs.at( row );
}

ProjectBaseItem* ProjectBaseItem::takeRow(int row)
{
    Q_D(ProjectBaseItem);
    Q_ASSERT(row >= 0 && row < d->childs.size());
    
    if( model() ) {
        model()->beginRemoveRows( index(), row, row );
    }
    ProjectBaseItem* olditem = d->childs.takeAt( row );
    olditem->d_func()->parent = 0;
    olditem->d_func()->row = -1;
    olditem->d_func()->model = 0;;
    
    if( model() ) {
        model()->endRemoveRows();
    }
    return olditem;
}

void ProjectBaseItem::removeRow( int row )
{
    delete takeRow(row);
}

QModelIndex ProjectBaseItem::index() const
{
    if( model() ) {
        return model()->indexFromItem( this );
    }
    return QModelIndex();
}

int ProjectBaseItem::rowCount() const
{
    Q_D(const ProjectBaseItem);
    return d->childs.count();
}

int ProjectBaseItem::type() const
{
    return ProjectBaseItem::BaseItem;
}

ProjectModel* ProjectBaseItem::model() const
{
    Q_D(const ProjectBaseItem);
    return d->model;
}

ProjectBaseItem* ProjectBaseItem::parent() const
{
    Q_D(const ProjectBaseItem);
    if( model() && model()->d->rootItem == d->parent ) {
        return 0;
    }
    return d->parent;
}

int ProjectBaseItem::row() const
{
    Q_D(const ProjectBaseItem);
    return d->row;
}

QString ProjectBaseItem::text() const
{
    Q_D(const ProjectBaseItem);
    return d->text;
}

void ProjectBaseItem::setModel( ProjectModel* model )
{
    Q_D(ProjectBaseItem);
    d->model = model;
    foreach( ProjectBaseItem* item, d->childs ) {
        item->setModel( model );
    }
}

void ProjectBaseItem::setRow( int row )
{
    Q_D(ProjectBaseItem);
    d->row = row;
}

void ProjectBaseItem::setText( const QString& text )
{
    Q_D(ProjectBaseItem);
    d->text = text;
    if( model() ) {
        model()->dataChanged( index(), index() );
    }
}

ProjectBaseItem::RenameStatus ProjectBaseItem::rename(const QString& newname)
{
    setText( newname );
    return RenameOk;
}

KDevelop::ProjectBaseItem::ProjectItemType baseType( int type )
{
    if( type == KDevelop::ProjectBaseItem::Folder || type == KDevelop::ProjectBaseItem::BuildFolder )
        return KDevelop::ProjectBaseItem::Folder;
    if( type == KDevelop::ProjectBaseItem::Target || type == KDevelop::ProjectBaseItem::ExecutableTarget
        || type == KDevelop::ProjectBaseItem::LibraryTarget)
        return KDevelop::ProjectBaseItem::Target;

    return static_cast<KDevelop::ProjectBaseItem::ProjectItemType>( type );
}

bool ProjectBaseItem::lessThan( const KDevelop::ProjectBaseItem* item ) const
{
    if(item->type() >= KDevelop::ProjectBaseItem::CustomProjectItemType ) {
        // For custom types we want to make sure that if they override lessThan, then we
        // prefer their lessThan implementation
        return !item->lessThan( this );
    }
    KDevelop::ProjectBaseItem::ProjectItemType leftType=baseType(type()), rightType=baseType(item->type());
    if(leftType==rightType)
    {
        if(leftType==KDevelop::ProjectBaseItem::File)
        {
            return file()->fileName().compare(item->file()->fileName(), Qt::CaseInsensitive) < 0;
        }
        return this->text()<item->text();
    }
    else
    {
        return leftType<rightType;
    }

    return false;
}

IProject* ProjectBaseItem::project() const
{
    Q_D(const ProjectBaseItem);
    return d->project;
}

void ProjectBaseItem::appendRow( ProjectBaseItem* item )
{
    Q_D(ProjectBaseItem);
    if( !item ) {
        return;
    }
    if( item->parent() ) {
        // Proper way is to first removeRow() on the original parent, then appendRow on this one
        kWarning() << "Ignoring double insertion of item" << item;
        return;
    }
    if( model() ) {
        model()->beginInsertRows( index(), d->childs.count(), d->childs.count() );
    }
    d->childs.append( item );
    item->setRow( d->childs.count() - 1 );
    item->d_func()->parent = this;
    item->setModel( model() );
    if( model() ) {
        model()->endInsertRows();
    }
}

KUrl ProjectBaseItem::url( ) const
{
    Q_D(const ProjectBaseItem);
    return d->m_url;
}

void ProjectBaseItem::setUrl( const KUrl& url )
{
    Q_D(ProjectBaseItem);
    d->m_url = url;
    setText( d->m_url.fileName() );
}

QString ProjectBaseItem::iconName() const
{
    return "";
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
        ProjectBaseItem* item = child( i );
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
        ProjectBaseItem* item = child( i );
        Q_ASSERT(item);
        if ( item && ( item->type() == Target || item->type() == LibraryTarget ||
             item->type() == ExecutableTarget ) )
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
        ProjectBaseItem* item = child( i );
        Q_ASSERT(item);
        if ( item && item->type() == File )
        {
            ProjectFileItem *kdevitem = dynamic_cast<ProjectFileItem*>( item );
            if ( kdevitem )
                lst.append( kdevitem );
        }

    }
    return lst;
}

void ProjectModel::resetModel()
{
    reset();
}

void ProjectModel::clear()
{
    for( int i = 0; i < d->rootItem->rowCount(); i++ ) {
        d->rootItem->removeRow( i );
    }
}


ProjectFolderItem::ProjectFolderItem( IProject* project, const KUrl & dir, ProjectBaseItem * parent )
        : ProjectBaseItem( project, dir.fileName(), parent )
{
    setUrl( dir );
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

QString ProjectFolderItem::folderName() const
{
    return url().fileName();
}


ProjectBaseItem::RenameStatus ProjectFolderItem::rename(const QString& newname)
{
    //TODO: Same as ProjectFileItem, so should be shared somehow
    KUrl dest = url().upUrl();
    dest.addPath(newname);
    if( !newname.contains('/') )
    {
        KIO::UDSEntry entry;
        //There exists a file with that name?
        if( !KIO::NetAccess::stat(dest, entry, 0) )
        {
            if( !project() || project()->projectFileManager()->renameFolder(this, dest) )
            {
                setUrl( dest );
                return ProjectBaseItem::RenameOk;
            } else
            {
                return ProjectBaseItem::ProjectManagerRenameFailed;
            }
        } else
        {
            return ProjectBaseItem::ExistingItemSameName;
        }
    } else
    {
        return ProjectBaseItem::InvalidNewName;
    }
}

bool ProjectFolderItem::hasFileOrFolder(const QString& name) const
{
    for ( int i = 0; i < rowCount(); ++i )
    {
        ProjectBaseItem* item = child( i );
        if ( ProjectFileItem* file = dynamic_cast<ProjectFileItem*>(item))
            if (file->fileName() == name)
                return true;

        if ( ProjectFolderItem* folder = dynamic_cast<ProjectFolderItem*>(item))
            if (folder->folderName() == name)
                return true;
    }
    return false;
}

ProjectBuildFolderItem::ProjectBuildFolderItem( IProject* project, const KUrl &dir, ProjectBaseItem *parent)
    : ProjectFolderItem( project, dir, parent )
{
}

QString ProjectFolderItem::iconName() const
{
    return "folder";
}

int ProjectBuildFolderItem::type() const
{
    return ProjectBaseItem::BuildFolder;
}

QString ProjectBuildFolderItem::iconName() const
{
    return "folder-development";
}

ProjectFileItem::ProjectFileItem( IProject* project, const KUrl & file, ProjectBaseItem * parent )
        : ProjectBaseItem( project, file.fileName(), parent )
{
    setUrl( file );
    // Need to this manually here as setUrl() is virtual and hence the above
    // only calls the version in ProjectBaseItem and not ours
    if( project ) {
        project->addToFileSet( KDevelop::IndexedString(file) );
    }
}

ProjectFileItem::~ProjectFileItem()
{
    if( project() ) {
        project()->removeFromFileSet(KDevelop::IndexedString(url()));
    }
}

QString ProjectFileItem::iconName() const
{
    return KMimeType::findByUrl(url(), 0, false, true)->iconName(url());
}

ProjectBaseItem::RenameStatus ProjectFileItem::rename(const QString& newname)
{
    KUrl dest = url().upUrl();
    dest.addPath(newname);
    if( !newname.contains('/') )
    {
        KIO::UDSEntry entry;
        //There exists a file with that name?
        if( !KIO::NetAccess::stat(dest, entry, 0) )
        {
            if( !project() || project()->projectFileManager()->renameFile(this, dest) )
            {
                setUrl( dest );
                return ProjectBaseItem::RenameOk;
            } else
            {
                return ProjectBaseItem::ProjectManagerRenameFailed;
            }
        } else
        {
            return ProjectBaseItem::ExistingItemSameName;
        }
    } else
    {
        return ProjectBaseItem::InvalidNewName;
    }
}

QString ProjectFileItem::fileName() const
{
    return url().fileName();
}

void ProjectFileItem::setUrl( const KUrl& url )
{
    if( project() ) {
        if(!this->url().isEmpty())
            project()->removeFromFileSet( KDevelop::IndexedString(this->url()) );
        project()->addToFileSet( KDevelop::IndexedString(url) );
    }

    ProjectBaseItem::setUrl( url );
}

int ProjectFileItem::type() const
{
    return ProjectBaseItem::File;
}

ProjectFileItem *ProjectFileItem::file() const
{
    return const_cast<ProjectFileItem*>( this );
}

ProjectTargetItem::ProjectTargetItem( IProject* project, const QString &name, ProjectBaseItem *parent )
    : ProjectBaseItem( project, name, parent )
{
}

QString ProjectTargetItem::iconName() const
{
    return "system-run";
}

int ProjectTargetItem::type() const
{
    return ProjectBaseItem::Target;
}

ProjectTargetItem *ProjectTargetItem::target() const
{
    return const_cast<ProjectTargetItem*>( this );
}

ProjectExecutableTargetItem::ProjectExecutableTargetItem( IProject* project, const QString &name, ProjectBaseItem *parent )
    : ProjectTargetItem(project, name, parent)
{
}

ProjectExecutableTargetItem *ProjectExecutableTargetItem::executable() const
{
    return const_cast<ProjectExecutableTargetItem*>( this );
}

int ProjectExecutableTargetItem::type() const
{
    return ProjectBaseItem::ExecutableTarget;
}

ProjectLibraryTargetItem::ProjectLibraryTargetItem( IProject* project, const QString &name, ProjectBaseItem *parent )
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
        QModelIndexList l = match(current, Qt::DisplayRole, currentName, -1, Qt::MatchExactly);
        foreach(const QModelIndex& idx, l) {
            //If this is not the last item, only match folders, as there may be targets and folders with the same name
            if(a == tofetch.size()-1 || itemFromIndex(idx)->folder()) {
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
        QString t = data(idx, Qt::DisplayRole).toString();
        list.prepend(t);
        QModelIndex parent = idx.parent();
        idx = parent.sibling(parent.row(), index.column());
    } while (idx.isValid());

    return list;
}

int ProjectModel::columnCount( const QModelIndex& ) const
{
    return 1;
}

int ProjectModel::rowCount( const QModelIndex& parent ) const
{
    ProjectBaseItem* item = d->itemFromIndex( parent );
    return item ? item->rowCount() : 0;
}

QModelIndex ProjectModel::parent( const QModelIndex& child ) const
{
    if( child.isValid() ) {
        ProjectBaseItem* item = static_cast<ProjectBaseItem*>( child.internalPointer() );
        return indexFromItem( item );
    }
    return QModelIndex();
}

QModelIndex ProjectModel::indexFromItem( const ProjectBaseItem* item ) const
{
    if( item && item->d_func()->parent ) {
        return createIndex( item->row(), 0, item->d_func()->parent );
    }
    return QModelIndex();
}

ProjectBaseItem* ProjectModel::itemFromIndex( const QModelIndex& index ) const
{
    if( index.row() >= 0 && index.column() == 0  && index.model() == this ) {
        ProjectBaseItem* parent = static_cast<ProjectBaseItem*>( index.internalPointer() );
        if( parent ) {
            return parent->child( index.row() );
        }
    }
    return 0;
}


QVariant ProjectModel::data( const QModelIndex& index, int role ) const
{
    if( ( role == Qt::DisplayRole || role == Qt::ToolTipRole || role == Qt::DecorationRole ) && index.isValid() ) {
        ProjectBaseItem* item = itemFromIndex( index );
        if( item ) {
            if( role == Qt::DecorationRole ) {
                return item->iconName();
            }
            return item->text();
        }
    }
    return QVariant();
}

ProjectModel::ProjectModel( QObject *parent )
        : QAbstractItemModel( parent ), d( new ProjectModelPrivate( this ) )
{
    d->rootItem = new ProjectBaseItem( 0, "", 0 );
    d->rootItem->setModel( this );
}

ProjectModel::~ProjectModel()
{
}


ProjectVisitor::ProjectVisitor()
{
}

QModelIndex ProjectModel::index( int row, int column, const QModelIndex& parent ) const
{
    if( hasIndex( row, column, parent ) ) {
        ProjectBaseItem* parentItem = d->itemFromIndex( parent );
        if( parentItem && row >= 0 && row < parentItem->rowCount() && column == 0 ) {
            return createIndex( row, column, parentItem );
        }
    }
    return QModelIndex();
}

void ProjectModel::appendRow( ProjectBaseItem* item )
{
    d->rootItem->appendRow( item );
}

void ProjectModel::removeRow( int row )
{
    d->rootItem->removeRow( row );
}

ProjectBaseItem* ProjectModel::takeRow( int row )
{
    return d->rootItem->takeRow( row );
}

bool ProjectModel::hasChildren(const QModelIndex& parent) const
{
    bool b = QAbstractItemModel::hasChildren(parent);
    return b;
}

bool ProjectModel::insertColumns(int, int, const QModelIndex&)
{
    // Not supported
    return false;
}

bool ProjectModel::insertRows(int, int, const QModelIndex&)
{
    // Not supported
    return false;
}

bool ProjectModel::setData(const QModelIndex&, const QVariant&, int)
{
    // Not supported
    return false;
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
