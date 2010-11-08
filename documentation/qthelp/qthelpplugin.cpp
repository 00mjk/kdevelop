/*  This file is part of KDevelop
    Copyright 2009 Aleix Pol <aleixpol@kde.org>
    Copyright 2010 Benjamin Port <port.benjamin@gmail.com>

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

#include "qthelpplugin.h"

#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <KAboutData>
#include <KSettings/Dispatcher>
#include <KComponentData>
#include <interfaces/icore.h>
#include <interfaces/idocumentationcontroller.h>
#include <kconfiggroup.h>
#include "kdebug.h"
#include "qthelpprovider.h"
#include "qthelpqtdoc.h"
#include "qthelpconfig.h"

QtHelpPlugin *QtHelpPlugin::s_plugin = 0;

K_PLUGIN_FACTORY_DEFINITION(QtHelpFactory,
                 registerPlugin<QtHelpPlugin>();
registerPlugin<QtHelpConfig>();
)
K_EXPORT_PLUGIN(QtHelpFactory(KAboutData("kdevqthelp","kdevqthelp", ki18n("QtHelp"), "0.1", ki18n("Check Qt Help documentation"), KAboutData::License_GPL)))

QtHelpPlugin::QtHelpPlugin(QObject* parent, const QVariantList& args)
    : KDevelop::IPlugin(QtHelpFactory::componentData(), parent)
    , m_qtHelpProviders()
    , m_qtDoc(0)
{
    KDEV_USE_EXTENSION_INTERFACE( KDevelop::IDocumentationProviderProvider )

    Q_UNUSED(args);
    s_plugin = this;
    readConfig();
    KSettings::Dispatcher::registerComponent( KComponentData("kdevqthelp"),
                                                    this, "readConfig" );
    connect(this, SIGNAL(changedProvidersList()), KDevelop::ICore::self()->documentationController(), SLOT(changedDoucmentationProviders()));
}

QtHelpPlugin::~QtHelpPlugin()
{
}


void QtHelpPlugin::readConfig()
{
    KConfigGroup cg(KGlobal::config(), "QtHelp Documentation");
    QStringList iconList = cg.readEntry("iconList", QStringList());
    QStringList nameList = cg.readEntry("nameList", QStringList());
    QStringList pathList = cg.readEntry("pathList", QStringList());
    bool loadQtDoc = cg.readEntry("loadQtDocs", true);
    foreach(QtHelpProvider* provider, m_qtHelpProviders) {
        m_qtHelpProviders.removeAll(provider);
        delete provider;
    }
    for(int i=0; i < pathList.length(); i++) {
        m_qtHelpProviders.append(new QtHelpProvider(this, QtHelpFactory::componentData(), pathList.at(i), nameList.at(i), iconList.at(i),QVariantList()));
    }

    if(m_qtDoc&&!loadQtDoc){
        delete m_qtDoc;
        m_qtDoc = 0;
    } else if(!m_qtDoc&&loadQtDoc) {
        m_qtDoc = new QtHelpQtDoc(this, QtHelpFactory::componentData(), QVariantList());
    }
    emit changedProvidersList();
}

void QtHelpPlugin::writeConfig(QStringList iconList, QStringList nameList, QStringList pathList, bool loadQtDoc)
{
    KConfigGroup cg(KGlobal::config(), "QtHelp Documentation");
    cg.writeEntry("iconList", iconList);
    cg.writeEntry("nameList", nameList);
    cg.writeEntry("pathList", pathList);
    cg.writeEntry("loadQtDocs", loadQtDoc);
}

void QtHelpPlugin::setQtDoc(QtHelpQtDoc* qtDoc)
{
    m_qtDoc = qtDoc;
}

QList<KDevelop::IDocumentationProvider*> QtHelpPlugin::providers()
{
    QList<KDevelop::IDocumentationProvider*> list;
    foreach(QtHelpProvider* provider, m_qtHelpProviders) {
        list.append(provider);
    }
    if(m_qtDoc){
        list.append(m_qtDoc);
    }
    return list;
}

QList<QtHelpProvider*> QtHelpPlugin::qtHelpProviderLoaded()
{
    return m_qtHelpProviders;
}

bool QtHelpPlugin::qtHelpQtDocLoaded(){
    return m_qtDoc;
}
