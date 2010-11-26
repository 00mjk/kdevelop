/* KDevelop CMake Support
 *
 * Copyright 2008 Matt Rogers <mattr@kde.org>
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

#include "cmakeparserutils.h"

#include <QStringList>
#include <qfileinfo.h>
#include <qdir.h>
#include <kdebug.h>
#include "variablemap.h"
#include <kprocess.h>
#include <kstandarddirs.h>
#include "cmakeprojectvisitor.h"
#include <ktempdir.h>
#include <cmakeprojectdata.h>

namespace CMakeParserUtils
{
    QList<int> parseVersion(const QString& version, bool* ok)
    {
        QList<int> versionNumList;
        *ok = false;
        QStringList versionStringList = version.split('.', QString::SkipEmptyParts);
        if (versionStringList.isEmpty())
            return versionNumList;

        foreach(const QString& part, versionStringList)
        {
            int i = part.toInt(ok);
            if (!*ok)
            {
                versionNumList.clear();
                return versionNumList;
            }
            else
                versionNumList.append(i);
        }
        return versionNumList;
    }

    QString valueFromSystemInfo(const QString& variable, const QString& systeminfo)
    {
        int idx = systeminfo.indexOf( variable );
        if( idx != -1 ) {
            // 2 == ' "'
            idx += variable.count() + 2;
            int end = systeminfo.indexOf( "\"", idx );
            if( end != -1 ) {
                return systeminfo.mid( idx, end-idx );
            }
        }
        return QString();
    }


    QPair<VariableMap,QStringList> initialVariables()
    {
        QString cmakeCmd=KStandardDirs::findExe("cmake");
        
        QString systeminfo=executeProcess(cmakeCmd, QStringList("--system-information"));
        
        VariableMap m_varsDef;
        QStringList modulePathDef=QStringList(CMakeParserUtils::valueFromSystemInfo( "CMAKE_ROOT", systeminfo ) + "/Modules");
        kDebug(9042) << "found module path is" << modulePathDef;
        m_varsDef.insert("CMAKE_BINARY_DIR", QStringList("#[bin_dir]"));
        m_varsDef.insert("CMAKE_INSTALL_PREFIX", QStringList("#[install_dir]"));
        m_varsDef.insert("CMAKE_COMMAND", QStringList(cmakeCmd));
        m_varsDef.insert("CMAKE_MAJOR_VERSION", QStringList(CMakeParserUtils::valueFromSystemInfo("CMAKE_MAJOR_VERSION", systeminfo)));
        m_varsDef.insert("CMAKE_MINOR_VERSION", QStringList(CMakeParserUtils::valueFromSystemInfo("CMAKE_MINOR_VERSION", systeminfo))); 
        m_varsDef.insert("CMAKE_PATCH_VERSION", QStringList(CMakeParserUtils::valueFromSystemInfo("CMAKE_PATCH_VERSION", systeminfo)));
        
        QStringList cmakeInitScripts;
        #ifdef Q_OS_WIN
            cmakeInitScripts << "CMakeMinGWFindMake.cmake";
            cmakeInitScripts << "CMakeMSYSFindMake.cmake";
            cmakeInitScripts << "CMakeNMakeFindMake.cmake";
            cmakeInitScripts << "CMakeVS8FindMake.cmake";
        #else
            cmakeInitScripts << "CMakeUnixFindMake.cmake";
        #endif
        cmakeInitScripts << "CMakeDetermineSystem.cmake";
        cmakeInitScripts << "CMakeSystemSpecificInformation.cmake";
        cmakeInitScripts << "CMakeDetermineCCompiler.cmake";
        cmakeInitScripts << "CMakeDetermineCXXCompiler.cmake";
        
        m_varsDef.insert("CMAKE_MODULE_PATH", modulePathDef);
        m_varsDef.insert("CMAKE_ROOT", QStringList(CMakeParserUtils::valueFromSystemInfo("CMAKE_ROOT", systeminfo)));
        
        //Defines the behaviour that can't be identified on initialization scripts
        #ifdef Q_OS_WIN32
            m_varsDef.insert("WIN32", QStringList("1"));
            m_varsDef.insert("CMAKE_HOST_WIN32", QStringList("1"));
        #else
            m_varsDef.insert("UNIX", QStringList("1"));
            m_varsDef.insert("CMAKE_HOST_UNIX", QStringList("1"));
        #endif
        #ifdef Q_OS_MAC
            m_varsDef.insert("APPLE", QStringList("1"));
            m_varsDef.insert("CMAKE_HOST_APPLE", QStringList("1"));
        #endif
        return QPair<VariableMap,QStringList>( m_varsDef, cmakeInitScripts );
    }

    QString executeProcess(const QString& execName, const QStringList& args)
    {
        kDebug(9042) << "Executing:" << execName << "::" << args /*<< "into" << *m_vars*/;
        
        KProcess p;
        p.setOutputChannelMode(KProcess::MergedChannels);
        p.setProgram(execName, args);
        KTempDir tmp(KStandardDirs::locateLocal("tmp", "kdevcmakemanager"));
        p.setWorkingDirectory( tmp.name() );
        p.start();
        
        if(!p.waitForFinished())
        {
            kDebug() << "failed to execute:" << execName;
        }
        
        QByteArray b = p.readAllStandardOutput();
        QString t;
        t.prepend(b.trimmed());
        kDebug(9042) << "executed" << execName << "<" << t;
        tmp.unlink();
        return t;
    }

    void printSubdirectories(const QList<Subdirectory>& subs)
    {
        Q_FOREACH(const Subdirectory& s, subs) {
            qDebug() << "lala " << s.name;
        }
    }
    
    KDevelop::ReferencedTopDUContext includeScript(const QString& file, KDevelop::ReferencedTopDUContext parent, CMakeProjectData* data, const QString& sourcedir)
    {
        kDebug(9042) << "Running cmake script: " << file;
        CMakeFileContent f = CMakeListsParser::readCMakeFile(file);
        
        data->vm.insert("CMAKE_CURRENT_BINARY_DIR", data->vm.value("CMAKE_BINARY_DIR"));
        data->vm.insert("CMAKE_CURRENT_LIST_FILE", QStringList(file));
        data->vm.insert("CMAKE_CURRENT_LIST_DIR", QStringList(QFileInfo(file).dir().absolutePath()));
        data->vm.insert("CMAKE_CURRENT_SOURCE_DIR", QStringList(sourcedir));
        
        CMakeProjectVisitor v(file, parent);
        v.setCacheValues(&data->cache);
        v.setVariableMap(&data->vm);
        v.setMacroMap(&data->mm);
        v.setModulePath(data->modulePath);
        v.walk(f, 0, true);
        
        data->projectName=v.projectName();
        data->subdirectories=v.subdirectories();
        data->definitions=v.definitions();
        data->includeDirectories=v.includeDirectories();
        data->targets=v.targets();
        data->properties=v.properties();
        
        //printSubdirectories(data->subdirectories);
        
        data->vm.remove("CMAKE_CURRENT_LIST_FILE");
        data->vm.remove("CMAKE_CURRENT_LIST_DIR");
        data->vm.remove("CMAKE_CURRENT_SOURCE_DIR");
        data->vm.remove("CMAKE_CURRENT_BINARY_DIR");
        
        return v.context();
    }

    
}
    
