/*
 * KDevelop Problem Reporter
 *
 * Copyright (c) 2008 Hamish Rodda <rodda@kde.org>
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

#include "problemhighlighter.h"

#include <KTextEditor/Document>

#include <language/duchain/indexedstring.h>
#include <language/editor/hashedstring.h>

#include <language/editor/editorintegrator.h>

using namespace KTextEditor;
using namespace KDevelop;

ProblemHighlighter::ProblemHighlighter(KTextEditor::Document* document)
    : m_document(document)
{
    Q_ASSERT(m_document);
}

ProblemHighlighter::~ProblemHighlighter()
{
    if(m_topHLRanges.isEmpty() || !m_document)
        return;

    KDevelop::EditorIntegrator editor;
    editor.setCurrentUrl(IndexedString(m_document->url()), true);

    LockedSmartInterface iface = editor.smart();
    if (iface)
        qDeleteAll(m_topHLRanges);
}

void ProblemHighlighter::setProblems(const QList<KDevelop::ProblemPointer>& problems)
{
    KDevelop::EditorIntegrator editor;
    IndexedString url(m_document->url());
    editor.setCurrentUrl(url, true);

    LockedSmartInterface iface = editor.smart();
    if (!iface)
        return;

    qDeleteAll(m_topHLRanges);
    m_topHLRanges.clear();

    KTextEditor::SmartRange* topRange = editor.topRange(iface, EditorIntegrator::Highlighting);
    m_topHLRanges.append(topRange);
    
    HashedString hashedUrl = url.str();

    foreach (const KDevelop::ProblemPointer& problem, problems) {
        if (problem->finalLocation().document() != hashedUrl || !problem->finalLocation().isValid())
            continue;

        KTextEditor::SmartRange* problemRange = editor.createRange(iface, problem->finalLocation());
//         *range = problem->finalLocation();
        if (problemRange->isEmpty())
            problemRange->smartEnd().advance(1);

        KTextEditor::Attribute::Ptr error(new KTextEditor::Attribute());
        if(problem->severity() == ProblemData::Error)
            error->setUnderlineColor(Qt::red);
        else if(problem->severity() == ProblemData::Warning)
            error->setUnderlineColor(Qt::magenta);
        else
            error->setUnderlineColor(Qt::yellow);
            
        error->setUnderlineStyle(QTextCharFormat::WaveUnderline);

        /*KTextEditor::Attribute::Ptr dyn(new KTextEditor::Attribute());
        dyn->setBackground(Qt::red);
        error->setDynamicAttribute(Attribute::ActivateMouseIn, dyn);
        error->setDynamicAttribute(Attribute::ActivateCaretIn, dyn);*/

        // TODO text hint for problem
        problemRange->setAttribute(error);
        problemRange->addWatcher(this);
        editor.exitCurrentRange(iface);
    }
    
    editor.exitCurrentRange(iface);
}

void ProblemHighlighter::rangeDeleted(KTextEditor::SmartRange *range)
{
    m_topHLRanges.removeAll(range);
}

void ProblemHighlighter::rangeContentsChanged(KTextEditor::SmartRange* range)
{
    range->setAttribute(KTextEditor::Attribute::Ptr());
}

