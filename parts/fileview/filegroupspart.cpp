/***************************************************************************
 *   Copyright (C) 2001-2002 by Bernd Gehrmann                             *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "filegroupspart.h"
#include "filegroupspart.moc"

#include <qwhatsthis.h>
#include <qvbox.h>
#include <qtimer.h>
#include <kaction.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kdevgenericfactory.h>

#include "kdevcore.h"
#include "kdevproject.h"
#include "kdevmainwindow.h"

#include "filegroupswidget.h"
#include "filegroupsconfigwidget.h"

typedef KDevGenericFactory<FileGroupsPart> FileGroupsFactory;
static const KAboutData data("kdevfilegroups", I18N_NOOP("File Group View"), "1.0");
K_EXPORT_COMPONENT_FACTORY( libkdevfilegroups, FileGroupsFactory( &data ) )

FileGroupsPart::FileGroupsPart(QObject *parent, const char *name, const QStringList &)
    : KDevPlugin("FileGroups", "filegroups", parent, name ? name : "FileGroupsPart")
{
    deleteRequested = false;
    setInstance(FileGroupsFactory::instance());

    m_filegroups = new FileGroupsWidget(this);
    m_filegroups->setCaption(i18n("File Group View"));
    m_filegroups->setIcon(SmallIcon("attach"));
    QWhatsThis::add(m_filegroups, i18n("<b>File group view</b><p>"
                                       "The file group viewer shows all files of the project, "
                                       "in groups which can be configured in project settings dialog, <b>File Groups</b> tab."));
    mainWindow()->embedSelectView(m_filegroups, i18n("File Groups"), i18n("File groups in the project directory"));

    connect( core(), SIGNAL(projectConfigWidget(KDialogBase*)),
             this, SLOT(projectConfigWidget(KDialogBase*)) );

    // File groups
    connect( project(), SIGNAL(addedFilesToProject(const QStringList&)),
             m_filegroups, SLOT(addFiles(const QStringList&)) );
    connect( project(), SIGNAL(removedFilesFromProject(const QStringList&)),
             m_filegroups, SLOT(removeFiles(const QStringList&)) );
/*    connect( project(), SIGNAL(addedFileToProject(const QString&)),
             m_filegroups, SLOT(addFile(const QString&)) );
    connect( project(), SIGNAL(removedFileFromProject(const QString&)),
             m_filegroups, SLOT(removeFile(const QString&)) );*/
    m_filegroups->refresh();
}

FileGroupsPart::~FileGroupsPart()
{
    deleteRequested = true;
    if (m_filegroups)
        mainWindow()->removeView(m_filegroups);
    delete m_filegroups;
}

void FileGroupsPart::refresh()
{
    if (deleteRequested)
        return;
    // This method may be called from m_filetree's slot,
    // so we make sure not to modify the list view during
    // the execution of the slot
    QTimer::singleShot(0, m_filegroups, SLOT(refresh()));
}

void FileGroupsPart::projectConfigWidget(KDialogBase *dlg)
{
    QVBox *vbox = dlg->addVBoxPage(i18n("File Groups"));
    FileGroupsConfigWidget *w = new FileGroupsConfigWidget(this, vbox, "file groups config widget");
    connect( dlg, SIGNAL(okClicked()), w, SLOT(accept()) );
}
