/***************************************************************************
 *   Copyright (C) 2003 by Jens Dagerbo                                    *
 *   jens.dagerbo@swipnet.se                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

//BEGIN Includes

#include <dcopclient.h>
#include <kapplication.h>
#include <kparts/part.h>
#include <kparts/componentfactory.h>
#include <klibloader.h>
#include <ktrader.h>
#include <kurl.h>
#include <kurlrequester.h>
#include <klineedit.h>
#include <kdebug.h>
#include <ktexteditor/editinterface.h>
#include <ktexteditor/editor.h>
#include <kdevcore.h>
#include <kdevmainwindow.h>
#include <kdevproject.h>
#include <kdevpartcontroller.h>
#include <kcombobox.h>
#include <klocale.h>

#include <qlayout.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qradiobutton.h>
#include <qstringlist.h>
#include <qptrlist.h>
#include <qregexp.h>
#include <qdialog.h>
#include <qfile.h>
#include <qdir.h>
#include <qtextstream.h>
#include <qdatastream.h>

#include <sys/types.h>
#include <unistd.h>

#include "replace_part.h"
#include "replace_widget.h"
#include "replacedlgimpl.h"
#include "replaceitem.h"
#include "replaceview.h"

//END Includes

ReplaceWidget::ReplaceWidget(ReplacePart *part)
        : QWidget(0, "replace widget"), m_part( part ),
        m_dialog( new ReplaceDlgImpl( this, "replace widget" ) ),
        _terminateOperation( false )
{
    // setup outputview
    QVBoxLayout * layout = new QVBoxLayout( this );
    QHBoxLayout * buttonlayout = new QHBoxLayout( layout );

    _cancel = new QPushButton( i18n("Cancel"), this );
    _replace = new QPushButton( i18n("Replace"), this );

    _cancel->setEnabled( false );
    _replace->setEnabled( false );

    buttonlayout->addWidget( _cancel );
    buttonlayout->addWidget( _replace );

    _listview = new ReplaceView( this );
    layout->addWidget( _listview );

    //  setup signals
    connect( m_dialog->find_button, SIGNAL( clicked() ), SLOT( find() ) );

    connect( _replace, SIGNAL( clicked() ), SLOT( replace() ) );
    connect( _cancel, SIGNAL( clicked() ), SLOT( clear() ) );
    connect( _listview, SIGNAL( editDocument( const QString &, int ) ), SLOT( editDocument( const QString &, int ) ) );

    connect( m_part->core(), SIGNAL( stopButtonClicked( KDevPlugin * ) ), SLOT( stopButtonClicked( KDevPlugin * ) ) );
}

//BEGIN Slots

void ReplaceWidget::showDialog()
{
    if ( ! m_part->project() )
        return; // This shouldn't happen, since the part only has project scope

    m_dialog->show( m_part->project()->projectDirectory() + "/" + m_part->project()->activeDirectory() + "/" );
}

void ReplaceWidget::find()
{
    _listview->clear();
    m_part->mainWindow()->raiseView(this);

    _listview->setReplacementData( m_dialog->expressionPattern(), m_dialog->replacementString() );

    if ( showReplacements() )
    {
        _cancel->setEnabled( true );
        _replace->setEnabled( true );
    }
    else
    {
        clear();
    }
}

void ReplaceWidget::replace()
{
    makeReplacements();
    clear();
}

void ReplaceWidget::clear()
{
    _listview->clear();

    _cancel->setEnabled( false );
    _replace->setEnabled( false );
}

void ReplaceWidget::editDocument( QString const & file, int line )
{
    m_part->partController()->editDocument( file, line );
}

void ReplaceWidget::stopButtonClicked( KDevPlugin * which )
{
    if ( which != 0 && which != m_part )
        return;
        
    _terminateOperation = true;
}

//END Slots

bool ReplaceWidget::showReplacements()
{
    ReplaceItem::s_listview_done = false;

    m_part->core()->running( m_part, true );

    bool completed = true;

    QStringList files = workFiles();
    QStringList openfiles = openProjectFiles();

    QStringList::ConstIterator it = files.begin();
    while ( it != files.end() )
    {
        if ( shouldTerminate() )
        {
            completed = false;
            break;
        }

        if ( openfiles.contains( *it ) )
        {
            if ( KTextEditor::EditInterface * ei = getEditInterfaceForFile( *it ) )
            {
                QString buffer = ei->text();
                QTextIStream stream( &buffer );
                _listview->showReplacementsForFile( stream, *it );
            }
        }
        else
        {
            QFile file( *it );
            if ( file.open ( IO_ReadOnly ) )
            {
                QTextStream stream( &file );
                _listview->showReplacementsForFile( stream, *it );
            }
        }
        ++it;

        kapp->processEvents( 100 );
    }

    m_part->core()->running( m_part, false );

    ReplaceItem::s_listview_done = true;

    return completed;
}

bool ReplaceWidget::makeReplacements()
{
    m_part->core()->running( m_part, true );

    bool completed = true;

    QStringList openfiles = openProjectFiles();

    ReplaceItem const * fileitem = _listview->firstChild();
    while ( fileitem )
    {
        // is this a good idea? should the replace operation be interruptable?
//         if ( shouldTerminate() )
//         {
//             completed = false;
//             break;
//         }

        if ( fileitem->isOn() )
        {
            QString currentfile = fileitem->file();

            if ( openfiles.contains( currentfile ) )
            {
                if ( KTextEditor::EditInterface * ei = getEditInterfaceForFile( currentfile ) )
                {
                    QString ibuffer = ei->text();
                    QString obuffer;
                    QTextStream istream( &ibuffer, IO_ReadOnly );
                    QTextStream ostream( &obuffer, IO_WriteOnly );

                    _listview->makeReplacementsForFile( istream, ostream, fileitem );

                    ei->removeText( 0, 0, ei->numLines()-1, UINT_MAX );
                    ei->insertText( 0, 0, obuffer );
                }
            }
            else
            {
                QString newfile = currentfile + "_kdevreplace_tempfile";
                QFile ifile( currentfile );
                QFile ofile( newfile );
                if ( ifile.open( IO_ReadOnly ) && ofile.open( IO_WriteOnly ) )
                {
                    QTextStream istream( &ifile );
                    QTextStream ostream( &ofile );

                    _listview->makeReplacementsForFile( istream, ostream, fileitem );

                    ifile.close();
                    ofile.close();

                    QDir().rename( newfile, currentfile, true );
                }
            }
            //Telling the project about the edited file
            m_part->project()->changedFile( relativeProjectPath( ( currentfile ) ) );
        }

        fileitem = fileitem->nextSibling();

        kapp->processEvents( 100 );
    }

    m_part->partController()->saveAllFiles();

    //BEGIN *** remove this ***
    //TODO - remove this when the parser supports the changedFile() signal
    QCString appName;
    appName.sprintf( "gideon-%d", getpid() );

    QCString replyType;
    QByteArray data, replyData;
    QDataStream arg(  data, IO_WriteOnly );

    (void) kapp->dcopClient()->call( appName,
                                     "KDevCppSupport",
                                     "parseProject()",
                                     data, replyType, replyData );
    //END *** remove this ***

    m_part->core()->running( m_part, false );

    return completed;
}

//BEGIN Helpers

QStringList ReplaceWidget::workFiles()
{
    if ( m_dialog->files_all_radio->isChecked() )
    {
        return allProjectFiles();
    }
    else if ( m_dialog->files_open_radio->isChecked() )
    {
        return openProjectFiles();
    }
    return subProjectFiles( m_dialog->path_urlreq->lineEdit()->text() );
}

QString ReplaceWidget::relativeProjectPath( QString path )
{
    QString project = m_part->project()->projectDirectory() + "/";
    if ( path.left( project.length() ) == project )
    {
        path = path.mid( project.length() );
    }
    return path;
}

QString ReplaceWidget::fullProjectPath( QString path )
{
    QString project = m_part->project()->projectDirectory() + "/";
    if ( path.left( project.length() ) != project )
    {
        path = project + path;
    }
    return path;
}


QStringList ReplaceWidget::allProjectFiles()
{
    QStringList allfiles = m_part->project()->allFiles();

    QStringList::iterator it = allfiles.begin();
    while ( it != allfiles.end() )
    {
        *it = fullProjectPath( *it );
        ++it;
    }
    return allfiles;
}

QStringList ReplaceWidget::subProjectFiles( QString const & subpath )
{
    QStringList projectfiles = allProjectFiles();

    QStringList::Iterator it = projectfiles.begin();
    while ( it != projectfiles.end() )
    {
        if ( (*it).left( subpath.length() ) != subpath)
        {
            it = projectfiles.remove( it );
        }
        else
        {
            ++it;
        }
    }
    return projectfiles;
}

QStringList ReplaceWidget::openProjectFiles()
{
    QStringList projectfiles = allProjectFiles();
    QStringList openfiles;

    if( const QPtrList<KParts::Part> * partlist = m_part->
            partController()->parts() )
    {
        QPtrListIterator<KParts::Part> it( *partlist );
        while ( KParts::Part* part = it.current() )
        {
            if ( KTextEditor::Editor * ed = dynamic_cast<KTextEditor::Editor *>( part ) )
            {
                QString editorpath = ed->url().path();
                if ( projectfiles.contains( editorpath ) )
                {
                    openfiles.append( editorpath );
                }
            }
            ++it;
        }
    }
    return openfiles;
}

KTextEditor::EditInterface * ReplaceWidget::getEditInterfaceForFile( QString const & file )
{
    if( const QPtrList<KParts::Part> * partlist = m_part->
            partController()->parts() )
    {
        QPtrListIterator<KParts::Part> it( *partlist );
        while ( KParts::Part* part = it.current() )
        {
            if ( KTextEditor::Editor * ed = dynamic_cast<KTextEditor::Editor *>( part ) )
            {
                if ( file == ed->url().path() )
                {
                    return dynamic_cast<KTextEditor::EditInterface *>( part );
                }
            }
            ++it;
        }
    }
    return 0;
}

bool ReplaceWidget::shouldTerminate()
{
    bool b = _terminateOperation;
    _terminateOperation = false;
    return b;
}

//END Helpers

#include "replace_widget.moc"
