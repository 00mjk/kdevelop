/***************************************************************************
 *   Copyright 2006 Alexander Dymo <adymo@kdevelop.org>             *
 *   Copyright 2007 Andreas Pakulat <apaku@gmx.de>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
#include "kdevfilemanagerplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <interfaces/icore.h>
#include <interfaces/iuicontroller.h>

#include "filemanager.h"

K_PLUGIN_FACTORY_WITH_JSON(KDevFileManagerFactory, "kdevfilemanager.json", registerPlugin<KDevFileManagerPlugin>();)

class KDevFileManagerViewFactory: public KDevelop::IToolViewFactory{
public:
    explicit KDevFileManagerViewFactory(KDevFileManagerPlugin *plugin): m_plugin(plugin) {}
    QWidget* create(QWidget *parent = nullptr) override
    {
        Q_UNUSED(parent)
        return new FileManager(m_plugin,parent);
    }

    QList<QAction*> toolBarActions( QWidget* w ) const override
    {
        auto* m = qobject_cast<FileManager*>(w);
        if( m )
            return m->toolBarActions();
        return KDevelop::IToolViewFactory::toolBarActions( w );
    }

    Qt::DockWidgetArea defaultPosition() const override
    {
        return Qt::LeftDockWidgetArea;
    }

    QString id() const override
    {
        return QStringLiteral("org.kdevelop.FileManagerView");
    }

    bool allowMultiple() const override
    {
        return true;
    }

private:
    KDevFileManagerPlugin *m_plugin;
};

KDevFileManagerPlugin::KDevFileManagerPlugin(QObject *parent, const QVariantList &/*args*/)
    :KDevelop::IPlugin(QStringLiteral("kdevfilemanager"), parent)
{
    setXMLFile(QStringLiteral("kdevfilemanager.rc"));

    QMetaObject::invokeMethod(this, "init", Qt::QueuedConnection);
}

void KDevFileManagerPlugin::init()
{
    m_factory = new KDevFileManagerViewFactory(this);
    core()->uiController()->addToolView(i18n("Filesystem"), m_factory);
}

KDevFileManagerPlugin::~KDevFileManagerPlugin()
{
}

void KDevFileManagerPlugin::unload()
{
    core()->uiController()->removeToolView(m_factory);
}

#include "kdevfilemanagerplugin.moc"

