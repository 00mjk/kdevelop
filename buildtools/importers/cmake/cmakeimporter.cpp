/* KDevelop CMake Support
 *
 * Copyright 2006 Matt Rogers <mattr@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "cmakeimporter.h"

#include <QList>
#include <QVector>
#include <QDomDocument>


#include <kurl.h>
#include <kio/job.h>

#include "kdevcore.h"
#include "kdevproject.h"
#include "kgenericfactory.h"
#include "kdevprojectmodel.h"

#include "cmakeconfig.h"
#include "cmakemodelitems.h"

typedef KGenericFactory<CMakeImporter> CMakeSupportFactory ;
K_EXPORT_COMPONENT_FACTORY( kdevcmakeimporter,
                            CMakeSupportFactory( "kdevcmakeimporter" ) )

CMakeImporter::CMakeImporter( QObject* parent,
                              const QStringList& )
    : Koncrete::BuildManager( CMakeSupportFactory::componentData(), parent ), m_rootItem(0L)
{
    m_project = 0;
/*    CMakeSettings* settings = CMakeSettings::self();

    //what do the settings say about our generator?
    QString generator = settings->generator();
    if ( generator.contains( "Unix" ) ) //use make
        m_builder = new KDevMakeBuilder()*/
}

CMakeImporter::~CMakeImporter()
{
    //delete m_rootItem;
}

Koncrete::Project* CMakeImporter::project() const
{
    return m_project;
}

KUrl CMakeImporter::buildDirectory() const
{
     return project()->folder();
}

QList<Koncrete::ProjectFolderItem*> CMakeImporter::parse( Koncrete::ProjectFolderItem* item )
{
    QList<Koncrete::ProjectFolderItem*> folderList;
    CMakeFolderItem* folder = dynamic_cast<CMakeFolderItem*>( item );
    if ( !folder )
        return folderList;

    FolderInfo fi = folder->folderInfo();
    for ( QStringList::iterator it = fi.includes.begin();
          it != fi.includes.end(); ++it )
    {
        KUrl urlCandidate = KUrl( ( *it ) );
        if ( m_includeDirList.indexOf( urlCandidate ) == -1 )
            m_includeDirList.append( urlCandidate );
    }

    foreach ( FolderInfo sfi, fi.subFolders )
        folderList.append( new CMakeFolderItem( sfi, folder ) );

    foreach ( TargetInfo ti, fi.targets )
    {
        CMakeTargetItem* targetItem = new CMakeTargetItem( ti, folder );
        foreach( QString sFile, ti.sources )
        {
            KUrl sourceFile = folder->url();
            sourceFile.adjustPath( KUrl::AddTrailingSlash );
            sourceFile.addPath( sFile );
            new Koncrete::ProjectFileItem( sourceFile, targetItem );
        }
    }


    return folderList;
}

Koncrete::ProjectItem* CMakeImporter::import( Koncrete::ProjectModel* model,
                                              const KUrl& fileName )
{
    QString buildDir = CMakeSettings::self()->buildFolder();
    kDebug( 9025 ) << k_funcinfo << "build dir is " << qPrintable( buildDir ) << endl;
    KUrl cmakeInfoFile(buildDir);
    cmakeInfoFile.adjustPath( KUrl::AddTrailingSlash );
    cmakeInfoFile.addPath("cmakeinfo.xml");
    kDebug(9025) << k_funcinfo << "file is " << cmakeInfoFile.path() << endl;
    if ( !cmakeInfoFile.isLocalFile() )
    {
        //FIXME turn this into a real warning
        kWarning(9025) << "not a local file. CMake support doesn't handle remote projects" << endl;
    }
    else
    {
        m_projectInfo = m_xmlParser.parse( cmakeInfoFile );
        FolderInfo rootFolder = m_projectInfo.rootFolder;
        m_rootItem = new CMakeFolderItem( rootFolder, 0 );
        m_rootItem->setText( Koncrete::Core::activeProject()->name() );
    }
    return m_rootItem;
}

KUrl CMakeImporter::findMakefile( Koncrete::ProjectFolderItem* dom ) const
{
    Q_UNUSED( dom );
    return KUrl();
}

KUrl::List CMakeImporter::findMakefiles( Koncrete::ProjectFolderItem* dom ) const
{
    Q_UNUSED( dom );
    return KUrl::List();
}

QList<Koncrete::ProjectTargetItem*> CMakeImporter::targets() const
{
    return QList<Koncrete::ProjectTargetItem*>();
}

KUrl::List CMakeImporter::includeDirectories() const
{
    return m_includeDirList;
}


// kate: indent-mode cstyle; space-indent on; indent-width 4; replace-tabs on;


