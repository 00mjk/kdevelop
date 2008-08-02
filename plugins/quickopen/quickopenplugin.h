/*
 * This file is part of KDevelop
 *
 * Copyright 2007 David Nolden <david.nolden.kdevelop@art-master.de>
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

#ifndef QUICKOPEN_PART_H
#define QUICKOPEN_PART_H

#include <interfaces/iplugin.h>
#include <language/interfaces/iquickopen.h>
#include <QtCore/QVariant>

#include <language/interfaces/quickopendataprovider.h>
#include "ui_quickopen.h"

namespace KDevelop {
  class SimpleCursor;
}

class QuickOpenModel;
class QuickOpenWidgetHandler;

class QuickOpenPlugin : public KDevelop::IPlugin, public KDevelop::IQuickOpen
{
    Q_OBJECT
    Q_INTERFACES( KDevelop::IQuickOpen )

public:
    QuickOpenPlugin( QObject *parent, const QVariantList & = QVariantList() );
    virtual ~QuickOpenPlugin();

    // KDevelop::Plugin methods
    virtual void unload();
    
    enum ModelTypes {
      Files = 1,
      Functions = 2,
      Classes = 4,
      All = Files + Functions + Classes
    };

    /**
     * Shows the quickopen dialog with the specified Model-types
     * @param modes A combination of ModelTypes
     * */
    void showQuickOpen( ModelTypes modes = All );

    virtual void registerProvider( const QStringList& scope, const QStringList& type, KDevelop::QuickOpenDataProviderBase* provider );

    virtual bool removeProvider( KDevelop::QuickOpenDataProviderBase* provider );

    virtual QSet<KDevelop::IndexedString> fileSet() const;

public slots:
    void quickOpen();
    void quickOpenFile();
    void quickOpenFunction();
    void quickOpenClass();
    void quickOpenDeclaration();
    void quickOpenDefinition();
    void quickOpenNavigate();
    void quickOpenNavigateFunctions();

private slots:
    void storeScopes( const QStringList& );

private:
    //Frees the model by closing active quickopen dialoags, and retuns whether successful.
    bool freeModel();

    QPair<KUrl, KDevelop::SimpleCursor> specialObjectJumpPosition() const;
    QWidget* specialObjectNavigationWidget() const;
    bool jumpToSpecialObject();
    
    QuickOpenModel* m_model;
    class ProjectFileDataProvider* m_projectFileData;
    class ProjectItemDataProvider* m_projectItemData;
    QStringList lastUsedScopes;
    
    //Contains a pointer to the current QuickOpenWidgetHandler, if a completion is active.
    //We can only have one widget at a time, because we manipulate the model.
    QPointer<QuickOpenWidgetHandler> m_currentWidgetHandler;
};

///Will delete itself once the dialog is closed, so use QPointer when referencing it permanently
class QuickOpenWidgetHandler : public QObject {
  Q_OBJECT
  public:
  /**
   * @param initialItems List of items that should initially be enabled in the quickopen-list. If empty, all are enabled.
   * @param initialScopes List of scopes that should initially be enabled in the quickopen-list. If empty, all are enabled.
   * @param listOnly when this is true, the given items will be listed, but all filtering using checkboxes is disabled.
   * @param noSearchFied when this is true, no search-line is shown.
   * */
  QuickOpenWidgetHandler( QuickOpenModel* model, const QStringList& initialItems, const QStringList& initialScopes, bool listOnly = false, bool noSearchField = false );
  ~QuickOpenWidgetHandler();

  ///Shows the dialog
  void run();

  signals:
  void scopesChanged( const QStringList& scopes );

  private slots:
  void currentChanged( const QModelIndex& current, const QModelIndex& previous );
  void accept();
  void textChanged( const QString& str );
  void updateProviders();
  void doubleClicked ( const QModelIndex & index );
  
  private:
  void callRowSelected();
  
  virtual bool eventFilter ( QObject * watched, QEvent * event );
  QDialog* m_dialog; //Warning: m_dialog is also the parent
  QuickOpenModel* m_model;
  public:
  Ui::QuickOpen o;
};

#endif // DUCHAINVIEW_PART_H

// kate: space-indent on; indent-width 2; tab-width 4; replace-tabs on; auto-insert-doxygen on
