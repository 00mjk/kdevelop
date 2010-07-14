/*
 * This file is part of KDevelop
 * Copyright 2010 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef STANDARDPATCHEXPORT_H
#define STANDARDPATCHEXPORT_H

#include <QObject>

namespace KDevelop {
class IPatchExporter;
}

class QMenu;
class QAction;
class PatchReviewPlugin;

/**
 * This class contains the minimum patch export interfaces that we won't want to have
 * as a plugin like the save as and the send by e-mail
 */

class StandardPatchExport : public QObject
{
    Q_OBJECT
    public:
        StandardPatchExport(PatchReviewPlugin* plugin, QObject* parent = 0);
        virtual ~StandardPatchExport();
        
        void addActions(QMenu* m);
        
    public slots:
        void runKIOExport();
        
    private:
        PatchReviewPlugin* m_plugin;
        QList<KDevelop::IPatchExporter*> m_exporters;
};

#endif // STANDARDPATCHEXPORT_H
