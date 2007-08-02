/* KDevelop QMake Support
 *
 * Copyright 2006 Andreas Pakulat <apaku@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "qmakeprojectfile.h"

#include <QtCore/QList>
#include <QtCore/QStringList>

#include <kurl.h>
#include <kdebug.h>

#include "qmakeast.h"

const QStringList QMakeProjectFile::FileVariables = QStringList() << "IDLS"
        << "RESOURCES" << "IMAGES" << "LEXSOURCES" << "DISTFILES"
        << "YACCSOURCES" << "TRANSLATIONS" << "HEADERS" << "SOURCES"
        << "INTERFACES" << "FORMS" ;

QMakeProjectFile::QMakeProjectFile( const KUrl& projectfile )
    : QMakeFile(projectfile), m_mkSpecs(0)
{
}

void QMakeProjectFile::setMkSpecs( QMakeMkSpecs* mkspecs )
{
    m_mkSpecs = mkspecs;
}

QList<QMakeProjectFile*> QMakeProjectFile::subProjects() const
{
    kDebug(9024) << k_funcinfo << "Fetching subprojects" << endl;
    QList<QMakeProjectFile*> list;
    foreach( QString subdir, variableValues( "SUBDIRS" ) )
    {
        kDebug(9024) << k_funcinfo << "Found value: " << subdir << endl;
        KUrl u = absoluteDirUrl();
        u.adjustPath( KUrl::AddTrailingSlash );
        u.setFileName( subdir.trimmed() );
        QMakeProjectFile* qmscope = new QMakeProjectFile( u );
        qmscope->setMkSpecs( m_mkSpecs );
        if( qmscope->read() )
        {
            list.append( qmscope );
        }
    }
    kDebug(9024) << k_funcinfo << "found " << list.size() << " subprojects" << endl;
    return list;
}

KUrl::List QMakeProjectFile::files() const
{
    kDebug(9024) << k_funcinfo << "Fetching files" << endl;


    KUrl::List list;
    foreach( QString variable, QMakeProjectFile::FileVariables )
    {
        foreach( QString value, variableValues(variable) )
        {
                KUrl u = absoluteDirUrl();
                u.adjustPath( KUrl::AddTrailingSlash );
                u.setFileName( value.trimmed() );
                list.append( u );
        }
    }
    list.append( absoluteFileUrl() );
    kDebug(9024) << k_funcinfo << "found " << list.size() << " files" << endl;
    return list;
}

QStringList QMakeProjectFile::targets() const
{
    kDebug(9024) << k_funcinfo << "Fetching targets" << endl;

    QStringList list;

    foreach( QString value, variableValues("INSTALLS") )
    {
        if( value.trimmed() != "target" )
        {
            list << value;
        }
    }
    kDebug(9024) << k_funcinfo << "found " << list.size() << " targets" << endl;
    return list;
}

QMakeProjectFile::~QMakeProjectFile()
{
}

// kate: space-indent on; indent-width 4; tab-width: 4; replace-tabs on; auto-insert-doxygen on
