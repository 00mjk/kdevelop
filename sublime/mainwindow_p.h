/***************************************************************************
 *   Copyright 2006-2007 Alexander Dymo  <adymo@kdevelop.org>       *
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
#ifndef SUBLIMEMAINWINDOW_P_H
#define SUBLIMEMAINWINDOW_P_H

#include <QMap>
#include <QObject>
#include <QSet>
#include <QTabBar>
#include <QHBoxLayout>
#include <QLabel>

#include "area.h"
#include "sublimedefs.h"

#include "mainwindow.h"
#include <qmenubar.h>
#include <qstylepainter.h>
#include <qstyleoption.h>
#include <qevent.h>
#include <KColorScheme>
#include "blur.h"

class QMenu;
class QAction;
class QSplitter;
class QDockWidget;
class QComboBox;

namespace Sublime {

class View;
class Container;
class Controller;
class AreaIndex;
class IdealMainWidget;
class IdealController;

class AreaTabButton : public QWidget {
    public:
    AreaTabButton(QString text, QIcon icon, uint iconSize, QWidget* parent, bool isCurrent, QWidget* _customButtonWidget) ;
    
    QLabel* iconLabel;
    QLabel* textLabel;
    QWidget* customButtonWidget;

    void setIsCurrent ( bool arg1 );

    private:
    bool m_isCurrent;
};

class AreaTabBar : public QTabBar {
    public:
    AreaTabBar(QWidget* parent) ;

    //Non-vertual overload: Respected by mainwindow.cpp
    void setCurrentIndex(int index) ;
    
    virtual QSize tabSizeHint ( int index ) const ;
    
    void clearTabs() {
        while(count())
            removeTab(0);
        buttons.clear();
        areaIds.clear();
    }
    
    QString areaId(int index) const {
        return areaIds[index];
    }
    
    void addCustomTab(QString text, QIcon icon, bool isCurrent, QString areaId, QWidget* customButtonWidget) {
        // plain sublime doens't provide customButtonWidget, only kdevelop shell does
        // TODO: adymo: refactor
        if (customButtonWidget) {
            customButtonWidget->setParent(this);
            customButtonWidget->show();
        }
        areaIds << areaId;
        buttons << new AreaTabButton(text, icon, 16, this, isCurrent, customButtonWidget);
        addTab(QString());
        setTabButton(count()-1, LeftSide, buttons.last());
    }
    
    virtual void paintEvent ( QPaintEvent* );
    
    private:
    QList<QString> areaIds;
    QList<AreaTabButton*> buttons;
    int m_currentIndex;
};

class AreaTabWidget : public QWidget {
    public:
    AreaTabWidget(QWidget* parent = 0) ;
    
    virtual QSize sizeHint() const ;
    
    virtual void paintEvent(QPaintEvent *ev);
    
    ///The widget is owned by this tab-widget
    void setTabSideWidget(QWidget* widget) {
        if(areaSideWidget)
            delete areaSideWidget;
        areaSideWidget = widget;
        m_leftLayout->insertWidget(0, areaSideWidget);
    }

    AreaTabBar* tabBar;
    QWidget* areaSideWidget;

    QHBoxLayout* m_layout;
    QVBoxLayout* m_leftLayout;
};

class MainWindowPrivate: public QObject {
    Q_OBJECT
public:
    MainWindowPrivate(MainWindow *w, Controller* controller);
    ~MainWindowPrivate();

    /**Use this to create tool views for an area.*/
    class IdealToolViewCreator {
    public:
        IdealToolViewCreator(MainWindowPrivate *_d): d(_d) {}
        Area::WalkerMode operator() (View *view, Sublime::Position position);
    private:
        MainWindowPrivate *d;
    };

    /**Use this to create views for an area.*/
    class ViewCreator {
    public:
        ViewCreator(MainWindowPrivate *_d): d(_d) {}
        Area::WalkerMode operator() (AreaIndex *index);
    private:
        MainWindowPrivate *d;
    };

    /**Reconstructs the mainwindow according to the current area.*/
    void reconstruct();
    /**Reconstructs the views according to the current area index.*/
    void reconstructViews();    
    /**Clears the area leaving mainwindow empty.*/
    void clearArea();
    
    /** Sets a @p w widget that will be shown when there are no documents on the area */
    void setBackgroundCentralWidget(QWidget* w);

    void activateFirstVisibleView();

    Controller *controller;
    Area *area;
    QList<View*> docks;
    QMap<View*, Container*> viewContainers;
    QMap<QWidget*, View*> widgetToView;

    View *activeView;
    View *activeToolView;

    QWidget *centralWidget;
    QWidget* bgCentralWidget;
    QSplitter* splitterCentralWidget;
    AreaTabWidget* areaSwitcher;

    IdealController *idealController;
    int ignoreDockShown;
    bool autoAreaSettingsSave;                       

public slots:
    void viewAdded(Sublime::AreaIndex *index, Sublime::View *view);
    void viewRemovedInternal(Sublime::AreaIndex *index, Sublime::View *view);
    void raiseToolView(Sublime::View* view);
    void aboutToRemoveView(Sublime::AreaIndex *index, Sublime::View *view);
    void toolViewAdded(Sublime::View *toolView, Sublime::Position position);
    void aboutToRemoveToolView(Sublime::View *toolView, Sublime::Position position);
    void toolViewMoved(Sublime::View *toolView, Sublime::Position position);
    void toggleArea(int index);

    void setTabBarLeftCornerWidget(QWidget* widget);

private slots:
    void switchToArea(QAction *action);
    void updateAreaSwitcher(Sublime::Area *area);
    void slotDockShown(Sublime::View*, Sublime::Position, bool);
    void widgetResized(Qt::DockWidgetArea dockArea, int thickness);
    void widgetCloseRequest(QWidget* widget);

    void showLeftDock(bool b);
    void showRightDock(bool b);
    void showBottomDock(bool b);
    void focusEditor();
    void toggleDocksShown();
    void selectNextDock();
    void selectPreviousDock();

private:
    void setBackgroundVisible(bool v);
    Qt::DockWidgetArea positionToDockArea(Position position);
    void cleanCentralWidget();

    MainWindow *m_mainWindow;
    // uses QPointer to make already-deleted splitters detectable
    QMap<AreaIndex*, QPointer<QSplitter> > m_indexSplitters;
    friend class AreaSelectionAction;

    QMap<Area*, QAction*> m_areaActions;
    QMap<QAction*, Area*> m_actionAreas;
    QWeakPointer<QWidget> m_leftTabbarCornerWidget;
};

}

#endif

