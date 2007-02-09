/* This file is part of KDevelop
Copyright (C) 2006 Adam Treat <treat@kde.org>

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

#ifndef KDEVENVWIDGET_H
#define KDEVENVWIDGET_H

#include <QDialog>

#include "kdevenvironment.h"
#include "ui_envsettings.h"

namespace KDevelop
{

class ConfigSkeleton;

class EnvWidget: public QDialog, private Ui::EnvironmentWidget
{
    Q_OBJECT
public:
    EnvWidget( QWidget *parent = 0 );
    virtual ~EnvWidget();

    void loadSettings();
    void saveSettings();
    void defaults();

signals:
    void changed( bool changed );

private slots:
    void newButtonClicked();
    void deleteButtonClicked();
    void processDefaultButtonClicked();
    void settingsChanged( int row, int column );
    void focusChanged( int row, int, int, int );

private:
    void load( bool defaults );
    bool isOverride( QTableWidgetItem *item ) const;
    bool isProcessDefault( QTableWidgetItem *item ) const;
    void setOverride( QTableWidgetItem *item );
    void setProcessDefault( QTableWidgetItem *item );
    void generateCurrentMaps();
    bool diff();

private:
    EnvironmentMap m_overrides;
    EnvironmentMap m_processDefaults;
    EnvironmentMap m_currentOverrides;
    EnvironmentMap m_currentProcessDefaults;

};

}
#endif

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on
