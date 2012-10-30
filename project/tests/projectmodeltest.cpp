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
#include <QtGui/QSortFilterProxyModel>
#include <QtCore/QThread>
#include <qtest_kde.h>

#include <projectmodel.h>
#include <projectproxymodel.h>
#include <tests/modeltest.h>
#include <tests/testproject.h>
#include <projectproxymodel.h>
#include <tests/kdevsignalspy.h>
#include <tests/autotestshell.h>
#include <tests/testcore.h>

using KDevelop::ProjectModel;
using KDevelop::ProjectBaseItem;
using KDevelop::ProjectFolderItem;
using KDevelop::ProjectFileItem;
using KDevelop::ProjectExecutableTargetItem;
using KDevelop::ProjectLibraryTargetItem;
using KDevelop::ProjectTargetItem;
using KDevelop::ProjectBuildFolderItem;

using KDevelop::TestProject;

class AddItemThread : public QThread
{
Q_OBJECT
public:
    AddItemThread( ProjectBaseItem* _parentItem, QObject* parent = 0 )
        : QThread( parent ), parentItem( _parentItem )
    {
    }
    virtual void run()
    {
        this->sleep( 1 );
        KUrl url = parentItem->url();
        url.addPath("folder1");
        ProjectFolderItem* folder = new ProjectFolderItem( 0, url, parentItem );
        url.addPath( "file1" );
        new ProjectFileItem( 0, url, folder );
        emit addedItems();
    }
signals:
    void addedItems();
private:
    ProjectBaseItem* parentItem;
};

class SignalReceiver : public QObject
{
Q_OBJECT
public:
    SignalReceiver(ProjectModel* _model, QObject* parent = 0)
        : QObject(parent), model( _model )
    {
    }
    QThread* threadOfSignalEmission() const
    {
        return threadOfReceivedSignal;
    }
private slots:
    void rowsInserted( const QModelIndex&, int, int )
    {
        threadOfReceivedSignal = QThread::currentThread();
    }
private:
    QThread* threadOfReceivedSignal;
    ProjectModel* model;
};

void debugItemModel(QAbstractItemModel* m, const QModelIndex& parent=QModelIndex(), int depth=0)
{
    Q_ASSERT(m);
    qDebug() << QByteArray(depth*2, '-') << m->data(parent).toString();
    for(int i=0; i<m->rowCount(parent); i++) {
        debugItemModel(m, m->index(i, 0, parent), depth+1);
    }
}

void ProjectModelTest::initTestCase()
{
    KDevelop::AutoTestShell::init();
    KDevelop::TestCore::initialize(KDevelop::Core::NoUi);

    qRegisterMetaType<QModelIndex>("QModelIndex");
    model = new ProjectModel( this );
    modelTest = new ModelTest( model, this );
    proxy = new ProjectProxyModel( model );
    new ModelTest(proxy, proxy);
    proxy->setSourceModel(model);
}

void ProjectModelTest::init()
{
    model->clear();
}

void ProjectModelTest::cleanupTestCase()
{
    delete model;
    KDevelop::TestCore::shutdown();
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
        << KUrl("file:///rootdir/")
        << QString::fromLatin1("rootdir")
        << ( QStringList() << "rootdir" )
        << 0;

    QTest::newRow("RootBuildFolder")
        << (int)ProjectBaseItem::BuildFolder
        << KUrl("file:///rootdir")
        << KUrl("file:///rootdir/")
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

void ProjectModelTest::testChangeWithProxyModel()
{
    QSortFilterProxyModel* proxy = new QSortFilterProxyModel( this );
    proxy->setSourceModel( model );
    ProjectFolderItem* root = new ProjectFolderItem( 0, KUrl("file:///folder1") );
    root->appendRow( new ProjectFileItem( 0, KUrl("file:///folder1/file1") ) );
    model->appendRow( root );

    QCOMPARE( model->rowCount(), 1 );
    QCOMPARE( proxy->rowCount(), 1 );

    model->removeRow( 0 );

    QCOMPARE( model->rowCount(), 0 );
    QCOMPARE( proxy->rowCount(), 0 );
}

void ProjectModelTest::testCreateSimpleHierarchy()
{
    QString folderName = "rootfolder";
    QString fileName = "file";
    QString targetName = "testtarged";
    QString cppFileName = "file.cpp";
    ProjectFolderItem* rootFolder = new ProjectFolderItem( 0, KUrl("file:///"+folderName) );
    QCOMPARE(rootFolder->baseName(), folderName);
    ProjectFileItem* file = new ProjectFileItem( 0, KUrl("file:///"+folderName+"/"+fileName), rootFolder );
    QCOMPARE(file->baseName(), fileName);
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
    QCOMPARE( parent->iconName(), QString() );
    QCOMPARE( parent->index(), QModelIndex() );

    QCOMPARE( child->type(), (int)ProjectBaseItem::BaseItem );

    QCOMPARE( child->lessThan( child2 ), true );
    QCOMPARE( child3->lessThan( child4 ), false );

    // Check that model is properly emitting data-changes
    model->appendRow( parent );
    QCOMPARE( parent->index(), model->index(0, 0, QModelIndex()) );
    QSignalSpy s( model, SIGNAL(dataChanged(QModelIndex,QModelIndex)) );
    parent->setUrl( KUrl("file:///newtest") );
    QCOMPARE( s.count(), 1 );
    QCOMPARE( model->data( parent->index() ).toString(), QString("newtest") );

    parent->removeRow( child->row() );
}

void ProjectModelTest::testTakeRow()
{
    ProjectBaseItem* parent = new ProjectBaseItem( 0, "test" );
    ProjectBaseItem* child = new ProjectBaseItem( 0, "test", parent );
    ProjectBaseItem* subchild = new ProjectBaseItem( 0, "subtest", child );

    model->appendRow( parent );

    QCOMPARE( parent->model(), model );
    QCOMPARE( child->model(), model );
    QCOMPARE( subchild->model(), model );

    parent->takeRow( child->row() );

    QCOMPARE( child->model(), static_cast<ProjectModel*>(0) );
    QCOMPARE( subchild->model(), static_cast<ProjectModel*>(0) );
}

void ProjectModelTest::testRename()
{
    QFETCH( int, itemType );
    QFETCH( QString, itemText );
    QFETCH( QString, newName );
    QFETCH( bool, datachangesignal );
    QFETCH( QString, expectedItemText );
    QFETCH( int, expectedRenameCode );

    TestProject* proj = new TestProject;
    ProjectFolderItem* rootItem = new ProjectFolderItem( proj, KUrl("file:///dummyprojectfolder"), 0);
    proj->setProjectItem( rootItem );
    model->appendRow( rootItem );

    new ProjectFileItem(proj, KUrl("existing"), rootItem);

    ProjectBaseItem* item = 0;
    if( itemType == ProjectBaseItem::Target ) {
        item = new ProjectTargetItem( proj, itemText, rootItem );
    } else if( itemType == ProjectBaseItem::File ) {
        item = new ProjectFileItem( proj, KUrl(itemText), rootItem );
    } else if( itemType == ProjectBaseItem::Folder ) {
        item = new ProjectFolderItem( proj, KUrl(itemText), rootItem );
    } else if( itemType == ProjectBaseItem::BuildFolder ) {
        item = new ProjectBuildFolderItem( proj, KUrl(itemText), rootItem );
    }
    Q_ASSERT( item );

    QCOMPARE(item->model(), model);
    QSignalSpy s( model, SIGNAL(dataChanged(QModelIndex,QModelIndex)) );
    ProjectBaseItem::RenameStatus stat = item->rename( newName );
    QCOMPARE( (int)stat, expectedRenameCode );
    if( datachangesignal ) {
        QCOMPARE( s.count(), 1 );
        QCOMPARE( qvariant_cast<QModelIndex>( s.takeFirst().at(0) ), item->index() );
    } else {
        QCOMPARE( s.count(), 0 );
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
    << QString::fromLatin1("existing")
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

void ProjectModelTest::testWithProject()
{
    TestProject* proj = new TestProject();
    ProjectFolderItem* rootItem = new ProjectFolderItem( proj, KUrl("file:///dummyprojectfolder"), 0);
    proj->setProjectItem( rootItem );
    model->appendRow( rootItem );
    ProjectBaseItem* item = model->itemFromIndex( model->index( 0, 0 ) );
    QCOMPARE( item, rootItem );
    QCOMPARE( item->text(), proj->name() );
    QCOMPARE( item->url(), proj->folder() );
}

void ProjectModelTest::testAddItemInThread()
{
    ProjectFolderItem* root = new ProjectFolderItem( 0, KUrl("file:///f1"), 0 );
    model->appendRow( root );
    AddItemThread t( root );
    SignalReceiver check( model );
    connect( model, SIGNAL(rowsInserted(QModelIndex,int,int)), &check, SLOT(rowsInserted(QModelIndex,int,int)), Qt::DirectConnection );
    KDevelop::KDevSignalSpy spy( &t, SIGNAL(addedItems()), Qt::QueuedConnection );
    t.start();
    QVERIFY(spy.wait( 10000 ));
    QCOMPARE( qApp->thread(), check.threadOfSignalEmission() );
}

void ProjectModelTest::testDeleteLots()
{
    delete modelTest;

    const int items = 10000;
    ProjectBaseItem* root = new ProjectBaseItem( 0, "root", 0 );
    model->appendRow( root );
    for ( int i = 0; i < items; ++i ) {
        new ProjectBaseItem( 0, QString::number(i), root );
    }
    QCOMPARE(root->children().size(), items);
    delete root;

    modelTest = new ModelTest( model, this );
}

void ProjectModelTest::testItemsForUrl()
{
    QFETCH(KUrl, url);
    QFETCH(ProjectBaseItem*, root);
    QFETCH(int, matches);

    model->appendRow(root);

    QList< ProjectBaseItem* > items = model->itemsForUrl(url);
    QCOMPARE(items.size(), matches);
    foreach(ProjectBaseItem* item, items) {
        QVERIFY(item->url().equals(url, KUrl::CompareWithoutTrailingSlash));
    }

    model->clear();
}

void ProjectModelTest::testItemsForUrl_data()
{
    QTest::addColumn<KUrl>("url");
    QTest::addColumn<ProjectBaseItem*>("root");
    QTest::addColumn<int>("matches");

    {
        ProjectFolderItem* root = new ProjectFolderItem(0, KUrl("file:////tmp/"));
        ProjectFileItem* file = new ProjectFileItem(0, KUrl(root->url(), "a"), root);
        QTest::newRow("find one") << file->url() << static_cast<ProjectBaseItem*>(root) << 1;
    }

    {
        ProjectFolderItem* root = new ProjectFolderItem(0, KUrl("file:////tmp/"));
        ProjectFolderItem* folder = new ProjectFolderItem(0, KUrl(root->url(), "a"), root);
        ProjectFileItem* file = new ProjectFileItem(0, KUrl(folder->url(), "foo"), folder);
        ProjectTargetItem* target = new ProjectTargetItem(0, "b", root);
        ProjectFileItem* file2 = new ProjectFileItem(0, file->url(), target);
        Q_UNUSED(file2);
        QTest::newRow("find two") << file->url() << static_cast<ProjectBaseItem*>(root) << 2;
    }
}

void ProjectModelTest::testProjectProxyModel()
{
    ProjectFolderItem* root = new ProjectFolderItem(0, KUrl("file:///tmp/"));
    new ProjectFileItem(0, KUrl("file:///tmp/a1/"), root);
    new ProjectFileItem(0, KUrl("file:///tmp/b1/"), root);
    new ProjectFileItem(0, KUrl("file:///tmp/c1/"), root);
    new ProjectFileItem(0, KUrl("file:///tmp/d1/"), root);
    model->appendRow(root);
    
    QModelIndex proxyRoot = proxy->mapFromSource(root->index());
    QCOMPARE(model->rowCount(root->index()), 4);
    QCOMPARE(proxy->rowCount(proxyRoot), 4);
    
    proxy->setFilterString("*1");
    QCOMPARE(proxy->rowCount(proxyRoot), 4);
    
    proxy->setFilterString("a*");
    debugItemModel(proxy);
    QCOMPARE(proxy->rowCount(proxyRoot), 1);
    
    model->clear();
}

void ProjectModelTest::testProjectFileSet()
{
    TestProject* project = new TestProject;

    QVERIFY(project->fileSet().isEmpty());
    KUrl url("file:///tmp/a");
    ProjectFileItem* item = new ProjectFileItem(project, url, project->projectItem());
    QCOMPARE(project->fileSet().size(), 1);
    qDebug() << url << project->fileSet().begin()->toUrl();
    QCOMPARE(project->fileSet().begin()->toUrl(), url);
    delete item;
    QVERIFY(project->fileSet().isEmpty());
}

QTEST_KDEMAIN( ProjectModelTest, GUI)
#include "projectmodeltest.moc"
#include "moc_projectmodeltest.cpp"
