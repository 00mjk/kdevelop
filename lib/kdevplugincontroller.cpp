/* This file is part of the KDE project
Copyright (C) 2004 Alexander Dymo <adymo@kdevelop.org>
Copyright     2006 Matt Rogers <mattr@kde.org

Based on code from Kopete
Copyright (c) 2002-2003 by Martijn Klingens <klingens@kde.org>

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
#include "kdevplugincontroller.h"

#include <QFile>
#include <QTimer>
#include <QApplication>
#include <kcmdlineargs.h>
#include <klibloader.h>
#include <kservice.h>
#include <kservicetypetrader.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <klocale.h>
#include <kmainwindow.h>
#include <kparts/componentfactory.h>
#include <assert.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <kaction.h>
#include <kaction.h>
#include <kxmlguifactory.h>
#include <kstaticdeleter.h>

#include "kdevcore.h"
#include "kdevplugin.h"
#include "kdevmakeinterface.h"
#include "kdevprofileengine.h"
#include "kdevapplicationinterface.h"
#include "kdevprojectcontroller.h"
#include "kdevdiffinterface.h"
#include "kdevcreatefile.h"
#include "kdevmainwindow.h"


#include "kdevcore.h" 
/*#include "partselectwidget.h"*/
#include "shellextension.h"

namespace Koncrete
{

class PluginController::Private
{
public:
    QList<KPluginInfo*> plugins;

    //map plugin infos to currently loaded plugins
    typedef QMap<KPluginInfo*, Plugin*> InfoToPluginMap;
    InfoToPluginMap loadedPlugins;

    // The plugin manager's mode. The mode is StartingUp until loadAllPlugins()
    // has finished loading the plugins, after which it is set to Running.
    // ShuttingDown and DoneShutdown are used during Kopete shutdown by the
    // async unloading of plugins.
    enum ShutdownMode { Running, ShuttingDown, DoneShutdown };
    ShutdownMode shutdownMode;

    QString profile;
    ProfileEngine engine;

    static KStaticDeleter<PluginController> deleter;
};

KStaticDeleter<PluginController> PluginController::Private::deleter;
PluginController* PluginController::s_self = 0L;

PluginController* PluginController::self()
{
    if ( !s_self )
        Private::deleter.setObject( s_self, new PluginController() );

    return s_self;
}

PluginController::PluginController()
    : QObject( qApp )
{
    d = new Private;
    d->profile = ShellExtension::getInstance() ->defaultProfile();
    d->plugins = KPluginInfo::fromServices( KServiceTypeTrader::self()->query( QLatin1String( "KDevelop/Plugin" ),
        QLatin1String( "[X-KDevelop-Version] == 4" ) ) );
    d->shutdownMode = Private::Running;
    KGlobal::ref();
}

PluginController::~PluginController()
{
    if ( d->shutdownMode != Private::DoneShutdown )
        kWarning(9000) << k_funcinfo << "Destructing plugin controller without going through the shutdown process! Backtrace is: " << endl << kBacktrace() << endl;

    // Quick cleanup of the remaining plugins, hope it helps
    // Note that deleting it.value() causes slotPluginDestroyed to be called, which
    // removes the plugin from the list of loaded plugins.
    while ( !d->loadedPlugins.empty() )
    {
        Private::InfoToPluginMap::ConstIterator it = d->loadedPlugins.begin();
        kWarning(9000) << k_funcinfo << "Deleting stale plugin '" << it.key()->pluginName()
                << "'" << endl;
        delete it.value();
    }

    delete d;
}

QString PluginController::currentProfile() const
{
    return d->profile;
}

ProfileEngine& PluginController::engine() const
{
    return d->engine;
}

KPluginInfo* PluginController::pluginInfo( Plugin* plugin ) const
{
    for ( Private::InfoToPluginMap::ConstIterator it = d->loadedPlugins.begin();
          it != d->loadedPlugins.end(); ++it )
    {
        if ( it.value() == plugin )
            return it.key();
    }
    return 0;
}

void PluginController::shutdown()
{
    if(d->shutdownMode != Private::Running)
    {
        kDebug(9000) << k_funcinfo << "called when not running. state = " << d->shutdownMode << endl;
        return;
    }

    d->shutdownMode = Private::ShuttingDown;
    

    // Ask all plugins to unload
    for ( Private::InfoToPluginMap::ConstIterator it = d->loadedPlugins.begin();
          it != d->loadedPlugins.end(); /* EMPTY */ )
    {
        // Plugins could emit their ready for unload signal directly in response to this,
        // which would invalidate the current iterator. Therefore, we copy the iterator
        // and increment it beforehand.
        Private::InfoToPluginMap::ConstIterator current( it );
        ++it;

        //Let the plugin do some stuff before unloading
        current.value()->prepareForUnload();
    }

    // When running under valgrind, don't enable the timer because it will almost
    // certainly fire due to valgrind's much slower processing
#if defined(HAVE_VALGRIND_H) && !defined(NDEBUG)
        if ( RUNNING_ON_VALGRIND )
            kDebug(9000) << k_funcinfo << "Running under valgrind, disabling plugin unload timeout guard" << endl;
        else
#endif
    
    QTimer::singleShot( 3000, this, SLOT( shutdownTimeout() ) );
}

KPluginInfo::List PluginController::query( const QString &serviceType,
        const QString &constraint )
{
    
    KPluginInfo::List infoList;
    KService::List serviceList = KServiceTypeTrader::self() ->query( serviceType,
            QString( "%1 and [X-KDevelop-Version] == %2" ).arg( constraint ).arg( KDEVELOP_PLUGIN_VERSION ) );
    
    infoList = KPluginInfo::fromServices( serviceList );
    return infoList;
}

KPluginInfo::List PluginController::queryPlugins( const QString &constraint )
{
    return query( "KDevelop/Plugin", constraint );
}

Plugin* PluginController::loadPlugin( const QString& pluginId )
{
    return loadPluginInternal( pluginId );
}

void PluginController::loadPlugins( PluginType type )
{
    KPluginInfo::List offers = d->engine.offers( d->profile, type );
    foreach( KPluginInfo* pi, offers )
    {
        loadPluginInternal( pi->pluginName() );
    }
}

void PluginController::unloadPlugins( PluginType type )
{
    //TODO see if this can be optimized so it's not something like O(n^2)
    KPluginInfo::List offers = d->engine.offers( d->profile, type );
    foreach( KPluginInfo* pi, offers )
    {
        foreach ( KPluginInfo* lpi, d->loadedPlugins.keys() )
        {
            if ( pi->pluginName() == lpi->pluginName() )
                unloadPlugin( pi->pluginName() );
        }
    }
}

QStringList PluginController::argumentsFromService( const KService::Ptr &service )
{
    QStringList args;
    if ( !service )
        // service is a reference to a pointer, so a check whether it is 0 is still required
        return args;
    QVariant prop = service->property( "X-KDevelop-Args" );
    if ( prop.isValid() )
        args = prop.toString().split( " " );
    return args;
}

QList<Plugin *> PluginController::loadedPlugins() const
{
    return d->loadedPlugins.values();
}

Plugin * PluginController::getExtension( const QString & serviceType, const QString & constraint )
{
    KPluginInfo::List offers = PluginController::query( serviceType, constraint );
    Plugin* ext = 0;
    for ( KPluginInfo::List::const_iterator it = offers.constBegin(); it != offers.constEnd(); ++it )
    {
        if  ( d->loadedPlugins.contains( (*it) ) )
        {
            ext = d->loadedPlugins[(*it)];
            break;
        }
    }
    return ext;
}


void PluginController::unloadPlugin( const QString & pluginId )
{
    if( Plugin *thePlugin = plugin( pluginId ) )
    {
        thePlugin->prepareForUnload();
    }
}

KUrl::List PluginController::profileResources( const QString &nameFilter )
{
    return d->engine.resources( currentProfile(), nameFilter );
}

KUrl::List PluginController::profileResourcesRecursive( const QString &nameFilter )
{
    return d->engine.resourcesRecursive( currentProfile(), nameFilter );
}

QString PluginController::changeProfile( const QString &newProfile )
{
    /* FIXME disabled for now
    QStringList unload;
    KService::List coreLoad;
    KService::List globalLoad;
    d->engine.diffProfiles( ProfileEngine::Core,
                           currentProfile(),
                           newProfile,
                           unload,
                           coreLoad );
    d->engine.diffProfiles( ProfileEngine::Global,
                           currentProfile(),
                           newProfile,
                           unload,
                           globalLoad );

    QString oldProfile = d->profile;
    d->profile = newProfile;

    unloadPlugins( unload );
    loadPlugins( coreLoad );
    loadPlugins( globalLoad );

    return oldProfile;
    */
    return QString();
}

KPluginInfo * PluginController::infoForPluginId( const QString &pluginId ) const
{
    QList<KPluginInfo *>::ConstIterator it;
    for ( it = d->plugins.begin(); it != d->plugins.end(); ++it )
    {
        if ( ( *it )->pluginName() == pluginId )
            return *it;
    }

    return 0L;
}

Plugin *PluginController::loadPluginInternal( const QString &pluginId )
{
    KPluginInfo *info = infoForPluginId( pluginId );
    if ( !info )
    {
        kWarning(9000) << k_funcinfo << "Unable to find a plugin named '" << pluginId << "'!" << endl;
        return 0L;
    }

    if ( d->loadedPlugins.contains( info ) )
        return d->loadedPlugins[ info ];

    kDebug(9000) << k_funcinfo << "Attempting to load '" << pluginId << "'" << endl;
    emit loadingPlugin( info->name() );
    int error = 0;
    Plugin *plugin = KServiceTypeTrader::createInstanceFromQuery<Plugin>( QLatin1String( "KDevelop/Plugin" ),
            QString::fromLatin1( "[X-KDE-PluginInfo-Name]=='%1'" ).arg( pluginId ), this, QStringList(), &error );

    if ( plugin )
    {
        d->loadedPlugins.insert( info, plugin );
        info->setPluginEnabled( true );

        connect( plugin, SIGNAL( destroyed( QObject * ) ), 
                 this, SLOT( pluginDestroyed( QObject * ) ) );
        connect( plugin, SIGNAL( readyToUnload( Plugin*) ), 
                 this, SLOT( pluginReadyForUnload( Plugin* ) ) );

        kDebug(9000) << k_funcinfo << "Successfully loaded plugin '" << pluginId << "'" << endl;

        emit pluginLoaded( plugin );
    }
    else
    {
        switch( error )
        {
            case KLibLoader::ErrNoServiceFound:
                kDebug(9000) << k_funcinfo << "No service implementing the given mimetype "
                        << "and fullfilling the given constraint expression can be found." << endl;
                break;

            case KLibLoader::ErrServiceProvidesNoLibrary:
                kDebug(9000) << "the specified service provides no shared library." << endl;
                break;

            case KLibLoader::ErrNoLibrary:
                kDebug(9000) << "the specified library could not be loaded." << endl;
                break;

            case KLibLoader::ErrNoFactory:
                kDebug(9000) << "the library does not export a factory for creating components." << endl;
                break;

            case KLibLoader::ErrNoComponent:
                kDebug(9000) << "the factory does not support creating components of the specified type." << endl;
                break;
        }

        kDebug(9000) << k_funcinfo << "Loading plugin '" << pluginId 
                << "' failed, KLibLoader reported error: '" << endl <<
                KLibLoader::self()->lastErrorMessage() << "'" << endl;
    }

    return plugin;
}


Plugin* PluginController::plugin( const QString& pluginId )
{
    KPluginInfo *info = infoForPluginId( pluginId );
    if ( !info )
        return 0L;

    if ( d->loadedPlugins.contains( info ) )
        return d->loadedPlugins[ info ];
    else
        return 0L;
}

void PluginController::pluginDestroyed( QObject* deletedPlugin )
{
    for ( Private::InfoToPluginMap::Iterator it = d->loadedPlugins.begin();
          it != d->loadedPlugins.end(); ++it )
    {
        if ( it.value() == deletedPlugin )
        {
            d->loadedPlugins.erase( it );
            break;
        }
    }

    if ( d->shutdownMode == Private::ShuttingDown && d->loadedPlugins.isEmpty() )
    {
        // Use a timer to make sure any pending deleteLater() calls have
        // been handled first
        QTimer::singleShot( 0, this, SLOT( shutdownDone() ) );
    }
}

void PluginController::pluginReadyForUnload( Plugin* plugin ) 
{
    kDebug(9000) << k_funcinfo << pluginInfo( plugin )->pluginName() << " ready for unload" << endl;
    delete plugin;
}

void PluginController::shutdownTimeout()
{
    // When we were already done the timer might still fire.
    // Do nothing in that case.
    if ( d->shutdownMode == Private::DoneShutdown )
        return;

    QStringList remaining;
    Private::InfoToPluginMap::ConstIterator it, itEnd;
    it = d->loadedPlugins.begin();
    itEnd = d->loadedPlugins.end();
    for ( ; it != itEnd; ++it )
        remaining.append( it.key()->pluginName() );

    kWarning(9000) << k_funcinfo << "Some plugins didn't shutdown in time!" << endl
            << "Remaining plugins: " << remaining << endl
            << "Forcing shutdown now." << endl;

    shutdownDone();
}

void PluginController::shutdownDone()
{
    d->shutdownMode = Private::DoneShutdown;
    KGlobal::deref();
}

}
#include "kdevplugincontroller.moc"
