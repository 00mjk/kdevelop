/*
 * KDevelop Class viewer
 *
 * Copyright 2006 Adam Treat <treat@kde.org>
 * Copyright (c) 2006-2007 Hamish Rodda <rodda@kde.org>
 * Copyright 2009 Lior Mualem <lior.m.kde@gmail.com>
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

#include "classwidget.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>

#include <KLocalizedString>
#include <QLineEdit>
#include <QTimer>

#include "language/classmodel/classmodel.h"
#include "classtree.h"
#include "classbrowserplugin.h"

using namespace KDevelop;
 
ClassWidget::ClassWidget(QWidget* parent, ClassBrowserPlugin* plugin)
  : QWidget(parent)
  , m_plugin(plugin)
  , m_model(new ClassModel())
  , m_tree(new ClassTree(this, plugin))
  , m_searchLine(new QLineEdit(this))
  , m_filterTimer(new QTimer(this))
{
  setObjectName(QStringLiteral("Class Browser Tree"));
  setWindowTitle(i18n("Classes"));
  setWindowIcon(QIcon::fromTheme(QStringLiteral("code-class"), windowIcon()));

  // Set tree in the plugin
  m_plugin->setActiveClassTree(m_tree);

  // Set model in the tree view
  m_tree->setModel(m_model);
  m_tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  m_tree->header()->setStretchLastSection(false);

  // We need notification in the model for the collapse/expansion of nodes.
  connect(m_tree, &ClassTree::collapsed,
          m_model, &ClassModel::collapsed);
  connect(m_tree, &ClassTree::expanded,
          m_model, &ClassModel::expanded);

  // Init filter timer
  m_filterTimer->setSingleShot(true);
  m_filterTimer->setInterval(500);
  connect(m_filterTimer, &QTimer::timeout, this, [this]() {
    m_model->updateFilterString(m_filterText);

    if (m_filterText.isEmpty())
      m_tree->collapseAll();
    else
      m_tree->expandToDepth(0);
  });

  // Init search box
  m_searchLine->setClearButtonEnabled( true );
  connect(m_searchLine, &QLineEdit::textChanged, this, [this](const QString& newFilter) {
    m_filterText = newFilter;
    m_filterTimer->start();
  });

  QLabel *searchLabel = new QLabel( i18n("S&earch:"), this );
  searchLabel->setBuddy( m_searchLine );

  QHBoxLayout* layout = new QHBoxLayout();
  layout->setSpacing( 5 );
  layout->setMargin( 0 );
  layout->addWidget(searchLabel);
  layout->addWidget(m_searchLine);

  setFocusProxy( m_searchLine );

  QVBoxLayout* vbox = new QVBoxLayout(this);
  vbox->setMargin(0);
  vbox->addLayout(layout);
  vbox->addWidget(m_tree);
  setLayout( vbox );

  setWhatsThis( i18n( "Class Browser" ) );
}

ClassWidget::~ClassWidget()
{
  delete m_model;
}

// kate: space-indent on; indent-width 2; tab-width: 4; replace-tabs on; auto-insert-doxygen on

