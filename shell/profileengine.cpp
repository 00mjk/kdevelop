/***************************************************************************
 *   Copyright 2004 Alexander Dymo <adymo@kdevelop.org>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
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
#include "profileengine.h"

#include <QtCore/QDir>

#include <kurl.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kstandarddirs.h>

#include "iplugin.h"
#include "plugincontroller.h"

namespace KDevelop
{

class ProfileEnginePrivate
{
public:
    Profile *m_rootProfile;
};

ProfileEngine::ProfileEngine()
    : d(new ProfileEnginePrivate)
{
    QStringList dirs = KGlobal::dirs()->findDirs("data", "kdevplatform/profiles");

    d->m_rootProfile = new Profile(0, "KDevelop");

    QString currPath = "/";
    QMap<QString, Profile*> passedPaths;

    for (QStringList::const_iterator it = dirs.constBegin(); it != dirs.constEnd(); ++it)
        processDir(*it, currPath, passedPaths, d->m_rootProfile);
}

ProfileEngine::~ProfileEngine()
{
    delete d->m_rootProfile;
}

void ProfileEngine::processDir(const QString &dir, const QString &currPath, QMap<QString, Profile*> &passedPaths, Profile *root)
{
//     kDebug(9501) << "processDir:" << dir << "" << currPath;

    QDir qDir(dir);
    QStringList entryList = qDir.entryList(QDir::Dirs);
    for (QStringList::const_iterator eit = entryList.constBegin(); eit != entryList.constEnd(); ++eit)
    {
        if ((*eit != "..") && (*eit != "."))
        {
            QString dirName = *eit;
            Profile *profile = 0;
            if (passedPaths.contains(currPath + dirName))
                profile = passedPaths[currPath + dirName];
            else
            {
                profile = new Profile(root, dirName);
                passedPaths[currPath + dirName] = profile;
            }
            processDir(dir + *eit + '/', currPath + dirName, passedPaths, profile);
        }
    }
}

KPluginInfo::List ProfileEngine::offers(const QString &profileName, PluginController::PluginType offerType)
{
    ProfileListing listing;
    Profile *profile = 0;
    getProfileWithListing(listing, &profile, profileName);

    if (!profile)
        return KPluginInfo::List();

    QString constraint = QString::fromLatin1("[X-KDevelop-Version] == %1").arg(KDEVELOP_PLUGIN_VERSION);
    switch (offerType) {
        case PluginController::Global:
            constraint += QString::fromLatin1(" and ( [X-KDevelop-Category] == 'Global'");
            constraint += QString::fromLatin1(" or [X-KDevelop-Category] == 'Core')");
            break;
        case PluginController::Project:
            constraint += QString::fromLatin1(" and [X-KDevelop-Category] == 'Project'");
            break;
    }
    QString constraint_add = "";
    Profile::EntryList properties = profile->list(Profile::Properties);
    int i = 0;
    for (Profile::EntryList::const_iterator it = properties.begin(); it != properties.end(); ++it)
        constraint_add += QString::fromLatin1(" %1 '%2' in [X-KDevelop-Properties]").
            arg((i++)==0?"":"or").arg((*it).name);
    if (!constraint_add.isEmpty())
        constraint += " and ( " + constraint_add + " ) ";

// //BEGIN debug
//     kDebug(9501) << "Query for Profile:" << endl
//               << constraint << endl << endl;
// //END debug

    KService::List list = KServiceTypeTrader::self()->query(QString::fromLatin1("KDevelop/Plugin"), constraint);
    QStringList names;

    Profile::EntryList disableList = profile->list(Profile::ExplicitDisable);
//     for (KServiceOfferList::iterator it = list.begin(); it != list.end(); ++it)
    KService::List::iterator it = list.begin();
    while (it != list.end())
    {
        QString name = (*it)->desktopEntryName();
        names.append(name);
        if (profile->hasInEntryList(disableList, name))
        {
            it = list.erase(it);
            continue;
        }
        ++it;
    }

    Profile::EntryList enableList = profile->list(Profile::ExplicitEnable);
    for (Profile::EntryList::const_iterator it = enableList.begin(); it != enableList.end(); ++it)
    {
        if (names.contains((*it).name))
            continue;
        QString constraint = QString::fromLatin1("[X-KDevelop-Version] == %1").arg(KDEVELOP_PLUGIN_VERSION);
        constraint += QString::fromLatin1("and [X-KDE-PluginInfo-Name] == '%1'").arg((*it).name);
        KService::List enable = KServiceTypeTrader::self()->query(QString::fromLatin1("KDevelop/Plugin"), constraint);
        list += enable;
    }

//BEGIN debug
//     kDebug(9501) << "=============" << endl
//         << " =============" << endl
//         << "     =============   Plugins for Profile:" << endl;
//     for (KService::List::const_iterator it = list.begin(); it != list.end(); ++it)
//         kDebug(9501) << "  " << (*it)->name();
//     kDebug(9501);
//END debug
    KPluginInfo::List pluginList = KPluginInfo::fromServices( list );
    return pluginList;
}

KPluginInfo::List ProfileEngine::allOffers(PluginController::PluginType offerType)
{
    QString constraint = QString::fromLatin1("[X-KDevelop-Version] == %1").arg(KDEVELOP_PLUGIN_VERSION);
    switch (offerType) {
        case PluginController::Global: //core and global have been combined
            constraint += QLatin1String(" and ( [X-KDevelop-Category] == 'Global'");
            constraint += QLatin1String(" or [X-KDevelop-Category] == 'Core' )");
            break;
        case PluginController::Project:
            constraint += QString::fromLatin1(" and [X-KDevelop-Category] == 'Project'");
            break;
    }
    KService::List list = KServiceTypeTrader::self()->query(QString::fromLatin1("KDevelop/Plugin"), constraint);
    KPluginInfo::List pluginList = KPluginInfo::fromServices( list );
    return pluginList;
}

void ProfileEngine::getProfileWithListing(ProfileListing &listing, Profile **profile,
    const QString &profileName)
{
    if (profileName == "KDevelop")
        *profile = d->m_rootProfile;
    else
    {
        walkProfiles<ProfileListing>(listing, d->m_rootProfile);
        *profile = listing.profiles[profileName];
    }
}

KUrl::List ProfileEngine::resources(const QString &profileName, const QString &nameFilter)
{
    ProfileListing listing;
    Profile *profile = 0;
    getProfileWithListing(listing, &profile, profileName);

    if (!profile)
        return KUrl::List();

    return resources(profile, nameFilter);
}

KUrl::List ProfileEngine::resources(Profile *profile, const QString &nameFilter)
{
    return profile->resources(nameFilter);
}

KUrl::List ProfileEngine::resourcesRecursive(const QString &profileName, const QString &nameFilter)
{
    ProfileListing listing;
    Profile *profile = 0;
    getProfileWithListing(listing, &profile, profileName);
    KUrl::List resources = profile->resources(nameFilter);

    ProfileListingEx listingEx(nameFilter);
    walkProfiles<ProfileListingEx>(listingEx, profile);

    resources += listingEx.resourceList;
    return resources;
}

void ProfileEngine::diffProfiles(PluginController::PluginType offerType, const QString &profile1,
    const QString &profile2, QStringList &unload, KPluginInfo::List &load)
{
    KPluginInfo::List offers1 = offers(profile1, offerType);
    KPluginInfo::List offers2 = offers(profile2, offerType);

    QStringList offers1List;
    for (KPluginInfo::List::const_iterator it = offers1.constBegin();
        it != offers1.constEnd(); ++it)
        offers1List.append(it->desktopEntryPath());
    QMap<QString, KPluginInfo> offers2List;

    for (KPluginInfo::List::const_iterator it = offers2.constBegin();
        it != offers2.constEnd(); ++it)
        offers2List[it->desktopEntryPath()] = *it;

//    kDebug(9501) << "OLD PROFILE:" << offers1List;
//    kDebug(9501) << "NEW PROFILE:" << offers2List;

    for (QStringList::const_iterator it = offers1List.constBegin();
        it != offers1List.constEnd(); ++it)
    {
//         kDebug(9501) << "checking:" << *it;
        if (offers2List.contains(*it))
        {
//             kDebug(9501) << " keep";
            offers2.removeAll(offers2List[*it]);
        }
        else
        {
//             kDebug(9501) << " unload";
            unload.append(*it);
        }
    }
    load = offers2;
}

Profile *ProfileEngine::findProfile(const QString & profileName)
{
    Profile *profile;
    ProfileListing listing;
    getProfileWithListing(listing, &profile, profileName);
    return profile;
}

void ProfileEngine::addResource(const QString &profileName, const KUrl &url)
{
    ProfileListing listing;
    Profile *profile = 0;
    getProfileWithListing(listing, &profile, profileName);

    if (!profile)
        return;

    profile->addResource(url);
}

Profile *ProfileEngine::rootProfile() const
{
    return d->m_rootProfile;
}

}
