/***************************************************************************
 *   Copyright (C) 2003 by Roberto Raggi                                   *
 *   roberto@kdevelop.org                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "settingsdialog.h"
#include <klistbox.h>
#include <kcombobox.h>
#include <qfile.h>
#include <qdir.h>
#include <qregexp.h>
#include <cstdlib>

#if QT_VERSION < 0x030100
/* original source from qt-3.2.1/src/widgets/qlistbox.cpp
QListBoxItem* QListBox::selectedItem() const
{
    if ( d->selectionMode != Single )
	return 0;
    if ( isSelected( currentItem() ) )
	return  d->current;
    return 0;
}
*/
QListBoxItem* QListBox_selectedItem(QListBox* cpQListBox)
{
    if ( cpQListBox->selectionMode() != QListBox::Single )
    return 0;
    if ( cpQListBox->isSelected( cpQListBox->currentItem() ) )
    return  cpQListBox->item(cpQListBox->currentItem());
    return 0;
}
#endif

SettingsDialog::SettingsDialog(QWidget* parent, const char* name, WFlags fl)
    : SettingsDialogBase(parent,name,fl)
{
    QStringList kdedirs;
    kdedirs.push_back( ::getenv("KDEDIR") + QString("/include") );
    kdedirs.push_back( "/usr/lib/kde/include" );
    kdedirs.push_back( "/usr/local/kde/include" );
    kdedirs.push_back( "/usr/local/include" );
    kdedirs.push_back( "/usr/kde/include" );
    kdedirs.push_back( "/usr/include/kde" );
    kdedirs.push_back( "/usr/include" );
    kdedirs.push_back( "/opt/kde3/include" );
    kdedirs.push_back( "/opt/kde/include" );

    for( QStringList::Iterator it=kdedirs.begin(); it!=kdedirs.end(); ++it )
    {
        QString kdedir = *it;
        if( !kdedir.isEmpty() && isValidKDELibsDir(kdedir) )
            if (!kdeListBox->findItem(kdedir, ExactMatch))
                kdeListBox->insertItem( kdedir );
    }
}

SettingsDialog::~SettingsDialog()
{
}

/*$SPECIALIZATION$*/
void SettingsDialog::slotSelectionChanged(QListBoxItem* item)
{
#if QT_VERSION < 0x030100
    if( !QListBox_selectedItem(kdeListBox) ){
#else
    if( !kdeListBox->selectedItem() ){
#endif
	emit enabled( false );
	return;
    }

    emit enabled( true );
}

bool SettingsDialog::isValidKDELibsDir( const QString & path ) const
{
    return QFile::exists( path + "/kapplication.h" );
}

QString SettingsDialog::kdeDir( ) const
{
    return kdeListBox->currentText();
}


#include "settingsdialog.moc"


