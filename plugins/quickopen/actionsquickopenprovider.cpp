/*
 * This file is part of KDevelop
 *
 * Copyright 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
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

#include "actionsquickopenprovider.h"

#include <KActionCollection>
#include <KLocalizedString>
#include <QIcon>
#include <QAction>
#include <QRegularExpression>

using namespace KDevelop;

class ActionsQuickOpenItem
    : public QuickOpenDataBase
{
public:
    ActionsQuickOpenItem(const QString& display, QAction* action)
        : QuickOpenDataBase()
        , m_action(action)
        , m_display(display)
    {}

    QString text() const override
    {
        return m_display;
    }
    QString htmlDescription() const override
    {
        QString desc = m_action->toolTip();
        const QKeySequence shortcut = m_action->shortcut();
        if (!shortcut.isEmpty()) {
            desc = i18nc("description (shortcut)", "%1 (%2)", desc, shortcut.toString());
        }
        return desc;
    }
    bool execute(QString&) override
    {
        m_action->trigger();
        return true;
    }
    QIcon icon() const override
    {
        // note: not the best icon, but can't find anything better
        static const QIcon fallbackIcon = QIcon::fromTheme("system-run");

        const QIcon icon = m_action->icon();
        if (icon.isNull()) {
            return fallbackIcon;
        }

        return icon;
    }

private:
    QAction* m_action;

    ///needed because it won't have the "E&xit" ampersand
    QString m_display;
};

ActionsQuickOpenProvider::ActionsQuickOpenProvider()
{
}

void ActionsQuickOpenProvider::setFilterText(const QString& text)
{
    if (text.size() < 2) {
        return;
    }
    m_results.clear();
    const QList<KActionCollection*> collections = KActionCollection::allCollections();
    QRegularExpression mnemonicRx(QStringLiteral("^(.*)&(.+)$"));
    for (KActionCollection* c : collections) {
        const QList<QAction*> actions = c->actions();
        for (QAction* action : actions) {
            if (!action->isEnabled()) {
                continue;
            }

            QString display = action->text();
            QRegularExpressionMatch match = mnemonicRx.match(display);
            if (match.hasMatch()) {
                display = match.captured(1) + match.captured(2);
            }

            if (display.contains(text, Qt::CaseInsensitive)) {
                m_results += QuickOpenDataPointer(new ActionsQuickOpenItem(display, action));
            }
        }
    }
}

uint ActionsQuickOpenProvider::unfilteredItemCount() const
{
    uint ret = 0;
    const QList<KActionCollection*> collections = KActionCollection::allCollections();
    for (KActionCollection* c : collections) {
        ret += c->count();
    }

    return ret;
}

QuickOpenDataPointer ActionsQuickOpenProvider::data(uint row) const
{
    return m_results.at(row);
}

uint ActionsQuickOpenProvider::itemCount() const
{
    return m_results.count();
}

void ActionsQuickOpenProvider::reset()
{
    m_results.clear();
}
