/* This file is part of KDevelop
    Copyright 2004 Roberto Raggi <roberto@kdevelop.org>

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
#include "genericmanager.h"
#include <project/projectmodel.h>

#include <interfaces/icore.h>
#include <interfaces/iproject.h>
#include <interfaces/iprojectcontroller.h>
#include <language/duchain/indexedstring.h>

#include <kdebug.h>
#include <kpluginloader.h>
#include <kpluginfactory.h>
#include <klocale.h>
#include <kconfiggroup.h>
#include <ksharedconfig.h>

#include <QDir>
#include <QExtensionFactory>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>

K_PLUGIN_FACTORY(GenericSupportFactory, registerPlugin<GenericProjectManager>(); )
K_EXPORT_PLUGIN(GenericSupportFactory("kdevgenericmanager"))

class GenericProjectManagerPrivate
{
};

GenericProjectManager::GenericProjectManager( QObject *parent, const QVariantList & args )
        : KDevelop::IPlugin( GenericSupportFactory::componentData(), parent ), KDevelop::IProjectFileManager(), d( new GenericProjectManagerPrivate )
{
    KDEV_USE_EXTENSION_INTERFACE( KDevelop::IProjectFileManager )
    Q_UNUSED( args )
}

GenericProjectManager::~GenericProjectManager()
{}

bool GenericProjectManager::isValid( const QFileInfo &fileInfo,
    const QStringList &includes, const QStringList &excludes ) const
{
    QString fileName = fileInfo.fileName();

    bool ok = fileInfo.isDir();
    for ( QStringList::ConstIterator it = includes.begin(); !ok && it != includes.end(); ++it )
    {
        QRegExp rx( *it, Qt::CaseSensitive, QRegExp::Wildcard );
        if ( rx.exactMatch( fileName ) )
        {
            ok = true;
        }
    }

    if ( !ok )
        return false;

    for ( QStringList::ConstIterator it = excludes.begin(); it != excludes.end(); ++it )
    {
        QRegExp rx( *it, Qt::CaseSensitive, QRegExp::Wildcard );
        if ( rx.exactMatch( fileName ) )
        {
            return false;
        }
    }

    return true;
}

QList<KDevelop::ProjectFolderItem*> GenericProjectManager::parse( KDevelop::ProjectFolderItem *item )
{
    QDir dir( item->url().toLocalFile() );

    QList<KDevelop::ProjectFolderItem*> folder_list;
    QFileInfoList entries = dir.entryInfoList();

    KConfigGroup filtersConfig = item->project()->projectConfiguration()->group("Filters");
    QStringList includes = filtersConfig.readEntry("Includes", QStringList("*"));
    QStringList excludes = filtersConfig.readEntry("Excludes", QStringList());

    for ( int i = 0; i < entries.count(); ++i )
    {
        QFileInfo fileInfo = entries.at( i );

        if ( !isValid( fileInfo, includes, excludes ) )
        {
            //kDebug(9000) << "skip:" << fileInfo.absoluteFilePath();
        }
        else if ( fileInfo.isDir() && fileInfo.fileName() != QLatin1String( "." )
                  && fileInfo.fileName() != QLatin1String( ".." ) )
        {
            KDevelop::ProjectFolderItem *folder = new KDevelop::ProjectFolderItem( item->project(), KUrl( fileInfo.absoluteFilePath() ), item );
            folder_list.append( folder );
        }
        else if ( fileInfo.isFile() )
        {
             new KDevelop::ProjectFileItem( item->project(), KUrl( fileInfo.absoluteFilePath() ), item );
             item->project()->addToFileSet( KDevelop::IndexedString( fileInfo.absoluteFilePath() ) );
        }
    }

    return folder_list;
}

KDevelop::ProjectFolderItem *GenericProjectManager::import( KDevelop::IProject *project )
{
    KDevelop::ProjectFolderItem *projectRoot=new KDevelop::ProjectFolderItem( project, project->folder(), 0 );
    projectRoot->setProjectRoot(true);
    return projectRoot;
}

KDevelop::ProjectFolderItem* GenericProjectManager::addFolder( const KUrl& url,
        KDevelop::ProjectFolderItem * folder )
{
    Q_UNUSED( url )
    Q_UNUSED( folder )
    return 0;
}


KDevelop::ProjectFileItem* GenericProjectManager::addFile( const KUrl& url,
        KDevelop::ProjectFolderItem * folder )
{
    Q_UNUSED( url )
    Q_UNUSED( folder )
    return 0;
}

bool GenericProjectManager::renameFolder( KDevelop::ProjectFolderItem * folder, const KUrl& url )
{
    Q_UNUSED( folder )
    Q_UNUSED( url )
    return false;
}

bool GenericProjectManager::renameFile( KDevelop::ProjectFileItem * file, const KUrl& url )
{
    Q_UNUSED(file)
    Q_UNUSED(url)
    return false;
}

bool GenericProjectManager::removeFolder( KDevelop::ProjectFolderItem * folder )
{
    Q_UNUSED( folder )
    return false;
}

bool GenericProjectManager::removeFile( KDevelop::ProjectFileItem * file )
{
    Q_UNUSED( file )
    return false;
}


#include "genericmanager.moc"
