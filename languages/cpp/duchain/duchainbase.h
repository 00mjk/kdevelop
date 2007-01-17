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

#ifndef DUCHAINBASE_H
#define DUCHAINBASE_H

class TopDUContext;
class QReadWriteLock;
class DUChainModel;

#include "kdevdocumentrangeobject.h"
#include <kdevexport.h>
/**
 * Base class for definition-use chain objects.
 */
class KDEVCPPLANGUAGE_EXPORT DUChainBase : public Koncrete::DocumentRangeObject
{
  friend class ProxyObject;

public:
  DUChainBase(KTextEditor::Range* range);
  virtual ~DUChainBase();

  int modelRow;

  inline unsigned int lastEncountered() const { return m_encountered; }
  /// TODO atomic set? or assert locked?
  void setEncountered(unsigned int encountered) { m_encountered = encountered; }

  virtual TopDUContext* topContext() const;

private:
  // For proxy object
  DUChainBase();

  unsigned int m_encountered;
};

#endif // DUCHAINBASE_H

// kate: indent-width 2;
