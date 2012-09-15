/************************************************************************
 * KDevelop4 Custom Buildsystem Support                                 *
 *                                                                      *
 * Copyright 2010 Andreas Pakulat <apaku@gmx.de>                        *
 *                                                                      *
 * This program is free software; you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation; either version 2 or version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful, but  *
 * WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU     *
 * General Public License for more details.                             *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program; if not, see <http://www.gnu.org/licenses/>. *
 ************************************************************************/

#include "projectpathswidget.h"

#include <QToolButton>

#include <KDebug>
#include <KLineEdit>
#include <KAction>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <assert.h>

#include "ui_projectpathswidget.h"
#include "projectpathsmodel.h"
#include <util/environmentgrouplist.h>
#include <interfaces/iproject.h>

extern int cbsDebugArea(); // from debugarea.cpp

ProjectPathsWidget::ProjectPathsWidget( QWidget* parent )
    : QWidget ( parent ), ui( new Ui::ProjectPathsWidget )
    , pathsModel( new ProjectPathsModel( this ) )
{
    ui->setupUi( this );

    // Hack to workaround broken setIcon(QIcon) overload in KPushButton, the function does not set the icon at all
    // So need to explicitly use the KIcon overload
    ui->addPath->setIcon(KIcon("list-add"));
    ui->replacePath->setIcon(KIcon("document-edit"));
    ui->removePath->setIcon(KIcon("list-remove"));

    // hack taken from kurlrequester, make the buttons a bit less in height so they better match the url-requester
    ui->addPath->setFixedHeight( ui->projectPaths->sizeHint().height() );
    ui->removePath->setFixedHeight( ui->projectPaths->sizeHint().height() );
    ui->replacePath->setFixedHeight( ui->projectPaths->sizeHint().height() );

    connect( ui->addPath, SIGNAL(clicked(bool)), SLOT(addProjectPath()) );
    connect( ui->replacePath, SIGNAL(clicked(bool)), SLOT(replaceProjectPath()) );
    connect( ui->removePath, SIGNAL(clicked(bool)), SLOT(deleteProjectPath()) );

    ui->projectPaths->setModel( pathsModel );
    connect( ui->projectPaths, SIGNAL(currentIndexChanged(int)), SLOT(projectPathSelected(int)) );
    connect( pathsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), SIGNAL(changed()) );
    connect( pathsModel, SIGNAL(rowsInserted(QModelIndex,int,int)), SIGNAL(changed()) );
    connect( pathsModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), SIGNAL(changed()) );
}

QList<CustomBuildSystemProjectPathConfig> ProjectPathsWidget::paths() const
{
    return pathsModel->paths();
}

void ProjectPathsWidget::setPaths( const QList<CustomBuildSystemProjectPathConfig>& paths )
{
    bool b = blockSignals( true );
    clear();
    pathsModel->setPaths( paths );
    blockSignals( b );
    ui->projectPaths->setCurrentIndex(0); // at least a project root item is present
    projectPathSelected(0);
    ui->languageParameters->setCurrentIndex(0);
    updateEnablements();
}

void ProjectPathsWidget::definesChanged( const Defines& defines )
{
    kDebug(cbsDebugArea()) << "defines changed";
    updatePathsModel( defines, ProjectPathsModel::DefinesDataRole );
}

void ProjectPathsWidget::includesChanged( const QStringList& includes )
{
    kDebug(cbsDebugArea()) << "includes changed";
    updatePathsModel( includes, ProjectPathsModel::IncludesDataRole );
}

void ProjectPathsWidget::updatePathsModel(const QVariant& newData, int role)
{
    QModelIndex idx = pathsModel->index( ui->projectPaths->currentIndex(), 0, QModelIndex() );
    if( idx.isValid() ) {
        bool b = pathsModel->setData( idx, newData, role );
        if( b ) {
            emit changed();
        }
    }
}

void ProjectPathsWidget::projectPathSelected( int index )
{
    assert(index >= 0);
    const QModelIndex midx = pathsModel->index( index, 0 );
    ui->includesWidget->setIncludes( pathsModel->data( midx, ProjectPathsModel::IncludesDataRole ).toStringList() );
    ui->definesWidget->setDefines( pathsModel->data( midx, ProjectPathsModel::DefinesDataRole ).toHash() );
    updateEnablements();
}

void ProjectPathsWidget::clear()  
{
    pathsModel->setPaths( QList<CustomBuildSystemProjectPathConfig>() );
    ui->includesWidget->clear();
    ui->definesWidget->clear();
    updateEnablements();
}

void ProjectPathsWidget::addProjectPath()
{
    kDebug(cbsDebugArea()) << "enabling project path editing" << pathsModel->data(pathsModel->index(0, 0), ProjectPathsModel::FullUrlDataRole).value<KUrl>();
    KFileDialog dlg(pathsModel->data(pathsModel->index(0, 0), ProjectPathsModel::FullUrlDataRole).value<KUrl>(), "", this);
    dlg.setMode( KFile::ExistingOnly | KFile::File | KFile::Directory );
    dlg.exec();
    kDebug(cbsDebugArea()) << "adding url:" << dlg.selectedUrl();
    pathsModel->addPath(dlg.selectedUrl());
    ui->projectPaths->setCurrentIndex(pathsModel->rowCount() - 1);
    kDebug(cbsDebugArea()) << "added url:" << pathsModel->rowCount();
    updateEnablements();
}

void ProjectPathsWidget::replaceProjectPath()
{
    KFileDialog dlg(pathsModel->data(pathsModel->index(0, 0), ProjectPathsModel::FullUrlDataRole).value<KUrl>(), "", this);
    dlg.setMode( KFile::ExistingOnly | KFile::File | KFile::Directory );
    dlg.exec();
    kDebug(cbsDebugArea()) << "adding url:" << dlg.selectedUrl();
    pathsModel->setData( pathsModel->index( ui->projectPaths->currentIndex(), 0 ), QVariant::fromValue<KUrl>(dlg.selectedUrl()), ProjectPathsModel::FullUrlDataRole );
    kDebug(cbsDebugArea()) << "added url:" << pathsModel->rowCount();
    updateEnablements();
}

void ProjectPathsWidget::deleteProjectPath()
{
    const QModelIndex idx = pathsModel->index( ui->projectPaths->currentIndex(), 0 );
    if( KMessageBox::questionYesNo( this, i18n("Are you sure you want to remove the configuration for the path '%1'?", pathsModel->data( idx, Qt::DisplayRole ).toString() ), "Remove Path Configuration" ) == KMessageBox::Yes ) {
        pathsModel->removeRows( ui->projectPaths->currentIndex(), 1 );
    }
    updateEnablements();
}
void ProjectPathsWidget::setProject(KDevelop::IProject* w_project)
{
    kDebug(cbsDebugArea()) << "setting project:" << w_project->projectFileUrl() << w_project->folder();
    pathsModel->setProject( w_project );
    ui->includesWidget->setProject( w_project );
}

void ProjectPathsWidget::updateEnablements() {
    // Disable removal of the project root entry which is always first in the list
    ui->removePath->setEnabled( ui->projectPaths->currentIndex() > 0 );
    ui->replacePath->setEnabled( ui->projectPaths->currentIndex() > 0 );
}

#include "projectpathswidget.moc"

