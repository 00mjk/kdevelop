#include <qwhatsthis.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qsqldatabase.h>

#include <kapplication.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kgenericfactory.h>
#include <kdebug.h>
#include <kaction.h>
#include <kparts/part.h>
#include <kdialogbase.h>
#include <ktexteditor/editinterface.h>
#include <kmessagebox.h>

#include "kdevcore.h"
#include "kdevmainwindow.h"
#include "kdevlanguagesupport.h"
#include "kdevpartcontroller.h"
#include "kdevproject.h"

#include "sqlsupport_part.h"
#include "sqlconfigwidget.h"
#include "sqlactions.h"
#include "sqloutputwidget.h"
#include "domutil.h"

typedef KGenericFactory<SQLSupportPart> SQLSupportFactory;
K_EXPORT_COMPONENT_FACTORY( libkdevsqlsupport, SQLSupportFactory( "kdevsqlsupport" ) );

SQLSupportPart::SQLSupportPart( QObject *parent, const char *name, const QStringList& )
        : KDevLanguageSupport ( "KDevPart", "kdevpart", parent, name ? name : "SQLSupportPart" )
{
    setInstance( SQLSupportFactory::instance() );
    setXMLFile( "kdevsqlsupport.rc" );

    KAction *action;
    action = new KAction( i18n( "&Run" ), "exec", Key_F9, this, SLOT( slotRun() ), actionCollection(), "build_execute" );

    dbAction = new SqlListAction( this, i18n( "&Database Connections" ), 0, this, SLOT(activeConnectionChanged()), actionCollection(), "connection_combo" );

    kdDebug( 9000 ) << "Creating SQLSupportPart" << endl;

    connect( core(), SIGNAL( projectConfigWidget( KDialogBase* ) ),
             this, SLOT( projectConfigWidget( KDialogBase* ) ) );
    connect( core(), SIGNAL( projectOpened() ), this, SLOT( projectOpened() ) );
    connect( core(), SIGNAL( projectClosed() ), this, SLOT( projectClosed() ) );
    connect( partController(), SIGNAL( savedFile( const QString& ) ), this, SLOT( savedFile( const QString& ) ) );

    m_widget = new SqlOutputWidget();
    mainWindow()->embedOutputView( m_widget, "SQL", "Output of SQL commands" );
}


SQLSupportPart::~SQLSupportPart()
{
    delete m_widget;
}

QString SQLSupportPart::cryptStr(const QString& aStr)
{
    QString result;
    for (unsigned int i = 0; i < aStr.length(); i++)
	result += (aStr[i].unicode() < 0x20) ? aStr[i] :
		  QChar(0x1001F - aStr[i].unicode());
    return result;
}

void SQLSupportPart::activeConnectionChanged()
{
    /// @todo
}

void SQLSupportPart::clearConfig()
{
    for ( QStringList::Iterator it = conNames.begin(); it != conNames.end(); ++it ) {
        if ( QSqlDatabase::contains( *it ) ) {
            QSqlDatabase::database( *it, false )->close();
            QSqlDatabase::removeDatabase( *it );
        } else {
            kdDebug( 9000 ) << "Could not find connection named " << (*it) << endl;
        }
    }
    conNames.clear();

    dbAction->refresh();
}

void SQLSupportPart::loadConfig()
{
    clearConfig();

    QDomDocument* doc = projectDom();

    QStringList db;
    int i = 0;
    QString conName;
    while ( true ) {
        QStringList sdb = DomUtil::readListEntry( *doc, "kdevsqlsupport/servers/server" + QString::number( i ), "el" );
        if ( (int)sdb.size() < 6 )
            break;

        conName = "KDEVSQLSUPPORT_";
        conName += QString::number( i );
        conNames << conName;
	QSqlDatabase* db = QSqlDatabase::addDatabase( sdb[0], QString( "KDEVSQLSUPPORT_%1" ).arg( i ) );
        db->setDatabaseName( sdb[1] );
        db->setHostName( sdb[2] );
        bool ok;
        int port = sdb[3].toInt( &ok );
        if ( ok )
            db->setPort( port );
        db->setUserName( sdb[4] );
        db->setPassword( cryptStr( sdb[5] ) );
        db->open();

        i++;
    }

    dbAction->refresh();
}

void SQLSupportPart::projectConfigWidget( KDialogBase *dlg )
{
    QVBox *vbox = dlg->addVBoxPage( QString( "SQL" ), i18n( "Specify your Database Connections" ) );
    SqlConfigWidget *w = new SqlConfigWidget( (QWidget*)vbox, "SQL config widget" );
    w->setProjectDom( projectDom() );
    w->loadConfig();
    connect( dlg, SIGNAL(okClicked()), w, SLOT(accept()) );
    connect( w, SIGNAL(newConfigSaved()), this, SLOT(loadConfig()) );
}

void SQLSupportPart::projectOpened()
{
    connect( project(), SIGNAL( addedFilesToProject( const QStringList & ) ),
             this, SLOT( addedFilesToProject( const QStringList & ) ) );
    connect( project(), SIGNAL( removedFilesFromProject( const QStringList & ) ),
             this, SLOT( removedFilesFromProject( const QStringList & ) ) );

    loadConfig();

    // We want to parse only after all components have been
    // properly initialized
    QTimer::singleShot( 0, this, SLOT( parse() ) );
}


void SQLSupportPart::projectClosed()
{
    clearConfig();
}

void SQLSupportPart::slotRun ()
{
    QString cName = dbAction->currentConnectionName();
    if ( cName.isEmpty() ) {
        KMessageBox::sorry( 0, i18n("Please select a valid database connection.") );
        return;
    }

    KTextEditor::EditInterface* doc = dynamic_cast<KTextEditor::EditInterface*>(partController()->activePart());
    if ( !doc )
        return; // show error message?

    mainWindow()->raiseView( m_widget );
    m_widget->showQuery( cName, doc->text() );
}

void SQLSupportPart::parse()
{
    kdDebug( 9000 ) << "SQLSupport: initialParse()" << endl;

    if ( project() ) {
        kapp->setOverrideCursor( waitCursor );
/*
	QStringList files = project() ->allFiles();
        for ( QStringList::Iterator it = files.begin(); it != files.end() ;++it ) {
            kdDebug( 9014 ) << "maybe parse " << project() ->projectDirectory() + "/" + ( *it ) << endl;
            parse( project() ->projectDirectory() + "/" + *it );
        }
*/
        emit updatedSourceInfo();
        kapp->restoreOverrideCursor();
    } else {
        kdDebug( 9000 ) << "No project" << endl;
    }
}

void SQLSupportPart::addedFilesToProject( const QStringList &fileList )
{
    QStringList::ConstIterator it;

    for ( it = fileList.begin(); it != fileList.end(); ++it ) {
//        parse( project() ->projectDirectory() + "/" + ( *it ) );
    }

    emit updatedSourceInfo();
}


void SQLSupportPart::removedFilesFromProject( const QStringList &fileList )
{
    QStringList::ConstIterator it;

    for ( it = fileList.begin(); it != fileList.end(); ++it ) {
//        classStore() ->removeWithReferences( project() ->projectDirectory() + "/" + ( *it ) );
    }

    emit updatedSourceInfo();
}

void SQLSupportPart::savedFile( const QString &fileName )
{
    if ( project() ->allFiles().contains( fileName.mid ( project() ->projectDirectory().length() + 1 ) ) ) {
//        parse( fileName );
//        emit updatedSourceInfo();
    }
}

KDevLanguageSupport::Features SQLSupportPart::features()
{
    return Features( 0 ); /// @todo...
}

KMimeType::List SQLSupportPart::mimeTypes( )
{
    KMimeType::List list;
    KMimeType::Ptr mime = KMimeType::mimeType( "text/plain" );
    if( mime )
	list << mime;
    return list;
}

#include "sqlsupport_part.moc"
