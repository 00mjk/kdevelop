/***************************************************************************
 *   Copyright (C) 1999-2001 by Bernd Gehrmann and the KDevelop Team       *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "makewidget.h"
#include "kdevcore.h"
#include "kdevmainwindow.h"
#include "kdevproject.h"
#include "kdevpartcontroller.h"
#include "processlinemaker.h"
#include "makeviewpart.h"
#include "makeitem.h"
#include "ktexteditor/document.h"
#include "ktexteditor/cursorinterface.h"
#include "ktexteditor/editinterface.h"

#include <kdebug.h>
#include <klocale.h>
#include <knotifyclient.h>
#include <kprocess.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kinstance.h>
#include <kstatusbar.h>
#include <kapplication.h>
#include <kconfig.h>

#include <qmessagebox.h>
#include <qapplication.h>
#include <qdir.h>
#include <qimage.h>
#include <qstylesheet.h>
#include <qtimer.h>
#include <qfileinfo.h>
#include <qclipboard.h>
#include <qpopupmenu.h>
#include <private/qrichtext_p.h>

#include <stdlib.h>
#include <limits.h>

static const char *const error_xpm[] =
    {
        "11 11 5 1",
        ". c None",
        "# c #313062",
        "a c #6261cd",
        "b c #c50000",
        "c c #ff8583",
        "...........",
        "...####....",
        ".a#bbbb#a..",
        ".#ccbbbb#..",
        "#bccbbbbb#.",
        "#bbbbbbbb#.",
        "#bbbbbbcb#.",
        "#bbbbbccb#.",
        ".#bbbccb#..",
        ".a#bbbb#a..",
        "...####...."
    };


static const char *const warning_xpm[] =
    {
        "11 11 5 1",
        ". c None",
        "# c #313062",
        "a c #6261cd",
        "b c #c5c600",
        "c c #ffff41",
        "...........",
        "...####....",
        ".a#bbbb#a..",
        ".#ccbbbb#..",
        "#bccbbbbb#.",
        "#bbbbbbbb#.",
        "#bbbbbbcb#.",
        "#bbbbbccb#.",
        ".#bbbccb#..",
        ".a#bbbb#a..",
        "...####...."
    };


static const char *const message_xpm[] =
    {
        "11 11 5 1",
        ". c None",
        "b c #3100c5",
        "# c #313062",
        "c c #3189ff",
        "a c #6265cd",
        "...........",
        "...####....",
        ".a#bbbb#a..",
        ".#ccbbbb#..",
        "#bccbbbbb#.",
        "#bbbbbbbb#.",
        "#bbbbbbcb#.",
        "#bbbbbccb#.",
        ".#bbbccb#..",
        ".a#bbbb#a..",
        "...####...."
    };

namespace
{
// code from qt-3.1.2 version of QDir::canonicalPath()
QString canonicalPath( QString const & path )
{
    QString r;
    char cur[PATH_MAX+1];
    if ( ::getcwd( cur, PATH_MAX ) )
    {
        char tmp[PATH_MAX+1];
        if( ::realpath( QFile::encodeName( path ), tmp ) )
        {
            r = QFile::decodeName( tmp );
        }
        //always make sure we go back to the current dir
        ::chdir( cur );
    }
    return r;
}

}

class SelectionPreserver
{
public:
	SelectionPreserver( QTextEdit& textEdit, bool stayAtEnd )
		: m_textEdit( textEdit )
		, m_atEnd( false )
	{
		int para, index;
		m_textEdit.getCursorPosition( &para, &index );

		m_atEnd = stayAtEnd
		      && para == m_textEdit.paragraphs() - 1
		      && index == m_textEdit.paragraphLength( para );

		m_textEdit.getSelection(&paraFrom, &indexFrom, &paraTo, &indexTo, 0);
	}

	~SelectionPreserver()
	{
		m_textEdit.setSelection(paraFrom, indexFrom, paraTo, indexTo, 0);

		if ( m_atEnd )
		{
			m_textEdit.moveCursor(QTextEdit::MoveEnd, false);
			m_textEdit.moveCursor(QTextEdit::MoveLineStart, false);//if linewrap is off we must avoid the jumping of the vertical scrollbar
		}
	}

	QTextEdit& m_textEdit;
	bool m_atEnd;
	int paraFrom, indexFrom, paraTo, indexTo;
};

MakeWidget::MakeWidget(MakeViewPart *part)
	: QTextEdit(0, "make widget")
	, m_directoryStatusFilter( m_errorFilter )
	, m_errorFilter( m_continuationFilter )
	, m_continuationFilter( m_actionFilter )
	, m_actionFilter( m_otherFilter )
	, m_paragraphs(0)
    , m_lastErrorSelected(-1)
	, m_part(part)
	, m_vertScrolling(false)
	, m_horizScrolling(false)
	, m_bCompiling(false)
{
	updateSettingsFromConfig();

	if ( m_bLineWrapping )
		setWordWrap(WidgetWidth);
	setWrapPolicy(Anywhere);
	setReadOnly(true);
	setMimeSourceFactory(new QMimeSourceFactory);
	mimeSourceFactory()->setImage("error", QImage((const char**)error_xpm));
	mimeSourceFactory()->setImage("warning", QImage((const char**)warning_xpm));
	mimeSourceFactory()->setImage("message", QImage((const char**)message_xpm));

	childproc = new KShellProcess("/bin/sh");
	procLineMaker = new ProcessLineMaker( childproc );

	connect( procLineMaker, SIGNAL(receivedStdoutLine(const QString&)),
	         this, SLOT(insertStdoutLine(const QString&) ));
	connect( procLineMaker, SIGNAL(receivedStderrLine(const QString&)),
	         this, SLOT(insertStderrLine(const QString&) ));

	connect( childproc, SIGNAL(processExited(KProcess*)),
	         this, SLOT(slotProcessExited(KProcess*) )) ;

	connect( &m_directoryStatusFilter, SIGNAL(item(EnteringDirectoryItem*)),
	         this, SLOT(slotEnteredDirectory(EnteringDirectoryItem*)) );
	connect( &m_directoryStatusFilter, SIGNAL(item(ExitingDirectoryItem*)),
	         this, SLOT(slotExitedDirectory(ExitingDirectoryItem*)) );
	connect( &m_errorFilter, SIGNAL(item(MakeItem*)),
	         this, SLOT(insertItem(MakeItem*)) );
	connect( &m_actionFilter, SIGNAL(item(MakeItem*)),
	         this, SLOT(insertItem(MakeItem*)) );
	connect( &m_otherFilter, SIGNAL(item(MakeItem*)),
	         this, SLOT(insertItem(MakeItem*)) );

	connect( verticalScrollBar(), SIGNAL(sliderPressed()),
	         this, SLOT(verticScrollingOn()) );
	connect( verticalScrollBar(), SIGNAL(sliderReleased()),
	         this, SLOT(verticScrollingOff()) );
	connect( horizontalScrollBar(), SIGNAL(sliderPressed()),
	         this, SLOT(horizScrollingOn()) );
	connect( horizontalScrollBar(), SIGNAL(sliderReleased()),
	         this, SLOT(horizScrollingOff()) );

	connect( m_part->partController(), SIGNAL(loadedFile(const QString&)),
	         this, SLOT(slotDocumentOpened(const QString&)) );
}

MakeWidget::~MakeWidget()
{
	delete mimeSourceFactory();
	delete childproc;
	delete procLineMaker;
}

void MakeWidget::queueJob(const QString &dir, const QString &command)
{
	commandList.append(command);
	dirList.append(dir);
	if (!isRunning())
	{
		// Store the current output view so that it
		// can be restored after a successful compilation
		m_part->mainWindow()->storeOutputViewTab();
		startNextJob();
	}
}

void MakeWidget::startNextJob()
{
	QStringList::Iterator it = commandList.begin();
	if ( it == commandList.end() )
		return;

	currentCommand = *it;
	commandList.remove(it);

	int i = currentCommand.findRev(" gmake");
	if ( i == -1 )
		i = currentCommand.findRev(" make");
	if ( i == -1 )
		m_bCompiling = false;
	else
	{
		QString s = currentCommand.right(currentCommand.length() - i);
		if ( s.contains("configure ")        ||
		     s.contains(" Makefile.cvs")     ||
		     s.contains(" clean")            ||
		     s.contains(" distclean")        ||
		     s.contains(" package-messages") ||
		     s.contains(" install") )
		{
		    m_bCompiling = false;
		}
		else {
		    m_bCompiling = true;
		}
	}

	it = dirList.begin();
	QString dir = *it;
	dirList.remove(it);

	clear(); // clear the widget
	for ( QValueVector<MakeItem*>::iterator it = m_items.begin(); it != m_items.end(); ++it )
		delete *it;
	m_items.clear();
	m_paragraphToItem.clear();
	m_paragraphs = 0;
	m_lastErrorSelected = -1;

	insertItem( new CommandItem( currentCommand ) );

	childproc->clearArguments();
	*childproc << currentCommand;
	childproc->start(KProcess::NotifyOnExit, KProcess::AllOutput);

	dirstack.clear();
	dirstack.push(new QString(dir));

	m_part->mainWindow()->raiseView(this);
	m_part->core()->running(m_part, true);
}

void MakeWidget::killJob()
{
	childproc->kill();
}

bool MakeWidget::isRunning()
{
	return childproc->isRunning();
}

void MakeWidget::copy()
{
	int parafrom=0, indexfrom=0, parato=0, indexto=0;
    getSelection(&parafrom, &indexfrom, &parato, &indexto);
    if( parafrom < 0 || indexfrom < 0 || parato < 0 || indexto < 0
    || ((parafrom == parato) && (indexfrom == indexto)) )
    {
        return;
    }

	QString selection;
	for(int i = parafrom; i<=parato; i++)
		selection += text(i) + "\n";

	selection.remove(0, indexfrom);
	int removeend = text(parato).length() - indexto;

	selection.remove((selection.length()-1) - removeend, removeend);

#if QT_VERSION >= 0x030100
	kapp->clipboard()->setText(selection, QClipboard::Clipboard);
#else
	kapp->clipboard()->setText(selection);
#endif
}

void MakeWidget::nextError()
{
	int parag;
	if (m_lastErrorSelected != -1)
		parag = m_lastErrorSelected;
	else
		parag = 0;

	for ( int it = parag + 1;
#if QT_VERSION >= 0x030100
	      it < (int)m_items.count();
#else
	      it < m_items.size();
#endif
	      ++it )
	{
		ErrorItem* item = dynamic_cast<ErrorItem*>( m_paragraphToItem[it] );
		if ( !item )
			continue;
		parag = it;
		document()->removeSelection(0);
		setSelection(parag, 0, parag+1, 0, 0);
		setCursorPosition(parag, 0);
		ensureCursorVisible();
		searchItem( it );
		return;
	}

	KNotifyClient::beep();
}

void MakeWidget::prevError()
{
	int parag;
	if (m_lastErrorSelected != -1)
		parag = m_lastErrorSelected;
	else
		parag = 0;

	for ( int it = parag - 1; it >= 0; --it)
	{
		ErrorItem* item = dynamic_cast<ErrorItem*>( m_paragraphToItem[it] );
		if ( !item )
			continue;
		parag = it;
		document()->removeSelection(0);
		setSelection(parag, 0, parag+1, 0, 0);
		setCursorPosition(parag, 0);
		ensureCursorVisible();
		searchItem( it );
		return;
	}

	KNotifyClient::beep();
}

void MakeWidget::contentsMouseReleaseEvent( QMouseEvent* e )
{
	QTextEdit::contentsMouseReleaseEvent(e);
	if ( e->button() != LeftButton )
		return;
	searchItem(paragraphAt(e->pos()));
}

void MakeWidget::keyPressEvent(QKeyEvent *e)
{
	if (e->key() == Key_Return || e->key() == Key_Enter)
	{
		int parag, index;
		getCursorPosition(&parag, &index);
		searchItem(parag);
	}
	else
		QTextEdit::keyPressEvent(e);
}

// returns the current directory for parag
QString MakeWidget::directory(int parag) const
{
	QValueVector<MakeItem*>::const_iterator it = qFind( m_items.begin(), m_items.end(), m_paragraphToItem[parag] );
	if ( it == m_items.end() )
		return QString::null;
	// run backwards over directories and figure out where we are
	while ( it != m_items.begin() ) {
		--it;
		EnteringDirectoryItem* edi = dynamic_cast<EnteringDirectoryItem*>( *it );
		if ( edi )
			return edi->directory + "/";
	}
	return QString::null;
}

// hackish function that will return true and put string "file" in "fName" if the file
// exists
static bool checkFileExists( const QString& file, QString& fName )
{
    if ( QFile::exists( file ) ) {
        fName = file;
        return true;
    }
    return false;
}

QString MakeWidget::guessFileName( const QString& fName, int parag ) const
{
    // pathological case
    if ( ! m_part->project() ) return fName;

    QString name;
    QString dir = directory( parag );

    if ( fName.startsWith( "/" ) )
    {
        // absolute path given
        name = fName;
    }
    else if ( !dir.isEmpty() )
    {
        name = dir + fName;
    }
    else
    {
        // now it gets tricky - no directory navigation messages,
        // no absolute path - let's guess.
        name = fName;
        if ( !checkFileExists( m_part->project()->projectDirectory() + "/" + fName, name ) &&
	     !checkFileExists( m_part->project()->projectDirectory() + "/" + m_part->project()->activeDirectory() + "/" + fName, name ) )
            checkFileExists( m_part->project()->buildDirectory() + "/" + fName, name );
    }

    kdDebug(9004) << "Opening file: " << fName << endl;

    // GNU make resolves symlinks. if "name" is a real path to a file the
    // project know by symlink path, we need to return the symlink path
    QStringList projectFiles = m_part->project()->allFiles();
    QStringList::iterator it = projectFiles.begin();
    while ( it != projectFiles.end() )
    {
        QString file = m_part->project()->projectDirectory() + "/" + *it;
        if ( name == canonicalPath( file ) )
        {
            kdDebug(9004) << "Found file in project - " << file << " == " << name << endl;
            return file;
        }
        ++it;
    }

    // this should only happen if the file is not in the project
    return name;
}

void MakeWidget::searchItem(int parag)
{
	ErrorItem* item = dynamic_cast<ErrorItem*>( m_paragraphToItem[parag] );
	if ( item )
	{
		// open the file
		kdDebug(9004) << "Opening file: " << guessFileName(item->fileName, parag) << endl;
		if (item->m_cursor) {
			uint line, col;
			item->m_cursor->position(&line, &col);
			kdDebug() << "Cursor new position: " << col << endl;
			m_part->partController()->editDocument(guessFileName(item->fileName, parag), line, col);
		} else {
			m_part->partController()->editDocument(guessFileName(item->fileName, parag), item->lineNum);
		}
		m_part->mainWindow()->statusBar()->message( item->m_error, 10000 );
		m_part->mainWindow()->lowerView(this);
        m_lastErrorSelected = parag;
	}
}

void MakeWidget::insertStdoutLine( const QString& line )
{
	if ( !appendToLastLine( line ) )
		m_directoryStatusFilter.processLine( line );
}

void MakeWidget::insertStderrLine( const QString& line )
{
	if ( !appendToLastLine( line ) )
		m_errorFilter.processLine( line );
}

void MakeWidget::slotProcessExited(KProcess *)
{
	if (childproc->normalExit())
	{
		if (childproc->exitStatus())
		{
			KNotifyClient::event( "ProcessError", i18n("The process has finished with errors"));
			emit m_part->commandFailed(currentCommand);
		}
		else
		{
			KNotifyClient::event( "ProcessSuccess", i18n("The process has finished successfully"));
			emit m_part->commandFinished(currentCommand);
		}
	}

	MakeItem* item = new ExitStatusItem( childproc->normalExit(), childproc->exitStatus() );
	insertItem( item );

	m_part->mainWindow()->statusBar()->message( QString("%1: %2").arg(currentCommand).arg(item->m_text), 3000);
	m_part->core()->running(m_part, false);

	// Defensive programming: We emit this with a single shot timer so that we go once again
	// through the event loop. After that, we can be sure that the process is really finished
	// and its KProcess object can be reused.
	if (childproc->normalExit() && !childproc->exitStatus())
	{
		QTimer::singleShot(0, this, SLOT(startNextJob()));
		if (commandList.isEmpty())
			// The last command on the list was successful so restore the
			// output view to what it had before the compilation process started
			m_part->mainWindow()->restoreOutputViewTab();
	}
	else
	{
		commandList.clear();
		dirList.clear();
	}
}

void MakeWidget::slotEnteredDirectory( EnteringDirectoryItem* item )
{
//	kdDebug(9004) << "Entering dir: " << item->directory << endl;
	QString* dir = new QString( item->directory );
	dirstack.push( dir );
	insertItem( item );
}

void MakeWidget::slotExitedDirectory( ExitingDirectoryItem* item )
{
	QString eDir = item->directory;
//	kdDebug(9004) << "Leaving dir: " << eDir << endl;
	QString *dir = dirstack.pop();
	if (!dir)
	{
		kdWarning(9004) << "Left more directories than entered: " << eDir;
	}
	else if (dir->compare(eDir) != 0)
	{
		kdWarning(9004) << "Expected directory: \"" << *dir << "\" but got \"" << eDir << "\"" << endl;
	}
	insertItem( item );
	delete dir;
}

bool MakeWidget::appendToLastLine( const QString& text )
{
#if QT_VERSION >= 0x030100
	if ( m_items.count() == 0 )
#else
	if ( m_items.size() == 0 )
#endif
		return false;
#if QT_VERSION >= 0x030100
	MakeItem* item = m_items[m_items.count() - 1];
#else
	MakeItem* item = m_items[m_items.size() - 1];
#endif
	if ( !item->append( text ) )
		return false;

	SelectionPreserver preserveSelection( *this, !m_vertScrolling && !m_horizScrolling );

	removeParagraph( paragraphs() - 1 );
	append( item->formattedText( m_compilerOutputLevel, brightBg() ) );



	return true;
}

void MakeWidget::insertItem( MakeItem* item )
{
	ErrorItem* e = dynamic_cast<ErrorItem*>(item);
	if (e)
		createCursor(e, 0L);

	m_items.push_back( item );

	if ( m_bCompiling && !item->visible( m_compilerOutputLevel ) )
		return;

	SelectionPreserver preserveSelection( *this, !m_vertScrolling && !m_horizScrolling );

	m_paragraphToItem.insert( m_paragraphs++, item );
	append( item->formattedText( m_compilerOutputLevel, brightBg() ) );
}

void MakeWidget::slotDocumentOpened( const QString & filename )
{
	KParts::Part* part = m_part->partController()->findOpenDocument(filename);
	KTextEditor::Document* doc = dynamic_cast<KTextEditor::Document*>(part);

	if (!doc) {
		kdWarning() << k_funcinfo << "Couldn't find the document that was just opened." << endl;
		return;
	}

	connect(part, SIGNAL(destroyed(QObject*)), this, SLOT(slotDocumentClosed(QObject*)) );

	for (QValueVector<MakeItem*>::Iterator it = m_items.begin(); it != m_items.end(); ++it) {
		ErrorItem* e = dynamic_cast<ErrorItem*>(*it);

		if (!e || e->m_cursor) continue;

		if (filename.endsWith(e->fileName))
			createCursor(e, doc);
	}
}

void MakeWidget::createCursor(ErrorItem* e, KTextEditor::Document* doc)
{
	// Disabled for now - comment out to test. You need an up-to-date cvs head katepart to avoid mem leaks
    return;

	// try to get a KTextEditor::Cursor, so that we can retain position in
	// a document even when it is edited
	if (!doc)
		doc = dynamic_cast<KTextEditor::Document*>(m_part->partController()->findOpenDocument(guessFileName(e->fileName, m_paragraphs + 1)));

	if (doc) {
		KTextEditor::EditInterface* edit = dynamic_cast<KTextEditor::EditInterface*>(doc);
		KTextEditor::CursorInterface* cursor = dynamic_cast<KTextEditor::CursorInterface*>(doc);
		if (cursor) {
			e->m_cursor = cursor->createCursor();
			uint col = 0;
			static QRegExp startOfText("[\\S]");
			if (edit) {
				int newcol = edit->textLine(e->lineNum).find(startOfText);
				if (newcol != -1) col = uint(newcol);
			}
			e->m_cursor->setPosition(e->lineNum, col);
			e->m_doc = doc;
		}
	}
}

void MakeWidget::slotDocumentClosed(QObject* doc)
{
	KTextEditor::Document* document = static_cast<KTextEditor::Document*>(doc);

	for (QValueVector<MakeItem*>::Iterator it = m_items.begin(); it != m_items.end(); ++it) {
		ErrorItem* e = dynamic_cast<ErrorItem*>(*it);

		if (!e || e->m_doc != document) continue;

		e->m_cursor = 0L;
		e->m_doc = 0L;
	}
}

bool MakeWidget::brightBg()
{
	int h,s,v;
	paletteBackgroundColor().hsv( &h, &s, &v );
	return (v > 127);
}

QPopupMenu* MakeWidget::createPopupMenu( const QPoint& pos )
{
	QPopupMenu* pMenu = QTextEdit::createPopupMenu(pos);
	pMenu->setCheckable(true);

	pMenu->insertSeparator();
	int id = pMenu->insertItem(i18n("Line Wrapping"), this, SLOT(toggleLineWrapping()) );
	pMenu->setItemChecked(id, m_bLineWrapping);

	pMenu->insertSeparator();
	id = pMenu->insertItem(i18n("Very Short Compiler Output"), this, SLOT(slotVeryShortCompilerOutput()) );
	pMenu->setItemChecked(id, m_compilerOutputLevel == eVeryShort);
	id = pMenu->insertItem(i18n("Short Compiler Output"), this, SLOT(slotShortCompilerOutput()) );
	pMenu->setItemChecked(id, m_compilerOutputLevel == eShort);
	id = pMenu->insertItem(i18n("Full Compiler Output"), this, SLOT(slotFullCompilerOutput()) );
	pMenu->setItemChecked(id, m_compilerOutputLevel == eFull);

	pMenu->insertSeparator();
	id = pMenu->insertItem(i18n("Show Directory Navigation Messages"), this, SLOT(toggleShowDirNavigMessages()));
	pMenu->setItemChecked(id, DirectoryItem::getShowDirectoryMessages());

	return pMenu;
}

void MakeWidget::toggleLineWrapping()
{
	m_bLineWrapping = !m_bLineWrapping;
	KConfig *pConfig = kapp->config();
	pConfig->setGroup("MakeOutputView");
	pConfig->writeEntry("LineWrapping", m_bLineWrapping);
	pConfig->sync();
	if (m_bLineWrapping) {
		setWordWrap(WidgetWidth);
	}
	else
	{
		setWordWrap(NoWrap);
	}
}

void MakeWidget::refill()
{
	clear();
	m_paragraphToItem.clear();
	m_paragraphs = 0;
	for( uint i = 0; i < m_items.size(); i++ )
	{
		if ( m_bCompiling && !m_items[i]->visible( m_compilerOutputLevel ) )
			continue;
		m_paragraphToItem.insert( m_paragraphs++, m_items[i] );
		append( m_items[i]->formattedText( m_compilerOutputLevel, brightBg() ) );
	}

}

void MakeWidget::slotVeryShortCompilerOutput() { setCompilerOutputLevel(eVeryShort); }
void MakeWidget::slotShortCompilerOutput() { setCompilerOutputLevel(eShort); }
void MakeWidget::slotFullCompilerOutput() { setCompilerOutputLevel(eFull); }

void MakeWidget::setCompilerOutputLevel(EOutputLevel level)
{
	m_compilerOutputLevel = level;
	KConfig *pConfig = kapp->config();
	pConfig->setGroup("MakeOutputView");
	pConfig->writeEntry("CompilerOutputLevel", (int) level);
	pConfig->sync();
	refill();
}

void MakeWidget::toggleShowDirNavigMessages()
{
	DirectoryItem::setShowDirectoryMessages( !DirectoryItem::getShowDirectoryMessages() );
	KConfig *pConfig = kapp->config();
	pConfig->setGroup("MakeOutputView");
	pConfig->writeEntry("ShowDirNavigMsg", DirectoryItem::getShowDirectoryMessages());
	pConfig->sync();
	refill();
}

void MakeWidget::updateSettingsFromConfig()
{
	KConfig *pConfig = kapp->config();
	pConfig->setGroup("MakeOutputView");
	setFont(pConfig->readFontEntry("Messages Font"));
	m_bLineWrapping = pConfig->readBoolEntry("LineWrapping", true);
	m_compilerOutputLevel = (EOutputLevel) pConfig->readNumEntry("CompilerOutputLevel", (int) eVeryShort);
	DirectoryItem::setShowDirectoryMessages( pConfig->readBoolEntry("ShowDirNavigMsg", false) );
}

#include "makewidget.moc"
