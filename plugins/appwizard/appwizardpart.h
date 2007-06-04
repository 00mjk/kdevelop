/***************************************************************************
 *   Copyright (C) 2001 by Bernd Gehrmann                                  *
 *   bernd@kdevelop.org                                                    *
 *   Copyright (C) 2004-2005 by Sascha Cunz                                *
 *   sascha@kdevelop.org                                                   *
 *   Copyright (C) 2007 by Alexander Dymo                                  *
 *   adymo@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _APPWIZARDPART_H_
#define _APPWIZARDPART_H_

#include <iplugin.h>

#include <QHash>

class ProjectTemplatesModel;
class ProjectSelectionPage;
class KArchiveDirectory;

class AppWizardPart: public KDevelop::IPlugin {
    Q_OBJECT
public:
    AppWizardPart(QObject *parent, const QStringList &);
    ~AppWizardPart();

private slots:
    void slotNewProject();

private:
    QString createProject(ProjectSelectionPage *selectionPage);
    void unpackArchive(const KArchiveDirectory *dir, const QString &dest);
    bool copyFile(const QString &source, const QString &dest);

    ProjectTemplatesModel *m_templatesModel;

    QHash<QString, QString> m_variables;
};

#endif

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
