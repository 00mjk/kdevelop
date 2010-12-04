/**************************************************************************
*   Copyright 2010 Silvère Lestang <silvere.lestang@gmail.com>            *
*   Copyright 2010 Julien Desgats <julien.desgats@gmail.com>              *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "grepoutputview.h"
#include "grepoutputmodel.h"
#include "grepoutputdelegate.h"
#include "ui_grepoutputview.h"
#include "grepviewplugin.h"
#include "grepdialog.h"
#include "greputil.h"

#include <QtGui/QAction>
#include <KMessageBox>

#include <interfaces/icore.h>

using namespace KDevelop;

GrepOutputViewFactory::GrepOutputViewFactory()
{}

QWidget* GrepOutputViewFactory::create(QWidget* parent)
{
    return new GrepOutputView(parent);
}

Qt::DockWidgetArea GrepOutputViewFactory::defaultPosition()
{
    return Qt::BottomDockWidgetArea;
}

QString GrepOutputViewFactory::id() const
{
    return "org.kdevelop.GrepOutputView";
}

GrepOutputView::GrepOutputView(QWidget* parent)
  : QWidget(parent)
{
    Ui::GrepOutputView::setupUi(this);

    setWindowTitle(i18n("Replace output view"));
    setWindowIcon(SmallIcon("edit-find"));
    
    m_prev = new QAction(KIcon("go-previous"), i18n("&Previous"), this);
    m_prev->setEnabled(false);
    m_next = new QAction(KIcon("go-next"), i18n("&Next"), this);
    m_next->setEnabled(false);
    QAction *separator = new QAction(this);
    separator->setSeparator(true);
    QAction *change_criteria = new QAction(KIcon("configure"), i18n("&Change criteria"), this);
    
    addAction(m_prev);
    addAction(m_next);
    addAction(separator);
    addAction(change_criteria);
    
    renewModel();
    resultsTreeView->setItemDelegate(GrepOutputDelegate::self());
    resultsTreeView->setHeaderHidden(true);

    connect(m_prev, SIGNAL(triggered(bool)), this, SLOT(selectPreviousItem()));
    connect(m_next, SIGNAL(triggered(bool)), this, SLOT(selectNextItem()));
    connect(applyButton, SIGNAL(clicked()),  this, SLOT(onApply()));
    
    KConfigGroup cg = KGlobal::config()->group( "GrepDialog" );
    replacementCombo->addItems( cg.readEntry("LastReplacementItems", QStringList()) );
    replacementCombo->setInsertPolicy(QComboBox::InsertAtTop);
    applyButton->setIcon(KIcon("dialog-ok-apply"));
    
    connect(change_criteria, SIGNAL(triggered(bool)), this, SLOT(showDialog()));
}

GrepOutputView::~GrepOutputView()
{
    KConfigGroup cg = KGlobal::config()->group( "GrepDialog" );
    cg.writeEntry("LastReplacementItems", qCombo2StringList(replacementCombo, true));
    emit outputViewIsClosed();
}

GrepOutputModel* GrepOutputView::renewModel()
{
    if (model()) {
        model()->deleteLater();
    }

    GrepOutputModel* newModel = new GrepOutputModel(resultsTreeView);
    resultsTreeView->setModel(newModel);
    // text may be already present
    newModel->setReplacement(replacementCombo->currentText());
    connect(newModel, SIGNAL(rowsRemoved(QModelIndex, int, int)),
            this, SLOT(rowsRemoved()));
    connect(resultsTreeView, SIGNAL(activated(QModelIndex)), newModel, SLOT(activate(QModelIndex)));
    connect(replacementCombo, SIGNAL(editTextChanged(QString)), newModel, SLOT(setReplacement(QString)));
    connect(newModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(expandRootElement(QModelIndex)));
    connect(newModel, SIGNAL(showErrorMessage(QString,int)), this, SLOT(showErrorMessage(QString)));
    return newModel;
}


GrepOutputModel* GrepOutputView::model()
{
    return static_cast<GrepOutputModel*>(resultsTreeView->model());
}

void GrepOutputView::setPlugin(GrepViewPlugin* plugin)
{
    m_plugin = plugin;
}

void GrepOutputView::setMessage(const QString& msg)
{
    messageLabel->setText(msg);
}

void GrepOutputView::showErrorMessage( const QString& errorMessage )
{
    setStyleSheet("QLabel { color : red; }");
    setMessage(errorMessage);
}

void GrepOutputView::showMessage( KDevelop::IStatus* , const QString& message )
{
    setStyleSheet("");
    setMessage(message);
}

void GrepOutputView::onApply()
{
    Q_ASSERT(model()->rowCount());
    // ask a confirmation before an empty string replacement
    if(replacementCombo->currentText().length() == 0 &&
       KMessageBox::questionYesNo(this, i18n("Start replacement"),
                                  i18n("Would you want to replace by empty string ?")) == KMessageBox::ButtonCode::No)
    {
        return;
    }

    setEnabled(false);
    model()->doReplacements();
    setEnabled(true);
}

void GrepOutputView::showDialog()
{
    m_plugin->showDialog(true);
}

void GrepOutputView::expandRootElement(const QModelIndex& parent)
{
    if(!parent.isValid())
    {
        resultsTreeView->setExpanded(model()->index(0,0), true);
    }

    m_prev->setEnabled(true);
    m_next->setEnabled(true);
    applyButton->setEnabled(true);
}

void GrepOutputView::selectPreviousItem()
{
    QModelIndex idx = resultsTreeView->currentIndex();
    if(idx.isValid())
    {
        QModelIndex prev_idx = model()->previousItemIndex(idx);
        resultsTreeView->setCurrentIndex(prev_idx);
        model()->activate(prev_idx);
    }
}

void GrepOutputView::selectNextItem()
{
    QModelIndex idx = resultsTreeView->currentIndex();
    if(idx.isValid())
    {
        QModelIndex next_idx = model()->nextItemIndex(idx);
        resultsTreeView->setCurrentIndex(next_idx);
        model()->activate(next_idx);
    }
}

void GrepOutputView::rowsRemoved()
{
    m_prev->setEnabled(model()->rowCount());
    m_next->setEnabled(model()->rowCount());
}
