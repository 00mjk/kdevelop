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

#include "navigationtooltip.h"
#include "../duchain/navigation/abstractnavigationwidget.h"
#include <QVBoxLayout>

namespace KDevelop {
NavigationToolTip::NavigationToolTip(QWidget* parent, const QPoint& point, QWidget* navigationWidget) : ActiveToolTip(
        parent, point)
    , m_navigationWidget(nullptr)
{
    Q_ASSERT(parent);
    setBackgroundRole(QPalette::Window);
    setNavigationWidget(navigationWidget);
}

void NavigationToolTip::sizeHintChanged()
{
    QSize size = m_navigationWidget->size();
    QSize hint = m_navigationWidget->sizeHint();
    if (hint.width() > size.width())
        size.setWidth(hint.width());
    if (hint.height() > size.height())
        size.setHeight(hint.height());
    if (size != m_navigationWidget->size())
        resize(size + QSize(15, 15));
}

void NavigationToolTip::setNavigationWidget(QWidget* widget)
{
    if (auto oldWidget = qobject_cast<AbstractNavigationWidget*>(m_navigationWidget)) {
        disconnect(oldWidget, &AbstractNavigationWidget::sizeHintChanged, this, &NavigationToolTip::sizeHintChanged);
    }
    m_navigationWidget = widget;
    if (auto newWidget = qobject_cast<AbstractNavigationWidget*>(widget)) {
        connect(newWidget, &AbstractNavigationWidget::sizeHintChanged, this, &NavigationToolTip::sizeHintChanged);
    }
    auto* layout = new QVBoxLayout;
    setLayout(layout);
    layout->setMargin(0);
    if (m_navigationWidget) {
        layout->addWidget(m_navigationWidget);
    }
}
}
