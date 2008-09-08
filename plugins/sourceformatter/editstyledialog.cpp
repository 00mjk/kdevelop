/* This file is part of KDevelop
*  Copyright (C) 2008 Cédric Pasteur <cedric.pasteur@free.fr>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.

*/
#include "editstyledialog.h"

#include <QVBoxLayout>
#include <KMessageBox>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/editorchooser.h>
#include <ktexteditor/configinterface.h>

#include <util/interfaces/isourceformatter.h>

EditStyleDialog::EditStyleDialog(ISourceFormatter *formatter, const KMimeType::Ptr &mime,
                                  const QString &content, QWidget *parent)
		: KDialog(parent), m_sourceFormatter(formatter), m_mimeType(mime)
{
	m_content = new QWidget();
	m_ui.setupUi(m_content);
	setMainWidget(m_content);

    m_settingsWidget = m_sourceFormatter->editStyleWidget(mime);
    if(!content.isEmpty())
        m_settingsWidget->load(QString(), content);
	init();
}

EditStyleDialog::~EditStyleDialog()
{
}

void EditStyleDialog::init()
{
	// add plugin settings widget
	QVBoxLayout *layout = new QVBoxLayout(m_ui.settingsWidgetParent);
	layout->addWidget(m_settingsWidget);
	m_ui.settingsWidgetParent->setLayout(layout);
	connect(m_settingsWidget, SIGNAL(previewTextChanged(const QString&)),
	        this, SLOT(updatePreviewText(const QString&)));

	// add texteditor preview
	KTextEditor::Editor *editor = KTextEditor::EditorChooser::editor();
	if (!editor)
		KMessageBox::error(this, i18n("A KDE text-editor component could not be found.\n"
		                              "Please check your KDE installation."));

	m_document = editor->createDocument(this);
	//m_document->setHighlightingMode("C++");

	m_view = qobject_cast<KTextEditor::View*>(
	           m_document->createView(m_ui.textEditor));
	QVBoxLayout *layout2 = new QVBoxLayout(m_ui.textEditor);
	layout2->addWidget(m_view);
	m_ui.textEditor->setLayout(layout2);
	m_view->show();

	KTextEditor::ConfigInterface *iface =
	  qobject_cast<KTextEditor::ConfigInterface*>(m_view);
	if (iface) {
		iface->setConfigValue("dynamic-word-wrap", false);
		iface->setConfigValue("icon-bar", false);
	}
}

void EditStyleDialog::updatePreviewText(const QString &text)
{
	m_document->setText(m_sourceFormatter->formatSource(text, m_mimeType));
}

QString EditStyleDialog::content()
{
    return m_settingsWidget->save();
}

#include "editstyledialog.moc"
