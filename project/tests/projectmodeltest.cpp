/***************************************************************************
 *   Copyright 2010 Andreas Pakulat <apaku@gmx.de>                         *
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

#include "projectmodeltest.h"
#include <QtTest/QTest>
#include <qtest_kde.h>

#include <projectmodel.h>
#include <tests/modeltest.h>

using KDevelop::ProjectModel;
using KDevelop::ProjectBaseItem;
using KDevelop::ProjectFolderItem;
using KDevelop::ProjectFileItem;
using KDevelop::ProjectExecutableTargetItem;
using KDevelop::ProjectLibraryTargetItem;
using KDevelop::ProjectTargetItem;
using KDevelop::ProjectBuildFolderItem;

void ProjectModelTest::initTestCase()
{
    qRegisterMetaType<QModelIndex>("QModelIndex");
    model = new ProjectModel( this );
    ModelTest* mt = new ModelTest( model, this );
}

void ProjectModelTest::init()
{
    model->clear();
}

void ProjectModelTest::testCreateFileSystemItems()
{
    QFETCH( int, itemType );
    QFETCH( KUrl, itemUrl );
    QFETCH( KUrl, expectedItemUrl );
    QFETCH( QString, expectedItemText );
    QFETCH( QStringList, expectedItemPath );
    QFETCH( int, expectedItemRow );

    ProjectBaseItem* newitem = 0;
    switch( itemType ) {
        case ProjectBaseItem::Folder:
            newitem = new ProjectFolderItem( 0, itemUrl );
            break;
        case ProjectBaseItem::BuildFolder:
            newitem = new ProjectBuildFolderItem( 0, itemUrl );
            break;
        case ProjectBaseItem::File:
            newitem = new ProjectFileItem( 0, itemUrl );
            break;
    }
    int origRowCount = model->rowCount();
    model->appendRow( newitem );
    QCOMPARE( model->rowCount(), origRowCount+1 );
    QCOMPARE( newitem->row(), expectedItemRow );
    QModelIndex idx = model->index( expectedItemRow, 0, QModelIndex() );
    QVERIFY( model->itemFromIndex( idx ) );
    QCOMPARE( model->itemFromIndex( idx ), newitem );
    QCOMPARE( newitem->text(), expectedItemText );
    QCOMPARE( newitem->url(), expectedItemUrl );
    if( itemType == ProjectBaseItem::File ) {
        QCOMPARE( dynamic_cast<ProjectFileItem*>( newitem )->fileName(), expectedItemText );
    }
    if( itemType == ProjectBaseItem::Folder || itemType == ProjectBaseItem::BuildFolder ) {
        QCOMPARE( dynamic_cast<ProjectFolderItem*>( newitem )->folderName(), expectedItemText );
    }
    QCOMPARE( newitem->type(), itemType );
    QCOMPARE( model->data( idx ).toString(), expectedItemText );
    QCOMPARE( model->indexFromItem( newitem ), idx );
    QCOMPARE( model->pathFromIndex( idx ), expectedItemPath );
    QCOMPARE( model->pathToIndex( expectedItemPath ), idx );
}

void ProjectModelTest::testCreateFileSystemItems_data()
{
    QTest::addColumn<int>( "itemType" );
    QTest::addColumn<KUrl>( "itemUrl" );
    QTest::addColumn<KUrl>( "expectedItemUrl" );
    QTest::addColumn<QString>( "expectedItemText" );
    QTest::addColumn<QStringList>( "expectedItemPath" );
    QTest::addColumn<int>( "expectedItemRow" );

    QTest::newRow("RootFolder")
        << (int)ProjectBaseItem::Folder
        << KUrl("file:///rootdir")
        << KUrl("file:///rootdir")
        << QString::fromLatin1("rootdir")
        << ( QStringList() << "rootdir" )
        << 0;

    QTest::newRow("RootBuildFolder")
        << (int)ProjectBaseItem::BuildFolder
        << KUrl("file:///rootdir")
        << KUrl("file:///rootdir")
        << QString::fromLatin1("rootdir")
        << ( QStringList() << "rootdir" )
        << 0;

    QTest::newRow("RootFile")
        << (int)ProjectBaseItem::File
        << KUrl("file:///rootfile")
        << KUrl("file:///rootfile")
        << QString::fromLatin1("rootfile")
        << ( QStringList() << "rootfile" )
        << 0;

}

void ProjectModelTest::testCreateTargetItems()
{
    QFETCH( int, itemType );
    QFETCH( QString, itemText );
    QFETCH( QString, expectedItemText );
    QFETCH( QStringList, expectedItemPath );
    QFETCH( int, expectedItemRow );

    ProjectBaseItem* newitem = 0;
    switch( itemType ) {
        case ProjectBaseItem::Target:
            newitem = new ProjectTargetItem( 0, itemText );
            break;
        case ProjectBaseItem::LibraryTarget:
            newitem = new ProjectLibraryTargetItem( 0, itemText );
            break;
    }
    int origRowCount = model->rowCount();
    model->appendRow( newitem );
    QCOMPARE( model->rowCount(), origRowCount+1 );
    QCOMPARE( newitem->row(), expectedItemRow );
    QModelIndex idx = model->index( expectedItemRow, 0, QModelIndex() );
    QVERIFY( model->itemFromIndex( idx ) );
    QCOMPARE( model->itemFromIndex( idx ), newitem );
    QCOMPARE( newitem->text(), expectedItemText );
    QCOMPARE( newitem->type(), itemType );
    QCOMPARE( model->data( idx ).toString(), expectedItemText );
    QCOMPARE( model->indexFromItem( newitem ), idx );
    QCOMPARE( model->pathFromIndex( idx ), expectedItemPath );
    QCOMPARE( model->pathToIndex( expectedItemPath ), idx );
}

void ProjectModelTest::testCreateTargetItems_data()
{
    QTest::addColumn<int>( "itemType" );
    QTest::addColumn<QString>( "itemText" );
    QTest::addColumn<QString>( "expectedItemText" );
    QTest::addColumn<QStringList>( "expectedItemPath" );
    QTest::addColumn<int>( "expectedItemRow" );

    QTest::newRow("RootTarget")
        << (int)ProjectBaseItem::Target
        << "target"
        << QString::fromLatin1("target")
        << ( QStringList() << "target" )
        << 0;

    QTest::newRow("RootLibraryTarget")
        << (int)ProjectBaseItem::LibraryTarget
        << "libtarget"
        << QString::fromLatin1("libtarget")
        << ( QStringList() << "libtarget" )
        << 0;
}

void ProjectModelTest::testCreateSimpleHierarchy()
{
    QString folderName = "rootfolder";
    QString fileName = "file";
    QString targetName = "testtarged";
    QString cppFileName = "file.cpp";
    ProjectFolderItem* rootFolder = new ProjectFolderItem( 0, KUrl("file:///"+folderName) );
    ProjectFileItem* file = new ProjectFileItem( 0, KUrl("file:///"+folderName+"/"+fileName), rootFolder );
    ProjectTargetItem* target = new ProjectTargetItem( 0, targetName );
    rootFolder->appendRow( target );
    ProjectFileItem* targetfile = new ProjectFileItem( 0, KUrl("file:///"+folderName+"/"+cppFileName), target );

    model->appendRow( rootFolder );

    QCOMPARE( model->rowCount(), 1 );
    QModelIndex folderIdx = model->index( 0, 0, QModelIndex() );
    QCOMPARE( model->data( folderIdx ).toString(), folderName );
    QCOMPARE( model->rowCount( folderIdx ), 2 );
    QCOMPARE( model->itemFromIndex( folderIdx ), rootFolder );
    QVERIFY( rootFolder->hasFileOrFolder( fileName ) );

    QModelIndex fileIdx = model->index( 0, 0, folderIdx );
    QCOMPARE( model->data( fileIdx ).toString(), fileName );
    QCOMPARE( model->rowCount( fileIdx ), 0 );
    QCOMPARE( model->itemFromIndex( fileIdx ), file );

    QModelIndex targetIdx = model->index( 1, 0, folderIdx );
    QCOMPARE( model->data( targetIdx ).toString(), targetName );
    QCOMPARE( model->rowCount( targetIdx ), 1 );
    QCOMPARE( model->itemFromIndex( targetIdx ), target );

    QModelIndex targetFileIdx = model->index( 0, 0, targetIdx );
    QCOMPARE( model->data( targetFileIdx ).toString(), cppFileName );
    QCOMPARE( model->rowCount( targetFileIdx ), 0 );
    QCOMPARE( model->itemFromIndex( targetFileIdx ), targetfile );

    rootFolder->removeRow( 1 );
    QCOMPARE( model->rowCount( folderIdx ), 1 );
    delete file;
    file = 0;

    // Check that we also find a folder with the fileName
    new ProjectFolderItem( 0, fileName, rootFolder );
    QVERIFY( rootFolder->hasFileOrFolder( fileName ) );

    delete rootFolder;
    QCOMPARE( model->rowCount(), 0 );
}

void ProjectModelTest::testItemSanity()
{
    ProjectBaseItem* parent = new ProjectBaseItem( 0, "test" );
    ProjectBaseItem* child = new ProjectBaseItem( 0, "test", parent );
    ProjectBaseItem* child2 = new ProjectBaseItem( 0, "ztest", parent );
    ProjectFileItem* child3 = new ProjectFileItem( 0, KUrl("file:///bcd"), parent );
    ProjectFileItem* child4 = new ProjectFileItem(  0, KUrl("file:///abcd"), parent  );

    // Just some basic santiy checks on the API
    QCOMPARE( parent->child( 0 ), child );
    QCOMPARE( parent->row(), -1 );
    QVERIFY( !parent->child( -1 ) );
    QVERIFY( !parent->file() );
    QVERIFY( !parent->folder() );
    QVERIFY( !parent->project() );
    QVERIFY( !parent->child( parent->rowCount() ) );
    QCOMPARE( parent->flags(), Qt::ItemFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled ) );
    QCOMPARE( parent->iconName(), QString() );
    QCOMPARE( parent->index(), QModelIndex() );

    child->setFlags( child->flags() | Qt::ItemIsEditable );
    QCOMPARE( child->flags(), Qt::ItemFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable ) );
    QCOMPARE( child->type(), (int)ProjectBaseItem::BaseItem );

    QCOMPARE( child->lessThan( child2 ), true );
    QCOMPARE( child3->lessThan( child4 ), false );

    // Check that model is properly emitting data-changes
    model->appendRow( parent );
    parent->setFlags( child->flags() | Qt::ItemIsEditable );
    QCOMPARE( parent->flags(), Qt::ItemFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable ) );
    QCOMPARE( parent->index(), model->index(0, 0, QModelIndex()) );
    parent->setText( "newtest" );
    QCOMPARE( model->data( parent->index() ).toString(), QString("newtest") );
}

void ProjectModelTest::testRename()
{
    QFETCH( int, itemType );
    QFETCH( QString, itemText );
    QFETCH( QString, newName );
    QFETCH( bool, datachangesignal );
    QFETCH( QString, expectedItemText );
    QFETCH( int, expectedRenameCode );

    ProjectBaseItem* item = 0;
    if( itemType == ProjectBaseItem::Target ) {
        item = new ProjectTargetItem( 0, itemText );
    } else if( itemType == ProjectBaseItem::File ) {
        item = new ProjectFileItem( 0, KUrl(itemText) );
    } else if( itemType == ProjectBaseItem::Folder ) {
        item = new ProjectFolderItem( 0, KUrl(itemText) );
    } else if( itemType == ProjectBaseItem::BuildFolder ) {
        item = new ProjectBuildFolderItem( 0, KUrl(itemText) );
    }
    Q_ASSERT( item );
    
    model->appendRow( item );
    QSignalSpy s( model, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)) );
    ProjectBaseItem::RenameStatus stat = item->rename( newName );
    QCOMPARE( (int)stat, expectedRenameCode );
    if( datachangesignal ) {
        QCOMPARE( s.count(), 1 );
        QCOMPARE( qvariant_cast<QModelIndex>( s.takeFirst().at(0) ), item->index() );
    }
    QCOMPARE( item->text(), expectedItemText );
}

void ProjectModelTest::testRename_data()
{
    QTest::addColumn<int>( "itemType" );
    QTest::addColumn<QString>( "itemText" );
    QTest::addColumn<QString>( "newName" );
    QTest::addColumn<bool>( "datachangesignal" );
    QTest::addColumn<QString>( "expectedItemText" );
    QTest::addColumn<int>( "expectedRenameCode" );
    
    QTest::newRow("RenameableTarget")
    << (int)ProjectBaseItem::Target
    << QString::fromLatin1("target")
    << QString::fromLatin1("othertarget")
    << true
    << QString::fromLatin1("othertarget")
    << (int)ProjectBaseItem::RenameOk;
    
    QTest::newRow("RenameableFile")
    << (int)ProjectBaseItem::File
    << QString::fromLatin1("newfile.cpp")
    << QString::fromLatin1("otherfile.cpp")
    << true
    << QString::fromLatin1("otherfile.cpp")
    << (int)ProjectBaseItem::RenameOk;
    
    QTest::newRow("RenameableFolder")
    << (int)ProjectBaseItem::Folder
    << QString::fromLatin1("newfolder")
    << QString::fromLatin1("otherfolder")
    << true
    << QString::fromLatin1("otherfolder")
    << (int)ProjectBaseItem::RenameOk;
    
    QTest::newRow("RenameableBuildFolder")
    << (int)ProjectBaseItem::BuildFolder
    << QString::fromLatin1("newbfolder")
    << QString::fromLatin1("otherbfolder")
    << true
    << QString::fromLatin1("otherbfolder")
    << (int)ProjectBaseItem::RenameOk;

    QTest::newRow("ExistingFileError")
    << (int)ProjectBaseItem::Folder
    << QString::fromLatin1("mynew")
    << QString::fromLatin1("bin")
    << false
    << QString::fromLatin1("mynew")
    << (int)ProjectBaseItem::ExistingItemSameName;

    QTest::newRow("InvalidNameError")
    << (int)ProjectBaseItem::File
    << QString::fromLatin1("mynew")
    << QString::fromLatin1("other/bash")
    << false
    << QString::fromLatin1("mynew")
    << (int)ProjectBaseItem::InvalidNewName;
}

QTEST_KDEMAIN( ProjectModelTest, GUI)
#include "projectmodeltest.moc"
