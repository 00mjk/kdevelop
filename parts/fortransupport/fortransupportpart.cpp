/***************************************************************************
 *   Copyright (C) 2001 by Bernd Gehrmann                                  *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   The tooltips for ftnchek contained in this source file are taken      *
 *   from the ftnchek man page. ftnchek is written by Robert Moniot and    *
 *   others.                                                               *
 *                                                                         *
 ***************************************************************************/

#include <qfileinfo.h>
#include <qpopupmenu.h>
#include <qstringlist.h>
#include <qtextstream.h>
#include <qtimer.h>
#include <qvbox.h>
#include <kapp.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kregexp.h>

#include "kdevcore.h"
#include "kdevproject.h"
#include "kdevmakefrontend.h"
#include "classstore.h"
#include "parsedmethod.h"
#include "domutil.h"

#include "ftnchekconfigwidget.h"
#include "fixedformparser.h"
#include "fortransupportpart.h"
#include "fortransupportfactory.h"


FortranSupportPart::FortranSupportPart(KDevApi *api, QObject *parent, const char *name)
    : KDevLanguageSupport(api, parent, name)
{
    setInstance(FortranSupportFactory::instance());
    
    setXMLFile("kdevfortransupport.rc");

    connect( core(), SIGNAL(projectConfigWidget(KDialogBase*)),
             this, SLOT(projectConfigWidget(KDialogBase*)) );
    connect( core(), SIGNAL(projectOpened()), this, SLOT(projectOpened()) );
    connect( core(), SIGNAL(projectClosed()), this, SLOT(projectClosed()) );
    connect( core(), SIGNAL(savedFile(const QString&)),
             this, SLOT(savedFile(const QString&)) );

    KAction *action;

    action = new KAction( i18n("&Ftnchek"), 0,
                          this, SLOT(slotFtnchek()),
                          actionCollection(), "project_ftnchek" );

    parser = 0;
}


FortranSupportPart::~FortranSupportPart()
{}


void FortranSupportPart::slotFtnchek()
{
   // Do something smarter here...
    if (makeFrontend()->isRunning()) {
        KMessageBox::sorry(0, i18n("There is currently a job running."));
        return;
    }

    core()->saveAllFiles();

    QDomDocument &dom = *projectDom();
    
    QString cmdline = "cd ";
    cmdline += project()->projectDirectory();
    cmdline += "&& ftnchek -nonovice ";

    if (DomUtil::readBoolEntry(dom, "/kdevfortransupport/ftnchek/division"))
        cmdline += "-division ";
    if (DomUtil::readBoolEntry(dom, "/kdevfortransupport/ftnchek/extern"))
        cmdline += "-extern ";
    if (DomUtil::readBoolEntry(dom, "/kdevfortransupport/ftnchek/declare"))
        cmdline += "-declare ";
    if (DomUtil::readBoolEntry(dom, "/kdevfortransupport/ftnchek/pure"))
        cmdline += "-pure ";
    
    cmdline += "-arguments=";
    if (DomUtil::readBoolEntry(dom, "/kdevfortransupport/ftnchek/argumentsall"))
        cmdline += "all ";
    else
        cmdline += DomUtil::readEntry(dom, "/kdevfortransupport/ftnchek/argumentsonly") + " ";

    cmdline += "-common=";
    if (DomUtil::readBoolEntry(dom, "/kdevfortransupport/ftnchek/commonall"))
        cmdline += "all ";
    else
        cmdline += DomUtil::readEntry(dom, "/kdevfortransupport/ftnchek/commononly") + " ";

    cmdline += "-truncation=";
    if (DomUtil::readBoolEntry(dom, "/kdevfortransupport/ftnchek/truncationall"))
        cmdline += "all ";
    else
        cmdline += DomUtil::readEntry(dom, "/kdevfortransupport/ftnchek/truncationonly") + " ";

    cmdline += "-usage=";
    if (DomUtil::readBoolEntry(dom, "/kdevfortransupport/ftnchek/usageall"))
        cmdline += "all ";
    else
        cmdline += DomUtil::readEntry(dom, "/kdevfortransupport/ftnchek/usageonly") + " ";

    cmdline += "-f77=";
    if (DomUtil::readBoolEntry(dom, "/kdevfortransupport/ftnchek/f77all"))
        cmdline += "all ";
    else
        cmdline += DomUtil::readEntry(dom, "/kdevfortransupport/ftnchek/f77only") + " ";

    cmdline += "-portability=";
    if (DomUtil::readBoolEntry(dom, "/kdevfortransupport/ftnchek/portabilityall"))
        cmdline += "all ";
    else
        cmdline += DomUtil::readEntry(dom, "/kdevfortransupport/ftnchek/portabilityonly") + " ";

    QStringList list = project()->allSourceFiles();
    QStringList::ConstIterator it;
    for (it = list.begin(); it != list.end(); ++it) {
        QFileInfo fi(*it);
        QString extension = fi.extension();
        if (extension == "f77" || extension == "f" || extension == "for"
            || extension == "ftn") {
            cmdline += *it + " ";
        }
    }
    
    makeFrontend()->queueCommand(QString::null, cmdline);
}


void FortranSupportPart::projectConfigWidget(KDialogBase *dlg)
{
    QVBox *vbox = dlg->addVBoxPage(i18n("Ftnchek"));
    FtnchekConfigWidget *w = new FtnchekConfigWidget(*projectDom(), vbox, "ftnchek config widget");
    connect( dlg, SIGNAL(okClicked()), w, SLOT(accept()) );
}


void FortranSupportPart::projectOpened()
{
    kdDebug(9019) << "projectOpened()" << endl;

    connect( project(), SIGNAL(addedFileToProject(const QString &)),
             this, SLOT(addedFileToProject(const QString &)) );
    connect( project(), SIGNAL(removedFileFromProject(const QString &)),
             this, SLOT(removedFileFromProject(const QString &)) );

    // We want to parse only after all components have been
    // properly initialized
    parser = new FixedFormParser(classStore());

    QTimer::singleShot(0, this, SLOT(initialParse()));
}


void FortranSupportPart::projectClosed()
{
    delete parser;
    parser = 0;
}


void FortranSupportPart::maybeParse(const QString fileName)
{
    QFileInfo fi(fileName);
    QString extension = fi.extension();
    if (extension == "f77" || extension == "f" || extension == "for"
        || extension == "ftn") {
        classStore()->removeWithReferences(fileName);
        parser->parse(fileName);
    }
}


void FortranSupportPart::initialParse()
{
    kdDebug(9019) << "initialParse()" << endl;
    
    if (project()) {
        kapp->setOverrideCursor(waitCursor);
        QStringList files = project()->allSourceFiles();
        for (QStringList::Iterator it = files.begin(); it != files.end() ;++it) {
            kdDebug(9019) << "maybe parse " << (*it) << endl;
            maybeParse(*it);
        }
        
        emit updatedSourceInfo();
        kapp->restoreOverrideCursor();
    } else {
        kdDebug(9019) << "No project" << endl;
    }
}


void FortranSupportPart::addedFileToProject(const QString &fileName)
{
    kdDebug(9019) << "addedFileToProject()" << endl;
    maybeParse(fileName);
    emit updatedSourceInfo();
}


void FortranSupportPart::removedFileFromProject(const QString &fileName)
{
    kdDebug(9019) << "removedFileFromProject()" << endl;
    classStore()->removeWithReferences(fileName);
    emit updatedSourceInfo();
}


void FortranSupportPart::savedFile(const QString &fileName)
{
    kdDebug(9019) << "savedFile()" << endl;

    if (project()->allSourceFiles().contains(fileName)) {
        maybeParse(fileName);
        emit updatedSourceInfo();
    }
}


KDevLanguageSupport::Features FortranSupportPart::features()
{
    return Features(Functions);
}

#include "fortransupportpart.moc"
