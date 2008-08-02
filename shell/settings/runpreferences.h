/* KDevelop Project Settings
 *
 * Copyright 2006  Matt Rogers <mattr@kde.org>
 * Copyright 2007-2008 Hamish Rodda <rodda@kde.org>
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

#ifndef KDEVRUNPREFERENCES_H
#define KDEVRUNPREFERENCES_H

#include <project/projectkcmodule.h>

#include <kurl.h>
#include <kstandarddirs.h>

class KConfigDialogManager;
class RunSettings;

namespace Ui
{
class RunSettings;
}

namespace KDevelop
{

class RunPreferences : public ProjectKCModule<RunSettings>
{
    Q_OBJECT
public:
    RunPreferences( QWidget *parent, const QVariantList &args );
    virtual ~RunPreferences();

    virtual void save();
    virtual void load();

    virtual KUrl localNonShareableFile() const
    {
        return KUrl::fromPath(
                   KStandardDirs::locate( "data", "kdevelop/data.kdev4" ) );
    }

private Q_SLOTS:
    void newRunConfig();
    void deleteRunConfig();
    void runConfigSelected(int index);
    void runConfigRenamed(const QString& newName);

private:
    bool configNameValid(const QString& name);

    Ui::RunSettings *preferencesDialog;
    KConfigDialogManager* m_manager;
    int m_currentRunTarget;
    QString m_currentRunTargetName;
    bool m_deletingCurrentRunTarget;
    QStringList m_runTargets;
};

}
#endif
