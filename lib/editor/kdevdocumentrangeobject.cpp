/* This file is part of KDevelop
    Copyright (C) 2006 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kdevdocumentrangeobject.h"

#include <QMutexLocker>

#include <ktexteditor/smartrange.h>
#include <ktexteditor/document.h>

#include "kdeveditorintegrator.h"

using namespace KTextEditor;

namespace Koncrete
{

DocumentRangeObject::DocumentRangeObject(Range* range)
  : m_range(0)
  , m_ownsRange(true)
  , m_url(0)
{
  setTextRange(range);
}

DocumentRangeObject::~ DocumentRangeObject( )
{
  if (m_range && m_range->isSmartRange())
    m_range->toSmartRange()->removeWatcher(this);

  if (m_ownsRange)
    EditorIntegrator::releaseRange(m_range);

  delete m_url;
}

void DocumentRangeObject::setTextRange( Range * range, bool ownsRange )
{
  Q_ASSERT(range);

  QMutexLocker lock(&m_rangeMutex);

  if (m_range == range)
    return;

  if (m_range) {
    // TODO.. overkill???
    if (m_range->isSmartRange()) {
      m_range->toSmartRange()->removeWatcher(this);
      Q_ASSERT(m_url);
      delete m_url;
      m_url = 0;
    }

    if (m_ownsRange)
      EditorIntegrator::releaseRange(m_range);
  }

  m_range = range;
  m_ownsRange = ownsRange;

  if (m_range->isSmartRange()) {
    m_range->toSmartRange()->addWatcher(this);
    m_url = new KUrl(url(m_range));
  }
}

const Range DocumentRangeObject::textRange( ) const
{
  QMutexLocker lock(&m_rangeMutex);
  return *m_range;
}

void DocumentRangeObject::setRange(const Range& range)
{
  QMutexLocker lock(&m_rangeMutex);
  *m_range = range;
}

const DocumentRange DocumentRangeObject::textDocRange() const
{
  QMutexLocker lock(&m_rangeMutex);
  return *static_cast<DocumentRange*>(m_range);
}

KUrl DocumentRangeObject::url() const
{
  QMutexLocker lock(&m_rangeMutex);
  return url(m_range);
}

KUrl DocumentRangeObject::url( const Range * range )
{
  if (range->isSmartRange())
    return static_cast<const SmartRange*>(range)->document()->url();
  else
    return static_cast<const DocumentRange*>(range)->document();
}

SmartRange* DocumentRangeObject::smartRange() const
{
  QMutexLocker lock(&m_rangeMutex);

  if (m_range->isSmartRange())
    return static_cast<SmartRange*>(m_range);

  return 0L;
}

bool DocumentRangeObject::contains(const DocumentCursor& cursor) const
{
  QMutexLocker lock(&m_rangeMutex);
  return url(m_range) == cursor.document() && m_range->contains(cursor);
}

Range* DocumentRangeObject::textRangePtr() const
{
  QMutexLocker lock(&m_rangeMutex);
  return m_range;
}

// kate: indent-width 2;

void DocumentRangeObject::rangeDeleted(KTextEditor::SmartRange * range)
{
  QMutexLocker lock(&m_rangeMutex);
  Q_ASSERT(range == m_range);
  Q_ASSERT(m_url);
  //Q_ASSERT(false);
  m_range = new DocumentRange(*m_url, *m_range);
}

KTextEditor::Range* DocumentRangeObject::takeRange()
{
  QMutexLocker lock(&m_rangeMutex);

  KTextEditor::Range* ret = m_range;

  if (m_range) {
    // TODO.. overkill???
    if (m_range->isSmartRange())
      m_range->toSmartRange()->removeWatcher(this);

    m_range = 0;
  }

  return ret;
}

}
