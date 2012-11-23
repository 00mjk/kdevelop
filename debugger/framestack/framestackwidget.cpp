/*
 * This file is part of KDevelop
 *
 * Copyright 1999 John Birch <jbb@kdevelop.org>
 * Copyright 2007 Hamish Rodda <rodda@kde.org>
 * Copyright 2009 Niko Sams <niko.sams@gmail.com>
 * Copyright 2009 Aleix Pol <aleixpol@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
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

#include "framestackwidget.h"

#include <QHeaderView>
#include <QScrollBar>
#include <QListView>
#include <QVBoxLayout>
#include <QLabel>
#include <QTreeView>
#include <QMenu>
#include <QApplication>
#include <QClipboard>

#include <KAction>
#include <KStandardAction>
#include <KDebug>
#include <KLocalizedString>
#include <KIcon>
#include <KTextEditor/Cursor>

#include <interfaces/icore.h>
#include <interfaces/idebugcontroller.h>
#include <interfaces/idocumentcontroller.h>
#include "framestackmodel.h"

namespace KDevelop {

FramestackWidget::FramestackWidget(IDebugController* controller, QWidget* parent)
: QSplitter(Qt::Horizontal, parent), m_session(0)
{
    connect(controller,
            SIGNAL(currentSessionChanged(KDevelop::IDebugSession*)),
            SLOT(currentSessionChanged(KDevelop::IDebugSession*)));
    connect(controller, SIGNAL(raiseFramestackViews()), SIGNAL(requestRaise()));

    setWhatsThis(i18n("<b>Frame stack</b><p>"
                    "Often referred to as the \"call stack\", "
                    "this is a list showing which function is "
                    "currently active, and what called each "
                    "function to get to this point in your "
                    "program. By clicking on an item you "
                    "can see the values in any of the "
                    "previous calling functions.</p>"));
    setWindowIcon(KIcon("view-list-text"));
    m_threadsWidget = new QWidget(this);
    m_threads = new QListView(m_threadsWidget);
    m_frames = new QTreeView(this);
    m_frames->setRootIsDecorated(false);
    m_frames->setSelectionMode(QAbstractItemView::ContiguousSelection);
    m_frames->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_frames->setAllColumnsShowFocus(true);
    m_frames->setContextMenuPolicy(Qt::CustomContextMenu);

    m_framesContextMenu = new QMenu(m_frames);

    KAction *selectAllAction = KStandardAction::selectAll(m_frames);
    selectAllAction->setShortcut(KShortcut()); //FIXME: why does CTRL-A conflict with Katepart (while CTRL-Cbelow doesn't) ?
    selectAllAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(selectAllAction, SIGNAL(triggered()), SLOT(selectAll()));
    m_framesContextMenu->addAction(selectAllAction);

    KAction *copyAction = KStandardAction::copy(m_frames);
    copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(copyAction, SIGNAL(triggered()), SLOT(copySelection()));
    m_framesContextMenu->addAction(copyAction);
    addAction(copyAction);

    connect(m_frames, SIGNAL(customContextMenuRequested(QPoint)), SLOT(frameContextMenuRequested(QPoint)));

    m_threadsWidget->setLayout(new QVBoxLayout());
    m_threadsWidget->layout()->addWidget(new QLabel(i18n("Threads:")));
    m_threadsWidget->layout()->addWidget(m_threads);
    m_threadsWidget->hide();
    addWidget(m_threadsWidget);
    addWidget(m_frames);

    setStretchFactor(1, 3);
    connect(m_frames->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(checkFetchMoreFrames()));
    connect(m_threads, SIGNAL(clicked(QModelIndex)), this, SLOT(setThreadShown(QModelIndex)));
    connect(m_frames, SIGNAL(clicked(QModelIndex)),
            SLOT(frameClicked(QModelIndex)));
}

FramestackWidget::~FramestackWidget() {}

void FramestackWidget::currentSessionChanged(KDevelop::IDebugSession* session)
{
    kDebug() << "Adding session:" << isVisible();

    m_session = session;

    m_threads->setModel(session ? session->frameStackModel() : 0);
    m_frames->setModel(session ? session->frameStackModel() : 0);

    if (session) {
        connect(session->frameStackModel(), SIGNAL(currentThreadChanged(int)),
                SLOT(currentThreadChanged(int)));
        currentThreadChanged(session->frameStackModel()->currentThread());
        connect(session->frameStackModel(), SIGNAL(currentFrameChanged(int)),
                SLOT(currentFrameChanged(int)));
        currentFrameChanged(session->frameStackModel()->currentFrame());
        connect(session, SIGNAL(stateChanged(KDevelop::IDebugSession::DebuggerState)),
                SLOT(sessionStateChanged(KDevelop::IDebugSession::DebuggerState)));
    }

    if (isVisible()) {
        showEvent(0);
    }
}

void KDevelop::FramestackWidget::hideEvent(QHideEvent* e)
{
    QWidget::hideEvent(e);
}

void KDevelop::FramestackWidget::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);
}

void KDevelop::FramestackWidget::setThreadShown(const QModelIndex& idx)
{
    m_session->frameStackModel()->setCurrentThread(idx);
}

void KDevelop::FramestackWidget::checkFetchMoreFrames()
{
    int val = m_frames->verticalScrollBar()->value();
    int max = m_frames->verticalScrollBar()->maximum();
    const int offset = 20;

    if (val + offset > max && m_session) {
        m_session->frameStackModel()->fetchMoreFrames();
    }
}

void KDevelop::FramestackWidget::currentThreadChanged(int thread)
{
    if (thread != -1) {
        IFrameStackModel* model = m_session->frameStackModel();
        QModelIndex idx = model->currentThreadIndex();
        m_threads->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        m_threadsWidget->setVisible(model->rowCount() > 1);
        m_frames->setModel(m_session->frameStackModel());
        m_frames->setRootIndex(idx);
        m_frames->header()->setResizeMode(0, QHeaderView::ResizeToContents);
    } else {
        m_threadsWidget->hide();
        m_threads->selectionModel()->clear();
        m_frames->setModel(0);
    }
}

void KDevelop::FramestackWidget::currentFrameChanged(int frame)
{
    if (frame != -1) {
        IFrameStackModel* model = m_session->frameStackModel();
        QModelIndex idx = model->currentFrameIndex();
        m_frames->selectionModel()->select(
            idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    } else {
        m_frames->selectionModel()->clear();
    }
}

void FramestackWidget::frameClicked(const QModelIndex& idx)
{
    IFrameStackModel::FrameItem f = m_session->frameStackModel()->frame(idx);
    /* If line is -1, then it's not a source file at all.  */
    if (f.line != -1) {
        QPair<KUrl, int> file = m_session->convertToLocalUrl(qMakePair(f.file, f.line));
        ICore::self()->documentController()->openDocument(file.first, KTextEditor::Cursor(file.second, 0));
    }

    m_session->frameStackModel()->setCurrentFrame(f.nr);
}

void FramestackWidget::frameContextMenuRequested(const QPoint& pos)
{
    m_framesContextMenu->popup( m_frames->mapToGlobal(pos) + QPoint(0, m_frames->header()->height()) );
}

void FramestackWidget::copySelection()
{
    QClipboard *cb = QApplication::clipboard();
    QModelIndexList indexes = m_frames->selectionModel()->selectedRows();
    QString content;
    Q_FOREACH( QModelIndex index, indexes) {
        IFrameStackModel::FrameItem frame = m_session->frameStackModel()->frame(index);
        if (frame.line == -1) {
            content += QString("#%1 %2() at %3\n").arg(frame.nr).arg(frame.name).arg(frame.file.pathOrUrl(KUrl::RemoveTrailingSlash));
        } else {
            content += QString("#%1 %2() at %3:%4\n").arg(frame.nr).arg(frame.name).arg(frame.file.pathOrUrl(KUrl::RemoveTrailingSlash)).arg(frame.line+1);
        }
    }
    cb->setText(content);
}

void FramestackWidget::selectAll()
{
    m_frames->selectAll();
}

void FramestackWidget::sessionStateChanged(KDevelop::IDebugSession::DebuggerState state)
{
    bool enable = state == IDebugSession::PausedState || state == IDebugSession::StoppedState;
    m_frames->setEnabled(enable);
    m_threads->setEnabled(enable);
}

}

#include "framestackwidget.moc"
