/* KDevelop CMake Support
 *
 * Copyright 2006 Matt Rogers <mattr@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <QString>

#include <iproject.h>
#include "cmakemodelitems.h"

CMakeFolderItem::CMakeFolderItem( KDevelop::IProject *project, const FolderInfo& fi, QStandardItem* item )
    : KDevelop::ProjectItem( project, fi.name, item )
{
    m_folderInfo = fi;
    KUrl::List includeList;
    foreach( QString inc, fi.includes )
    {
        includeList.append( KUrl( inc ) );
    }
    setIncludeDirectories( includeList );
}

CMakeFolderItem::~CMakeFolderItem()
{
}

FolderInfo CMakeFolderItem::folderInfo() const
{
    return m_folderInfo;
}

CMakeTargetItem::CMakeTargetItem( KDevelop::IProject *project, const TargetInfo& target, CMakeFolderItem* item)
    : KDevelop::ProjectTargetItem( project, target.name, item )
{
    m_includeList = item->includeDirectories();
    m_targetInfo = target;
}


CMakeTargetItem::~CMakeTargetItem()
{
}

TargetInfo CMakeTargetItem::targetInfo() const
{
    return m_targetInfo;
}

const KDevelop::DomUtil::PairList& CMakeTargetItem::defines() const
{
    return m_defines;
}

const KUrl::List& CMakeTargetItem::includeDirectories() const
{
    return m_includeList;
}

const QHash< QString, QString >& CMakeTargetItem::environment() const
{
    return m_environment;
}

