/************************************************************************
 * KDevelop4 Custom Buildsystem Support                                 *
 *                                                                      *
 * Copyright 2010 Andreas Pakulat <apaku@gmx.de>                        *
 *                                                                      *
 * This program is free software; you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation; either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful, but  *
 * WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU     *
 * General Public License for more details.                             *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program; if not, see <http://www.gnu.org/licenses/>. *
 ************************************************************************/

#include "custombuildsystemplugintest.h"

#include <QtTest/QtTest>

#include <qtest_kde.h>

#include <tests/autotestshell.h>
#include <tests/testcore.h>
#include <tests/kdevsignalspy.h>
#include <shell/sessioncontroller.h>
#include <kio/netaccess.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/isession.h>
#include <KDebug>
#include <interfaces/iproject.h>
#include <project/interfaces/ibuildsystemmanager.h>
#include <project/projectmodel.h>
#include <kconfiggroup.h>

using KDevelop::Core;
using KDevelop::ICore;
using KDevelop::IProject;
using KDevelop::TestCore;
using KDevelop::AutoTestShell;
using KDevelop::KDevSignalSpy;

void deleteDir( QDir dir )
{
    foreach( const QString& f, dir.entryList( QDir::NoDotAndDotDot | QDir::AllEntries ) ) {
        if( QFileInfo( f ).isDir() ) {
            deleteDir( QDir( dir.absoluteFilePath( f ) ) );
            dir.rmdir( f );
        } else {
            dir.remove( f );
        }
    }
}

void CustomBuildSystemPluginTest::cleanupTestCase()
{
    QDir sessiondir( KDevelop::SessionController::sessionDirectory() );
    QString sessionid = m_core->activeSession()->id().toString();
    m_core->cleanup();
    delete m_core;

    // Delete the session dir and remove the default session entry from the config.
    deleteDir( sessiondir.absoluteFilePath( sessionid ) );
    sessiondir.rmdir( sessionid );
    KGlobal::config()->group( KDevelop::SessionController::cfgSessionGroup() ).deleteEntry( KDevelop::SessionController::cfgActiveSessionEntry() );
    KGlobal::config()->sync();
}
void CustomBuildSystemPluginTest::initTestCase()
{
    AutoTestShell::init();
    m_core = new KDevelop::TestCore();
    m_core->initialize( Core::Default );
}

void CustomBuildSystemPluginTest::loadSimpleProject()
{
    KUrl projecturl( PROJECTS_SOURCE_DIR"/simpleproject/simpleproject.kdev4" );
    KDevSignalSpy* projectSpy = new KDevSignalSpy( ICore::self()->projectController(), SIGNAL( projectOpened( KDevelop::IProject* ) ) );
    ICore::self()->projectController()->openProject( projecturl );
    // Wait for the project to be opened
    if( !projectSpy->wait( 20000 ) ) {
        kFatal() << "Expected project to be loaded within 20 seconds, but this didn't happen";
    }
    IProject* project = ICore::self()->projectController()->findProjectByName( "SimpleProject" );
    QVERIFY( project );
    KUrl::List includes = project->buildSystemManager()->includeDirectories( project->projectItem() );
    
    QHash<QString,QString> defines;
    defines.insert( "_DEBUG", "" );
    defines.insert( "VARIABLE", "VALUE" );
    QCOMPARE( includes, KUrl::List( QStringList() << "/usr/include/mydir" ) );
    QCOMPARE( project->buildSystemManager()->defines( project->projectItem() ), defines );
    QCOMPARE( project->buildSystemManager()->buildDirectory( project->projectItem() ), KUrl( "file:///home/andreas/projects/testcustom/build/" ) );
}

void CustomBuildSystemPluginTest::buildDirProject()
{
    KUrl projecturl( PROJECTS_SOURCE_DIR"/builddirproject/builddirproject.kdev4" );
    KDevSignalSpy* projectSpy = new KDevSignalSpy( ICore::self()->projectController(), SIGNAL( projectOpened( KDevelop::IProject* ) ) );
    ICore::self()->projectController()->openProject( projecturl );
    // Wait for the project to be opened
    if( !projectSpy->wait( 20000 ) ) {
        kFatal() << "Expected project to be loaded within 20 seconds, but this didn't happen";
    }
    IProject* project = ICore::self()->projectController()->findProjectByName( "BuilddirProject" );
    QVERIFY( project );
   
    KUrl currentBuilddir = project->buildSystemManager()->buildDirectory( project->projectItem() );

    QCOMPARE( currentBuilddir, KUrl( projecturl.directory(KUrl::AppendTrailingSlash) ) );
}


void CustomBuildSystemPluginTest::loadMultiPathProject()
{
    KUrl projecturl( PROJECTS_SOURCE_DIR"/multipathproject/multipathproject.kdev4" );
    KDevSignalSpy* projectSpy = new KDevSignalSpy( ICore::self()->projectController(), SIGNAL( projectOpened( KDevelop::IProject* ) ) );
    ICore::self()->projectController()->openProject( projecturl );
    // Wait for the project to be opened
    if( !projectSpy->wait( 20000 ) ) {
        kFatal() << "Expected project to be loaded within 20 seconds, but this didn't happen";
    }
    IProject* project = ICore::self()->projectController()->findProjectByName( "MultiPathProject" );
    QVERIFY( project );
    KDevelop::ProjectBaseItem* mainfile = 0;
    foreach( KDevelop::ProjectBaseItem* i, project->files() ) {
        if( i->text() == "main.cpp" ) {
            mainfile = i;
            break;
        }
    }
    QVERIFY(mainfile);
    KUrl::List includes = project->buildSystemManager()->includeDirectories( mainfile );

    QHash<QString,QString> defines;
    defines.insert( "BUILD", "debug" );
    QCOMPARE( includes, KUrl::List( QStringList() << "/usr/local/include/mydir" ) );
    QCOMPARE( project->buildSystemManager()->defines( mainfile ), defines );
    QCOMPARE( project->buildSystemManager()->buildDirectory( mainfile ).url(), KUrl( "file:///home/andreas/projects/testcustom/build2/src" ).url() );
}

QTEST_KDEMAIN(CustomBuildSystemPluginTest, GUI)

#include "custombuildsystemplugintest.moc"
