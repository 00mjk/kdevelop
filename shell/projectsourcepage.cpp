/***************************************************************************
 *   Copyright (C) 2010 by Aleix Pol Gonzalez <aleixpol@kde.org>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "projectsourcepage.h"
#include "ui_projectsourcepage.h"
#include <interfaces/iplugin.h>
#include <interfaces/icore.h>
#include <interfaces/iplugincontroller.h>
#include <interfaces/iruncontroller.h>
#include <vcs/interfaces/ibasicversioncontrol.h>
#include <vcs/widgets/vcslocationwidget.h>
#include <vcs/vcsjob.h>

#include <QVBoxLayout>

#include <KColorScheme>
#include <KLocalizedString>
#include <KMessageBox>

#include <interfaces/iprojectprovider.h>

using namespace KDevelop;

static const int FROM_FILESYSTEM_SOURCE_INDEX = 0;

ProjectSourcePage::ProjectSourcePage(const QUrl& initial, QWidget* parent)
    : QWidget(parent)
{
    m_ui = new Ui::ProjectSourcePage;
    m_ui->setupUi(this);

    m_ui->status->setCloseButtonVisible(false);
    m_ui->status->setMessageType(KMessageWidget::Error);

    m_ui->workingDir->setUrl(initial);
    m_ui->workingDir->setMode(KFile::Directory);
    m_ui->remoteWidget->setLayout(new QVBoxLayout(m_ui->remoteWidget));

    m_ui->sources->addItem(QIcon::fromTheme(QStringLiteral("folder")), i18n("From File System"));
    m_plugins.append(0);

    IPluginController* pluginManager = ICore::self()->pluginController();
    QList<IPlugin*> plugins = pluginManager->allPluginsForExtension( QStringLiteral("org.kdevelop.IBasicVersionControl") );
    foreach( IPlugin* p, plugins )
    {
        m_plugins.append(p);
        m_ui->sources->addItem(QIcon::fromTheme(pluginManager->pluginInfo(p).iconName()), p->extension<IBasicVersionControl>()->name());
    }

    plugins = pluginManager->allPluginsForExtension( QStringLiteral("org.kdevelop.IProjectProvider") );
    foreach( IPlugin* p, plugins )
    {
        m_plugins.append(p);
        m_ui->sources->addItem(QIcon::fromTheme(pluginManager->pluginInfo(p).iconName()), p->extension<IProjectProvider>()->name());
    }

    connect(m_ui->workingDir, &KUrlRequester::textChanged, this, &ProjectSourcePage::reevaluateCorrection);
    connect(m_ui->sources, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ProjectSourcePage::setSourceIndex);
    connect(m_ui->get, &QPushButton::clicked, this, &ProjectSourcePage::checkoutVcsProject);

    emit isCorrect(false);

    setSourceIndex(FROM_FILESYSTEM_SOURCE_INDEX);

    if(!m_plugins.isEmpty())
        m_ui->sources->setCurrentIndex(1);
}

ProjectSourcePage::~ProjectSourcePage()
{
    delete m_ui;
}

void ProjectSourcePage::setSourceIndex(int index)
{
    m_locationWidget = 0;
    m_providerWidget = 0;
    QLayout* remoteWidgetLayout = m_ui->remoteWidget->layout();
    QLayoutItem *child;
    while ((child = remoteWidgetLayout->takeAt(0)) != 0) {
        delete child->widget();
        delete child;
    }

    IBasicVersionControl* vcIface = vcsPerIndex(index);
    IProjectProvider* providerIface;
    bool found=false;
    if(vcIface) {
        found=true;
        m_locationWidget=vcIface->vcsLocation(m_ui->sourceBox);
        connect(m_locationWidget, &VcsLocationWidget::changed, this, &ProjectSourcePage::locationChanged);

        remoteWidgetLayout->addWidget(m_locationWidget);
    } else {
        providerIface = providerPerIndex(index);
        if(providerIface) {
            found=true;
            m_providerWidget=providerIface->providerWidget(m_ui->sourceBox);
            connect(m_providerWidget, &IProjectProviderWidget::changed, this, &ProjectSourcePage::projectChanged);

            remoteWidgetLayout->addWidget(m_providerWidget);
        }
    }
    reevaluateCorrection();

    m_ui->sourceBox->setVisible(found);
}

IBasicVersionControl* ProjectSourcePage::vcsPerIndex(int index)
{
    IPlugin* p = m_plugins.value(index);
    if(!p)
        return 0;
    else
        return p->extension<KDevelop::IBasicVersionControl>();
}

IProjectProvider* ProjectSourcePage::providerPerIndex(int index)
{
    IPlugin* p = m_plugins.value(index);
    if(!p)
        return 0;
    else
        return p->extension<KDevelop::IProjectProvider>();
}

VcsJob* ProjectSourcePage::jobPerCurrent()
{
    QUrl url=m_ui->workingDir->url();
    IPlugin* p=m_plugins[m_ui->sources->currentIndex()];
    VcsJob* job=0;

    if(IBasicVersionControl* iface=p->extension<IBasicVersionControl>()) {
        Q_ASSERT(iface && m_locationWidget);
        job=iface->createWorkingCopy(m_locationWidget->location(), url);
    } else if(m_providerWidget) {
        job=m_providerWidget->createWorkingCopy(url);
    }
    return job;
}

void ProjectSourcePage::checkoutVcsProject()
{
    QUrl url=m_ui->workingDir->url();
    QDir d(url.toLocalFile());
    if(!url.isLocalFile() && !d.exists()) {
        bool corr = d.mkpath(d.path());
        if(!corr) {
            KMessageBox::error(0, i18n("Could not create the directory: %1", d.path()));
            return;
        }
    }

    VcsJob* job=jobPerCurrent();
    if (!job) {
        return;
    }

    m_ui->sources->setEnabled(false);
    m_ui->sourceBox->setEnabled(false);
    m_ui->workingDir->setEnabled(false);
    m_ui->get->setEnabled(false);
    m_ui->creationProgress->setValue(m_ui->creationProgress->minimum());
    connect(job, &VcsJob::result, this, &ProjectSourcePage::projectReceived);
    // Can't use new signal-slot syntax, KJob::percent is private :/
    connect(job, SIGNAL(percent(KJob*,ulong)), SLOT(progressChanged(KJob*,ulong)));
    connect(job, &VcsJob::infoMessage, this, &ProjectSourcePage::infoMessage);
    ICore::self()->runController()->registerJob(job);
}

void ProjectSourcePage::progressChanged(KJob*, unsigned long value)
{
    m_ui->creationProgress->setValue(value);
}

void ProjectSourcePage::infoMessage(KJob* , const QString& text, const QString& /*rich*/)
{
    m_ui->creationProgress->setFormat(i18nc("Format of the progress bar text. progress and info",
                                            "%1 : %p%", text));
}

void ProjectSourcePage::projectReceived(KJob* job)
{
    if (job->error()) {
        m_ui->creationProgress->setValue(0);
    } else {
        m_ui->creationProgress->setValue(m_ui->creationProgress->maximum());
    }

    reevaluateCorrection();
    m_ui->creationProgress->setFormat(QStringLiteral("%p%"));
}

void ProjectSourcePage::reevaluateCorrection()
{
    //TODO: Probably we should just ignore remote URL's, I don't think we're ever going
    //to support checking out to remote directories
    const QUrl cwd = m_ui->workingDir->url();
    const QDir dir = cwd.toLocalFile();

    // case where we import a project from local file system
    if (m_ui->sources->currentIndex() == FROM_FILESYSTEM_SOURCE_INDEX) {
        emit isCorrect(dir.exists());
        return;
    }

    // all other cases where remote locations need to be specified
    bool correct=!cwd.isRelative() && (!cwd.isLocalFile() || QDir(cwd.adjusted(QUrl::RemoveFilename).toLocalFile()).exists());
    emit isCorrect(correct && m_ui->creationProgress->value() == m_ui->creationProgress->maximum());

    bool validWidget = ((m_locationWidget && m_locationWidget->isCorrect()) ||
                       (m_providerWidget && m_providerWidget->isCorrect()));
    bool validToCheckout = correct && validWidget; //To checkout, if it exists, it should be an empty dir
    if (validToCheckout && cwd.isLocalFile() && dir.exists()) {
        validToCheckout = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty();
    }

    m_ui->get->setEnabled(validToCheckout);
    m_ui->creationProgress->setEnabled(validToCheckout);

    if(!correct)
        setStatus(i18n("You need to specify a valid or nonexistent directory to check out a project"));
    else if(!m_ui->get->isEnabled() && m_ui->workingDir->isEnabled())
        setStatus(i18n("You need to specify a valid project location"));
    else
        clearStatus();
}

void ProjectSourcePage::locationChanged()
{
    Q_ASSERT(m_locationWidget);
    if(m_locationWidget->isCorrect()) {
        QString currentUrl = m_ui->workingDir->text();
        currentUrl = currentUrl.left(currentUrl.lastIndexOf('/')+1);

        QUrl current = QUrl::fromUserInput(currentUrl + m_locationWidget->projectName());
        m_ui->workingDir->setUrl(current);
    }
    else
        reevaluateCorrection();
}

void ProjectSourcePage::projectChanged(const QString& name)
{
    Q_ASSERT(m_providerWidget);
    QString currentUrl = m_ui->workingDir->text();
    currentUrl = currentUrl.left(currentUrl.lastIndexOf('/')+1);

    QUrl current = QUrl::fromUserInput(currentUrl + name);
    m_ui->workingDir->setUrl(current);
}

void ProjectSourcePage::setStatus(const QString& message)
{
    m_ui->status->setText(message);
    m_ui->status->animatedShow();
}

void ProjectSourcePage::clearStatus()
{
    m_ui->status->animatedHide();
}

QUrl ProjectSourcePage::workingDir() const
{
    return m_ui->workingDir->url();
}
