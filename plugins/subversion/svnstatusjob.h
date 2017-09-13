/***************************************************************************
 *   This file is part of KDevelop                                         *
 *   Copyright 2007 Andreas Pakulat <apaku@gmx.de>                         *
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

#ifndef KDEVPLATFORM_PLUGIN_SVNSTATUSJOB_H
#define KDEVPLATFORM_PLUGIN_SVNSTATUSJOB_H

#include "svnjobbase.h"

#include <vcs/vcsstatusinfo.h>
#include <QUrl>

class SvnInternalStatusJob;

class SvnStatusJob : public SvnJobBaseImpl<SvnInternalStatusJob>
{
    Q_OBJECT
public:

    explicit SvnStatusJob( KDevSvnPlugin* parent );
    QVariant fetchResults() override;
    void start() override;

    void setLocations( const QList<QUrl>& locations );
    void setRecursive( bool );

public Q_SLOTS:
    void addToStats( const KDevelop::VcsStatusInfo& );
private:
    QList<QVariant> m_stats;
};

#endif

