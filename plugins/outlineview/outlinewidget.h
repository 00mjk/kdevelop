/*
 *   KDevelop outline view
 *   Copyright 2010, 2015 Alex Richardson <alex.richardson@gmx.de>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#pragma once

#include <QWidget>
#include <QModelIndex>

class KRecursiveFilterProxyModel;

class QAction;
class QTreeView;
class QLineEdit;
class OutlineModel;
class OutlineViewPlugin;

class OutlineWidget : public QWidget
{
    Q_OBJECT

public:
    OutlineWidget(QWidget* parent, OutlineViewPlugin* plugin);
    ~OutlineWidget() override;

private:
    OutlineViewPlugin* m_plugin;
    OutlineModel* m_model;
    QTreeView* m_tree;
    KRecursiveFilterProxyModel* m_proxy;
    QLineEdit* m_filter;
    QAction* m_sortAlphabeticallyAction;

    Q_DISABLE_COPY(OutlineWidget)
public slots:
    void activated(QModelIndex);
    void expandFirstLevel();
};
