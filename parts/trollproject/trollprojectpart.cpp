/***************************************************************************
 *   Copyright (C) 2003 by Thomas Hasart                                   *
 *   thasart@gmx.de                                                        *
 *   Copyright (C) 2001-2002 by Bernd Gehrmann                             *
 *   bernd@kdevelop.org                                                    *
 *   Copyright (C) 2002 by Jakob Simon-Gaarde                              *
 *   jakob@jsg.dk                                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "trollprojectpart.h"

#include <qdir.h>
#include <qfileinfo.h>
#include <qwhatsthis.h>
#include <kdeversion.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <qmessagebox.h>
#include <kgenericfactory.h>
#include <kaction.h>
#include <kparts/part.h>
#include <kprocess.h>
#include <makeoptionswidget.h>


#include "domutil.h"
#include "kdevcore.h"
#include "kdevmainwindow.h"
#include "kdevmakefrontend.h"
#include "kdevappfrontend.h"
#include "kdevpartcontroller.h"
#include "trollprojectwidget.h"
#include "runoptionswidget.h"
#include "config.h"


typedef KGenericFactory<TrollProjectPart> TrollProjectFactory;
K_EXPORT_COMPONENT_FACTORY( libkdevtrollproject, TrollProjectFactory( "kdevtrollproject" ) );

TrollProjectPart::TrollProjectPart(QObject *parent, const char *name, const QStringList& args )
    : KDevProject("TrollProject", "trollproject", parent, name ? name : "TrollProjectPart")
{
    setInstance(TrollProjectFactory::instance());

    if ( args.count() == 1 && args[0] == "TMake" )
        m_tmakeProject = true;
    else
        m_tmakeProject = false;

    setXMLFile("kdevtrollproject.rc");

    m_executeAfterBuild = false;

    m_widget = new TrollProjectWidget(this);
    m_widget->setIcon(SmallIcon("make"));
    m_widget->setCaption(i18n("Project"));
    QWhatsThis::add(m_widget, i18n("Project Tree\n\n"
                                   "The project tree consists of two parts. The 'overview' "
                                   "in the upper half shows the subprojects, each one having a "
                                   ".pro file. The 'details' view in the lower half shows the "
                                   "targets for the active subproject selected in the overview."));

    mainWindow()->embedSelectViewRight(m_widget, i18n("QMake Manager"), i18n("project manager"));

    KAction *action;

    const QIconSet icon(SmallIcon("compfile"));
    action = new KAction( i18n("Compile &File"), icon, 0,
                          m_widget, SLOT(slotBuildFile()),
                          actionCollection(),"build_compilefile"  );


    action = new KAction( i18n("&Build Project"), "make_kdevelop", Key_F8,
                          m_widget, SLOT(slotBuildProject()),
                          actionCollection(), "build_build_project" );

    action = new KAction( i18n("&Rebuild Project"),SmallIcon ( "rebuild" ) , 0,
                          m_widget, SLOT(slotRebuildProject()),
                          actionCollection(),"build_rebuild_project"  );
    
    action = new KAction( i18n("&Clean Project"), 0,
                          m_widget, SLOT(slotCleanProject()),
                          actionCollection(), "build_clean_project" );

    action = new KAction( i18n("Execute Main Program"), "exec", 0,
                          m_widget, SLOT(slotExecuteProject()),
                          actionCollection(), "build_execute_project" );





    action = new KAction( i18n("&Build Target Project"), "make_kdevelop", 0,
                          m_widget, SLOT(slotBuildTarget()),
                          actionCollection(), "build_build_target" );

    action = new KAction( i18n("&Rebuild Target Project"),SmallIcon ( "rebuild" ) , 0,
                          m_widget, SLOT(slotRebuildTarget()),
                          actionCollection(),"build_rebuild_target"  );

    action = new KAction( i18n("&Clean Target Project"), 0,
                          m_widget, SLOT(slotCleanTarget()),
                          actionCollection(), "build_clean_target" );

    action = new KAction( i18n("Execute Target Program"), "exec", 0,
                          m_widget, SLOT(slotExecuteTarget()),
                          actionCollection(), "build_execute_target" );
                          
    connect( core(), SIGNAL(projectConfigWidget(KDialogBase*)),
             this, SLOT(projectConfigWidget(KDialogBase*)) );

    connect( makeFrontend(), SIGNAL(commandFinished(const QString&)),
             this, SLOT(slotCommandFinished(const QString&)) );

    m_qmakeHeader = i18n("# File generated by kdevelop's qmake manager. \n"
                         "# ------------------------------------------- \n"
                         "# Subdir relative project main directory: %s\n"
                         "# Target is %s %s\n");
}


TrollProjectPart::~TrollProjectPart()
{
    if (m_widget)
        mainWindow()->removeView(m_widget);
    delete m_widget;
}

QString TrollProjectPart::makeEnvironment()
{
    // Get the make environment variables pairs into the environstr string
    // in the form of: "ENV_VARIABLE=ENV_VALUE"
    // Note that we quote the variable value due to the possibility of
    // embedded spaces
    DomUtil::PairList envvars =
        DomUtil::readPairListEntry(*projectDom(), "/kdevtrollproject/make/envvars", "envvar", "name", "value");

    QString environstr;
    DomUtil::PairList::ConstIterator it;
    for (it = envvars.begin(); it != envvars.end(); ++it) {
        environstr += (*it).first;
        environstr += "=";
#if (KDE_VERSION > 305)
        environstr += KProcess::quote((*it).second);
#else
        environstr += KShellProcess::quote((*it).second);
#endif
        environstr += " ";
    }
    return environstr;
}

void TrollProjectPart::projectConfigWidget(KDialogBase *dlg)
{
    QVBox *vbox;
    vbox = dlg->addVBoxPage(i18n("Run Options"));
    RunOptionsWidget *optdlg = new RunOptionsWidget(*projectDom(), "/kdevtrollproject", buildDirectory(), vbox);
    
    vbox = dlg->addVBoxPage(i18n("Make Options"));
    MakeOptionsWidget *w4 = new MakeOptionsWidget(*projectDom(), "/kdevtrollproject", vbox);
    connect( dlg, SIGNAL(okClicked()), w4, SLOT(accept()) );

    connect( dlg, SIGNAL(okClicked()), optdlg, SLOT(accept()) );
}


void TrollProjectPart::openProject(const QString &dirName, const QString &projectName)
{
    m_widget->openProject(dirName);
    m_projectName = projectName;
}


void TrollProjectPart::closeProject()
{
    m_widget->closeProject();
}


QString TrollProjectPart::projectDirectory()
{
  return m_widget->projectDirectory();
}


QString TrollProjectPart::buildDirectory()
{
  return m_widget->projectDirectory();
}

QString TrollProjectPart::projectName()
{
    return m_projectName;
}


QString TrollProjectPart::mainProgram()
{

    QDomDocument &dom = *projectDom();
    return QDir::cleanDirPath(projectDirectory() + "/" + DomUtil::readEntry(dom, "/kdevtrollproject/run/mainprogram"));
}


QString TrollProjectPart::activeDirectory()
{
    QDomDocument &dom = *projectDom();

    return DomUtil::readEntry(dom, "/kdevtrollproject/general/activedir");
}


QStringList TrollProjectPart::allFiles()
{
    return m_widget->allFiles();
}


void TrollProjectPart::addFile(const QString &fileName)
{
	QStringList fileList;
	fileList.append ( fileName );
	
	this->addFiles ( fileList );
}

void TrollProjectPart::addFiles ( const QStringList &fileList )
{
	QStringList::ConstIterator it;

	for ( it = fileList.begin(); it != fileList.end(); ++it )
	{
		m_widget->addFile ( *it );
	}

	emit addedFilesToProject ( fileList );
}

void TrollProjectPart::removeFile(const QString & /* fileName */)
{
    // FIXME:
/*	QStringList fileList;
	fileList.append ( fileName );
	
	this->removeFiles ( fileList );*/
}

void TrollProjectPart::removeFiles ( const QStringList& fileList )
{
// FIXME:
// 	QStringList::ConstIterator it;	
// 	
// 	it = fileList.begin();
//
// 	for ( ; it != fileList.end(); ++it )
// 	{
// 		FIXME:
// 	}

	emit removedFilesFromProject ( fileList );
}
/*
void TrollProjectPart::startMakeCommand(const QString &dir, const QString &target)
{
    partController()->saveAllFiles();

    QFileInfo fi(dir + "/Makefile");
    if (!fi.exists()) {
        int r = KMessageBox::questionYesNo(m_widget, i18n("There is no Makefile in this directory. Run qmake first?"));
        if (r == KMessageBox::No)
            return;
        startQMakeCommand(dir);
    }
    QDomDocument &dom = *projectDom();

    if (target=="clean")
    {
      QString cmdline = DomUtil::readEntry(dom, "/kdevtrollproject/make/makebin");
      if (cmdline.isEmpty())
          cmdline = MAKE_COMMAND;
      cmdline += " clean";
      QString dircmd = "cd ";
      dircmd += dir;
      dircmd += " && ";
      cmdline.prepend(makeEnvironment());
      makeFrontend()->queueCommand(dir, dircmd + cmdline);
    }

    QString cmdline = DomUtil::readEntry(dom, "/kdevtrollproject/make/makebin");
    if (cmdline.isEmpty())
        cmdline = MAKE_COMMAND;
    if (!DomUtil::readBoolEntry(dom, "/kdevtrollproject/make/abortonerror"))
        cmdline += " -k";
    int jobs = DomUtil::readIntEntry(dom, "/kdevtrollproject/make/numberofjobs");
    if (jobs != 0) {
        cmdline += " -j";
        cmdline += QString::number(jobs);
    }
    if (DomUtil::readBoolEntry(dom, "/kdevtrollproject/make/dontact"))
        cmdline += " -n";

    cmdline += " ";
    cmdline += target;

    QString dircmd = "cd ";
    dircmd += dir;
    dircmd += " && ";

    cmdline.prepend(makeEnvironment());
    makeFrontend()->queueCommand(dir, dircmd + cmdline);
}
*/

void TrollProjectPart::startQMakeCommand(const QString &dir)
{
    QFileInfo fi(dir);

    QString cmdline = QString::fromLatin1( isTMakeProject() ? "tmake " : "qmake " );
//    cmdline += fi.baseName() + ".pro";
    QDir d(dir);
    QStringList l = d.entryList("*.pro");

    cmdline += l.count()?l[0]:(fi.baseName() + ".pro");

//    cmdline += QString::fromLatin1( " -o Makefile" );

    QString dircmd = "cd ";
    dircmd += dir;
    dircmd += " && ";

    cmdline.prepend(makeEnvironment());
    makeFrontend()->queueCommand(dir, dircmd + cmdline);
}

void TrollProjectPart::queueCmd(const QString &dir, const QString &cmd)
{
    makeFrontend()->queueCommand(dir, cmd);
}

void TrollProjectPart::slotCommandFinished( const QString& command )
{
    Q_UNUSED( command );

    if( m_buildCommand != command )
        return;

    m_buildCommand = QString::null;

    m_timestamp.clear();
    QStringList fileList = allFiles();
    QStringList::Iterator it = fileList.begin();
    while( it != fileList.end() ){
        QString fileName = *it;
        ++it;

        m_timestamp[ fileName ] = QFileInfo( projectDirectory(), fileName ).lastModified();
    }

    emit projectCompiled();

    if( m_executeAfterBuild ){
        m_widget->slotExecuteProject();
        m_executeAfterBuild = false;
    }
}

bool TrollProjectPart::isDirty()
{
    QStringList fileList = allFiles();
    QStringList::Iterator it = fileList.begin();
    while( it != fileList.end() ){
        QString fileName = *it;
        ++it;

        QMap<QString, QDateTime>::Iterator it = m_timestamp.find( fileName );
        QDateTime t = QFileInfo( projectDirectory(), fileName ).lastModified();
        if( it == m_timestamp.end() || *it != t ){
            return true;
        }
    }

    return false;
}



#include "trollprojectpart.moc"
