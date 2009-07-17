/* Copyright 2008 Aleix Pol <aleixpol@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef APPLYCHANGESWIDGET_H
#define APPLYCHANGESWIDGET_H

#include <KDialog>
#include "../languageexport.h"

namespace KTextEditor { class Document; class Range;}
class KUrl;
class QModelIndex;

namespace KDevelop
{
class IndexedString;
class ApplyChangesWidgetPrivate;

class KDEVPLATFORMLANGUAGE_EXPORT ApplyChangesWidget : public KDialog
{
    Q_OBJECT
    public:
        ApplyChangesWidget(const QString& info, const KUrl& url, QWidget* parent=0);
        ~ApplyChangesWidget();
        
        KTextEditor::Document* document() const;
        
        void addDocuments(const IndexedString & original, const IndexedString & modified);
    
    private Q_SLOTS:
        void change (KTextEditor::Document *document, const KTextEditor::Range &oldRange,
                const KTextEditor::Range &newRange);
        void insertion (KTextEditor::Document *document, const KTextEditor::Range &range);
        void removal (KTextEditor::Document *document, const KTextEditor::Range &range);
        void jump( const QModelIndex &);
        void switchEditView() {};
        
        void indexChanged(int);
        
    private:
        ApplyChangesWidgetPrivate * d;
};

}

#endif
