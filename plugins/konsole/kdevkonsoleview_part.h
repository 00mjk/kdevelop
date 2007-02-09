/***************************************************************************
*   Copyright (C) 2003 by KDevelop Authors                                *
*   kdevelop-devel@kde.org                                                *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef __KDEVPART_KDEVKONSOLEVIEW_H__
#define __KDEVPART_KDEVKONSOLEVIEW_H__

#include <qpointer.h>
#include "kdevplugin.h"

class KDevKonsoleView;

class KDevKonsoleViewPart : public KDevelop::Plugin
{
    Q_OBJECT
public:
    KDevKonsoleViewPart( QObject *parent, const QStringList & );
    virtual ~KDevKonsoleViewPart();

    // KDevelop::Plugin methods
    virtual QWidget *pluginView() const;
    virtual Qt::DockWidgetArea dockWidgetAreaHint() const;

private:
    QPointer<KDevKonsoleView> m_konsoleView;
};

#endif

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on
