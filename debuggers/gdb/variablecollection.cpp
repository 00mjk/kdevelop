/*
 * GDB Debugger Support
 *
 * Copyright 2007 Hamish Rodda <rodda@kde.org>
 * Copyright 2008 Vladimir Prus <ghost@cs.msu.su>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
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

#include "variablecollection.h"
#include "tooltipwidget.h"

#include "gdbcontroller.h"
#include "tooltipwidget.h"

#include "mi/gdbmi.h"
#include "gdbcommand.h"

#include "stringhelpers.h"

#include <interfaces/icore.h>
#include <interfaces/idocument.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/iuicontroller.h>
#include "sublime/controller.h"
#include "sublime/view.h"

#include <KLocale>
#include <KDebug>
#include <KTextEditor/TextHintInterface>
#include <KTextEditor/Document>
#include <KParts/PartManager>


#include <QFont>


//#include "modeltest.h"

using namespace KDevelop;
using namespace GDBDebugger;

Variable::Variable(TreeModel* model, TreeItem* parent, 
                   GDBController* controller, const QString& expression,
                   const QString& display)
  : TreeItem(model, parent), controller_(controller), activeCommands_(0), 
    inScope_(true), topLevel_(true)
{
    expression_ = expression;
    // FIXME: should not duplicate the data, instead overload 'data'
    // and return expression_ directly.
    if (display.isEmpty())
        setData(QVector<QVariant>() << expression << QString());
    else
        setData(QVector<QVariant>() << display << QString());
}

Variable::Variable(TreeModel* model, TreeItem* parent, 
                   GDBController* controller,
                   const GDBMI::Value& r)
: TreeItem(model, parent), controller_(controller), activeCommands_(0),
  inScope_(true), topLevel_(false)
{
    varobj_ = r["name"].literal();
    itemData.push_back(r["exp"].literal());
    itemData.push_back(r["value"].literal());
    setHasMoreInitial(r["numchild"].toInt());
    allVariables_[varobj_] = this;
}

void Variable::handleCreation(const GDBMI::Value& r)
{
    inScope_ = true;
    varobj_ = r["name"].literal();
    setHasMore(r["numchild"].toInt());
    itemData[1] = r["value"].literal();
    allVariables_[varobj_] = this;
}

QString Variable::varobj() const
{
    return varobj_;
}

QString Variable::expression() const
{
    return expression_;
}

Variable::~Variable()
{
    if (!varobj_.isEmpty())
    {
        // Delete only top-level variable objects.
        if (topLevel_
            && !controller_->stateIsOn(s_dbgNotStarted))
        {
            controller_->addCommand(new GDBCommand(GDBMI::VarDelete, varobj_));
        }
        
        allVariables_.remove(varobj_);
    }
}

void Variable::createVarobjMaybe()
{
    if (!varobj_.isEmpty())
        return;

    if (!controller_->stateIsOn(s_appNotStarted))
    {
        // WARNING: Locals handling presently rely on the fact that
        // we create @-varobj here. If changing, adjust callers accordingly.
        controller_->addCommand(
            new GDBCommand(
                GDBMI::VarCreate, 
                QString("var%1 @ %2").arg(nextId_++).arg(expression_),
                this, &Variable::handleCreated, true));
    }
}

void Variable::update(const GDBMI::Value& value)
{
    if (value.hasField("type_changed") 
        && value["type_changed"].literal() == "true")
    {
        clear();
        setHasMore(value["new_num_children"].toInt() != 0);
    }

    if (value.hasField("in_scope") && value["in_scope"].literal() == "false")
    {
        inScope_ = false;
    }
    else
    {
        inScope_ = true;
        itemData[1] = value["value"].literal();
    }
    reportChange();
}

void Variable::die()
{
    removeSelf();
    deleteLater();
}

void Variable::fetchMoreChildren()
{
    // FIXME: should not even try this if app is not started.
    // Probably need to disable open, or something
    if (!controller_->stateIsOn(s_appNotStarted))
    {
        activeCommands_ = 1;
        controller_->addCommand(
            new GDBCommand(GDBMI::VarListChildren,
                           QString("--all-values %1").arg(varobj_),
                           this,
                           // FIXME: handle error?
                           &Variable::handleChildren, this));
    }
}

QVariant Variable::data(int column, int role) const
{
    if (column == 1 && role == Qt::ForegroundRole 
        && !inScope_)
    {
        // FIXME: returning hardcoded gray is bad,
        // but we don't have access to any widget, or pallette
        // thereof, at this point.
        return QColor(128, 128, 128);
    }
    return TreeItem::data(column, role);
}

void Variable::handleCreated(const GDBMI::ResultRecord &r)
{
    if (r.reason == "error")
    {
        /* Probably should mark this disabled, or something.  */        
    }
    else
    {
        handleCreation(r);
        reportChange();
    }
}

void Variable::handleChildren(const GDBMI::ResultRecord &r)
{
    --activeCommands_;
    const GDBMI::Value& children = r["children"];
    for (int i = 0; i < children.size(); ++i)
    {
        const GDBMI::Value& child = children[i];
        const QString& exp = child["exp"].literal();
        if (exp == "public" || exp == "protected" || exp == "private")
        {
            ++activeCommands_;
            controller_->addCommand(
                new GDBCommand(GDBMI::VarListChildren,
                               QString("--all-values %1")
                               .arg(child["name"].literal()),
                               this,
                               // FIXME: handle error?
                               &Variable::handleChildren, this));
        }
        else
        {
            Variable* var = new Variable(model(), this, controller_,
                                         child);
            appendChild(var);
        }
    }

    setHasMore(activeCommands_ != 0);
}

Variable* Variable::findByName(const QString& name)
{
    if (allVariables_.count(name) == 0)
        return 0;
    return allVariables_[name];
}

void Variable::markAllDead()
{
    QMap<QString, Variable*>::iterator i, e;
    for (i = allVariables_.begin(), e = allVariables_.end(); i != e; ++i)
    {
        i.value()->varobj_.clear();
        i.value()->inScope_ = false;
        i.value()->reportChange();
    }
    allVariables_.clear();
}

int Variable::nextId_ = 0;

QMap<QString, Variable*> Variable::allVariables_;

Watches::Watches(TreeModel* model, TreeItem* parent)
: TreeItem(model, parent), finishResult_(0)
{
    setData(QVector<QVariant>() << "Auto" << QString());
}

Variable* Watches::add(const QString& expression)
{
    Variable* v = new Variable(model(), this,
                               controller(), expression);
    appendChild(v);
    v->createVarobjMaybe();
    return v;
}

GDBController* Watches::controller()
{
    return static_cast<VariablesRoot*>(parent())->controller();
}

Variable *Watches::addFinishResult(const QString& convenienceVarible)
{
    finishResult_ = new Variable(model(), this,
                                 controller(), convenienceVarible, "$ret");
    appendChild(finishResult_);
    finishResult_->createVarobjMaybe();
    return finishResult_;
}

void Watches::removeFinishResult()
{
    if (finishResult_)
    {
        finishResult_->die();
        finishResult_ = 0;
    }
}



QVariant Watches::data(int column, int role) const
{
#if 0
    if (column == 0 && role == Qt::FontRole)
    {
        /* FIXME: is creating font again and agian efficient? */
        QFont f = font();
        f.setBold(true);
        return f;
    }
#endif
    return TreeItem::data(column, role);
}

void Watches::reinstall()
{
    for (int i = 0; i < childItems.size(); ++i)
    {
        Variable* v = static_cast<Variable*>(child(i));
        v->createVarobjMaybe();
    }
}

Locals::Locals(KDevelop::TreeModel* model, KDevelop::TreeItem* parent)
: TreeItem(model, parent)
{
    setData(QVector<QVariant>() << "Locals" << QString());
}

GDBController* Locals::controller()
{
    return static_cast<VariablesRoot*>(parent())->controller();
}


void Locals::update()
{
    // FIXME: also get arguments.
    controller()->addCommand(
        new GDBCommand(GDBMI::StackListLocals, "1", this,
                       &Locals::handleListLocalVars));    
}

void Locals::handleListLocalVars(const GDBMI::ResultRecord& r)
{
    const GDBMI::Value& locals = r["locals"];
    QSet<QString> existing, current;
    for (int i = 0; i < childCount(); i++)
    {
        existing << static_cast<Variable*>(child(i))->expression();
    }

    for (int i = 0; i < locals.size(); i++)
    {
        const GDBMI::Value& var = locals[i];
        current << var["name"].literal();
        // If we currently don't display this local var, add it.
        if( !existing.contains( var["name"].literal() ) )
        {
            Variable* v = new Variable(
                static_cast<VariableCollection*>(model()), 
                this, controller(), var["name"].literal() );
            appendChild( v, false );
            v->createVarobjMaybe();
        }
    }

    for (int i = 0; i < childItems.size(); ++i)
    {
        Variable* v = static_cast<Variable*>(child(i));
        if (!current.contains(v->expression()))
        {
            removeChild(i);
            --i;
            // FIXME: check that -var-delete is sent.
            delete v;
        }
    }

    // Variables which changed just value are updated by a call to -var-update.
    // Variables that changed type -- likewise.
}

VariablesRoot::VariablesRoot(TreeModel* model)
: TreeItem(model)
{
    watches_ = new Watches(model, this);
    appendChild(watches_, true);
    locals_ = new Locals(model, this);
    appendChild(locals_, true);
}

void VariablesRoot::addVariable( const GDBMI::Value& var )
{
    Variable* v = new Variable( static_cast<VariableCollection*>(model()), this, controller(), var["name"].literal() );
    appendChild( v, false );
    v->createVarobjMaybe();
}

GDBController* VariablesRoot::controller()
{
    return static_cast<VariableCollection*>(model())->controller();
}

VariableCollection::VariableCollection(GDBController* parent)
: TreeModel(QVector<QString>() << "Name" << "Value", parent),
  controller_(parent)
{
    universe_ = new VariablesRoot(this);
    setRootItem(universe_);

    //new ModelTest(this);

    connect (ICore::self()->documentController(), 
	     SIGNAL( textDocumentCreated( KDevelop::IDocument* ) ), 
	     this, 
	     SLOT( textDocumentCreated( KDevelop::IDocument* ) ) );
}



GDBController* VariableCollection::controller()
{
    return controller_;
}


VariableCollection::~ VariableCollection()
{
}

void VariableCollection::textDocumentCreated(KDevelop::IDocument* doc)
{
  connect( doc->textDocument(), 
	   SIGNAL( viewCreated( KTextEditor::Document* , KTextEditor::View* ) ), 
	   this, SLOT( viewCreated( KTextEditor::Document*, KTextEditor::View* ) ) );

  foreach( KTextEditor::View* view, doc->textDocument()->views() )
    viewCreated( doc->textDocument(), view );
}

void VariableCollection::viewCreated(KTextEditor::Document* doc, 
                                     KTextEditor::View* view)
{
    using namespace KTextEditor;
    TextHintInterface *iface = dynamic_cast<TextHintInterface*>(view);
    if( !iface )
        return;

    iface->enableTextHints(500);

    connect(view, 
            SIGNAL(needTextHint(const KTextEditor::Cursor&, QString&)),
            this, 
            SLOT(textHintRequested(const KTextEditor::Cursor&, QString&)));    
}

void VariableCollection::
textHintRequested(const KTextEditor::Cursor& cursor, QString&)
{
    // Don't do anything if there's already an open tooltip.
    if (activeTooltip_)
        return;

    if (controller_->stateIsOn(s_appNotStarted))
        return;

    // Figure what is the parent widget and what is the text to show    
    KTextEditor::View* view = dynamic_cast<KTextEditor::View*>(sender());
    if (!view)
        return;

    KTextEditor::Document* doc = view->document();
    QString line = doc->line(cursor.line());
    int index = cursor.column();
    QChar c = line[index];
    if (!c.isLetterOrNumber() && c != '_')
        return;

    int start = Utils::expressionAt(line, index);
    int end = index;
    for (; end < line.size(); ++end)
    {
        QChar c = line[end];
        if (!(c.isLetterOrNumber() || c == '_'))
            break;
    }
    if (!(start < end))
        return;

    QString expression(line.mid(start, end-start));
    expression = expression.trimmed();

    if (expression.isEmpty())
        return;

    QPoint local = view->cursorToCoordinate(cursor);
    QPoint global = view->mapToGlobal(local);
    QWidget* w = view->childAt(local);
    if (!w)
        w = view;
    
    activeTooltip_ = new VariableToolTip(w, global+QPoint(30,30), controller_, expression);
}

void VariableCollection::slotEvent(event_t event)
{
    switch(event)
    {
        case program_exited:
        case debugger_exited:
        {
            Variable::markAllDead();
#if 0
            // Remove all locals.
            foreach (AbstractVariableItem* item, m_items) {
                // only remove frame items
                if (qobject_cast<FrameItem*>(item))
                {
                    deleteItem(item);
                }
                else
                {
                    item->deregisterWithGdb();
                }
            }
#endif
            break;
        }

        case connected_to_program:
            watches()->reinstall();
            break;

        case program_state_changed:

            // Fall-through intended.

        case thread_or_frame_changed:

            // FIXME: probably should do this only on the
            // first stop.
            watches()->reinstall();
            locals()->update();
            update();

            #if 0
            {
                FrameItem *frame = currentFrame();

                frame->setDirty();
            }
            #endif
            break;

        default:
            break;
    }
}

void VariableCollection::update()
{
    controller()->addCommand(
        new GDBCommand(GDBMI::VarUpdate, "--all-values *", this,
                       &VariableCollection::handleVarUpdate));
}

void VariableCollection::handleVarUpdate(const GDBMI::ResultRecord& r)
{
    const GDBMI::Value& changed = r["changelist"];
    for (int i = 0; i < changed.size(); ++i)
    {
        const GDBMI::Value& var = changed[i];
        Variable* v = Variable::findByName(var["name"].literal());
        // v can be NULL here if we've already removed locals after step,
        // but the corresponding -var-delete command is still in the queue.
        if (v)
            v->update(var);        
    }
}

#if 0
void VariableCollection::addItem(AbstractVariableItem * item)
{
    item->registerWithGdb();

    beginInsertRows(QModelIndex(), m_items.count(), m_items.count());
    m_items.append(item);
    endInsertRows();
}

void VariableCollection::deleteItem(AbstractVariableItem * item)
{
    int index = m_items.indexOf(item);
    Q_ASSERT(index != -1);
    if (index == -1)
        return;

    item->deleteAllChildren();

    beginRemoveRows(QModelIndex(), index, index);
    delete m_items.takeAt(index);
    endRemoveRows();

#if 0
    if (item->isRegisteredWithGdb())
        item->deregisterWithGdb();
#endif
}

GDBController * VariableCollection::controller() const
{
    return static_cast<GDBController*>(const_cast<QObject*>(QObject::parent()));
}

int VariableCollection::rowCount(const QModelIndex & parent) const
{
    if (!parent.isValid())
        return m_items.count();

    if (parent.column() != 0)
        return 0;

    AbstractVariableItem* item = itemForIndex(parent);
    if (!item)
        return 0;

    return item->children().count();
}

QModelIndex VariableCollection::index(int row, int column, const QModelIndex & parent) const
{
    if (row < 0 || column < 0 || column > AbstractVariableItem::ColumnLast)
        return QModelIndex();

    if (!parent.isValid()) {
        if (row >= m_items.count())
            return QModelIndex();

        return createIndex(row, column, m_items.at(row));
    }

    if (parent.column() != 0)
        return QModelIndex();

    AbstractVariableItem* item = itemForIndex(parent);
    if (item && row < item->children().count())
        return createIndex(row, column, item->children().at(row));

    return QModelIndex();
}

QModelIndex VariableCollection::indexForItem(AbstractVariableItem * item, int column) const
{
    if (!item)
        return QModelIndex();

    if (AbstractVariableItem* parent = item->abstractParent()) {
        int row = parent->children().indexOf(item);
        if (row == -1)
            return QModelIndex();

        return createIndex(row, column, item);
    }

    int row = m_items.indexOf(item);
    if (row == -1)
        return QModelIndex();

    return createIndex(row, column, item);
}

AbstractVariableItem * VariableCollection::itemForIndex(const QModelIndex & index) const
{
    return static_cast<AbstractVariableItem*>(index.internalPointer());
}

AbstractVariableItem * VariableCollection::parentForIndex(const QModelIndex & index) const
{
    if (AbstractVariableItem* item = itemForIndex(index))
        return item->abstractParent();

    return 0;
}

int VariableCollection::columnCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return AbstractVariableItem::ColumnLast + 1;
}

QVariant VariableCollection::data(const QModelIndex & index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (AbstractVariableItem* item = itemForIndex(index))
        return item->data(index.column(), role);

    return "<error - no item>";
}

Qt::ItemFlags VariableCollection::flags(const QModelIndex & index) const
{
    if (AbstractVariableItem* item = itemForIndex(index))
        return item->flags(index.column());

    return 0;
}

QVariant VariableCollection::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);

    switch (role) {
        case Qt::DisplayRole:
            switch (section) {
                case AbstractVariableItem::ColumnName:
                    return i18n("Variable");

                case AbstractVariableItem::ColumnValue:
                    return i18n("Value");

                case AbstractVariableItem::ColumnType:
                    return i18n("Type");
            }
            break;
    }

    return QVariant();
}

QModelIndex VariableCollection::parent(const QModelIndex & index) const
{
    AbstractVariableItem* parent = parentForIndex(index);
    if (!parent)
        return QModelIndex();

    AbstractVariableItem* grandParent = parent->abstractParent();
    if (!grandParent)
        return createIndex(m_items.indexOf(parent), 0, parent);

    return createIndex(grandParent->children().indexOf(parent), 0, parent);
}

bool VariableCollection::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (index.column() != AbstractVariableItem::ColumnName)
        return false;

    VariableItem* item = qobject_cast<VariableItem*>(itemForIndex(index));
    if (!item)
        return false;

    if (role == Qt::EditRole) {
        item->setVariableName(value.toString());
        return true;

    } else {
        kWarning() << "Unsupported set data role" << role;
    }

    return false;
}

void VariableCollection::slotAddWatchVariable(const QString &watchVar)
{
    // FIXME need thread +/- frame info??
    VariableItem *varItem = new VariableItem(findWatch());
    varItem->setExpression(watchVar);
    findWatch()->addChild(varItem);
}

void VariableCollection::slotEvaluateExpression(const QString &expression)
{
    if (!recentExpressions_)
    {
        recentExpressions_ = new RecentItem(this);
        addItem(recentExpressions_);
    }

    // FIXME need thread +/- frame info??
    VariableItem *varItem = new VariableItem(recentExpressions_);
    varItem->setExpression(expression);
    varItem->setFrozen();
    addItem(varItem);
}

FrameItem *VariableCollection::findFrame(int frameNo, int threadNo) const
{
    foreach (AbstractVariableItem* item, m_items)
        if (FrameItem* frame = qobject_cast<FrameItem*>(item))
            if (frame->matchDetails(frameNo, threadNo))
                return frame;

    return 0;
}

WatchItem *VariableCollection::findWatch()
{
    foreach (AbstractVariableItem* item, m_items)
        if (WatchItem* watch = qobject_cast<WatchItem*>(item))
            return watch;

    WatchItem* item = new WatchItem(this);
    addItem(item);
    return item;
}

void VariableCollection::prepareInsertItems(AbstractVariableItem * parent, int index, int count)
{
    beginInsertRows(indexForItem(parent), index, index + count - 1);
}

void VariableCollection::completeInsertItems()
{
    endInsertRows();
}

void VariableCollection::prepareRemoveItems(AbstractVariableItem * parent, int index, int count)
{
    beginRemoveRows(indexForItem(parent), index, index + count - 1);
}

void VariableCollection::completeRemoveItems()
{
    endRemoveRows();
}

void VariableCollection::dataChanged(AbstractVariableItem * item, int column)
{
    QModelIndex index = indexForItem(item, column);
    if (index.isValid())
        emit QAbstractItemModel::dataChanged(index, index);
}

AbstractVariableItem * VariableCollection::itemForVariableObject(const QString & variableObject) const
{
    if (m_variableObjectMap.contains(variableObject))
        return m_variableObjectMap[variableObject];

    return 0;
}

void VariableCollection::addVariableObject(const QString & variableObject, AbstractVariableItem * item)
{
    m_variableObjectMap.insert(variableObject, item);
}

void VariableCollection::removeVariableObject(const QString & variableObject)
{
    m_variableObjectMap.remove(variableObject);
}

FrameItem* VariableCollection::currentFrame()
{
    FrameItem* frame = findFrame(controller()->currentFrame(), controller()->currentThread());
    if (!frame)
    {
        frame = new FrameItem(this);
        frame->setThread(controller()->currentThread());
        frame->setFrame(controller()->currentFrame());
        addItem(frame);
    }
    return frame;
}

bool VariableCollection::canFetchMore(const QModelIndex & parent) const
{
    if (AbstractVariableItem* item = itemForIndex(parent))
        if (item->isChildrenDirty() && item->hasChildren())
            return true;

    return false;
}

void VariableCollection::fetchMore(const QModelIndex & parent)
{
    if (AbstractVariableItem* item = itemForIndex(parent))
        item->refreshChildren();
}

bool VariableCollection::hasChildren(const QModelIndex & parent) const
{
    if (!parent.isValid())
        return m_items.count();

    if (parent.column() != 0)
        return false;

    if (AbstractVariableItem* item = itemForIndex(parent))
        return item->hasChildren();

    // Shouldn't hit this
    Q_ASSERT(false);
    return false;
}

VariableItem* VariableCollection::createVariableItem(const QString & type, AbstractVariableItem * parent)
{
    static QRegExp qstring("^(const)?[ ]*QString[ ]*&?$");
    if (qstring.exactMatch(type))
        return new QStringVariableItem(parent);

    static QRegExp qlist("^(const)?[ ]*QList.*");
    if (qlist.exactMatch(type))
        return new QListVariableItem(parent);

    return new VariableItem(parent);
}

#endif

#include "variablecollection.moc"
