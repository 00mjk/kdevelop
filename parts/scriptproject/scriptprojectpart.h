/***************************************************************************
 *   Copyright (C) 2001 by Bernd Gehrmann                                  *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _SCRIPTPROJECTPART_H_
#define _SCRIPTPROJECTPART_H_

#include <qdict.h>

#include "kdevproject.h"

class ScriptProjectWidget;
class QListViewItem;


class ScriptProjectPart : public KDevProject
{
    Q_OBJECT

public:
    ScriptProjectPart( KDevApi *api, QObject *parent=0, const char *name=0 );
    ~ScriptProjectPart();

protected:
    virtual void openProject(const QString &dirName);
    virtual void closeProject();

    virtual QString mainProgram();
    virtual QString projectDirectory();
    virtual QStringList allSourceFiles();

private slots:
    void slotItemExecuted(QListViewItem *item);

private:
    QGuardedPtr<ScriptProjectWidget> m_widget;
    friend class ScriptProjectWidget;
};

#endif

