/* This file is part of KDevelop
    Copyright 2013 Milian Wolff <mail@milianw.de>
    Copyright 2008 Alexander Dymo <adymo@kdevelop.org>

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
#ifndef KDEVPLATFORM_PLUGIN_PROJECTFILTERCONFIGPAGE_H
#define KDEVPLATFORM_PLUGIN_PROJECTFILTERCONFIGPAGE_H

#include <project/projectconfigpage.h>

#include "projectfiltersettings.h"

namespace Ui
{
class ProjectFilterSettings;
}

namespace KDevelop
{

class FilterModel;
class ProjectFilterProvider;

class ProjectFilterConfigPage : public ProjectConfigPage<ProjectFilterSettings>
{
    Q_OBJECT
public:
    ProjectFilterConfigPage(ProjectFilterProvider* provider, const KDevelop::ProjectConfigOptions& options, QWidget* parent);
    ~ProjectFilterConfigPage() override;

    QString name() const override;
    QIcon icon() const override;
    QString fullName() const override;

protected:
    bool eventFilter(QObject* object, QEvent* event) override;

private slots:
    void add();
    void remove();
    void moveUp();
    void moveDown();
    void selectionChanged();
    void emitChanged();

public Q_SLOTS:
    void apply() override;
    void reset() override;
    void defaults() override;

private:
    void checkFilters();

    FilterModel *m_model;
    ProjectFilterProvider* m_projectFilterProvider;
    QScopedPointer<Ui::ProjectFilterSettings> m_ui;
};

}
#endif
