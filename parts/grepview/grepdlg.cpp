/***************************************************************************
 *   Copyright (C) 1999-2001 by Bernd Gehrmann and the KDevelop Team       *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "grepdlg.h"

#include <qlayout.h>
#include <qpushbutton.h>
#include <qregexp.h>
#include <qhbox.h>
#include <qwhatsthis.h>
#include <qstringlist.h>
#include <qfiledialog.h>
#include <kbuttonbox.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <qlabel.h>


const char *template_desc[] = {
    "verbatim",
    "assignment",
    "->MEMBER(",
    "class::MEMBER(",
    "OBJECT->member(",
    "[OBJECT method",
    0
};

const char *template_str[] = {
    "%s",
    "\\<%s\\>[\\t ]*=[^=]",
    "\\->[\\t ]*\\<%s\\>[\\t ]*(",
    "[a-z0-9_$]\\+[\\t ]*::[\\t ]*\\<%s\\>[\\t ]*(",
    "\\<%s\\>[\\t ]*\\->[\\t ]*[a-z0-9_$]\\+[\\t ]*(",
    "\\[[\\t ]*\\<%s\\>[\\t ]*[a-zA-Z0-9_$:]",
    0
};

const char *filepatterns[] = {
    "*.h,*.hxx,*.cpp,*.cc,*.C,*.cxx,*.idl,*.c,*.m",
    "*.cpp,*.cc,*.C,*.cxx,*.c,*.m",
    "*.h,*.hxx,*.idl",
    "*.java",
    "*",
    0
};


GrepDialog::GrepDialog(QWidget *parent, const char *name)
    : QDialog(parent, name, false)
{
    setCaption(i18n("Search in Files"));

    config = kapp->config();
    config->setGroup("GrepDialog");

    QGridLayout *layout = new QGridLayout(this, 6, 2, 10, 4);
    layout->addRowSpacing(4, 10);
    layout->setRowStretch(4, 0);
    layout->setColStretch(0, 0);
    layout->setColStretch(1, 20);

    QLabel *pattern_label = new QLabel(i18n("&Pattern:"), this);
    layout->addWidget(pattern_label, 0, 0, AlignRight | AlignVCenter);

    pattern_combo = new QComboBox(true, this);
    pattern_label->setBuddy(pattern_combo);
    pattern_combo->setFocus();
    pattern_combo->insertStringList(config->readListEntry("LastSearchItems"));
    pattern_combo->setInsertionPolicy(QComboBox::NoInsertion);
    layout->addWidget(pattern_combo, 0, 1);
    
    QLabel *template_label = new QLabel(i18n("&Template:"), this);
    layout->addWidget(template_label, 1, 0, AlignRight | AlignVCenter);

    QBoxLayout *template_layout = new QHBoxLayout(4);
    layout->addLayout(template_layout, 1, 1);
    
    template_edit = new QLineEdit(this);
    template_label->setBuddy(template_edit);
    template_edit->setText(template_str[0]);
    template_layout->addWidget(template_edit, 1);

    QComboBox *template_combo = new QComboBox(false, this);
    template_combo->insertStrList(template_desc);
    template_layout->addWidget(template_combo, 0);

    QLabel *files_label = new QLabel(i18n("&Files:"), this);
    layout->addWidget(files_label, 2, 0, AlignRight | AlignVCenter);

    files_combo = new QComboBox(true, this);
    files_label->setBuddy(files_combo->focusProxy());
    files_combo->insertStrList(filepatterns);
    layout->addWidget(files_combo, 2, 1);

    QLabel *dir_label = new QLabel(i18n("Di&rectory:"), this);
    layout->addWidget(dir_label, 3, 0, AlignRight | AlignVCenter);

    QBoxLayout *dir_layout = new QHBoxLayout(4);
    layout->addLayout(dir_layout, 3, 1);
    
    dir_combo = new QComboBox(true, this);
    dir_combo->insertStringList(config->readListEntry("LastSearchPaths"));
    dir_combo->setInsertionPolicy(QComboBox::NoInsertion);
    dir_combo->setEditText(QDir::homeDirPath());

    dir_label->setBuddy(dir_combo);
    dir_combo->setMinimumWidth(dir_combo->fontMetrics().maxWidth()*25);
    dir_layout->addWidget(dir_combo, 10);

    QPushButton *dir_button = new QPushButton("...", this, "dirButton");
    dir_button->setFixedSize(30, 25);
    dir_layout->addWidget(dir_button);
    
    recursive_box = new QCheckBox(i18n("&Recursive"), this);
    recursive_box->setChecked(true);
    dir_layout->addSpacing(10);
    dir_layout->addWidget(recursive_box);

    QBoxLayout *button_layout = new QHBoxLayout(4);
    layout->addLayout(button_layout, 5, 1);
    QPushButton *search_button = new QPushButton(i18n("&Search"), this);
    search_button->setDefault(true);
    QPushButton *done_button = new QPushButton(i18n("Cancel"), this);
    button_layout->addStretch();
    button_layout->addWidget(search_button);
    button_layout->addWidget(done_button);

    resize(sizeHint());

    QWhatsThis::add(pattern_combo,
		    i18n("<qt>Enter the regular expression you want to search for here.<p>"
			 "Possible meta characters are:"
                         "<ul>"
			 "<li><b>.</b> - Matches any character"
			 "<li><b>^</b> - Matches the beginning of a line"
			 "<li><b>$</b> - Matches the end of a line"
	                 "<li><b>\\&lt;</b> - Matches the beginning of a word"
                         "<li><b>\\&gt;</b> - Matches the end of a word"
			 "</ul>"
			 "The following repetition operators exist:"
                         "<ul>"
			 "<li><b>?</b> - The preceding item is matches at most once"
			 "<li><b>*</b> - The preceding item is matched zero or more times"
			 "<li><b>+</b> - The preceding item is matched once or more times"
			 "<li><b>{<i>n</i>}</b> - The preceding item is matched exactly <i>n</i> times"
			 "<li><b>{<i>n</i>,}</b> - The preceding item is matched <i>n</i> or more times"
			 "<li><b>{,<i>n</i>}</b> - The preceding item is matched at most <i>n</i> times"
			 "<li><b>{<i>n</i>,<i>m</i>}</b> - The preceding item is matched at least <i>n</i>, "
			 "but at most <i>m</i> times."
			 "</ul>"
			 "Furthermore, backreferences to bracketed subexpressions are "
			 "available via the notation \\<i>n</i>.</qt>"
			 ));
    QWhatsThis::add(files_combo,
		    i18n("Enter the file name pattern of the files to search here. "
			 "You may give several patterns separated by commas"));
    QWhatsThis::add(template_edit,
		    i18n("You can choose a template for the pattern from the combo box"
			 "and edit it here. The string %s in the template is replaced"
			 "by the pattern input field, resulting in the regular expression"
			 "to search for."));

    connect( template_combo, SIGNAL(activated(int)),
	     SLOT(templateActivated(int)) );
    connect( dir_button, SIGNAL(clicked()),
	     SLOT(dirButtonClicked()) );
    connect( search_button, SIGNAL(clicked()),
	     SLOT(slotSearchClicked()) );
    connect( done_button, SIGNAL(clicked()),
	     SLOT(hide()) );
}

// Returns the contents of a QComboBox as a QStringList
static QStringList qCombo2StringList( QComboBox* combo )
{
    QStringList list;
    if (!combo)
	return list;
    for (int i = 0; i < combo->count(); ++i ) {
	list << combo->text(i);
    }
    return list;
}


GrepDialog::~GrepDialog()
{
    config->setGroup("GrepDialog");
    // memorize the last patterns and paths
    config->writeEntry("LastSearchItems", qCombo2StringList(pattern_combo));
    config->writeEntry("LastSearchPaths", qCombo2StringList(dir_combo));
}


void GrepDialog::dirButtonClicked()
{
    dir_combo->setEditText(QFileDialog::getExistingDirectory(dir_combo->currentText()));
}


void GrepDialog::templateActivated(int index)
{
    template_edit->setText(template_str[index]);
}

// Find out whether the string s is already contained in combo
static bool qComboContains( const QString& s, QComboBox* combo )
{
    if (!combo)
	return false;
    for (int i = 0; i < combo->count(); ++i ) {
	if (combo->text(i) == s) {
	    return true;
	}
    }
    return false;
}

void GrepDialog::slotSearchClicked()
{
    if (pattern_combo->currentText().isEmpty()) {
	KMessageBox::sorry(this, i18n("Please enter a search pattern"));
	pattern_combo->setFocus();
	return;
    }
    // add the last patterns and paths to the combo
    if (!qComboContains(pattern_combo->currentText(), pattern_combo)) {
	pattern_combo->insertItem(pattern_combo->currentText(), 0);
    }
    if (pattern_combo->count() > 15) {
	pattern_combo->removeItem(15);
    }
    if (!qComboContains(dir_combo->currentText(), dir_combo)) {
	dir_combo->insertItem(dir_combo->currentText(), 0);
    }
    if (dir_combo->count() > 15) {
	dir_combo->removeItem(15);
    }

    emit searchClicked();
    hide();
}

#include "grepdlg.moc"
