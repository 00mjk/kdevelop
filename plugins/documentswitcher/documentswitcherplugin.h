/***************************************************************************
 *   Copyright 2009 Andreas Pakulat <apaku@gmx.de>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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
#ifndef KDEVPLATFORM_PLUGIN_DOCUMENTSWITCHERPLUGIN_H
#define KDEVPLATFORM_PLUGIN_DOCUMENTSWITCHERPLUGIN_H

#include <interfaces/iplugin.h>
#include <QVariant>
#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(PLUGIN_DOCUMENTSWITCHER)

class QStandardItemModel;
namespace Sublime
{
    class View;
    class MainWindow;
    class Area;
class MainWindow;
}

class DocumentSwitcherTreeView;
class QModelIndex;

class QAction;

class DocumentSwitcherPlugin: public KDevelop::IPlugin {
    Q_OBJECT
public:
    explicit DocumentSwitcherPlugin( QObject *parent, const QVariantList &args = QVariantList() );
    ~DocumentSwitcherPlugin() override;

    void unload() override;
public slots:
    void itemActivated( const QModelIndex& );
    void switchToClicked(const QModelIndex& );
private slots:
    void addView( Sublime::View* );
    void changeView( Sublime::View* );
    void addMainWindow( Sublime::MainWindow* );
    void changeArea( Sublime::Area* );
    void removeView( Sublime::View* );
    void removeMainWindow(QObject*);
    void walkForward();
    void walkBackward();
protected:
    bool eventFilter( QObject*, QEvent* ) override;
private:
    void setViewGeometry(Sublime::MainWindow* window);
    void storeAreaViewList( Sublime::MainWindow* mainwindow, Sublime::Area* area );
    void enableActions();
    void fillModel( Sublime::MainWindow* window );
    void walk(const int from, const int to);
    // Need to use QObject here as we only have a QObject* in
    // the removeMainWindow method and cannot cast it to the mainwindow anymore
    QMap<QObject*, QHash<Sublime::Area*, QList<Sublime::View*> > > documentLists;
    DocumentSwitcherTreeView* view;
    QStandardItemModel* model;
    QAction* forwardAction;
    QAction* backwardAction;
};

#endif

