/***************************************************************************
 *   Copyright (C) 2004 by Alexander Dymo <adymo@kdevelop.org>             *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/
#include "profileengine.h"

#include <QDir>

#include <kurl.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kstandarddirs.h>

#include <kdevplugin.h>

ProfileEngine::ProfileEngine()
{
    QStringList dirs = KGlobal::dirs()->findDirs("data", "kdevelop/profiles");

    m_rootProfile = new Profile(0, "KDevelop");

    QString currPath = "/";
    QMap<QString, Profile*> passedPaths;

    for (QStringList::const_iterator it = dirs.constBegin(); it != dirs.constEnd(); ++it)
        processDir(*it, currPath, passedPaths, m_rootProfile);
}

ProfileEngine::~ProfileEngine()
{
    delete m_rootProfile;
}

void ProfileEngine::processDir(const QString &dir, const QString &currPath, QMap<QString, Profile*> &passedPaths, Profile *root)
{
//     kDebug() << "processDir: " << dir << " " << currPath << endl;

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
            processDir(dir + *eit + "/", currPath + dirName, passedPaths, profile);
        }
    }
}

KService::List ProfileEngine::offers(const QString &profileName, OfferType offerType)
{
    ProfileListing listing;
    Profile *profile = 0;
    getProfileWithListing(listing, &profile, profileName);

    if (!profile)
        return KService::List();

    QString constraint = QString::fromLatin1("[X-KDevelop-Version] == %1").arg(KDEVELOP_PLUGIN_VERSION);
    switch (offerType) {
        case Global:
            constraint += QString::fromLatin1(" and [X-KDevelop-Scope] == 'Global'");
            break;
        case Project:
            constraint += QString::fromLatin1(" and [X-KDevelop-Scope] == 'Project'");
            break;
        case Core:
            constraint += QString::fromLatin1(" and [X-KDevelop-Scope] == 'Core'");
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
//     kDebug(9000) << "Query for Profile:" << endl
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
        constraint += QString::fromLatin1("and [Name] == '%1'").arg((*it).name);
        KService::List enable = KServiceTypeTrader::self()->query(QString::fromLatin1("KDevelop/Plugin"), constraint);
        list += enable;
    }

/*//BEGIN debug
    kDebug() << "=============" << endl
        << "    =============" << endl
        << "        =============   Plugins for Profile:" << endl;
    for (KServiceOfferList::const_iterator it = list.begin(); it != list.end(); ++it)
        kDebug() << "        " << (*it)->name() << endl;
    kDebug() << endl << endl;
//END debug*/

    return list;
}

KService::List ProfileEngine::allOffers(OfferType offerType)
{
    QString constraint = QString::fromLatin1("[X-KDevelop-Version] == %1").arg(KDEVELOP_PLUGIN_VERSION);
    switch (offerType) {
        case Global:
            constraint += QString::fromLatin1(" and [X-KDevelop-Scope] == 'Global'");
            break;
        case Project:
            constraint += QString::fromLatin1(" and [X-KDevelop-Scope] == 'Project'");
            break;
        case Core:
            constraint += QString::fromLatin1(" and [X-KDevelop-Scope] == 'Core'");
            break;
    }
    return KServiceTypeTrader::self()->query(QString::fromLatin1("KDevelop/Plugin"), constraint);
}

void ProfileEngine::getProfileWithListing(ProfileListing &listing, Profile **profile,
    const QString &profileName)
{
    if (profileName == "KDevelop")
        *profile = m_rootProfile;
    else
    {
        walkProfiles<ProfileListing>(listing, m_rootProfile);
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

void ProfileEngine::diffProfiles(OfferType offerType, const QString &profile1,
    const QString &profile2, QStringList &unload, KService::List &load)
{
    KService::List offers1 = offers(profile1, offerType);
    KService::List offers2 = offers(profile2, offerType);

    QStringList offers1List;
    for (KService::List::const_iterator it = offers1.constBegin();
        it != offers1.constEnd(); ++it)
        offers1List.append((*it)->desktopEntryName());
    QMap<QString, KService::Ptr> offers2List;
    for (KService::List::const_iterator it = offers2.constBegin();
        it != offers2.constEnd(); ++it)
        offers2List[(*it)->desktopEntryName()] = *it;

//    kDebug() << "OLD PROFILE: " << offers1List << endl;
//    kDebug() << "NEW PROFILE: " << offers2List << endl;

    for (QStringList::const_iterator it = offers1List.constBegin();
        it != offers1List.constEnd(); ++it)
    {
//         kDebug() << "checking: " << *it << endl;
        if (offers2List.contains(*it))
        {
//             kDebug() << "    keep" << endl;
            offers2.removeAll(offers2List[*it]);
        }
        else
        {
//             kDebug() << "    unload" << endl;
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
