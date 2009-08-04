/*  This file is part of KDevelop
    Copyright 2009 Andreas Pakulat <apaku@gmx.de>

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

#include "launchconfigurationdialog.h"
#include <QtGui/QSplitter>
#include <QtGui/QTreeView>
#include <QtGui/QStackedWidget>
#include <QtGui/QGridLayout>
#include <QtGui/QToolButton>

#include <klocale.h>
#include <klineedit.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kpushbutton.h>
#include <ktabwidget.h>
#include <kmessagebox.h>

#include <interfaces/launchconfigurationpage.h>
#include <interfaces/iproject.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/isession.h>

#include "core.h"
#include "runcontroller.h"
#include "launchconfiguration.h"

#include "ui_launchconfigtypepage.h"
#include <interfaces/ilauncher.h>
#include <interfaces/ilaunchmode.h>

namespace KDevelop
{

//TODO: Maybe use KPageDialog instead, might make the model stuff easier and the default-size stuff as well
LaunchConfigurationDialog::LaunchConfigurationDialog(QWidget* parent): KDialog(parent), currentPageChanged( false )
{
    setButtons( KDialog::Ok | KDialog::Cancel | KDialog::Apply );
    setButtonFocus( KDialog::Ok );
    button( KDialog::Apply )->setEnabled( false );
    
    setupUi( mainWidget() );
    
    addConfig->setIcon( KIcon("list-add") );
    addConfig->setEnabled( false );
    deleteConfig->setIcon( KIcon("list-remove") );
    deleteConfig->setEnabled( false );
    
    model = new LaunchConfigurationsModel( tree );
    tree->setModel( model );
    tree->setIndentation( 5 );
    tree->setExpandsOnDoubleClick( true );
    tree->setSelectionBehavior( QAbstractItemView::SelectRows );
    tree->setSelectionMode( QAbstractItemView::SingleSelection );
    tree->setUniformRowHeights( true );
    tree->setItemDelegateForColumn(0, new QItemDelegate( tree ) );
    QItemDelegate* dlg = new QItemDelegate( tree );
    dlg->setItemEditorFactory( new QItemEditorFactory() );
    dlg->itemEditorFactory()->registerEditor( QVariant::String, new LaunchConfigurationTypeEditorCreator() );
    tree->setItemDelegateForColumn( 1, dlg );
    
    connect( addConfig, SIGNAL(clicked()), this, SLOT(createConfiguration()));
    connect( deleteConfig, SIGNAL(clicked()), this, SLOT(deleteConfiguration()));
    
    connect( tree->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(selectedConfig(QItemSelection,QItemSelection)));
    QModelIndex idx = model->indexForConfig( Core::self()->runControllerInternal()->defaultLaunch() );
    kDebug() << "selecting index:" << idx;
    if( !idx.isValid() )
    {
        for( int i = 0; i < model->rowCount(); i++ )
        {
            if( model->rowCount( model->index( i, 0, QModelIndex() ) ) > 0 )
            {
                idx = model->index( 1, 0, model->index( i, 0, QModelIndex() ) );
                break;
            }
        }
        if( !idx.isValid() )
        {
            idx = model->index( 0, 0, QModelIndex() );
        }
    }
    tree->selectionModel()->select( QItemSelection( idx, idx ), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );
    tree->selectionModel()->setCurrentIndex( idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );
    tree->resizeColumnToContents( 0 );
    setInitialSize( QSize( 700, 500 ) );
    
    connect( this, SIGNAL(okClicked()), SLOT(saveConfig()) );
    connect( this, SIGNAL(applyClicked()), SLOT(saveConfig()) );
}

void LaunchConfigurationDialog::selectedConfig(QItemSelection selected, QItemSelection deselected )
{
    if( !deselected.indexes().isEmpty() && currentPageChanged )
    {
        LaunchConfiguration* l = model->configForIndex( deselected.indexes().first() );
        if( l )
        {
            disconnect(l, SIGNAL(nameChanged(QString)), this,  SLOT(updateNameLabel(QString)));
            if( KMessageBox::questionYesNo( this, i18n("Selected Launch Configuration has unsaved changes. Do you want to save it?"), i18n("Unsaved Changes") ) == KMessageBox::Yes )
            {
                saveConfig( deselected.indexes().first() );
            } else {
                LaunchConfigTypePage* tab = qobject_cast<LaunchConfigTypePage*>( stack->currentWidget() );
                tab->setLaunchConfiguration( l );
                currentPageChanged = false;
            }
        }
    }
    updateNameLabel("");
    if( !selected.indexes().isEmpty() )
    {
        LaunchConfiguration* l = model->configForIndex( selected.indexes().first() );
        if( l )
        {
            //TODO: enable removal button
            LaunchConfigurationType* type = l->type();
            LaunchConfigTypePage* tab;
            if( typeWidgets.contains( type ) )
            {
                tab = typeWidgets.value( type );
            } else
            {
                tab = new LaunchConfigTypePage( type, stack );
                connect( tab, SIGNAL(changed()), SLOT(pageChanged()) );
                stack->addWidget( tab );
            }
            tab->setLaunchConfiguration( l );
            updateNameLabel( l->name() );
            connect( l, SIGNAL(nameChanged(QString)), SLOT(updateNameLabel(QString)) );
            stack->setCurrentWidget( tab );
            
            addConfig->setEnabled( true );
            deleteConfig->setEnabled( true );
        } else 
        {
            addConfig->setEnabled( true );
            deleteConfig->setEnabled( false );
            stack->setCurrentIndex( 0 );
        }
    } else 
    {
        addConfig->setEnabled( false );
        deleteConfig->setEnabled( false );
        stack->setCurrentIndex( 0 );
    }
}

void LaunchConfigurationDialog::saveConfig( const QModelIndex& idx )
{
    LaunchConfigTypePage* tab = qobject_cast<LaunchConfigTypePage*>( stack->currentWidget() );
    if( tab )
    {
        tab->save();
        button( KDialog::Apply )->setEnabled( false );
        currentPageChanged = false;
    }
}

void LaunchConfigurationDialog::saveConfig()
{
    if( !tree->selectionModel()->selectedRows().isEmpty() )
    {
        saveConfig( tree->selectionModel()->selectedRows().first() );
    }
}


void LaunchConfigurationDialog::pageChanged()
{
    currentPageChanged = true;
    button( KDialog::Apply )->setEnabled( true );
}


void LaunchConfigurationDialog::deleteConfiguration()
{
    if( !tree->selectionModel()->selectedRows().isEmpty() )
    {
        model->deleteConfiguration( tree->selectionModel()->selectedRows().first() );
        tree->resizeColumnToContents( 0 );
    }
}


void LaunchConfigurationDialog::updateNameLabel( const QString& name )
{
    configName->setText( i18n("<b>%1</b>", name ) );
}


void LaunchConfigurationDialog::createConfiguration()
{
    if( !tree->selectionModel()->selectedRows().isEmpty() )
    {
        QModelIndex idx = tree->selectionModel()->selectedRows().first();
        if( idx.parent().isValid() )
        {
            idx = idx.parent();
        }
        model->createConfiguration( idx );
        QModelIndex newindex = model->index( model->rowCount( idx ) - 1, 0, idx );
        tree->selectionModel()->select( newindex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );
        tree->selectionModel()->setCurrentIndex( newindex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows );
        tree->edit( newindex );
        tree->resizeColumnToContents( 0 );
    }
}

LaunchConfigurationsModel::LaunchConfigurationsModel(QObject* parent): QAbstractItemModel(parent)
{
    GenericPageItem* global = new GenericPageItem;
    global->text = i18n("Global");
    global->row = 0;
    topItems << global;
    foreach( IProject* p, Core::self()->projectController()->projects() )
    {
        ProjectItem* t = new ProjectItem;
        t->project = p;
        t->row = topItems.count();
        topItems << t;
    }
    foreach( LaunchConfiguration* l, Core::self()->runControllerInternal()->launchConfigurations() )
    {
        LaunchItem* t = new LaunchItem;
        t->launch = l;
        TreeItem* parent;
        if( l->project() ) {
            parent = findItemForProject( l->project() );
        } else {
            parent = topItems.at(0);
        }
        t->parent = parent;
        t->row = parent->children.count();
        parent->children.append( t );
    }    
}

LaunchConfigurationsModel::ProjectItem* LaunchConfigurationsModel::findItemForProject( IProject* p )
{
    foreach( TreeItem* t, topItems )
    {
        ProjectItem pi = dynamic_cast<ProjectItem*>( t );
        if( pi && pi->project == p ) 
        {
            return t;
        }
    }
    Q_ASSERT(false);
    return 0;
}

int LaunchConfigurationsModel::columnCount(const QModelIndex& parent) const
{
    return 2;
}

QVariant LaunchConfigurationsModel::data(const QModelIndex& index, int role) const
{
    if( index.isValid() && index.column() >= 0 && index.column() < 2 )
    {
        TreeItem* t = static_cast<TreeItem*>( index.internalPointer() );
        switch( role ) 
        {
            case Qt::DisplayRole:
            {
                LaunchItem* li = dynamic_cast<LaunchItem*>( t );
                if( li ) 
                {
                    if( index.column() == 0 )
                    {
                        return li->launch->name();   
                    } else if( index.column() == 1 )
                    {
                        return li->launch->type()->name();
                    }
                }
                ProjectItem* pi = dynamic_cast<ProjectItem*>( t );
                if( pi && index.column() == 0 ) 
                {
                    return pi->project->name();
                }
                GenericPageItem* gpi = dynamic_cast<GenericPageItem*>( t );
                if( gpi && index.column() == 0 )
                {
                    return gpi->text;
                }
                break;
            }
            case Qt::DecorationRole:
            {
                LaunchItem* li = dynamic_cast<LaunchItem*>( t );
                if( index.column() == 0 && li )
                {
                    return li->launch->type()->icon();
                } else if( index.column() == 1 )
                {
                    return KIcon();
                }
            }
            case Qt::EditRole:
            {
                LaunchItem* li = dynamic_cast<LaunchItem*>( t );
                if( li )
                {
                    if( index.column() == 0 )
                    {
                        return li->launch->name();
                    } else if ( index.column() == 1 )
                    {
                        return li->launch->type()->id();
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    return QVariant();
}

QModelIndex LaunchConfigurationsModel::index(int row, int column, const QModelIndex& parent) const
{
    if( !hasIndex( row, column, parent ) )
        return QModelIndex();
    TreeItem* tree;
    
    if( !parent.isValid() )
    {   
        tree = topItems.at( row );
    } else 
    {
        TreeItem* t = static_cast<TreeItem*>( parent.internalPointer() );
        tree = t->children.at( row );
    }
    if( tree )
    {
        return createIndex( row, column, tree );
    }
    return QModelIndex();
}

QModelIndex LaunchConfigurationsModel::parent(const QModelIndex& child) const
{
    if( child.isValid()  )
    {
        TreeItem* t = static_cast<TreeItem*>( child.internalPointer() );
        if( t->parent )
        {
            return createIndex( t->parent->row, 0, t->parent );
        }
    }
    return QModelIndex();
}

int LaunchConfigurationsModel::rowCount(const QModelIndex& parent) const
{
    if( parent.column() > 0 )
        return 0;
    if( parent.isValid() )
    {
        TreeItem* t = static_cast<TreeItem*>( parent.internalPointer() );
        return t->children.count();
    } else
    {
        return topItems.count();
    }
    return 0;
}

QVariant LaunchConfigurationsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole ) 
    {
        if( section == 0 ) 
        {
            return i18nc("Name of the Launch Configurations", "Name");
        } else if( section == 1 ) 
        {
            return i18nc("The type of the Launch Configurations (i.e. Python Application, C++ Application)", "Type");
        }
    }
    return QVariant();
}

Qt::ItemFlags LaunchConfigurationsModel::flags(const QModelIndex& index) const
{
    if( index.isValid() && index.column() >= 0 
        && index.column() < columnCount( QModelIndex() ) ) 
    {
        TreeItem* t = static_cast<TreeItem*>( index.internalPointer() );
        if( t && dynamic_cast<LaunchItem*>( t ) )
        {
            return Qt::ItemFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
        } else if( t )
        {
            return Qt::ItemFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
        }
    }
    return Qt::NoItemFlags;
}

bool LaunchConfigurationsModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if( index.isValid() && index.parent().isValid() && role == Qt::EditRole ) 
    {
        if( index.row() >= 0 && index.row() < rowCount( index.parent() ) ) 
        {
            LaunchItem* t = dynamic_cast<LaunchItem*>( static_cast<TreeItem*>( index.internalPointer() ) );
            if( t )
            {
                if( index.column() == 0 )
                {
                    t->launch->setName( value.toString() );
                } else if( index.column() == 1 )
                {
                    t->launch->setType( value.toString() );
                }
            }
        }
    }
    return false;
}

LaunchConfiguration* LaunchConfigurationsModel::configForIndex(const QModelIndex& idx )
{
    if( idx.isValid() )
    {
        LaunchItem* item = dynamic_cast<LaunchItem*>( static_cast<TreeItem*>( idx.internalPointer() ) );
        if( item )
        {
            return item->launch;
        }
    }
    return 0;
}

QModelIndex LaunchConfigurationsModel::indexForConfig( LaunchConfiguration* l )
{
    if( l )
    {
        TreeItem* tparent = topItems.at( 0 );
        if( l->project() )
        {
            foreach( TreeItem* t, topItems )
            {
                ProjectItem* pi = dynamic_cast<ProjectItem*>( t );
                if( pi && pi->project == l->project() )
                {
                    tparent = t;
                    break;
                }
            }
        }
        
        if( tparent )
        {
            foreach( TreeItem* c, tparent->children )
            {
                LaunchItem* li = dynamic_cast<LaunchItem*>( c );
                if( li->launch && li->launch == l )
                {
                    return index( c->row, 0, index( tparent->row, 0, QModelIndex() ) );
                }
            }
        }
    }
    return QModelIndex();
}


void LaunchConfigurationsModel::deleteConfiguration( const QModelIndex& index )
{
    LaunchItem* t = dynamic_cast<LaunchItem*>( static_cast<TreeItem*>( index.internalPointer() ) );
    if( !t )
        return;
    beginRemoveRows( parent( index ), index.row(), index.row() );
    t->parent->children.removeAll( t );
    Core::self()->runControllerInternal()->removeLaunchConfiguration( t->launch );
    endRemoveRows();
}

void LaunchConfigurationsModel::createConfiguration(const QModelIndex& parent )
{
    TreeItem* t = static_cast<TreeItem*>( parent.internalPointer() );
    ProjectItem* ti = dynamic_cast<ProjectItem*>( t );
    if( parent.isValid() && t && !Core::self()->runController()->launchConfigurationTypes().isEmpty() )
    {
        KConfigGroup launchGroup;
        if( ti )
        {
            launchGroup = ti->project->projectConfiguration()->group( RunController::LaunchConfigurationsGroup );
            
        } else if( !t->parent )
        {
            launchGroup = Core::self()->activeSession()->config()->group( RunController::LaunchConfigurationsGroup );
            
        } else 
        {
            kWarning() << "Expected project or global item, but didn't find either";
            return;
        }
        beginInsertRows( parent, rowCount( parent ), rowCount( parent ) );
        QStringList configs = launchGroup.readEntry( RunController::LaunchConfigurationsListEntry, QStringList() );
        uint num = 0;
        QString baseName = "Launch Configuration";
        while( configs.contains( QString( "%1 %2" ).arg( baseName ).arg( num ) ) )
        {
            num++;
        }
        QString groupName = QString( "%1 %2" ).arg( baseName ).arg( num );
        KConfigGroup launchConfigGroup = launchGroup.group( groupName );
        LaunchConfigurationType* type = Core::self()->runController()->launchConfigurationTypes().at(0);
        launchConfigGroup.writeEntry(LaunchConfiguration::LaunchConfigurationNameEntry, i18n( "New Configuration" ) );
        launchConfigGroup.writeEntry(LaunchConfiguration::LaunchConfigurationTypeEntry, type->id() );
        launchConfigGroup.sync();
        configs << groupName;
        launchGroup.writeEntry( RunController::LaunchConfigurationsListEntry, configs );
        launchGroup.sync();
        LaunchConfiguration* l = new LaunchConfiguration( launchConfigGroup, ( ti ? ti->project : 0 ) );
        l->setLauncherForMode( type->launchers().at( 0 )->supportedModes().at(0), type->launchers().at( 0 )->id() );
        Core::self()->runControllerInternal()->addLaunchConfiguration( l );
        LaunchItem* item = new LaunchItem;
        item->launch = l;
        item->parent = t;
        item->row = t->children.count();
        t->children.append( item );
        endInsertRows();
    }
}

LaunchConfigurationTypeComboBox::LaunchConfigurationTypeComboBox( QWidget* parent )
    : KComboBox( parent )
{
    foreach( LaunchConfigurationType* t, Core::self()->runControllerInternal()->launchConfigurationTypes() )
    {
        addItem( t->name(), t->id() );
    }
}

QVariant LaunchConfigurationTypeComboBox::currentData() const
{
    return itemData( currentIndex() );
}

void LaunchConfigurationTypeComboBox::changeCurrentIndex( int idx )
{
    emit currentDataChanged( itemData( idx ) );
}

void LaunchConfigurationTypeComboBox::setCurrentData( const QVariant& data )
{
    setCurrentIndex( findData( data ) );
}

LaunchConfigurationTypeEditorCreator::LaunchConfigurationTypeEditorCreator()
{
}

QWidget* LaunchConfigurationTypeEditorCreator::createWidget(QWidget* parent) const
{
    LaunchConfigurationTypeComboBox* cb = new LaunchConfigurationTypeComboBox( parent );
    cb->setFrame( false );
    return cb;
}

QByteArray LaunchConfigurationTypeEditorCreator::valuePropertyName() const
{
    return "currentData";
}

LaunchConfigTypePage::LaunchConfigTypePage( LaunchConfigurationType* type, QWidget* parent ) 
    : QWidget(parent), ui( new Ui::LaunchConfigTypePage )
{
    ui->setupUi( this );
    ui->launcher->setEnabled( false );
    foreach( ILauncher* l, type->launchers() )
    {
        foreach( const QString& s, l->supportedModes() )
        {
            ILaunchMode* m = Core::self()->runControllerInternal()->launchModeForId( s );
            if( m )
            {
                int idx = ui->mode->findData( m->id() );
                if( idx == -1 )
                {
                    ui->mode->addItem( m->icon(), m->name(), m->id() );
                    launchersForModes.insert( m->id(), QStringList() << l->id() );
                } else
                {
                    launchersForModes[ m->id() ] << l->id();
                }
            } else
            {
                kWarning() << "Ooops, config type" << type->id() << "provides a launcher with an unknown launch mode:" << s;
                return;
            }
        }
    }
    
    foreach( LaunchConfigurationPageFactory* fac, type->configPages() )
    {
        LaunchConfigurationPage* page = fac->createWidget( ui->tab );
        connect( page, SIGNAL(changed()), SIGNAL(changed()) );
        ui->tab->addTab( page, page->icon(), page->title() );
    }
    
    connect( ui->mode, SIGNAL(currentIndexChanged(int)), SLOT(changeMode(int)) );
    connect( ui->launcher, SIGNAL(currentIndexChanged(int)), SLOT(changeLauncher(int)) );
    connect( ui->launcher, SIGNAL(currentIndexChanged(int)), SIGNAL(changed()) );
    
}

void LaunchConfigTypePage::changeLauncher( int idx )
{
    foreach( LaunchConfigurationPage* p, launcherPages )
    {
        ui->tab->removePage( p );
        delete p;
    }
    launcherPages.clear();
    ILauncher* l = config->type()->launcherForId( ui->launcher->itemData( idx ).toString() );
    if( l )
    {
        config->setLauncherForMode( ui->mode->itemData( ui->mode->currentIndex() ).toString(), ui->launcher->itemData( idx ).toString() );
        foreach( LaunchConfigurationPageFactory* fac, l->configPages() )
        {
            LaunchConfigurationPage* page = fac->createWidget( ui->tab );
            connect( page, SIGNAL(changed()), SIGNAL(changed()) );
            ui->tab->addTab( page, page->icon(), page->title() );
            page->loadFromConfiguration( config->config(), config->project() );
            launcherPages << page;
        }
    }
    
}

void LaunchConfigTypePage::changeMode( int idx )
{
    QString id = ui->mode->itemData( idx ).toString();
    QStringList launchers = launchersForModes[id];
    
    if( launchers.count() > 1 )
    {
        ui->launcher->setEnabled( true );
    } else
    {
        ui->launcher->setEnabled( false );
    }
    bool b = ui->launcher->blockSignals( true );
    ui->launcher->clear();
    foreach( const QString& lid, launchers )
    {
        ui->launcher->addItem( config->type()->launcherForId( lid )->name(), lid );
    }
    int lidx = 0;
    if( launchers.contains( config->launcherForMode( id ) ) )
    {
        lidx = ui->launcher->findData( config->launcherForMode( id ) );
    } 
    ui->launcher->setCurrentIndex( lidx );
    changeLauncher( lidx );
    ui->launcher->blockSignals( b );
}

void LaunchConfigTypePage::setLaunchConfiguration( KDevelop::LaunchConfiguration* l )
{
    config = l;
    changeMode( ui->mode->findData( "execute" ) );
    changeLauncher( ui->launcher->findData( config->launcherForMode( "execute" ) ) );
    foreach( LaunchConfigurationPage* p, ui->tab->findChildren<LaunchConfigurationPage*>() )
    {
        p->loadFromConfiguration( config->config(), config->project() );
    }
}

void LaunchConfigTypePage::save()
{
    QList<LaunchConfigurationPage*> pages = ui->tab->findChildren<LaunchConfigurationPage*>();
    foreach( LaunchConfigurationPage* p, pages )
    {
        p->saveToConfiguration( config->config() );
    }
    config->config().sync();
}


}

#include "launchconfigurationdialog.moc"
