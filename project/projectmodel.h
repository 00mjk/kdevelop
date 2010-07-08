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

#ifndef KDEVPROJECTMODEL_H
#define KDEVPROJECTMODEL_H

#include <QtCore/QAbstractItemModel>
#include "projectexport.h"
#include <KDE/KUrl>
#include <KDE/KSharedConfig>

template<typename T1,typename T2> struct QPair;
template<typename T> class QList;

namespace KDevelop
{

class IProject;
class ProjectFolderItem;
class ProjectBuildFolderItem;
class ProjectFileItem;
class ProjectTargetItem;
class ProjectExecutableTargetItem;
class ProjectLibraryTargetItem;
class ProjectModel;

class KDEVPLATFORMPROJECT_EXPORT ProjectVisitor
{
public:
    ProjectVisitor();
    virtual ~ProjectVisitor();
    virtual void visit( IProject* );
    virtual void visit( ProjectBuildFolderItem* );
    virtual void visit( ProjectExecutableTargetItem* );
    virtual void visit( ProjectFolderItem* );
    virtual void visit( ProjectFileItem* );
    virtual void visit( ProjectLibraryTargetItem* );
};

/**
 * Interface that allows a developer to implement the three basic types of
 * items you would see in a multi-project
 * \li Folder
 * \li Project
 * \li Custom Target
 * \li Library Target
 * \li Executable Target
 * \li File
 */
class KDEVPLATFORMPROJECT_EXPORT ProjectBaseItem
{
    public:
        ProjectBaseItem( IProject*, const QString &name, ProjectBaseItem *parent = 0 );
        virtual ~ProjectBaseItem();


        enum ProjectItemType
        {
            BaseItem = 0          /** item is a base item */,
            BuildFolder = 1       /** item is a buildable folder */,
            Folder = 2            /** item is a folder */,
            ExecutableTarget = 3  /** item is an executable target */,
            LibraryTarget = 4     /** item is a library target */,
            Target = 5            /** item is a target */,
            File = 6              /** item is a file */,
            CustomProjectItemType = 100 /** type which should be used as base for custom types */
        };

        enum RenameStatus
        {
            RenameOk = 0,
            ExistingItemSameName = 1,
            ProjectManagerRenameFailed = 2,
            InvalidNewName = 3
        };

        /** @returns Returns the project that the item belongs to.  */
        IProject* project() const;

        /** @returns If this item is a folder, it returns a pointer to the folder, otherwise returns a 0 pointer. */
        virtual ProjectFolderItem *folder() const;

        /** @returns If this item is a target, it returns a pointer to the target, otherwise returns a 0 pointer. */
        virtual ProjectTargetItem *target() const;

        /** @returns If this item is a file, it returns a pointer to the file, otherwise returns a 0 pointer. */
        virtual ProjectFileItem *file() const;

        /** @returns If this item is a file, it returns a pointer to the file, otherwise returns a 0 pointer. */
        virtual ProjectExecutableTargetItem *executable() const;

        /** @returns Returns a list of the folders that have this object as the parent. */
        QList<ProjectFolderItem*> folderList() const;

        /** @returns Returns a list of the targets that have this object as the parent. */
        QList<ProjectTargetItem*> targetList() const;

        /** @returns Returns a list of the files that have this object as the parent. */
        QList<ProjectFileItem*> fileList() const;

        virtual bool lessThan( const KDevelop::ProjectBaseItem* ) const;

        /** @returns the @p row item in the list of children of this item or 0 if there is no such child. */
        ProjectBaseItem* child( int row ) const;
        /** @returns a valid QModelIndex for usage with the model API for this item. */
        QModelIndex index() const;
        /** @returns The parent item if this item has one, else it return 0. */
        virtual ProjectBaseItem* parent() const;
        /** @returns the displayed text of this item. */
        QString text() const;
        /** @returns the row in the list of children of this items parent, or -1. */
        int row() const;
        /**
         * Allows to change the displayed text of this item.
         * @param text the new text
         */
        void setText( const QString& text );

        /** @returns the number of children of this item, or 0 if there are none. */
        int rowCount() const;

        /** @returns the model to which this item belongs, or 0 if its not associated to a model. */
        ProjectModel* model() const;

        /**
         * Adds a new child item to this item.
         */
        void appendRow( ProjectBaseItem* item );
        
        /**
         * Removes and deletes the item at the given row if there is one.
         */
        void removeRow( int row );

        /** @returns RTTI info, allows to know the type of item */
        virtual int type() const;

        /** @returns a string to pass to KIcon as icon-name suitable to represent this item. */
        virtual QString iconName() const;

        /**
         * Set the url of this item.
         * Note this function never renames the item in the project manager or on the filesystem,
         * it only changes the url and possibly the text nothing else.
         */
        virtual void setUrl( const KUrl& );

        /** Get the url of this item (if any) */
        KUrl url() const;

        /**
         * Renames the item to the new name.
         * @returns status information wether the renaming succeeded.
         */
        virtual RenameStatus rename( const QString& newname );

    protected:
        class ProjectBaseItemPrivate* const d_ptr;
        ProjectBaseItem( ProjectBaseItemPrivate& dd );
        void setRow( int row );
        void setModel( ProjectModel* model );
    private:
        Q_DECLARE_PRIVATE(ProjectBaseItem)
        friend class ProjectModel;
};

/**
 * Implementation of the ProjectBaseItem interface that is specific to a
 * folder
 */
class KDEVPLATFORMPROJECT_EXPORT ProjectFolderItem: public ProjectBaseItem
{
public:
    ProjectFolderItem( IProject*, const KUrl &dir, ProjectBaseItem *parent = 0 );

    virtual ~ProjectFolderItem();

    virtual ProjectFolderItem *folder() const;

    ///Reimplemented from QStandardItem
    virtual int type() const;

    /** Get the folder name, equal to url().fileName() but faster (precomputed) */
    QString folderName() const;

    /** @returns Returns whether this folder directly contains the specified file or folder. */
    bool hasFileOrFolder(const QString& name) const;
    
    virtual QString iconName() const;
    virtual RenameStatus rename(const QString& newname);
};


/**
 * Folder which contains buildable targets as part of a buildable project
 */
class KDEVPLATFORMPROJECT_EXPORT ProjectBuildFolderItem: public ProjectFolderItem
{
public:
    ProjectBuildFolderItem( IProject*, const KUrl &dir, ProjectBaseItem *parent = 0 );

    ///Reimplemented from QStandardItem
    virtual int type() const;
    virtual QString iconName() const;
};

/**
 * Object which represents a target in a build system.
 *
 * This object contains all properties specific to a target.
 */
class KDEVPLATFORMPROJECT_EXPORT ProjectTargetItem: public ProjectBaseItem
{
public:
    ProjectTargetItem( IProject*, const QString &name, ProjectBaseItem *parent = 0 );

    ///Reimplemented from QStandardItem
    virtual int type() const;

    virtual ProjectTargetItem *target() const;
    virtual QString iconName() const;
};

/**
 * Object which represents an executable target in a build system.
 *
 * This object contains all properties specific to an executable.
 */
class KDEVPLATFORMPROJECT_EXPORT ProjectExecutableTargetItem: public ProjectTargetItem
{
    public:
        ProjectExecutableTargetItem( IProject*, const QString &name, ProjectBaseItem *parent = 0 );

        virtual ProjectExecutableTargetItem *executable() const;
        virtual int type() const;
        virtual KUrl builtUrl() const=0;
        virtual KUrl installedUrl() const=0;
};


/**
 * Object which represents a library target in a build system.
 *
 * This object contains all properties specific to a library.
 */
class KDEVPLATFORMPROJECT_EXPORT ProjectLibraryTargetItem: public ProjectTargetItem
{
    public:
        ProjectLibraryTargetItem(IProject* project, const QString &name, ProjectBaseItem *parent = 0 );

        virtual int type() const;
};

/**
 * Object which represents a file.
 */
class KDEVPLATFORMPROJECT_EXPORT ProjectFileItem: public ProjectBaseItem
{
public:
    ProjectFileItem( IProject*, const KUrl& file, ProjectBaseItem *parent = 0 );
    ~ProjectFileItem();

    ///Reimplemented from QStandardItem
    virtual int type() const;

    virtual ProjectFileItem *file() const;

    /** Get the file name, equal to url().fileName() but faster (precomputed) */
    QString fileName() const;

    virtual void setUrl( const KUrl& );
    virtual QString iconName() const;
    virtual RenameStatus rename(const QString& newname);
};

/**
 * Class providing some convenience methods for accessing the project model
 * @todo: maybe switch to QAbstractItemModel, would make the implementation
 *        for at least the checkbox-behaviour easier
 */
class KDEVPLATFORMPROJECT_EXPORT ProjectModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    ProjectModel( QObject *parent = 0 );
    virtual ~ProjectModel();

    void resetModel();

    void clear();

    void appendRow( ProjectBaseItem* item );
    void removeRow( int row );

    QModelIndex pathToIndex(const QStringList& tofetch) const;
    QStringList pathFromIndex(const QModelIndex& index) const;

    QModelIndex indexFromItem( const ProjectBaseItem* item ) const;
    ProjectBaseItem* itemFromIndex( const QModelIndex& ) const;

    virtual int columnCount( const QModelIndex& parent = QModelIndex() ) const;
    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual QModelIndex parent( const QModelIndex& child ) const;
    virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;
    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    virtual bool hasChildren(const QModelIndex& parent = QModelIndex()) const;

    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    virtual bool insertColumns(int column, int count, const QModelIndex& parent = QModelIndex());
    virtual bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());

private:
    class ProjectModelPrivate* const d;
    friend class ProjectBaseItem;
};

KDEVPLATFORMPROJECT_EXPORT QStringList joinProjectBasePath( const QStringList& partialpath, KDevelop::ProjectBaseItem* item );
KDEVPLATFORMPROJECT_EXPORT QStringList removeProjectBasePath( const QStringList& fullpath, KDevelop::ProjectBaseItem* item );

}

#endif // KDEVPROJECTMODEL_H
