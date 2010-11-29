/*  This file is part of KDevelop
    Copyright 2010 Yannick Motta <yannick.motta@gmail.com>
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

#include <KPluginFactory>
#include <KPluginLoader>
#include <KAboutData>
#include <KMimeType>
#include <KSettings/Dispatcher>
#include <KUrl>
#include <KIcon>

#include <interfaces/idocumentation.h>

#include <language/duchain/duchain.h>
#include <language/duchain/declaration.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/indexedstring.h>
#include <language/duchain/parsingenvironment.h>

#include <language/duchain/classdeclaration.h>
#include <language/duchain/functiondeclaration.h>
#include <language/duchain/classmemberdeclaration.h>
#include <language/duchain/classfunctiondeclaration.h>

#include <QtCore/QFile>

#include <interfaces/icore.h>
#include <interfaces/idocumentationcontroller.h>

#include <QProgressBar>

#include "manpageplugin.h"
#include "manpagedocumentation.h"
#include "manpagemodel.h"

using namespace KDevelop;

K_PLUGIN_FACTORY(ManPageFactory, registerPlugin<ManPagePlugin>(); )
K_EXPORT_PLUGIN(ManPageFactory(KAboutData("kdevmanpage","kdevmanpage", ki18n("ManPage"),
                                          "1", ki18n("Check Man Page documentation"),
                               KAboutData::License_GPL)))

ManPagePlugin::ManPagePlugin(QObject* parent, const QVariantList& args)
    : IPlugin(ManPageFactory::componentData(), parent)
{
    KDEV_USE_EXTENSION_INTERFACE( KDevelop::IDocumentationProvider )
    Q_UNUSED(args);
    ManPageDocumentation::s_provider = this;
    m_model = new ManPageModel(this);
    QObject::connect(m_model, SIGNAL(sectionCount(int)), this, SLOT(sectionCount(int)) );
    QObject::connect(m_model, SIGNAL(sectionParsed()), this, SLOT(sectionParsed()) );
    m_progressBar = new QProgressBar();
}

ManPagePlugin::~ManPagePlugin()
{
    delete m_model;
}

QString ManPagePlugin::name() const
{
    return QString("Man Page");
}

QIcon ManPagePlugin::icon() const
{
    static KIcon icon("x-office-address-book");
    return icon;
}

ManPageModel* ManPagePlugin::model() const{
    return m_model;
}

QProgressBar* ManPagePlugin::progressBar() const{
    return m_progressBar;
}

void ManPagePlugin::deleteProgressBar() {
    delete m_progressBar;
    m_progressBar = 0;
}

KSharedPtr< IDocumentation > ManPagePlugin::documentationForDeclaration( Declaration* dec ) const
{
    Q_ASSERT(dec);
    Q_ASSERT(dec->topContext());
    Q_ASSERT(dec->topContext()->parsingEnvironmentFile());
    static const KDevelop::IndexedString cppLanguage("C++");
    if (dec->topContext()->parsingEnvironmentFile()->language() != cppLanguage) {
        return KSharedPtr<IDocumentation>();
    }

    QString identifier = dec->identifier().toString();
    if(m_model->containsIdentifier(identifier)){
        KDevelop::DUChainReadLocker lock;
        KDevelop::QualifiedIdentifier qid = dec->qualifiedIdentifier();
        if(qid.count() == 1){
            return KSharedPtr<IDocumentation>(new ManPageDocumentation(qMakePair(identifier+"a", KUrl("man:"+identifier))));
        }
    }
    return  KSharedPtr<IDocumentation>();
}

QAbstractListModel* ManPagePlugin::indexModel() const
{
    return m_model->indexList();
}

KSharedPtr< IDocumentation > ManPagePlugin::documentationForIndex(const QModelIndex& index) const
{
    QString name = index.data().toString();
    return KSharedPtr<IDocumentation>(new ManPageDocumentation(qMakePair(name, KUrl("man:"+name))));
}

KSharedPtr< IDocumentation > ManPagePlugin::homePage() const
{
    return KSharedPtr<KDevelop::IDocumentation>(new ManPageHomeDocumentation);
}

void ManPagePlugin::sectionParsed()
{
   m_progressBar->setValue(m_progressBar->value() + 1);
}

void ManPagePlugin::sectionCount(int count)
{
   m_progressBar->setMinimum(0);
   m_progressBar->setMaximum(count);
   m_progressBar->setValue(0);
}

#include "manpageplugin.moc"
