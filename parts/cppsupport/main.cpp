

#include "driver.h"
#include "ast.h"
#include "lexer.h"
#include "tag_creator.h"

#include <qfileinfo.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qregexp.h>
#include <qprocess.h>

#include <catalog.h>
#include <kstandarddirs.h>

#include <iostream>
#include <stdlib.h>
#include <unistd.h>

class RppDriver: public Driver
{
public:
    RppDriver( Catalog* c )
        : catalog( c ), m_generateTags( true )
    {
        setup();
    }

    virtual ~RppDriver()
    {
        TagCreator::destroyDocumentation();
    }

    void setGenerateTags( bool b )
    {
        m_generateTags = b;
    }

    void addDocDirectory( const QString& dir )
    {
        m_docDirectoryList.append( dir );
        TagCreator::setDocumentationDirectories( m_docDirectoryList );
    }

    void fileParsed( const QString& fileName )
    {
        std::cout << (m_generateTags ? "generate tags for " : "checking ") << fileName << std::endl;

        QValueList<Problem> l = problems( fileName );
        QValueList<Problem>::Iterator it = l.begin();
        while( it != l.end() ){
            const Problem& p = *it;
            ++it;
            std::cout << fileName << ":" << p.line() << ":" << p.column() << ": " << p.text() << std::endl;
        }

        TranslationUnitAST::Node ast = takeTranslationUnit( fileName );

        if( m_generateTags ){
            TagCreator w( fileName, catalog );
            w.parseTranslationUnit( ast.get() );
        }
    }

    void setupLexer( Lexer* lex )
    {
        //lex->disableSkipWords();
        Driver::setupLexer( lex );
    }

    // setup the preprocessor
    // code provided by Reginald Stadlbauer <reggie@trolltech.com>
    void setup()
    {
	QString kdedir = getenv( "KDEDIR" );
	if( !kdedir.isNull() )
	    addIncludePath( kdedir + "/include" );

	QString qtdir = getenv( "QTDIR" );
	if( !qtdir.isNull() )
	    addIncludePath( qtdir + "/include" );

	QString qmakespec = getenv( "QMAKESPEC" );
	if ( qmakespec.isNull() )
	    qmakespec = "linux-g++";
	// #### implement other mkspecs and find a better way to find the
	// #### proper mkspec (althoigh this will be no fun :-)

	addIncludePath( qtdir + "/mkspecs/" + qmakespec );

	if ( qmakespec == "linux-g++" ) {
	    addIncludePath( "/include" );
	    addIncludePath( "/usr/include" );
	    addIncludePath( "/ust/local/include" );
	    QProcess proc;
	    proc.addArgument( "gcc" );
	    proc.addArgument( "-print-file-name=include" );
	    if ( !proc.start() ) {
		std::cerr << "*error* Couldn't start gcc" << std::endl;
		return;
	    }
	    while ( proc.isRunning() )
		usleep( 1 );

            QString gccLibPath = proc.readStdout();
            gccLibPath = gccLibPath.replace( QRegExp("[\r\n]"), "" );
	    addIncludePath( gccLibPath );
	    addIncludePath( "/usr/include/g++-3" );
	    addIncludePath( "/usr/include/g++" );
	    proc.clearArguments();
	    proc.addArgument( "gcc" );
	    proc.addArgument( "-E" );
	    proc.addArgument( "-dM" );
	    proc.addArgument( "-ansi" );
	    proc.addArgument( "-" );
	    if ( !proc.start() ) {
		std::cerr << "*error* Couldn't start gcc" << std::endl;
		return;
	    }
	    while ( !proc.isRunning() )
		usleep( 1 );
	    proc.closeStdin();
	    while ( proc.isRunning() )
		usleep( 1 );
	    while ( proc.canReadLineStdout() ) {
		QString l = proc.readLineStdout();
		QStringList lst = QStringList::split( ' ', l );
		if ( lst.count() != 3 )
		    continue;
		addMacro( Macro( lst[1], lst[2] ) );
	    }
	    addMacro( Macro( "__cplusplus", "1" ) );
	} else if ( qmakespec == "win32-borland" ) {
	    QString incl = getenv( "INCLUDE" );
	    QStringList includePaths = QStringList::split( ';', incl );
	    QStringList::Iterator it = includePaths.begin();
	    while( it != includePaths.end() ){
		addIncludePath( (*it).stripWhiteSpace() );
		++it;
	    }
	    // ### I am sure there are more standard include paths on
	    // ### windows. I will fix that soon
	    // ### Also do the compiler specific defines on windows
	}
    }

private:
    Catalog* catalog;
    bool m_generateTags;
    QStringList m_docDirectoryList;
};

void parseDirectory( Driver& driver, QDir& dir, bool rec, bool parseAllFiles )
{
    {
	QStringList fileList;
	if( parseAllFiles )
	    fileList = dir.entryList( QDir::Files );
	else
	    fileList = dir.entryList( "*.h;*.H;*.hh;*.hxx;*.hpp;*.tlh" );

	QStringList::Iterator it = fileList.begin();
	while( it != fileList.end() ){
	    QString fn = dir.path() + "/" + (*it);
	    ++it;

	    driver.parseFile( fn );
	}
    }

    if( rec ) {
	QStringList fileList = dir.entryList( QDir::Dirs );
	QStringList::Iterator it = fileList.begin();
	while( it != fileList.end() ){
	    if( (*it).startsWith(".") ){
		++it;
		continue;
	    }

	    QDir subdir( dir.path() + "/" + (*it) );
	    ++it;

	    parseDirectory( driver, subdir, rec, parseAllFiles );
	}
    }
}

int main( int argc, char* argv[] )
{
    KStandardDirs stddir;

    if( argc < 3 ){
        std::cerr << "usage: r++ dbname directories..." << std::endl << std::endl;
        return -1;
    }

    bool rec = false;
    bool parseAllFiles = false;

    QString datadir = stddir.localkdedir() + "/" + KStandardDirs::kde_default( "data" );

    if( !QFile::exists(datadir + "/kdevcppsupport/pcs/") ){
        std::cerr << "*error* " << datadir + "/kdevcppsupport/pcs/" << " doesn't exists!!" << std::endl << std::endl;
        return -1;
    }

    QString dbFileName = datadir + "/kdevcppsupport/pcs/" + argv[ 1 ] + ".db";
    // std::cout << "dbFileName = " << dbFileName << std::endl;
    if( QFile::exists(dbFileName) ){
        std::cerr << "*error* " << "database " << dbFileName << " already exists!" << std::endl << std::endl;
        return -1;
    }


    Catalog catalog;
    catalog.open( dbFileName );
    catalog.addIndex( "kind" );
    catalog.addIndex( "name" );
    catalog.addIndex( "scope" );
    catalog.addIndex( "fileName" );

    RppDriver driver( &catalog );
    driver.setResolveDependencesEnabled( true );

    for( int i=2; i<argc; ++i ){
        QString s( argv[i] );
        if( s == "-r" || s == "--recursive" ){
           rec = true;
           continue;
       } else if( s == "-a" || s == "--all" ){
	   parseAllFiles = true;
	   continue;
       } else if( s == "-f" || s == "--fast" ){
	   driver.setResolveDependencesEnabled( false );
	   continue;
       } else if( s == "-c" || s == "--check-only" ){
           driver.setGenerateTags( false );
	   continue;
       }  else if ( s.startsWith("-d") ){
           driver.addDocDirectory( s.mid(2) );
           continue;
       }

       QDir dir( s );
       if( !dir.exists() ){
           std::cerr<< "*error* " << "the directory " << dir.path() << " doesn't exists!" << std::endl << std::endl;
           continue;
       }

       parseDirectory( driver, dir, rec, parseAllFiles );
    }

    return 0;
}
