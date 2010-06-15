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

#ifndef EXTERNALSCRIPTJOB_H
#define EXTERNALSCRIPTJOB_H

#include <QtCore/QProcess>
#include <outputview/outputjob.h>

#include "externalscriptitem.h"

#include <KTextEditor/Range>

namespace KDevelop
{
class ProcessLineMaker;
class OutputModel;
class Document;
}

namespace KTextEditor
{
class Document;
}

class KProcess;

class ExternalScriptJob : public KDevelop::OutputJob
{
  Q_OBJECT

public:
  ExternalScriptJob( ExternalScriptItem* item, QObject* parent );
  virtual void start();
  virtual bool doKill();
  KDevelop::OutputModel* model();

private slots:
  void processError( QProcess::ProcessError );
  void processFinished( int, QProcess::ExitStatus );

private:
  void appendLine( const QString &l );
  KProcess* m_proc;
  KDevelop::ProcessLineMaker* m_lineMaker;
  ExternalScriptItem::ReplaceMode m_replaceMode;
  ExternalScriptItem::InputMode m_inputMode;
  KTextEditor::Document* m_document;
  /// invalid when whole doc should be replaced
  KTextEditor::Range m_selectionRange;
};

#endif // EXTERNALSCRIPTJOB_H

// kate: indent-mode cstyle; space-indent on; indent-width 2; replace-tabs on;
