/*
 *  Copyright (C) 2003 Roberto Raggi (roberto@kdevelop.org)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 *
 */

#include "quickopen_part.h"
#include "quickopenclassdialog.h"
#include "quickopenfunctiondialog.h"
#include "quickopenfiledialog.h"

#include <kaction.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kdevgenericfactory.h>

#include <kmainwindow.h>

#include <kdevmainwindow.h>
#include <kdevcore.h>
#include <kdevpartcontroller.h>
#include <kdevproject.h>

typedef KDevGenericFactory<QuickOpenPart> QuickOpenFactory;
static const KAboutData data("kdevquickopen", I18N_NOOP("Quick Open..."), "1.0");
K_EXPORT_COMPONENT_FACTORY( libkdevquickopen, QuickOpenFactory( &data ) )

QuickOpenPart::QuickOpenPart(QObject *parent, const char *name, const QStringList& )
    : KDevPlugin("KDevPart", "kdevpart", parent, name ? name : "QuickOpenPart" )
{
    setInstance(QuickOpenFactory::instance());
    setXMLFile("kdevpart_quickopen.rc");

    m_actionQuickOpen = new KAction( i18n("Quick Open File..."), CTRL + SHIFT + Key_O,
				       this, SLOT(slotQuickFileOpen()),
				       actionCollection(), "quick_open" );
    m_actionQuickOpen->setToolTip(i18n("Quick open file in project"));
    m_actionQuickOpen->setWhatsThis(i18n("<b>Quick open</b><p>Provides a file name input form with completion listbox to quickly open file in a project."));

    m_actionQuickOpenClass = new KAction( i18n("Quick Open Class..."), CTRL + ALT + Key_C,
				          this, SLOT(slotQuickOpenClass()),
				          actionCollection(), "quick_open_class" );
    m_actionQuickOpenClass->setToolTip(i18n("Find class in project"));
    m_actionQuickOpenClass->setWhatsThis(i18n("<b>Find class</b><p>Provides a class name input form with completion listbox to quickly open a file where the class is defined."));

    m_actionFunctionOpen = new KAction( i18n("Quick Open Method..."), CTRL + ALT + Key_M, this, SLOT(slotQuickOpenFunction()), actionCollection(), "quick_open_function" );
    m_actionFunctionOpen->setToolTip(i18n("Quick open function in project"));

    connect( core(), SIGNAL(projectOpened()), this, SLOT(slotProjectOpened()) );
    connect( core(), SIGNAL(projectClosed()), this, SLOT(slotProjectClosed()) );
}


QuickOpenPart::~QuickOpenPart()
{
}

void QuickOpenPart::slotProjectOpened( )
{
}

void QuickOpenPart::slotProjectClosed( )
{
}

void QuickOpenPart::slotQuickFileOpen( )
{
    QuickOpenFileDialog dlg( this, mainWindow()->main() );
    dlg.exec();
}

void QuickOpenPart::slotQuickOpenClass( )
{
    QuickOpenClassDialog dlg( this, mainWindow()->main() );
    dlg.exec();
}

void QuickOpenPart::slotQuickOpenFunction()
{
    QuickOpenFunctionDialog dlg( this, mainWindow()->main() );
    dlg.exec();
}

#include "quickopen_part.moc"
