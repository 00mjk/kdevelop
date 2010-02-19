/************************************************************************
 * KDevelop4 Custom Buildsystem Support                                 *
 *                                                                      *
 * Copyright 2010 Andreas Pakulat <apaku@gmx.de>                        *
 *                                                                      *
 * This program is free software; you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation; either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful, but  *
 * WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU     *
 * General Public License for more details.                             *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program; if not, see <http://www.gnu.org/licenses/>. *
 ************************************************************************/

#ifndef CUSTOMBUILDSYSTEMCONFIG_H
#define CUSTOMBUILDSYSTEMCONFIG_H

#include <QMap>
#include <QList>
#include <QStringList>
#include <QHash>
#include <KUrl>

struct CustomBuildSystemTool
{
    enum ActionType { Build = 0, Configure, Install, Clean, Prune, Undefined };
    CustomBuildSystemTool() : enabled( false ), type( Undefined ) {}
    bool enabled;
    KUrl executable;
    QString arguments;
    QString envGrp;
    ActionType type;
};

Q_DECLARE_METATYPE( CustomBuildSystemTool );

struct CustomBuildSystemProjectPathConfig
{
    QString path;
    QStringList includes;
    QHash<QString,QVariant> defines;
};

struct CustomBuildSystemConfig
{
    QString title;
    QString grpName;
    KUrl buildDir;
    QHash<CustomBuildSystemTool::ActionType, CustomBuildSystemTool> tools;
    QList<CustomBuildSystemProjectPathConfig> projectPaths;
};

#endif