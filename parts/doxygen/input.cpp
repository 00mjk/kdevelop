/***************************************************************************
 *   Copyright (C) 1997-2000 by Dimitri van Heesch                         *
 *   dimitri@stack.nl                                                      *
 *   Copyright (C) 2001 by Bernd Gehrmann                                  *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qlabel.h>
#include <qlayout.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qtooltip.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <kiconloader.h>

#include "input.h"


static const char * const add_xpm_data[] =
{
  "16 16 5 1",
  ". c None",
  "* c #0328f9",
  "# c #354396",
  "a c #353740",
  "c c #999999",
  "................",
  "......###.......",
  "......#*ac......",
  "......#*ac......",
  "......#*ac......",
  "......#*ac......",
  ".######*a#####..",
  ".#***********ac.",
  ".#aaaaa*aaaaaac.",
  "..cccc#*acccccc.",
  "......#*ac......",
  "......#*ac......",
  "......#*ac......",
  "......#aac......",
  ".......ccc......",
  "................"
};
const char **add_xpm = (const char **)add_xpm_data;

static const char * const del_xpm_data[] =
{
  "16 16 5 1",
  ". c None",
  "* c #0328f9",
  "# c #354396",
  "a c #353740",
  "c c #999999",
  "................",
  "................",
  "................",
  "................",
  "................",
  "................",
  ".#############..",
  ".#***********ac.",
  ".aaaaaaaaaaaaac.",
  "..ccccccccccccc.",
  "................",
  "................",
  "................",
  "................",
  "................",
  "................"
};
const char **del_xpm = (const char **)del_xpm_data;

static char *update_xpm_data[] = 
{
  "16 16 5 1",
  /* colors */
  ". c #0328f9",
  "# c #354396",
  "a c #353740",
  "b c None",
  "c c #999999",
  /* pixels */
  "bbbbbbbbbbbbbbbb",
  "bbbbbbbb#####acb",
  "bbbbbbbb#....abb",
  "bbc##cbb#...acbb",
  "bb#..abb#....abb",
  "bc#..abb#.a..acb",
  "b#..acbbaac#..ab",
  "b#..abbbcbb#..ab",
  "b#..abbbbbb#..ab",
  "b#..acbbbbc#..ab",
  "bc#..#cbbc#..acb",
  "bb#...####...acb",
  "bbca........acbb",
  "bbbbaa....aaccbb",
  "bbbbbcaaaaccbbbb",
  "bbbbbbbbbbbbbbbb"
};
const char **update_xpm = (const char **)update_xpm_data;


InputBool::InputBool(const QString &text, QWidget * parent, bool &flag)
    : QCheckBox(text, parent), state(flag)
{
    init();
    
    connect(this, SIGNAL(toggled(bool)), this, SLOT(valueChanged(bool)));
}


InputBool::~InputBool()
{}


void InputBool::init()
{
    setChecked(state);
}


void InputBool::valueChanged(bool s)
{
    if (s != state) {
        emit changed();
        emit toggle(text(), s);
    }
    state = s;
}


void InputBool::setEnabled(bool b)
{
    QCheckBox::setEnabled(b);
}


InputInt::InputInt(const QString &label, QWidget *parent, int &val, int minVal, int maxVal)
    : QWidget(parent), m_val(val), m_minVal(minVal), m_maxVal(maxVal)
{
    QHBoxLayout *layout = new QHBoxLayout(this, 5);
    
    lab = new QLabel(label, this);
    sp = new QSpinBox(minVal, maxVal, 1, this);
    
    init();
  
    layout->addWidget(lab);
    layout->addWidget(sp);
    layout->addStretch(1);

    connect(sp, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
}


InputInt::~InputInt()
{}


void InputInt::init()
{
    m_val = QMAX(m_minVal, m_val);
    m_val = QMIN(m_maxVal, m_val);
    sp->setValue(m_val);
}


void InputInt::valueChanged(int val)
{
    if (val != m_val)
        emit changed(); 
    m_val = val;
}


void InputInt::setEnabled(bool state)
{
    lab->setEnabled(state);
    sp->setEnabled(state);
}


InputString::InputString(const QString & label, 
                         QWidget *parent, QCString &s, StringMode m)
    : QWidget(parent), str(s), sm(m), m_values(0), m_index(0)
{
    le = 0; br = 0; com = 0;
    
    if (m == StringFixed) {
        QHBoxLayout *layout = new QHBoxLayout(this, 5);
        lab = new QLabel(label, this);
        layout->addWidget(lab);
        com = new QComboBox(this); 
        layout->addWidget(com);
        layout->addStretch(1);
    } else {
        QGridLayout *layout = new QGridLayout(this, 1, m==StringFree? 1 : 3, 5);
        lab = new QLabel(label, this);
        layout->addWidget(lab, 0, 0);
        le = new QLineEdit(this);
        le->setText(s);
        layout->addWidget(le, 0, 1);
        
        if (m == StringFile || m == StringDir) {
            br = new QPushButton(this);
            br->setPixmap(SmallIcon(m==StringFile? "document" : "folder"));
            QToolTip::add(br, m==StringFile? i18n("Browse to a file") : i18n("Browse to a folder"));
            layout->addWidget(br, 0, 2);
        }
    }
    
    if (le)
        connect( le,   SIGNAL(textChanged(const QString&)), 
                 this, SLOT(textChanged(const QString&)) );
    if (br)
        connect( br,   SIGNAL(clicked()), this, SLOT(browse()) );
    if (com)
        connect( com,  SIGNAL(activated(const QString &)), 
                 this, SLOT(textChanged(const QString &)) );
}

InputString::~InputString()
{
    if (m_values)
        delete m_values;
}


void InputString::init()
{
    if (sm == StringFixed) {
        int *itemIndex = m_values->find(str);
        if (itemIndex) 
            com->setCurrentItem(*itemIndex);
        else
            com->setCurrentItem(0);
    } else
        le->setText(str);
}


void InputString::addValue(const char *s)
{
    if (sm == StringFixed) {
        if (!m_values)
            m_values = new QDict<int>;
        m_values->setAutoDelete(true);
        m_values->insert(s, new int(m_index++));
        com->insertItem(s);
    }
}


void InputString::clear()
{
    le->setText("");
    if (!str.isEmpty()) {
        emit changed();
        str = "";
    }
}


void InputString::textChanged(const QString &s)
{
    if (str!=(const char *)s) {
        str = s;
        emit changed();
    }
}

void InputString::setEnabled(bool state)
{
    lab->setEnabled(state);
    if (le)
        le->setEnabled(state);
    if (br)
        br->setEnabled(state);
    if (com)
        com->setEnabled(state);
}


void InputString::browse()
{
    if (sm == StringFile) {
        QString fileName = KFileDialog::getOpenFileName();
    
        if (!fileName.isNull()) {
            le->setText(fileName);
            if (str != (const char *)le->text()) {
                str = le->text(); 
                emit changed();
            }
        }
    } else { // sm==StringDir
        QString dirName = KFileDialog::getExistingDirectory();

        if (!dirName.isNull()) {
            le->setText( dirName ); 	
            if (str != (const char *)le->text()) {
                str = le->text();
                emit changed();
            }
        }	
    }
}


InputStrList::InputStrList(const QString & label, 
                           QWidget *parent, QStrList &sl, ListMode lm)
    : QWidget(parent), strList(sl)
{
    QGridLayout *layout = new QGridLayout(this, 2, 2, 5);
    lab = new QLabel( label, this );
    layout->addWidget(lab, 0, 0);
    
    QWidget *dw = new QWidget(this); /* dummy widget used for layouting */
    QHBoxLayout *boxlayout = new QHBoxLayout(dw, 0, 5);
    le  = new QLineEdit(dw);
    boxlayout->addWidget(le, 1);

    add = new QPushButton(dw);
    add->setPixmap(QPixmap( add_xpm ));
    QToolTip::add(add, i18n("Add item"));
    boxlayout->addWidget(add);
    
    del = new QPushButton(dw);
    del->setPixmap(QPixmap( del_xpm ));
    QToolTip::add(del, i18n("Delete selected item"));
    boxlayout->addWidget(del);

    upd = new QPushButton(dw); 
    upd->setPixmap(QPixmap( update_xpm ));
    QToolTip::add(upd, i18n("Update selected item"));
    boxlayout->addWidget(upd);
    
    lb  = new QListBox(this);
    lb->setMinimumSize(400, 100);
    init();
    lb->setVScrollBarMode(QScrollView::Auto);
    lb->setHScrollBarMode(QScrollView::Auto);
    
    brFile = 0;
    brDir = 0;
    if (lm != ListString) {
        if (lm & ListFile) {
            brFile = new QPushButton(dw);
            brFile->setPixmap(SmallIcon("document"));
            QToolTip::add(brFile, i18n("Browse to a file"));
            boxlayout->addWidget(brFile);
        } 
        if (lm & ListDir) {
            brDir = new QPushButton(dw);
            brDir->setPixmap(SmallIcon("folder"));
            QToolTip::add(brDir, i18n("Browse to a folder"));
            boxlayout->addWidget(brDir);
        }
    }
    layout->addWidget(dw, 0, 1);
    layout->addWidget(lb, 1, 1);
    
    connect( le,   SIGNAL(returnPressed()), 
             this, SLOT(addString()) );
    connect( add,  SIGNAL(clicked()), 
             this, SLOT(addString()) );
    connect( del,  SIGNAL(clicked()), 
             this, SLOT(delString()) );
    connect( upd,  SIGNAL(clicked()), 
             this, SLOT(updateString()) );
    if (brFile) 
        connect( brFile, SIGNAL(clicked()),
                 this, SLOT(browseFiles()) );
    if (brDir)
        connect( brDir, SIGNAL(clicked()),
                 this, SLOT(browseDir()) );
    connect( lb,   SIGNAL(selected(const QString &)), 
             this, SLOT(selectText(const QString &)) );
    
    strList = sl;
}


InputStrList::~InputStrList()
{}


void InputStrList::init()
{
    le->clear();
    lb->clear();
    char *s = strList.first();
    while (s) {
        lb->insertItem(s);
        s = strList.next();
    }
}


void InputStrList::addString()
{
    if (!le->text().isEmpty()) {
        lb->insertItem(le->text());
        strList.append(le->text());
        emit changed();
        le->clear();
    }
}


void InputStrList::delString()
{
    if (lb->currentItem() != -1) {
        int itemIndex = lb->currentItem();
        lb->removeItem(itemIndex);
        strList.remove(itemIndex);
        emit changed();
    }
}


void InputStrList::updateString()
{
    if (lb->currentItem() != -1 && !le->text().isEmpty()) {
        lb->changeItem(le->text(),lb->currentItem());
        strList.insert(lb->currentItem(),le->text());
        strList.remove(lb->currentItem()+1);
        emit changed();
    }
}


void InputStrList::selectText(const QString &s)
{
    le->setText(s);
}


void InputStrList::setEnabled(bool state)
{
    lab->setEnabled(state);
    le->setEnabled(state);
    add->setEnabled(state);
    del->setEnabled(state);
    upd->setEnabled(state);
    lb->setEnabled(state);
    if (brFile)
        brFile->setEnabled(state);
    if (brDir)
        brDir->setEnabled(state);
}


void InputStrList::browseFiles()
{
    QStringList fileNames = KFileDialog::getOpenFileNames();	
    
    if (!fileNames.isEmpty()) {
        QStringList::Iterator it;
        for (it = fileNames.begin(); it != fileNames.end(); ++it) {
            lb->insertItem(*it);
            strList.append(*it);
            emit changed();
        }
        le->setText(*fileNames.begin());
    }
}


void InputStrList::browseDir()
{	
    QString dirName = KFileDialog::getExistingDirectory();	
    
    if (!dirName.isNull()) {
        lb->insertItem(dirName);
        strList.append(dirName);
        emit changed();
        le->setText(dirName);
    }
}
