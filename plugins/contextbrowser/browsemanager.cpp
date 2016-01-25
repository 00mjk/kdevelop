/*
  * This file is part of KDevelop
 *
 * Copyright 2008 David Nolden <david.nolden.kdevelop@art-master.de>
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

#include "browsemanager.h"

#include <QApplication>
#include <QMouseEvent>
#include <QTimer>

#include <KTextEditor/Document>
#include <KTextEditor/View>
#include <interfaces/icore.h>
#include <interfaces/idocumentcontroller.h>
#include "contextbrowserview.h"
#include <interfaces/ilanguagecontroller.h>
#include <language/interfaces/ilanguagesupport.h>
#include <language/duchain/duchainutils.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/duchain.h>
#include <language/duchain/declaration.h>
#include <ktexteditor/codecompletioninterface.h>
#include <language/duchain/functiondefinition.h>
#include <language/duchain/forwarddeclaration.h>

#include "contextbrowser.h"
#include "debug.h"

using namespace KDevelop;
using namespace KTextEditor;

EditorViewWatcher::EditorViewWatcher(QObject* parent)
    : QObject(parent)
{
    connect(ICore::self()->documentController(), &IDocumentController::textDocumentCreated, this, &EditorViewWatcher::documentCreated);
    foreach(KDevelop::IDocument* document, ICore::self()->documentController()->openDocuments())
        documentCreated(document);
}

void EditorViewWatcher::documentCreated( KDevelop::IDocument* document ) {
    KTextEditor::Document* textDocument = document->textDocument();
    if(textDocument) {
        connect(textDocument, &Document::viewCreated, this, &EditorViewWatcher::viewCreated);
        foreach(KTextEditor::View* view, textDocument->views()) {
            Q_ASSERT(view->parentWidget());
            addViewInternal(view);
        }
    }
}

void EditorViewWatcher::addViewInternal(KTextEditor::View* view) {
    m_views << view;
    viewAdded(view);
    connect(view, &View::destroyed, this, &EditorViewWatcher::viewDestroyed);
}

void EditorViewWatcher::viewAdded(KTextEditor::View*) {
}

void EditorViewWatcher::viewDestroyed(QObject* view) {
    m_views.removeAll(static_cast<KTextEditor::View*>(view));
}

void EditorViewWatcher::viewCreated(KTextEditor::Document* /*doc*/, KTextEditor::View* view) {
    Q_ASSERT(view->parentWidget());
    addViewInternal(view);
}

QList<KTextEditor::View*> EditorViewWatcher::allViews() {
    return m_views;
}

void BrowseManager::eventuallyStartDelayedBrowsing() {
    if(m_browsingByKey && m_browingStartedInView)
        emit startDelayedBrowsing(m_browingStartedInView);
}

BrowseManager::BrowseManager(ContextBrowserPlugin* controller)
    : QObject(controller)
    , m_plugin(controller)
    , m_browsingByKey(0)
    , m_watcher(this)
{
    m_delayedBrowsingTimer = new QTimer(this);
    m_delayedBrowsingTimer->setSingleShot(true);

    connect(m_delayedBrowsingTimer, &QTimer::timeout, this, &BrowseManager::eventuallyStartDelayedBrowsing);

    foreach(KTextEditor::View* view, m_watcher.allViews())
        viewAdded(view);
}

KTextEditor::View* viewFromWidget(QWidget* widget) {
    if(!widget)
        return 0;
    KTextEditor::View* view = qobject_cast<KTextEditor::View*>(widget);
    if(view)
        return view;
    else
        return viewFromWidget(widget->parentWidget());
}

bool BrowseManager::eventFilter(QObject * watched, QEvent * event) {
    QWidget* widget = qobject_cast<QWidget*>(watched);
    Q_ASSERT(widget);
    KTextEditor::View* view = viewFromWidget(widget);
    if(!view)
        return false;

    QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);

    const int browseKey = Qt::Key_Control;
    const int magicModifier = Qt::Key_Alt;

    //Eventually start key-browsing
    if(keyEvent && (keyEvent->key() == browseKey || keyEvent->key() == magicModifier) && !m_browsingByKey && keyEvent->type() == QEvent::KeyPress) {
        m_browsingByKey = keyEvent->key();

        if(keyEvent->key() == magicModifier) {
            if(dynamic_cast<KTextEditor::CodeCompletionInterface*>(view) && dynamic_cast<KTextEditor::CodeCompletionInterface*>(view)->isCompletionActive())
            {
                //Do nothing, completion is active.
            }else{
                m_delayedBrowsingTimer->start(300);
                m_browingStartedInView = view;
            }
        }
    }

    QFocusEvent* focusEvent = dynamic_cast<QFocusEvent*>(event);
    //Eventually stop key-browsing
    if((keyEvent && m_browsingByKey && keyEvent->key() == m_browsingByKey && keyEvent->type() == QEvent::KeyRelease)
        || (focusEvent && focusEvent->lostFocus())) {
        m_browsingByKey = 0;
        emit stopDelayedBrowsing();
    }

    QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);

    if(mouseEvent) {
        if (mouseEvent->type() == QEvent::MouseButtonPress && mouseEvent->button() == Qt::XButton1) {
            m_plugin->historyPrevious();
            return true;
        }
        if (mouseEvent->type() == QEvent::MouseButtonPress && mouseEvent->button() == Qt::XButton2) {
            m_plugin->historyNext();
            return true;
        }
    }

    if(!m_browsingByKey) {
        resetChangedCursor();
        return false;
    }

    if(mouseEvent) {
        KTextEditor::View* iface = dynamic_cast<KTextEditor::View*>(view);
        if(!iface) {
            qCDebug(PLUGIN_CONTEXTBROWSER) << "Update kdelibs for the browsing-mode to work";
            return false;
        }

        QPoint coordinatesInView = widget->mapTo(view, mouseEvent->pos());

        KTextEditor::Cursor textCursor = iface->coordinatesToCursor(coordinatesInView);
        if(textCursor.isValid()) {
            ///@todo find out why this is needed, fix the code in kate
            if(textCursor.column() > 0)
                textCursor.setColumn(textCursor.column()-1);

            QUrl viewUrl = view->document()->url();
            auto languages = ICore::self()->languageController()->languagesForUrl(viewUrl);

            QPair<QUrl, KTextEditor::Cursor> jumpTo;

            //Step 1: Look for a special language object(Macro, included header, etc.)
            foreach (const auto language, languages) {
                jumpTo = language->specialLanguageObjectJumpCursor(viewUrl, KTextEditor::Cursor(textCursor));
                if(jumpTo.first.isValid() && jumpTo.second.isValid())
                    break; //Found a special object to jump to
            }

            //Step 2: Look for a declaration/use
            if(!jumpTo.first.isValid() || !jumpTo.second.isValid()) {
                Declaration* foundDeclaration = 0;
                KDevelop::DUChainReadLocker lock( DUChain::lock() );
                foundDeclaration = DUChainUtils::declarationForDefinition( DUChainUtils::itemUnderCursor(view->document()->url(), KTextEditor::Cursor(textCursor)) );

                if(foundDeclaration && (foundDeclaration->url().toUrl() == view->document()->url()) && foundDeclaration->range().contains( foundDeclaration->transformToLocalRevision(KTextEditor::Cursor(textCursor)))) {
                    ///A declaration was clicked directly. Jumping to it is useless, so jump to the definition or something useful

                    bool foundBetter = false;

                    Declaration* definition = FunctionDefinition::definition(foundDeclaration);
                    if(definition) {
                        foundDeclaration = definition;
                        foundBetter = true;
                    }
                    ForwardDeclaration* forward = dynamic_cast<ForwardDeclaration*>(foundDeclaration);
                    if(forward) {
                        TopDUContext* standardContext = DUChainUtils::standardContextForUrl(view->document()->url());
                        if(standardContext) {
                            Declaration* resolved = forward->resolve(standardContext);
                            if(resolved) {
                                foundDeclaration = resolved; //This probably won't work
                                foundBetter = true;
                            }
                        }
                    }
                    //This will do a search without visibility-restriction, and that search will prefer non forward declarations
                    if(!foundBetter) {
                        Declaration* betterDecl = foundDeclaration->id().getDeclaration(0);
                        if(betterDecl) {
                            foundDeclaration = betterDecl;
                            foundBetter = true;
                        }
                    }
                }

                if( foundDeclaration ) {
                    jumpTo.first = foundDeclaration->url().toUrl();
                    jumpTo.second = foundDeclaration->rangeInCurrentRevision().start();
                }
            }
            if(jumpTo.first.isValid() && jumpTo.second.isValid()) {
                if(mouseEvent->button() == Qt::LeftButton) {
                    if(mouseEvent->type() == QEvent::MouseButtonPress) {
                        m_buttonPressPosition = textCursor;
//                         view->setCursorPosition(textCursor);
//                         return false;
                    }else if(mouseEvent->type() == QEvent::MouseButtonRelease && textCursor == m_buttonPressPosition) {
                        ICore::self()->documentController()->openDocument(jumpTo.first, jumpTo.second);
//                         event->accept();
//                         return true;
                    }
                }else if(mouseEvent->type() == QEvent::MouseMove) {
                    //Make the cursor a "hand"
                    setHandCursor(widget);
                    return false;
                }
            }
        }
        resetChangedCursor();
    }
    return false;
}

void BrowseManager::resetChangedCursor() {
    QMap<QPointer<QWidget>, QCursor> cursors = m_oldCursors;
    m_oldCursors.clear();

    for(QMap<QPointer<QWidget>, QCursor>::iterator it = cursors.begin(); it != cursors.end(); ++it)
        if(it.key())
            it.key()->setCursor(QCursor(Qt::IBeamCursor));
}

void BrowseManager::setHandCursor(QWidget* widget) {
    if(m_oldCursors.contains(widget))
        return; //Nothing to do
    m_oldCursors[widget] = widget->cursor();
    widget->setCursor(QCursor(Qt::PointingHandCursor));
}

void BrowseManager::applyEventFilter(QWidget* object, bool install) {
    if(install)
        object->installEventFilter(this);
    else
        object->removeEventFilter(this);

    foreach(QObject* child, object->children())
        if(qobject_cast<QWidget*>(child))
            applyEventFilter(qobject_cast<QWidget*>(child), install);
}

void BrowseManager::viewAdded(KTextEditor::View* view) {
    applyEventFilter(view, true);
    //We need to listen for cursorPositionChanged, to clear the shift-detector. The problem: Kate listens for the arrow-keys using shortcuts,
    //so those keys are not passed to the event-filter

    // can't use new signal/slot syntax here, these signals are only defined in KateView
    // TODO: should we really depend on kate internals here?
    connect(view, SIGNAL(navigateLeft()), m_plugin, SLOT(navigateLeft()));
    connect(view, SIGNAL(navigateRight()), m_plugin, SLOT(navigateRight()));
    connect(view, SIGNAL(navigateUp()), m_plugin, SLOT(navigateUp()));
    connect(view, SIGNAL(navigateDown()), m_plugin, SLOT(navigateDown()));
    connect(view, SIGNAL(navigateAccept()), m_plugin, SLOT(navigateAccept()));
    connect(view, SIGNAL(navigateBack()), m_plugin, SLOT(navigateBack()));
}

void Watcher::viewAdded(KTextEditor::View* view) {
    m_manager->viewAdded(view);
}

Watcher::Watcher(BrowseManager* manager)
    : EditorViewWatcher(manager), m_manager(manager) {
    foreach(KTextEditor::View* view, allViews())
        m_manager->applyEventFilter(view, true);
}


