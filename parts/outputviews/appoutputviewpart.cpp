/***************************************************************************
 *   Copyright (C) 1999-2002 by Bernd Gehrmann                             *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "appoutputviewpart.h"

#include <qwhatsthis.h>
#include <qdir.h>
#include <kdebug.h>
#include <klocale.h>
#include <kaction.h>
#include <kiconloader.h>

#include "kdevproject.h"
#include "kdevcore.h"
#include "kdevmainwindow.h"
#include "outputviewsfactory.h"
#include "appoutputwidget.h"


AppOutputViewPart::AppOutputViewPart(QObject *parent, const char *name, const QStringList &)
    : KDevAppFrontend("ApplicationOutput", "appoutput", parent, name ? name : "AppOutputViewPart")
{
    setInstance(OutputViewsFactory::instance());

    m_dcop = new KDevAppFrontendIface(this);

    m_widget = new AppOutputWidget(this);
    m_widget->setIcon( SmallIcon("openterm") );
    m_widget->setCaption(i18n("Application Output"));
    QWhatsThis::add(m_widget, i18n("<b>Application output</b><p>"
                                   "The stdout/stderr output window is a replacement for "
                                   "terminal-based application communication. Running terminal "
                                   "applications use this instead of a terminal window."));

    mainWindow()->embedOutputView(m_widget, i18n("Application"), i18n("Output of the executed user program"));

    connect( core(), SIGNAL(stopButtonClicked(KDevPlugin*)),
             this, SLOT(slotStopButtonClicked(KDevPlugin*)) );
    connect(m_widget, SIGNAL(processExited(KProcess*)), SIGNAL(processExited()));
}


AppOutputViewPart::~AppOutputViewPart()
{
	if ( m_widget )
    	mainWindow()->removeView( m_widget );
    delete m_widget;
    delete m_dcop;
}

void AppOutputViewPart::slotStopButtonClicked( KDevPlugin* which )
{
    if ( which != 0 && which != this )
        return;
    stopApplication();
}

void AppOutputViewPart::stopApplication()
{
    m_widget->killJob();
}

/**
  * If directory is empty it will use the user's home directory.
  * If inTerminal is true, the program is started in an external
  * konsole.
  */
void AppOutputViewPart::startAppCommand(const QString &directory, const QString &program, bool inTerminal)
{
    QString cmd;

    if (inTerminal) {
        cmd = "konsole";
        if ( !directory.isNull() ) {
          // If a directory was specified, use it
          cmd += " --workdir " + directory;
        }
        cmd += " -e /bin/sh -c '";
        cmd += program;
        cmd += "; echo \"";
        cmd += i18n("Press Enter to continue!");
        cmd += "\";read'";
    } else
        cmd = program;

    if ( directory.isNull() )
      // use the user's home directory
      m_widget->startJob(QDir::homeDirPath(), cmd);
    else
      // use the supplied directory
      m_widget->startJob(directory, cmd);
    mainWindow()->raiseView(m_widget);
}


bool AppOutputViewPart::isRunning()
{
    return m_widget->isRunning();
}


void AppOutputViewPart::insertStdoutLine(const QString &line)
{
    m_widget->insertStdoutLine(line);
}


void AppOutputViewPart::insertStderrLine(const QString &line)
{
    m_widget->insertStderrLine(line);
}

void AppOutputViewPart::clearView()
{
    m_widget->clear();
}

#include "appoutputviewpart.moc"
