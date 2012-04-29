/*  This file is part of KDevelop
    Copyright 2009 Andreas Pakulat <apaku@gmx.de>
    Copyright 2010 Milian Wolff <mail@milianw.de>

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

#include "externalscriptjob.h"

#include "externalscriptitem.h"
#include "externalscriptoutputmodel.h"
#include "externalscriptdebug.h"
#include "externalscriptplugin.h"

#include <QFileInfo>
#include <QApplication>

#include <KProcess>
#include <KLocalizedString>
#include <KMessageBox>
#include <KShell>

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <outputview/outputmodel.h>
#include <outputview/outputdelegate.h>
#include <util/processlinemaker.h>

#include <interfaces/icore.h>
#include <interfaces/iuicontroller.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/iproject.h>


ExternalScriptJob::ExternalScriptJob( ExternalScriptItem* item, ExternalScriptPlugin* parent )
    : KDevelop::OutputJob( parent ),
    m_plugin( parent ),
    m_proc( 0 ), m_lineMaker( 0 ),
    m_outputMode( item->outputMode() ), m_inputMode( item->inputMode() ),
    m_errorMode( item->errorMode() ),
    m_document( 0 ), m_selectionRange( KTextEditor::Range::invalid() ),
    m_showOutput( item->showOutput() )
{
  debug() << "creating external script job";

  setCapabilities( Killable );
  setStandardToolView( KDevelop::IOutputView::RunView );
  setBehaviours( KDevelop::IOutputView::AllowUserClose | KDevelop::IOutputView::AutoScroll );
  ExternalScriptOutputModel* model = new ExternalScriptOutputModel( this );
  
  /// TODO implement some functionality where filtering strategy can be selected via the GUI
  model->setFilteringStrategy(KDevelop::OutputModel::ScriptErrorFilter);
  setModel( model, KDevelop::IOutputView::TakeOwnership );
  setDelegate( m_plugin->delegate() );

  // also merge when error mode "equals" output mode
  if ( (m_outputMode == ExternalScriptItem::OutputInsertAtCursor
          && m_errorMode == ExternalScriptItem::ErrorInsertAtCursor) ||
       (m_outputMode == ExternalScriptItem::OutputReplaceDocument
          && m_errorMode == ExternalScriptItem::ErrorReplaceDocument) ||
       (m_outputMode == ExternalScriptItem::OutputReplaceSelectionOrDocument
          && m_errorMode == ExternalScriptItem::ErrorReplaceSelectionOrDocument) ||
       (m_outputMode == ExternalScriptItem::OutputReplaceSelectionOrInsertAtCursor
          && m_errorMode == ExternalScriptItem::ErrorReplaceSelectionOrInsertAtCursor) ||
       // also these two otherwise they clash...
       (m_outputMode == ExternalScriptItem::OutputReplaceSelectionOrInsertAtCursor
          && m_errorMode == ExternalScriptItem::ErrorReplaceSelectionOrDocument) ||
       (m_outputMode == ExternalScriptItem::OutputReplaceSelectionOrDocument
          && m_errorMode == ExternalScriptItem::ErrorReplaceSelectionOrInsertAtCursor) )
  {
    m_errorMode = ExternalScriptItem::ErrorMergeOutput;
  }

  KDevelop::IDocument* active = KDevelop::ICore::self()->documentController()->activeDocument();

  if ( m_outputMode != ExternalScriptItem::OutputNone || m_inputMode != ExternalScriptItem::InputNone
       || m_errorMode != ExternalScriptItem::ErrorNone )
  {
    if ( !active || !active->isTextDocument() || !active->textDocument()->activeView() ) {
      KMessageBox::error( QApplication::activeWindow(),
                          i18n( "Cannot run script '%1' since it tries to access "
                                "the editor contents but no document is open.", item->text() ),
                          i18n( "No Document Open" )
                        );
      return;
    }

    m_document = active->textDocument();

    connect( m_document, SIGNAL(aboutToClose(KTextEditor::Document*)),
             this, SLOT(kill()) );

    m_selectionRange = m_document->activeView()->selectionRange();
    m_cursorPosition = m_document->activeView()->cursorPosition();
  }

  if ( item->saveMode() == ExternalScriptItem::SaveCurrentDocument && active ) {
    active->save();
  } else if ( item->saveMode() == ExternalScriptItem::SaveAllDocuments ) {
    foreach ( KDevelop::IDocument* doc, KDevelop::ICore::self()->documentController()->openDocuments() ) {
      doc->save();
    }
  }

  QString command = item->command();
  QString workingDir = item->workingDirectory();
  
  if(item->performParameterReplacement())
    command.replace( "%i", QString::number( QCoreApplication::applicationPid() ) );

  if ( active ) {
    const KUrl url = active->url();

    if ( url.isLocalFile() && workingDir.isEmpty() ) {
      ///TODO: make configurable, use fallback to project dir
      workingDir = active->url().directory();
    }

    ///TODO: make those placeholders escapeable
    if(item->performParameterReplacement())
    {
      command.replace( "%u", KShell::quoteArg( url.pathOrUrl() ) );

      ///TODO: does that work with remote files?
      QFileInfo info( url.pathOrUrl() );

      command.replace( "%f", KShell::quoteArg( info.filePath() ) );
      command.replace( "%b", KShell::quoteArg( info.baseName() ) );
      command.replace( "%n", KShell::quoteArg( info.fileName() ) );
      command.replace( "%d", KShell::quoteArg( info.path() ) );

      if ( active->textDocument() && active->textDocument()->activeView() && active->textDocument()->activeView()->selection() ) {
        command.replace( "%s", KShell::quoteArg( active->textDocument()->activeView()->selectionText() ) );
      }

      if ( KDevelop::IProject* project = KDevelop::ICore::self()->projectController()->findProjectForUrl( url ) ) {
        command.replace( "%p", project->folder().pathOrUrl() );
      }
    }
  }

  m_proc = new KProcess( this );
  if ( !workingDir.isEmpty() ) {
    m_proc->setWorkingDirectory( workingDir );
  }
  m_lineMaker = new KDevelop::ProcessLineMaker( m_proc, this );
  connect( m_lineMaker, SIGNAL(receivedStdoutLines(QStringList)),
           model, SLOT(appendStdoutLines(QStringList)) );
  connect( m_lineMaker, SIGNAL(receivedStderrLines(QStringList)),
           model, SLOT(appendStderrLines(QStringList)) );
  connect( m_proc, SIGNAL(error(QProcess::ProcessError)),
           SLOT(processError(QProcess::ProcessError)) );
  connect( m_proc, SIGNAL(finished(int,QProcess::ExitStatus)),
           SLOT(processFinished(int,QProcess::ExitStatus)) );

  // Now setup the process parameters
  debug() << "setting command:" << command;

  if ( m_errorMode == ExternalScriptItem::ErrorMergeOutput ) {
    m_proc->setOutputChannelMode( KProcess::MergedChannels );
  } else {
    m_proc->setOutputChannelMode( KProcess::SeparateChannels );
  }
  m_proc->setShellCommand( command );

  setObjectName( command );
}

void ExternalScriptJob::start()
{
  debug() << "launching?" << m_proc;

  if ( m_proc ) {
    if ( m_showOutput ) {
      startOutput();
    }
    appendLine( i18n( "Running external script: %1", m_proc->program().join( " " ) ) );
    m_proc->start();

    if ( m_inputMode != ExternalScriptItem::InputNone ) {
      QString inputText;

      switch ( m_inputMode ) {
        case ExternalScriptItem::InputNone:
          // do nothing;
          break;
        case ExternalScriptItem::InputSelectionOrNone:
          if ( m_selectionRange.isValid() ) {
            inputText = m_document->text(m_selectionRange);
          } // else nothing
          break;
        case ExternalScriptItem::InputSelectionOrDocument:
          if ( m_selectionRange.isValid() ) {
            inputText = m_document->text(m_selectionRange);
          } else {
            inputText = m_document->text();
          }
          break;
        case ExternalScriptItem::InputDocument:
          inputText = m_document->text();
          break;
      }

      ///TODO: what to do with the encoding here?
      ///      maybe ask Christoph for what kate returns...
      m_proc->write( inputText.toUtf8() );

      m_proc->closeWriteChannel();
    }
  } else {
    kWarning() << "No process, something went wrong when creating the job";
    // No process means we've returned early on from the constructor, some bad error happened
    emitResult();
  }
}

bool ExternalScriptJob::doKill()
{
  if ( m_proc ) {
    m_proc->kill();
    appendLine( i18n( "*** Killed Application ***" ) );
  }

  return true;
}

void ExternalScriptJob::processFinished( int exitCode , QProcess::ExitStatus status )
{
  m_lineMaker->flushBuffers();

  if ( exitCode == 0 && status == QProcess::NormalExit ) {

    ExternalScriptOutputModel* model = dynamic_cast<ExternalScriptOutputModel*>(OutputJob::model());
    Q_ASSERT(model);
    //model->addPending();

    if ( m_outputMode != ExternalScriptItem::OutputNone ) {
      QStringList lines = model->stdOut();

      if ( !lines.isEmpty() ) {
        QString output = lines.join( "\n" );
        switch ( m_outputMode ) {
          case ExternalScriptItem::OutputNone:
            // do nothing;
            break;
          case ExternalScriptItem::OutputCreateNewFile:
            KDevelop::ICore::self()->documentController()->openDocumentFromText( output );
            break;
          case ExternalScriptItem::OutputInsertAtCursor:
            m_document->insertText( m_cursorPosition, output );
            break;
          case ExternalScriptItem::OutputReplaceSelectionOrInsertAtCursor:
            if ( m_selectionRange.isValid() ) {
              m_document->replaceText( m_selectionRange, output );
            } else {
              m_document->insertText( m_cursorPosition, output );
            }
            break;
          case ExternalScriptItem::OutputReplaceSelectionOrDocument:
            if ( m_selectionRange.isValid() ) {
              m_document->replaceText( m_selectionRange, output );
            } else {
              m_document->setText( output );
            }
            break;
          case ExternalScriptItem::OutputReplaceDocument:
            m_document->setText( output );
            break;
        }
      }
    }
    if ( m_errorMode != ExternalScriptItem::ErrorNone && m_errorMode != ExternalScriptItem::ErrorMergeOutput ) {
      QString output = model->stdErr().join( "\n" );

      if ( !output.isEmpty() ) {
        switch ( m_errorMode ) {
          case ExternalScriptItem::ErrorNone:
          case ExternalScriptItem::ErrorMergeOutput:
            // do nothing;
            break;
          case ExternalScriptItem::ErrorCreateNewFile:
            KDevelop::ICore::self()->documentController()->openDocumentFromText( output );
            break;
          case ExternalScriptItem::ErrorInsertAtCursor:
            m_document->insertText( m_cursorPosition, output );
            break;
          case ExternalScriptItem::ErrorReplaceSelectionOrInsertAtCursor:
            if ( m_selectionRange.isValid() ) {
              m_document->replaceText( m_selectionRange, output );
            } else {
              m_document->insertText( m_cursorPosition, output );
            }
            break;
          case ExternalScriptItem::ErrorReplaceSelectionOrDocument:
            if ( m_selectionRange.isValid() ) {
              m_document->replaceText( m_selectionRange, output );
            } else {
              m_document->setText( output );
            }
            break;
          case ExternalScriptItem::ErrorReplaceDocument:
            m_document->setText( output );
            break;
        }
      }
    }

    appendLine( i18n( "*** Exited normally ***" ) );
  } else {
    if ( status == QProcess::NormalExit )
      appendLine( i18n( "*** Exited with return code: %1 ***", QString::number( exitCode ) ) );
    else
      if ( error() == KJob::KilledJobError )
        appendLine( i18n( "*** Process aborted ***" ) );
      else
        appendLine( i18n( "*** Crashed with return code: %1 ***", QString::number( exitCode ) ) );
  }

  debug() << "Process done";

  emitResult();
}

void ExternalScriptJob::processError( QProcess::ProcessError error )
{
  if ( error == QProcess::FailedToStart ) {
    setError( -1 );
    QString errmsg =  i18n( "Could not start program '%1'. Make sure that the "
                            "path is specified correctly.", m_proc->program().join( " " ) );
    KMessageBox::error( QApplication::activeWindow(), errmsg, i18n( "Could not start application" ) );
    setErrorText( errmsg );
    emitResult();
  }

  debug() << "Process error";
}

void ExternalScriptJob::appendLine( const QString& l )
{
  if ( KDevelop::OutputModel* m = model() ) {
    m->appendLine( l );
  }
}

KDevelop::OutputModel* ExternalScriptJob::model()
{
  return dynamic_cast<KDevelop::OutputModel*>( OutputJob::model() );
}


#include "externalscriptjob.moc"

// kate: indent-mode cstyle; space-indent on; indent-width 2; replace-tabs on;
