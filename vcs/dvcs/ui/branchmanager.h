/***************************************************************************
 *   Copyright 2008 Evgeniy Ivanov <powerfox@kde.ru>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef BRANCH_MANAGER_H
#define BRANCH_MANAGER_H

#include <QtGui/QStringListModel>
#include <KDE/KDialog>
#include <QStandardItemModel>

class BranchesListModel;
class KJob;
namespace Ui { class BranchDialogBase; }

namespace KDevelop
{
    class DistributedVersionControlPlugin;
}

class BranchManager : public KDialog
{
    Q_OBJECT
public:
    BranchManager(const QString &_repo, KDevelop::DistributedVersionControlPlugin* executor, QWidget *parent = 0);
    ~BranchManager();
    
    bool isValid() const { return m_valid; }

signals:
    void checkedOut(KJob*);

private slots:
    void createBranch();
    void delBranch();
    void checkoutBranch();

private:
    QString repo;
    KDevelop::DistributedVersionControlPlugin* d;

    Ui::BranchDialogBase* m_ui;
    BranchesListModel* m_model;
    bool m_valid;
    
};

class BranchesListModel : public QStandardItemModel
{
    Q_OBJECT
    public:
        BranchesListModel(KDevelop::DistributedVersionControlPlugin* dvcsplugin, const QString& repo, QObject* parent = 0);
        
        void createBranch(const QString& baseBranch, const QString& newBranch);
        void removeBranch(const QString& branch);
        
        KDevelop::DistributedVersionControlPlugin* dvcsPlugin() const { return dvcsplugin; }
        QString repository() const { return repo; }
        
    public slots:
        void resetCurrent();
        
    private:
        KDevelop::DistributedVersionControlPlugin* dvcsplugin;
        QString repo;
};


#endif
