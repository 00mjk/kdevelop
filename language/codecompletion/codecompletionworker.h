/*
 * KDevelop Generic Code Completion Support
 *
 * Copyright 2006-2008 Hamish Rodda <rodda@kde.org>
 * Copyright 2007-2008 David Nolden <david.nolden.kdevelop@art-master.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KDEV_CODECOMPLETIONWORKER_H
#define KDEV_CODECOMPLETIONWORKER_H

#include <QThread>
#include <QList>

#include <KTextEditor/Cursor>
#include <KTextEditor/Range>

#include "language/languageexport.h"
#include "language/duchain/duchainpointer.h"

class QMutex;

namespace KTextEditor {
  class Range;
  class View;
  class Cursor;
}

namespace KDevelop
{

class CodeCompletion;

class CompletionTreeElement;

class KDEVPLATFORMLANGUAGE_EXPORT CodeCompletionWorker : public QThread
{
  Q_OBJECT

  public:
    CodeCompletionWorker(QObject* parent);
    virtual ~CodeCompletionWorker();

    void abortCurrentCompletion();

  Q_SIGNALS:
    void foundDeclarations(QList<KSharedPtr<CompletionTreeElement> >, void* completionContext);

  protected:
    virtual void run();
    
    virtual void computeCompletions(DUContextPointer context, const KTextEditor::Cursor& position, KTextEditor::View* view, const KTextEditor::Range& contextRange, const QString& contextText) = 0;

    bool& aborting();
    
  protected Q_SLOTS:
    void computeCompletions(KDevelop::DUContextPointer context, const KTextEditor::Cursor& position, KTextEditor::View* view);

  private:
    QMutex* m_mutex;
    bool m_abort;
};

}

#endif // KDEVCODECOMPLETIONWORKER_H
