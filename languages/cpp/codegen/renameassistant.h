/*
   Copyright 2010 Olivier de Gaalon <olivier.jg@gmail.com>

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

#ifndef CPP_RENAMEEASSISTANT_H
#define CPP_RENAMEEASSISTANT_H

#include <interfaces/iassistant.h>
#include <language/duchain/identifier.h>
#include <language/duchain/indexedstring.h>
#include <language/duchain/declaration.h>


namespace KDevelop {
class ParseJob;
}
namespace Cpp {

class RenameAssistant : public KDevelop::ITextAssistant {
Q_OBJECT
public:
  RenameAssistant(KTextEditor::View *view);
  void textChanged(KTextEditor::Range invocationRange, QString removedText = QString());
  bool isUseful() { return m_isUseful; }

private:
  void reset();
  KDevelop::Declaration* getValidDeclarationForChangedRange(KTextEditor::Range changed);

  KDevelop::Identifier m_oldDeclarationName;
  QString m_newDeclarationName;
  KDevelop::PersistentMovingRange::Ptr m_newDeclarationRange;
  QMap <KDevelop::IndexedString, QList <KDevelop::RangeInRevision > > m_oldDeclarationUses;
  KDevelop::IndexedString m_documentUrl;
  bool m_isUseful;
};

}

#endif // CPP_RENAMEEASSISTANT_H
