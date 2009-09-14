/* KDevelop CMake Support
 *
 * Copyright 2009 Aleix Pol <aleixpol@kde.org>
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

#ifndef CMAKEDOCUMENTATION_H
#define CMAKEDOCUMENTATION_H

#include <QString>
#include <QMap>
#include <QStringList>
#include <QVariantList>
#include <interfaces/iplugin.h>
#include <interfaces/idocumentationprovider.h>

namespace KDevelop { class Declaration; }
class CMakeManager;
class KUrl;

class CMakeDocumentation : public QObject
{
    Q_OBJECT
    public:
        enum Type { Command, Variable, Module, Property, Policy };
        CMakeDocumentation(const QString& cmakeCmd, CMakeManager* m);
        KSharedPtr<KDevelop::IDocumentation> description(const QString& identifier, const KUrl& file);
        
        virtual KSharedPtr< KDevelop::IDocumentation > documentationForDeclaration(KDevelop::Declaration* declaration);
        
        QStringList names(Type t) const;
    public slots:
        void delayedInitialization();
        
    private:
        void collectIds(const QString& param, Type type);
        
        QMap<QString, Type> m_typeForName;
        QString mCMakeCmd;
        const CMakeManager* m_manager;
};

#endif // CMAKEDOCUMENTATION_H
