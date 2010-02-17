/***************************************************************************
 *   This file is part of KDevelop                                         *
 *   Copyright 2008 Andreas Pakulat <apaku@gmx.de>                         *
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

#ifndef SNIPPETCOMPLETIONMODEL_H
#define SNIPPETCOMPLETIONMODEL_H

#include <ktexteditor/codecompletionmodel.h>

namespace KTextEditor
{
class Document;
class View;
}

class SnippetCompletionItem;

class SnippetCompletionModel : public KTextEditor::CodeCompletionModel
{
    Q_OBJECT
public:
    SnippetCompletionModel();
    ~SnippetCompletionModel();

    QVariant data( const QModelIndex& idx, int role = Qt::DisplayRole ) const;
    void completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range, InvocationType invocationType);
    void executeCompletionItem( KTextEditor::Document* doc, const KTextEditor::Range& w, int row ) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex& index) const;
private:
    void initData(KTextEditor::View* view);
    QList<SnippetCompletionItem*> m_snippets;
};

#endif
