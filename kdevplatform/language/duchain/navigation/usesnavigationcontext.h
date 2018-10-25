/*
   Copyright 2008 David Nolden <david.nolden.kdevelop@art-master.de>

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

#ifndef KDEVPLATFORM_USESNAVIGATIONCONTEXT_H
#define KDEVPLATFORM_USESNAVIGATIONCONTEXT_H

#include "abstractnavigationwidget.h"
#include <language/languageexport.h>

namespace KDevelop {
class UsesWidget;
class KDEVPLATFORMLANGUAGE_EXPORT UsesNavigationContext
    : public AbstractNavigationContext
{
    Q_OBJECT

public:
    explicit UsesNavigationContext(KDevelop::IndexedDeclaration declaration,
                                   AbstractNavigationContext* previousContext = nullptr);

    ~UsesNavigationContext() override;

    QString name() const override;
    QWidget* widget() const override;
    QString html(bool shorten) override;

private:
    KDevelop::IndexedDeclaration m_declaration;
    UsesWidget* m_widget;
};
}

#endif
