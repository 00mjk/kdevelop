/* This file is part of KDevelop

    Copyright 2008 Andreas Pakulat <apaku@gmx.de>
    Copyright 2010 David Nolden <david.nolden.kdevelop@art-master.de>

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

#ifndef SESSIONCHOOSERDIALOG_H
#define SESSIONCHOOSERDIALOG_H

#include <QDialog>
#include <QTimer>

class QDialogButtonBox;
class QModelIndex;
class QListView;
class QLineEdit;
class QAbstractItemModel;

namespace KDevelop {

class SessionChooserDialog : public QDialog
{
    Q_OBJECT
public:
    SessionChooserDialog(QListView* view, QAbstractItemModel* model, QLineEdit* filter);

    bool eventFilter(QObject* object, QEvent* event) override;

    QWidget* mainWidget() const;

public Q_SLOTS:
    void updateState();
    void doubleClicked(const QModelIndex& index);
    void filterTextChanged();

private Q_SLOTS:
    void deleteButtonPressed();
    void showDeleteButton();
    void itemEntered(const QModelIndex& index);

private:
    QListView* const m_view;
    QAbstractItemModel* const m_model;
    QLineEdit* const m_filter;
    QTimer m_updateStateTimer;

    QDialogButtonBox* m_buttonBox;
    QWidget* m_mainWidget;
    QPushButton* m_deleteButton;
    QTimer m_deleteButtonTimer;
    int m_deleteCandidateRow;
};

}

#endif // SESSIONCHOOSERDIALOG_H
