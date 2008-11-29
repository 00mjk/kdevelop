/*
 * KDevelop DUChain viewer
 *
 * Copyright 2006 Hamish Rodda <rodda@kde.org>
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

#include "duchainmodel.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <ktemporaryfile.h>
#include <kprocess.h>

#include <interfaces/idocument.h>
#include <interfaces/icore.h>
#include <interfaces/ilanguagecontroller.h>
#include <language/backgroundparser/backgroundparser.h>
#include <language/backgroundparser/parsejob.h>

#include <language/editor/hashedstring.h>

#include <language/duchain/dumpdotgraph.h>
#include <language/duchain/topducontext.h>
#include <language/duchain/declaration.h>
#include <language/duchain/parsingenvironment.h>
#include <language/duchain/use.h>
#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/duchainpointer.h>

#include "duchainviewplugin.h"
#include <language/duchain/functiondefinition.h>

//#include "modeltest.h"

using namespace KTextEditor;
using namespace KDevelop;

///When the context is a proxy-context, returns the assigned content-context.
TopDUContext* getRealContext(TopDUContext* ctx) {
  if(ctx && ctx->parsingEnvironmentFile() && ctx->parsingEnvironmentFile()->isProxyContext() && !ctx->importedParentContexts().isEmpty())
    return dynamic_cast<TopDUContext*>(ctx->importedParentContexts()[0].context(0));
  else
    return ctx;
}

ProxyObject::ProxyObject(DUChainBase* _parent, DUChainBase* _object)
: DUChainBase(_object->range())
  , parent(_parent)
  , object(_object)
{
  setSmartRange(_object->smartRange(), DUChainBase::DontOwn);
}

DUChainModel::DUChainModel(DUChainViewPlugin* parent)
  : QAbstractItemModel(parent)
  , m_chain(0)
{
  //new ModelTest(this);
  connect( plugin()->core()->languageController()->backgroundParser(), SIGNAL(parseJobFinished(KDevelop::ParseJob*)), this, SLOT(parseJobFinished(KDevelop::ParseJob*)));

  bool success = connect(DUChain::self()->notifier(), SIGNAL(branchAdded(KDevelop::DUContextPointer)), SLOT(branchAdded(KDevelop::DUContextPointer)), Qt::QueuedConnection);
  Q_ASSERT(success);
}

DUChainViewPlugin* DUChainModel::plugin() const {
  return qobject_cast<DUChainViewPlugin*>(QObject::parent());
}

DUChainModel::~DUChainModel()
{
  qDeleteAll(m_knownObjects.values());
}

void DUChainModel::parseJobFinished(KDevelop::ParseJob* job)
{
  if( KUrl(job->document().str()) == m_document && job->duChain() ) {
    setTopContext(TopDUContextPointer(job->duChain()->weakPointer()));
  }

}

void DUChainModel::documentActivated(KDevelop::IDocument* document)
{
  if (document) {
    DUChainReadLocker readLock(DUChain::lock());
    TopDUContext* ptr = DUChain::self()->chainForDocument(document->url());
    TopDUContextPointer chain(ptr);
    if (chain && chain != m_chain)
      setTopContext(chain);
    else {
      m_document = document->url();
    }
  }
}

void DUChainModel::setTopContext(TopDUContextPointer context)
{
  DUChainReadLocker readLock(DUChain::lock());

  if( context )
    m_document = KUrl(context->url().str());
  else
    m_document = KUrl();

  context = TopDUContextPointer(getRealContext(context.data()));
  
  if (m_chain != context)
    m_chain = context;

  qDeleteAll(m_proxyObjects.values());
  m_proxyObjects.clear();

  m_objectLists.clear();
  m_modelRow.clear();

  reset();
}

int DUChainModel::columnCount(const QModelIndex & parent) const
{
  Q_UNUSED(parent);

  return 1;
}

DUChainBasePointer* DUChainModel::objectForIndex(const QModelIndex& index) const
{
  return static_cast<DUChainBasePointer*>(index.internalPointer());
}

QModelIndex DUChainModel::index(int row, int column, const QModelIndex & parent) const
{
  if (row < 0 || column < 0 || column > 0 || !m_chain)
    return QModelIndex();

  if (!parent.isValid()) {
    if (parent.row() > 0 || parent.column() > 0)
      return QModelIndex();

	return createIndex(row,column,createPointerForObject(m_chain.data()));
  }

  DUChainReadLocker readLock(DUChain::lock());

  DUChainBasePointer* base = objectForIndex(parent);
  if (!base || !base->data())
    return QModelIndex();

  QList<DUChainBasePointer*>* items = childItems(base);

  if (!items)
    return QModelIndex();

  if (row >= items->count())
    return QModelIndex();

  return createIndex(row, column, items->at(row));
}

QModelIndex DUChainModel::parent(const QModelIndex & index) const
{
  if (!index.isValid())
    return QModelIndex();

  DUChainReadLocker readLock(DUChain::lock());

  DUChainBasePointer* basep = objectForIndex(index);
  if (!basep || !basep->data())
    return QModelIndex();

  DUChainBase* base = basep->data();

  if (ProxyObject* proxy = dynamic_cast<ProxyObject*>(base))
    return createParentIndex(createPointerForObject(proxy->parent));

  if (DUContext* context = dynamic_cast<DUContext*>(base))
    if (context && context->parentContext())
      return createParentIndex(createPointerForObject(context->parentContext()));
    else
      return QModelIndex();

  if (Declaration* dec = dynamic_cast<Declaration*>(base))
    return createParentIndex(createPointerForObject(dec->context()));

//   if (Use* use = dynamic_cast<Use*>(base))
//     return createParentIndex(createPointerForObject(use->declaration()));

  // Shouldn't really hit this
  //Q_ASSERT(false);
  return QModelIndex();
}

QVariant DUChainModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return QVariant();

  DUChainReadLocker readLock(DUChain::lock());

  DUChainBasePointer* basep = objectForIndex(index);
  if (!basep || !basep->data())
    return QVariant();

  DUChainBase* base = basep->data();
  
  ProxyObject* proxy = dynamic_cast<ProxyObject*>(base);
  if (proxy)
    base = proxy->object;

  if(role == Qt::ToolTipRole)
    return i18n("Range: %1:%2 -> %3 %4", base->range().start.line, base->range().start.column, base->range().end.line, base->range().start.column);
  
  if (DUContext* context = dynamic_cast<DUContext*>(base)) {
    switch (role) {
      case Qt::DisplayRole:
        if (proxy)
          return i18n("Imported Context: %1", context->localScopeIdentifier().toString());
        else if (context == m_chain.data())
          return i18n("Top level context");
        else
          return i18n("Context: %1", context->localScopeIdentifier().toString());
    }

  } else if (Declaration* dec = dynamic_cast<Declaration*>(base)) {
    switch (role) {
      case Qt::DisplayRole: {
        if(dynamic_cast<FunctionDefinition*>(dec) && static_cast<FunctionDefinition*>(dec)->declaration(m_chain.data()))
          return i18n("Definition: %1", static_cast<FunctionDefinition*>(dec)->declaration(m_chain.data())->identifier().toString());
        else
          return i18n("Declaration: %1", dec->identifier().toString());
      }
    }

  }/* else if (Use* use = dynamic_cast<Use*>(base)) {
    switch (role) {
      case Qt::DisplayRole:
        return i18n("Use: %1", use->declaration() ? use->declaration()->identifier().toString() : i18n("[No definition found]"));
    }

  }*/ else {
    switch (role) {
      case Qt::DisplayRole:
        return i18n("Unknown object!");
    }
   }

  return QVariant();
}

int DUChainModel::rowCount(const QModelIndex & parent) const
{
  if (!m_chain)
    return 0;

  if (!parent.isValid())
    return 1;

  DUChainReadLocker readLock(DUChain::lock());

  DUChainBasePointer* base = objectForIndex(parent);
  if (!base || !base->data())
    return 0;

  QList<DUChainBasePointer*>* items = childItems(base);
  if (!items)
    return 0;

  return items->count();
}

#define TEST_NEXT(iterator, index) \
      if (!first.isValid()) { \
        current = nextItem(iterator, firstInit); \
        if (current.isValid() && (current < first || !first.isValid())) { \
          first = current; \
          found = index; \
        } \
      }

QList<DUChainBasePointer*>* DUChainModel::childItems(DUChainBasePointer* parentp) const
{
  Q_ASSERT(parentp->data());
  
  if (m_objectLists.contains(parentp))
    return m_objectLists[parentp];

  DUChainBase* parent = parentp->data();

  ProxyObject* proxy = dynamic_cast<ProxyObject*>(parent);
  if (proxy)
    parent = proxy->object;

  QList<DUChainBasePointer*>* list = 0;

  if (DUContext* context = dynamic_cast<DUContext*>(parent)) {

    QVector<DUContext*> importedParentContextsData;
    ///@todo Think whether this can be called for top-contexts, and if it can, care about endless recursion because of loops.
    foreach( const DUContext::Import &p, context->importedParentContexts() )
      if( p.context(0) )
        importedParentContextsData << p.context(0);

    QVectorIterator<DUContext*> contexts = context->childContexts();
    QVectorIterator<DUContext*> importedParentContexts = importedParentContextsData;
    QVectorIterator<Declaration*> declarations = context->localDeclarations();
//     QListIterator<Use*> uses = context->uses();


    bool firstInit = true;
    forever {
      DUChainBase* currentItem = 0;
      Cursor first = Cursor::invalid(), current;
      int found = 0;

      TEST_NEXT(contexts, 1)
      TEST_NEXT(importedParentContexts, 2)
      TEST_NEXT(declarations, 3)

      if (first.isValid()) {
        switch (found) {
          case 1:
            currentItem = item(contexts);
            break;
          case 2:
            currentItem = proxyItem(context, importedParentContexts);
            break;
          case 3:
            currentItem = item(declarations);
            break;
          default:
            Q_ASSERT(false);
            break;
        }

        if (!list)
          list = new QList<DUChainBasePointer*>();

        DUChainBasePointer* currentPointer = createPointerForObject(currentItem);
        m_modelRow[currentPointer] = list->count();
        list->append(currentPointer);

        first = Cursor::invalid();
      } else {
        break;
      }

      firstInit = false;
    }

  } else if (Declaration* dec = dynamic_cast<Declaration*>(parent)) {
    if (!dec->isDefinition() && FunctionDefinition::definition(dec)) {
      list = new QList<DUChainBasePointer*>();
      list->append(createPointerForObject(FunctionDefinition::definition(dec)));
    }

//     foreach (Use* use, dec->uses()) {
//       if (!list)
//         list = new QList<DUChainBasePointer*>();
// 
//       list->append(createPointerForObject(use));
//     }

  } else {
    // No child items for definitions or uses
    //kDebug(9500) << "No child items for definitions or uses";
  }

  m_objectLists.insert(parentp, list);

  return list;
}

/*bool DUChainModel::hasChildren(const QModelIndex & parent) const
{
  if (!parent.isValid())
    return true;

  DUChainReadLocker readLock(DUChain::lock());

  DUChainBase* base = objectForIndex(parent);
  if (!base)
    return false;

  if (m_objectLists.contains(base))
    return !m_objectLists[base]->isEmpty();

  ProxyObject* proxy = dynamic_cast<ProxyObject*>(base);
  if (proxy)
    base = proxy->object;

  if (DUContext* context = dynamic_cast<DUContext*>(base))
    return !context->childContexts().isEmpty() || !context->importedParentContexts().isEmpty() || !context->localDeclarations().isEmpty() || !context->localDefinitions().isEmpty() || !context->uses().isEmpty();

  else if (Declaration* dec = dynamic_cast<Declaration*>(base))
    return dec->definition() || !dec->uses().isEmpty();

  return false;
}*/

void DUChainModel::branchAdded(DUContextPointer context)
{
  DUChainReadLocker readLock(DUChain::lock());

  if (!context)
    return;

  if (!m_chain || !m_chain->parentContextOf(context.data()))
    return;

  DUChainBasePointer* parent = pointerForObject(context->parentContext());

  if (!parent || !m_objectLists.contains(parent) || !m_modelRow.contains(parent))
    // No entry for parent, ok - it will be created if the view interrogates for it
    return;

  QList<DUChainBasePointer*>* list = childItems(parent);

  DUChainBasePointer* contextPointer = createPointerForObject(context.data());

  int index = findInsertIndex(*list, context.data());

  if (list->at(index) == contextPointer)
    // Already added...
    return;

  beginInsertRows(createIndex(m_modelRow[parent], 0, parent), index, index);
  list->insert(index, contextPointer);
  endInsertRows();

  // Don't worry about children, they will be queried for if the view needs it
}

/*switch (relationship) {
    case DUChainObserver::ChildContexts:
      switch (change) {
        case DUChainObserver::Deletion:
        case DUChainObserver::Removal: {
          if (m_objectLists.contains(context)) {
            m_objectLists.remove(context);
            m_knownObjects.remove(ro->data());
            delete ro;
          }
          // fallthrough
        }
        if( *context == m_chain ) {
          //Top-context was deleted
          setTopContext(TopDUContextPointer());
          return;
        }


        case DUChainObserver::Change: {
          int index = list->indexOf(ro);
          Q_ASSERT(index != -1);
          beginRemoveRows(createIndex(m_modelRow[context], 0, context), index, index);
          list->removeAt(index);
          endRemoveRows();
          if (change == DUChainObserver::Removal || change == DUChainObserver::Deletion)
            break;
          // else fallthrough
        }
*/

KDevelop::DUChainBasePointer* DUChainModel::pointerForObject(KDevelop::DUChainBase* object) const
{
  if (m_knownObjects.contains(object))
    return m_knownObjects[object];

  return 0L;
}

KDevelop::DUChainBasePointer* DUChainModel::createPointerForObject(KDevelop::DUChainBase* object) const
{
  KDevelop::DUChainBasePointer* ret = 0L;

  if (!m_knownObjects.contains(object)) {
    ret = new KDevelop::DUChainBasePointer(object->weakPointer());
    m_knownObjects.insert(object, ret);

  } else {
    ret = m_knownObjects[object];
  }

  return ret;
}

QModelIndex DUChainModel::createParentIndex(DUChainBasePointer* type) const
{
  return createIndex(m_modelRow[type], 0, type);
}

int DUChainModel::findInsertIndex(QList<DUChainBasePointer*>& list, DUChainBase* object) const
{
  for (int i = 0; i < list.count(); ++i)
    if (DUChainBase* at = list.at(i)->data())
      if (at->range().textRange().start() > object->range().textRange().start())
        return i;

  return list.count();
}

void DUChainModel::doubleClicked ( const QModelIndex & index ) {
  DUChainReadLocker readLock(DUChain::lock());
  if( index.isValid() ) {
    DUChainBase* base = objectForIndex(index)->data();
    DUContext* ctx = dynamic_cast<DUContext*>(base);
    if( base && !ctx && dynamic_cast<Declaration*>(base) )
      ctx = static_cast<Declaration*>(base)->internalContext();

    if(ctx) {
      KTemporaryFile tempFile;

      {
        QString suffix = (dynamic_cast<TopDUContext*>(ctx) ?
                            static_cast<TopDUContext*>(ctx)->url().str()
                            : ctx->localScopeIdentifier().toString());
        suffix = suffix.replace('/', '_');
        suffix = suffix.replace(':', '.');
        suffix = suffix.replace(' ', '_');
        suffix += ".temp.dot";
        tempFile.setSuffix( suffix );

        if( !tempFile.open() ) {
          readLock.unlock();
          KMessageBox::error(0, i18n("Cannot create temporary file \"%1\" with suffix \"%2\"", tempFile.fileName(), suffix));
          return;
        }
        DumpDotGraph dump;
        tempFile.write( dump.dotGraph( ctx ).toLocal8Bit() ); //Shorten if it is a top-context, because it would become too much output
      }

      tempFile.setAutoRemove(false);
      QString fileName = tempFile.fileName();
      tempFile.close();
      kDebug(9500) << "Wrote dot-graph of context " << ctx << " into " << fileName;
      KProcess proc; ///@todo this is a simple hack. Maybe do it with mime-types etc.
      proc << "kgraphviewer" << fileName;
      if( !proc.startDetached() ) {
        KProcess proc2;
        proc2 << "dotty" << fileName;
        if( !proc2.startDetached() ) {
          readLock.unlock();
          KMessageBox::error(0, i18n("Could not open %1 with kgraphviewer or dotty.", fileName));
        }
      }

    }
  }
}

#include "duchainmodel.moc"

// kate: space-indent on; indent-width 2; tab-width 4; replace-tabs on; auto-insert-doxygen on
