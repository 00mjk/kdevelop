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

#include "quickopenplugin.h"

#include <cassert>
#include <typeinfo>
#include <QtGui/QTreeView>
#include <QtGui/QHeaderView>
#include <QDialog>
#include <QKeyEvent>
#include <QApplication>
#include <QScrollBar>
#include <QCheckBox>
#include <QMetaObject>

#include <kbuttongroup.h>
#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <kparts/mainwindow.h>
#include <kactioncollection.h>
#include <kaction.h>
#include <kshortcut.h>
#include <kdebug.h>

#include <interfaces/ilanguage.h>
#include <interfaces/icore.h>
#include <interfaces/iuicontroller.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/ilanguagecontroller.h>
#include <interfaces/iprojectcontroller.h>
#include <language/interfaces/ilanguagesupport.h>
#include <language/editor/hashedstring.h>
#include <language/duchain/duchainutils.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/duchain.h>
#include <language/duchain/types/identifiedtype.h>
#include <language/duchain/indexedstring.h>
#include <language/duchain/types/functiontype.h>

#include "expandingtree/expandingdelegate.h"
#include "ui_quickopen.h"
#include "quickopenmodel.h"
#include "projectfilequickopen.h"
#include "projectitemquickopen.h"
#include "declarationlistquickopen.h"
#include "customlistquickopen.h"
#include <language/duchain/functiondefinition.h>

using namespace KDevelop;

const bool noHtmlDestriptionInOutline = true;

class QuickOpenDelegate : public ExpandingDelegate {
public:
  QuickOpenDelegate(ExpandingWidgetModel* model, QObject* parent = 0L) : ExpandingDelegate(model, parent) {
  }
  virtual QList<QTextLayout::FormatRange> createHighlighting(const QModelIndex& index, QStyleOptionViewItem& option) const {
    QList<QVariant> highlighting = index.data(KTextEditor::CodeCompletionModel::CustomHighlight).toList();
    if(!highlighting.isEmpty())
      return highlightingFromVariantList(highlighting);
    return ExpandingDelegate::createHighlighting( index, option );
  }

};

K_PLUGIN_FACTORY(KDevQuickOpenFactory, registerPlugin<QuickOpenPlugin>(); )
K_EXPORT_PLUGIN(KDevQuickOpenFactory("kdevquickopen"))

Declaration* cursorDeclaration() {
  IDocument* doc = ICore::self()->documentController()->activeDocument();
  if(!doc)
    return 0;

  KTextEditor::Document* textDoc = doc->textDocument();
  if(!textDoc)
    return 0;

  KTextEditor::View* view = textDoc->activeView();
  if(!view)
    return 0;

  KDevelop::DUChainReadLocker lock( DUChain::lock() );

  return DUChainUtils::declarationForDefinition( DUChainUtils::itemUnderCursor( doc->url(), SimpleCursor(view->cursorPosition()) ) );
}

///The first declaration(or definition's declaration) that belongs to a context that surrounds the current cursor
Declaration* cursorContextDeclaration() {
  IDocument* doc = ICore::self()->documentController()->activeDocument();
  if(!doc)
    return 0;

  KTextEditor::Document* textDoc = doc->textDocument();
  if(!textDoc)
    return 0;

  KTextEditor::View* view = textDoc->activeView();
  if(!view)
    return 0;

  KDevelop::DUChainReadLocker lock( DUChain::lock() );

  TopDUContext* ctx = DUChainUtils::standardContextForUrl(doc->url());
  if(!ctx)
    return 0;

  SimpleCursor cursor(view->cursorPosition());

  DUContext* subCtx = ctx->findContext(cursor);

  while(subCtx && !subCtx->owner())
    subCtx = subCtx->parentContext();

  Declaration* definition = 0;

  if(!subCtx || !subCtx->owner())
    definition = DUChainUtils::declarationInLine(cursor, ctx);
  else
    definition = subCtx->owner();

  if(!definition)
    return 0;

  return DUChainUtils::declarationForDefinition(definition);
}

//Returns only the name, no template-parameters or scope
QString cursorItemText() {
  KDevelop::DUChainReadLocker lock( DUChain::lock() );

  Declaration* decl = cursorDeclaration();
  if(!decl)
    return QString();

  IDocument* doc = ICore::self()->documentController()->activeDocument();
  if(!doc)
    return QString();

  TopDUContext* context = DUChainUtils::standardContextForUrl( doc->url() );

  if( !context ) {
    kDebug() << "Got no standard context";
    return QString();
  }

  AbstractType::Ptr t = decl->abstractType();
  IdentifiedType* idType = dynamic_cast<IdentifiedType*>(t.unsafeData());
  if( idType && idType->declaration(context) )
    decl = idType->declaration(context);

  if(!decl->qualifiedIdentifier().isEmpty())
    return decl->qualifiedIdentifier().last().identifier().str();

  return QString();
}

QuickOpenWidgetHandler::QuickOpenWidgetHandler( QuickOpenModel* model, const QStringList& initialItems, const QStringList& initialScopes, bool listOnly, bool noSearchField ) : m_model(model) {
  m_dialog = new QDialog( ICore::self()->uiController()->activeMainWindow() );

  o.setupUi( m_dialog );
  o.list->header()->hide();
  o.list->setRootIsDecorated( false );
  o.list->setVerticalScrollMode( QAbstractItemView::ScrollPerItem );
  connect(o.list->verticalScrollBar(), SIGNAL(valueChanged(int)), m_model, SLOT(placeExpandingWidgets()));

  o.list->setItemDelegate( new QuickOpenDelegate( m_model, o.list ) );

  if(!listOnly) {
    QStringList allTypes = m_model->allTypes();
    QStringList allScopes = m_model->allScopes();

    QVBoxLayout *itemsLayout = new QVBoxLayout;
    itemsLayout->setAlignment(Qt::AlignTop);

    foreach( QString type, allTypes )
    {
      QCheckBox* check = new QCheckBox( type );
      itemsLayout->addWidget( check );

      if( initialItems.isEmpty() || initialItems.contains( type ) )
        check->setCheckState( Qt::Checked );
      else
        check->setCheckState( Qt::Unchecked );

      connect( check, SIGNAL(stateChanged(int)), this, SLOT(updateProviders()) );
    }

    itemsLayout->addStretch( 1 );
    o.itemsGroup->setLayout( itemsLayout );

    QVBoxLayout *scopesLayout = new QVBoxLayout;
    scopesLayout->setAlignment(Qt::AlignTop);


    foreach( QString scope, allScopes )
    {
      QCheckBox* check = new QCheckBox( scope );
      scopesLayout->addWidget( check );

      if( initialScopes.isEmpty() || initialScopes.contains( scope ) )
        check->setCheckState( Qt::Checked );
      else
        check->setCheckState( Qt::Unchecked );

      connect( check, SIGNAL(stateChanged(int)), this, SLOT(updateProviders()) );
    }

    scopesLayout->addStretch( 1 );
    o.scopeGroup->setLayout( scopesLayout );
  }else{
    o.list->setFocusPolicy(Qt::StrongFocus);
    o.scopeGroup->hide();
    o.itemsGroup->hide();
  }

  if( noSearchField ) {
    o.searchLine->hide();
    o.searchLabel->hide();
  }
  o.searchLine->installEventFilter( this );
  o.list->installEventFilter( this );
  o.buttonBox->installEventFilter( this );

  connect( o.searchLine, SIGNAL(textChanged( const QString& )), this, SLOT(textChanged( const QString& )) );
  connect( m_dialog, SIGNAL(accepted()), this, SLOT(accept()) );

  connect( o.list, SIGNAL(doubleClicked( const QModelIndex& )), this, SLOT(doubleClicked( const QModelIndex& )) );

  updateProviders();

  m_model->restart();

  o.list->setColumnWidth( 0, 20 );
}

void QuickOpenWidgetHandler::run() {
  o.list->setModel( 0 );
  m_dialog->show();
  //This is a workaround for a problem in KStyle where the scroll-mode is overwritten with ScrollPerPixel during show()
  //Usually all this stuff should be done BEFORE show() is called
  o.list->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
  m_model->setTreeView( o.list );
  o.list->setModel( m_model );
  connect( o.list->selectionModel(), SIGNAL(currentRowChanged( const QModelIndex&, const QModelIndex& )), this, SLOT(currentChanged( const QModelIndex&, const QModelIndex& )) );
  connect( o.list->selectionModel(), SIGNAL(selectionChanged( const QModelIndex&, const QModelIndex& )), this, SLOT(currentChanged( const QModelIndex&, const QModelIndex& )) );
}

QuickOpenWidgetHandler::~QuickOpenWidgetHandler() {
  //if( m_model->treeView() == o.list )
  m_model->setTreeView( 0 );
  delete m_dialog;
}

void QuickOpenWidgetHandler::updateProviders() {
  QStringList checkedItems;
  QStringList checkedScopes;

  foreach( QObject* obj, o.itemsGroup->children() ) {
    QCheckBox* box = qobject_cast<QCheckBox*>( obj );
    if( box ) {
      if( box->checkState() == Qt::Checked )
        checkedItems << box->text().remove('&');
    }
  }

  foreach( QObject* obj, o.scopeGroup->children() ) {
    QCheckBox* box = qobject_cast<QCheckBox*>( obj );
    if( box ) {
      if( box->checkState() == Qt::Checked )
        checkedScopes << box->text().remove('&');
    }
  }

  emit scopesChanged( checkedScopes );
  m_model->enableProviders( checkedItems, checkedScopes );
}


void QuickOpenWidgetHandler::textChanged( const QString& str ) {
  m_model->textChanged( str );

  QModelIndex currentIndex = m_model->index(0, 0, QModelIndex());
  o.list->selectionModel()->setCurrentIndex( currentIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows | QItemSelectionModel::Current );

  callRowSelected();
}

void QuickOpenWidgetHandler::callRowSelected() {
  QModelIndex currentIndex = o.list->selectionModel()->currentIndex();
  if( currentIndex.isValid() )
    m_model->rowSelected( currentIndex );
  else
    kDebug() << "current index is not valid";
}

void QuickOpenWidgetHandler::currentChanged( const QModelIndex& /*current*/, const QModelIndex& /*previous */) {
  callRowSelected();
}

void QuickOpenWidgetHandler::accept() {
  QString filterText = o.searchLine->text();
  m_model->execute( o.list->currentIndex(), filterText );
}

void QuickOpenWidgetHandler::doubleClicked ( const QModelIndex & index ) {
  QString filterText = o.searchLine->text();
  if(  m_model->execute( index, filterText ) )
    m_dialog->close();
  else if( filterText != o.searchLine->text() )
    o.searchLine->setText( filterText );
}


bool QuickOpenWidgetHandler::eventFilter ( QObject * watched, QEvent * event )
{
  if( event->type() == QEvent::KeyPress  ) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

    switch( keyEvent->key() ) {
      case Qt::Key_Down:
      case Qt::Key_Up:
      {
        if( keyEvent->modifiers() == Qt::ShiftModifier ) {
          QWidget* w = m_model->expandingWidget(o.list->selectionModel()->currentIndex());
          if( KDevelop::QuickOpenEmbeddedWidgetInterface* interface =
              dynamic_cast<KDevelop::QuickOpenEmbeddedWidgetInterface*>( w ) ){
            if( keyEvent->key() == Qt::Key_Down )
              interface->down();
            else
              interface->up();
            return true;
          }
          return false;
        }
      }
      case Qt::Key_PageUp:
      case Qt::Key_PageDown:
        if(watched == o.list )
          return false;
        QApplication::sendEvent( o.list, event );
      //callRowSelected();
        return true;

      case Qt::Key_Left: {
        //Expand/unexpand
        if( keyEvent->modifiers() == Qt::ShiftModifier ) {
          //Eventually Send action to the widget
          QWidget* w = m_model->expandingWidget(o.list->selectionModel()->currentIndex());
          if( KDevelop::QuickOpenEmbeddedWidgetInterface* interface =
              dynamic_cast<KDevelop::QuickOpenEmbeddedWidgetInterface*>( w ) ){
            interface->previous();
            return true;
          }
        } else {
          QModelIndex row = o.list->selectionModel()->currentIndex();
          if( row.isValid() ) {
            row = row.sibling( row.row(), 0 );

            if( m_model->isExpanded( row ) ) {
              m_model->setExpanded( row, false );
              return true;
            }
          }
        }
        return false;
      }
      case Qt::Key_Right: {
        //Expand/unexpand
        if( keyEvent->modifiers() == Qt::ShiftModifier ) {
          //Eventually Send action to the widget
          QWidget* w = m_model->expandingWidget(o.list->selectionModel()->currentIndex());
          if( KDevelop::QuickOpenEmbeddedWidgetInterface* interface =
              dynamic_cast<KDevelop::QuickOpenEmbeddedWidgetInterface*>( w ) ){
            interface->next();
            return true;
          }
        } else {
          QModelIndex row = o.list->selectionModel()->currentIndex();
          if( row.isValid() ) {
            row = row.sibling( row.row(), 0 );

            if( !m_model->isExpanded( row ) ) {
              m_model->setExpanded( row, true );
              return true;
            }
          }
        }
        return false;
      }
      case Qt::Key_Return:
      case Qt::Key_Enter: {
        if( keyEvent->modifiers() == Qt::ShiftModifier ) {
          //Eventually Send action to the widget
          QWidget* w = m_model->expandingWidget(o.list->selectionModel()->currentIndex());
          if( KDevelop::QuickOpenEmbeddedWidgetInterface* interface =
              dynamic_cast<KDevelop::QuickOpenEmbeddedWidgetInterface*>( w ) ){
            interface->accept();
            return true;
          }
        } else {
          QString filterText = o.searchLine->text();
          if( m_model->execute( o.list->currentIndex(), filterText ) ) {
            m_dialog->close();
          } else {
            //Maybe the filter-text was changed:
            if( filterText != o.searchLine->text() ) {
              o.searchLine->setText( filterText );
            }
          }
        }
        return true;
      }
    }
  }

  return false;
}

QuickOpenPlugin::QuickOpenPlugin(QObject *parent,
                                 const QVariantList&)
    : KDevelop::IPlugin(KDevQuickOpenFactory::componentData(), parent)
{
    KDEV_USE_EXTENSION_INTERFACE( KDevelop::IQuickOpen )
    setXMLFile( "kdevquickopen.rc" );
    m_model = new QuickOpenModel( 0 );

    KActionCollection* actions = actionCollection();

    KAction* quickOpen = actions->addAction("quick_open");
    quickOpen->setText( i18n("&Quick Open") );
    quickOpen->setShortcut( Qt::CTRL | Qt::ALT | Qt::Key_Q );
    connect(quickOpen, SIGNAL(triggered(bool)), this, SLOT(quickOpen()));

    KAction* quickOpenFile = actions->addAction("quick_open_file");
    quickOpenFile->setText( i18n("Quick Open &File") );
    quickOpenFile->setShortcut( Qt::CTRL | Qt::ALT | Qt::Key_O );
    connect(quickOpenFile, SIGNAL(triggered(bool)), this, SLOT(quickOpenFile()));

    KAction* quickOpenClass = actions->addAction("quick_open_class");
    quickOpenClass->setText( i18n("Quick Open &Class") );
    quickOpenClass->setShortcut( Qt::CTRL | Qt::ALT | Qt::Key_C );
    connect(quickOpenClass, SIGNAL(triggered(bool)), this, SLOT(quickOpenClass()));

    KAction* quickOpenFunction = actions->addAction("quick_open_function");
    quickOpenFunction->setText( i18n("Quick Open &Function") );
    quickOpenFunction->setShortcut( Qt::CTRL | Qt::ALT | Qt::Key_M );
    connect(quickOpenFunction, SIGNAL(triggered(bool)), this, SLOT(quickOpenFunction()));

    KAction* quickOpenDeclaration = actions->addAction("quick_open_jump_declaration");
    quickOpenDeclaration->setText( i18n("Jump to Declaration") );
    quickOpenDeclaration->setShortcut( Qt::CTRL | Qt::Key_Period );
    connect(quickOpenDeclaration, SIGNAL(triggered(bool)), this, SLOT(quickOpenDeclaration()));

    KAction* quickOpenDefinition = actions->addAction("quick_open_jump_definition");
    quickOpenDefinition->setText( i18n("Jump to Definition") );
    quickOpenDefinition->setShortcut( Qt::CTRL | Qt::Key_Comma );
    connect(quickOpenDefinition, SIGNAL(triggered(bool)), this, SLOT(quickOpenDefinition()));

    KAction* quickOpenNavigate = actions->addAction("quick_open_navigate");
    quickOpenNavigate->setText( i18n("Navigate Declaration") );
    quickOpenNavigate->setShortcut( Qt::ALT | Qt::Key_Space );
    connect(quickOpenNavigate, SIGNAL(triggered(bool)), this, SLOT(quickOpenNavigate()));

    KAction* quickOpenNavigateFunctions = actions->addAction("quick_open_outline");
    quickOpenNavigateFunctions->setText( i18n("Outline") );
    quickOpenNavigateFunctions->setShortcut( Qt::CTRL| Qt::ALT | Qt::Key_N );
    connect(quickOpenNavigateFunctions, SIGNAL(triggered(bool)), this, SLOT(quickOpenNavigateFunctions()));
    KConfigGroup quickopengrp = KGlobal::config()->group("QuickOpen");
    lastUsedScopes = quickopengrp.readEntry("SelectedScopes", QStringList() << "Project" << "Includes" << "Includers" );
    {
      m_projectFileData = new ProjectFileDataProvider();
      QStringList scopes, items;
      scopes << i18n("Project");
      items << i18n("Files");
      m_model->registerProvider( scopes, items, m_projectFileData );
    }
    {
      m_projectItemData = new ProjectItemDataProvider(this);
      QStringList scopes, items;
      scopes << i18n("Project");
      items << ProjectItemDataProvider::supportedItemTypes();
      m_model->registerProvider( scopes, items, m_projectItemData );
    }
}

QuickOpenPlugin::~QuickOpenPlugin()
{
  freeModel();

  delete m_model;
  delete m_projectFileData;
  delete m_projectItemData;
}

void QuickOpenPlugin::unload()
{
}

void QuickOpenPlugin::showQuickOpen( ModelTypes modes )
{
  if(!freeModel())
    return;

  QStringList initialItems;
  if( modes & Files )
    initialItems << i18n("Files");

  if( modes & Functions )
    initialItems << i18n("Functions");

  if( modes & Classes )
    initialItems << i18n("Classes");

  m_currentWidgetHandler = new QuickOpenWidgetHandler( m_model, initialItems, lastUsedScopes );
  connect( m_currentWidgetHandler, SIGNAL( scopesChanged( const QStringList& ) ), this, SLOT( storeScopes( const QStringList& ) ) );
  m_currentWidgetHandler->run();
}


void QuickOpenPlugin::storeScopes( const QStringList& scopes )
{
  lastUsedScopes = scopes;
  KConfigGroup grp = KGlobal::config()->group("QuickOpen");
  grp.writeEntry( "SelectedScopes", scopes );
}

void QuickOpenPlugin::quickOpen()
{
  showQuickOpen( All );
}

void QuickOpenPlugin::quickOpenFile()
{
  showQuickOpen( Files );
}

void QuickOpenPlugin::quickOpenFunction()
{
  showQuickOpen( Functions );
}

void QuickOpenPlugin::quickOpenClass()
{
  showQuickOpen( Classes );
}

QSet<KDevelop::IndexedString> QuickOpenPlugin::fileSet() const {
  return m_model->fileSet();
}

void QuickOpenPlugin::registerProvider( const QStringList& scope, const QStringList& type, KDevelop::QuickOpenDataProviderBase* provider )
{
  m_model->registerProvider( scope, type, provider );
}

bool QuickOpenPlugin::removeProvider( KDevelop::QuickOpenDataProviderBase* provider )
{
  m_model->removeProvider( provider );
  return true;
}

void QuickOpenPlugin::quickOpenDeclaration()
{
  if(jumpToSpecialObject())
    return;

  KDevelop::DUChainReadLocker lock( DUChain::lock() );
  Declaration* decl = cursorDeclaration();

  if(!decl) {
    kDebug() << "Found no declaration for cursor, cannot jump";
    return;
  }
  decl->activateSpecialization();

  IndexedString u = decl->url();
  SimpleCursor c = decl->range().start;

  if(u.str().isEmpty()) {
    kDebug() << "Got empty url for declaration" << decl->toString();
    return;
  }

  lock.unlock();
  core()->documentController()->openDocument(KUrl(u.str()), c.textCursor());
}

///Returns all languages for that url that have a language support, and prints warnings for other ones.
QList<KDevelop::ILanguage*> languagesWithSupportForUrl(KUrl url) {
  QList<KDevelop::ILanguage*> languages = ICore::self()->languageController()->languagesForUrl(url);
  QList<KDevelop::ILanguage*> ret;
  foreach( KDevelop::ILanguage* language, languages) {
    if(language->languageSupport()) {
      ret << language;
    }else{
      kDebug() << "got no language-support for language" << language->name();
    }
  }
  return ret;
}

QWidget* QuickOpenPlugin::specialObjectNavigationWidget() const
{
  if( !ICore::self()->documentController()->activeDocument() || !ICore::self()->documentController()->activeDocument()->textDocument() || !ICore::self()->documentController()->activeDocument()->textDocument()->activeView() )
    return false;

  KUrl url = ICore::self()->documentController()->activeDocument()->url();

  foreach( KDevelop::ILanguage* language, languagesWithSupportForUrl(url) ) {
    QWidget* w = language->languageSupport()->specialLanguageObjectNavigationWidget(url, SimpleCursor(ICore::self()->documentController()->activeDocument()->textDocument()->activeView()->cursorPosition()) );
    if(w)
      return w;
  }

  return 0;
}

QPair<KUrl, SimpleCursor> QuickOpenPlugin::specialObjectJumpPosition() const {
  if( !ICore::self()->documentController()->activeDocument() || !ICore::self()->documentController()->activeDocument()->textDocument() || !ICore::self()->documentController()->activeDocument()->textDocument()->activeView() )
    return qMakePair(KUrl(), SimpleCursor());

  KUrl url = ICore::self()->documentController()->activeDocument()->url();

  foreach( KDevelop::ILanguage* language, languagesWithSupportForUrl(url) ) {
    QPair<KUrl, SimpleCursor> pos = language->languageSupport()->specialLanguageObjectJumpCursor(url, SimpleCursor(ICore::self()->documentController()->activeDocument()->textDocument()->activeView()->cursorPosition()) );
    if(pos.second.isValid()) {
      return pos;
    }
  }

  return qMakePair(KUrl(), SimpleCursor::invalid());
}

bool QuickOpenPlugin::jumpToSpecialObject()
{
  QPair<KUrl, SimpleCursor> pos = specialObjectJumpPosition();
  if(pos.second.isValid()) {
    if(pos.first.isEmpty()) {
      kDebug() << "Got empty url for special language object";
      return false;
    }

    ICore::self()->documentController()->openDocument(pos.first, pos.second.textCursor());
    return true;
  }
  return false;
}

void QuickOpenPlugin::quickOpenDefinition()
{
  if(jumpToSpecialObject())
    return;

  KDevelop::DUChainReadLocker lock( DUChain::lock() );
  Declaration* decl = cursorDeclaration();

  if(!decl) {
    kDebug() << "Found no declaration for cursor, cannot jump";
    return;
  }

  IndexedString u = decl->url();
  SimpleCursor c = decl->range().start;
  if(FunctionDefinition* def = FunctionDefinition::definition(decl)) {
    def->activateSpecialization();
    u = def->url();
    c = def->range().start;
  }else{
    kDebug() << "Found no definition for declaration";
    decl->activateSpecialization();
  }

  if(u.str().isEmpty()) {
    kDebug() << "Got empty url for declaration" << decl->toString();
    return;
  }

  lock.unlock();
  core()->documentController()->openDocument(KUrl(u.str()), c.textCursor());
}

void QuickOpenPlugin::quickOpenNavigate()
{
  if(!freeModel())
    return;
  KDevelop::DUChainReadLocker lock( DUChain::lock() );

  QWidget* widget = specialObjectNavigationWidget();
  Declaration* decl = cursorDeclaration();

  if(widget || decl) {

    QuickOpenModel* model = new QuickOpenModel(0);
    model->setExpandingWidgetHeightIncrease(200); //Make the widget higher, since it's the only visible item

    if(widget) {
      QPair<KUrl, SimpleCursor> jumpPos = specialObjectJumpPosition();

      CustomItem item;
      item.m_widget = widget;
      item.m_executeTargetPosition = jumpPos.second;
      item.m_executeTargetUrl = jumpPos.first;

      QList<CustomItem> items;
      items << item;

      model->registerProvider( QStringList(), QStringList(), new CustomItemDataProvider(items) );
    }else{
      DUChainItem item;

      item.m_item = IndexedDeclaration(decl);
      item.m_text = decl->qualifiedIdentifier().toString();

      QList<DUChainItem> items;
      items << item;

      model->registerProvider( QStringList(), QStringList(), new DeclarationListDataProvider(this, items) );
    }

    //Change the parent so there are no conflicts in destruction order
    m_currentWidgetHandler = new QuickOpenWidgetHandler( model, QStringList(), QStringList(), true, true );
    model->setParent(m_currentWidgetHandler);
    model->setExpanded(model->index(0,0, QModelIndex()), true);

    m_currentWidgetHandler->run();
  }

  if(!decl) {
    kDebug() << "Found no declaration for cursor, cannot navigate";
    return;
  }
}

bool QuickOpenPlugin::freeModel()
{
  if(m_currentWidgetHandler)
    delete m_currentWidgetHandler;
  m_currentWidgetHandler = 0;

  return true;
}

void QuickOpenPlugin::quickOpenNavigateFunctions()
{
  if(!freeModel())
    return;

  IDocument* doc = ICore::self()->documentController()->activeDocument();
  if(!doc) {
    kDebug() << "No active document";
    return;
  }

  KDevelop::DUChainReadLocker lock( DUChain::lock() );

  TopDUContext* context = DUChainUtils::standardContextForUrl( doc->url() );

  if( !context ) {
    kDebug() << "Got no standard context";
    return;
  }

  QuickOpenModel* model = new QuickOpenModel(0);

  QList<DUChainItem> items;

  class OutlineFilter : public DUChainUtils::DUChainItemFilter {
  public:
    OutlineFilter(QList<DUChainItem>& _items) : items(_items) {
    }
    virtual bool accept(Declaration* decl) {
      if(decl->range().isEmpty())
        return false;
      if(decl->isFunctionDeclaration() || (decl->internalContext() && decl->internalContext()->type() == DUContext::Class)) {

        DUChainItem item;
        item.m_item = IndexedDeclaration(decl);
        item.m_text = decl->toString();
        items << item;

        return true;
      } else
        return false;
    }
    virtual bool accept(DUContext* ctx) {
      if(ctx->type() == DUContext::Class || ctx->type() == DUContext::Namespace || ctx->type() == DUContext::Global )
        return true;
      else
        return false;
    }
    QList<DUChainItem>& items;
  } filter(items);

  DUChainUtils::collectItems( context, filter );

  if(noHtmlDestriptionInOutline) {
    for(int a = 0; a < items.size(); ++a)
      items[a].m_noHtmlDestription = true;
  }

  Declaration* cursorDecl = cursorContextDeclaration();

  model->registerProvider( QStringList(), QStringList(), new DeclarationListDataProvider(this, items, true) );

  m_currentWidgetHandler = new QuickOpenWidgetHandler( model, QStringList(), QStringList(), true );
  //Select the declaration that contains the cursor
  if(cursorDecl) {
    int num = 0;
    foreach(const DUChainItem& item, items) {
      if(item.m_item.data() == cursorDecl) {
        m_currentWidgetHandler->o.list->setCurrentIndex( model->index(num,0,QModelIndex()) );
        m_currentWidgetHandler->o.list->scrollTo( model->index(num,0,QModelIndex()), QAbstractItemView::PositionAtCenter );
      }
      ++num;
    }
  }
  model->setParent(m_currentWidgetHandler);
  m_currentWidgetHandler->run();
}

#include "quickopenplugin.moc"

// kate: space-indent on; indent-width 2; tab-width 4; replace-tabs on; auto-insert-doxygen on
