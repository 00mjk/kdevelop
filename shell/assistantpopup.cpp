/*
   Copyright 2009 David Nolden <david.nolden.kdevelop@art-master.de>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "assistantpopup.h"
#include <QVBoxLayout>
#include <QToolButton>
#include <QLabel>
#include <klocalizedstring.h>
#include <KAction>

void AssistantPopup::updateActions() {
    m_assistantActions = m_assistant->actions();
    if (!m_assistant->title().isEmpty()) {
        QLabel* title = new QLabel("<b>" + m_assistant->title() + ":<b>");
        title->setTextFormat(Qt::RichText);
        addWidget(title);
    }
    foreach(KDevelop::IAssistantAction::Ptr action, m_assistantActions)
        addWidget(widgetForAction(action));
    addSeparator();
    addWidget(widgetForAction(KDevelop::IAssistantAction::Ptr()));
    resize(sizeHint());
    move((parentWidget()->width() - width())/2, parentWidget()->height() - height());
}

AssistantPopup::AssistantPopup(QWidget* parent, KDevelop::IAssistant::Ptr assistant) : QToolBar(parent), m_assistant(assistant) {
    Q_ASSERT(assistant);
    setAutoFillBackground(true);
    updateActions();
}

QWidget* AssistantPopup::widgetForAction(KDevelop::IAssistantAction::Ptr action) {
    QToolButton* button = new QToolButton;
    KAction* realAction;
    QString buttonText;
    int index = m_assistantActions.indexOf(action);
    if(index == -1) {
        realAction = new KAction(button);
        buttonText = "&0 - " + i18n("Hide");
    }
    else {
        realAction = action->toKAction();
        buttonText = QString("&%1 - ").arg(index+1) + action->description();
    }
    realAction->setParent(button);
    connect(realAction, SIGNAL(triggered(bool)), SLOT(executeHideAction()));
    button->setDefaultAction(realAction);
    button->setText(buttonText);
    return button;
}

void AssistantPopup::executeHideAction() {
    m_assistant->doHide();
}

KSharedPtr< KDevelop::IAssistant > AssistantPopup::assistant() const {
    return m_assistant;
}

#include "assistantpopup.moc"
