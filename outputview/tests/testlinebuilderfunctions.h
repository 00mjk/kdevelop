/*
    This file is part of KDevelop
    Copyright (C) 2012  Morten Danielsen Volden mvolden2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef KDEVPLATFORM_TESTLINEBUILDERFUNCTIONS_H
#define KDEVPLATFORM_TESTLINEBUILDERFUNCTIONS_H

#include <QString>
#include <QFileInfo>

#define PROJECTS_SOURCE_DIR

namespace KDevelop
{

static QString projectPath()
{
    /// Use existing directory with one file
    return QFileInfo(QStringLiteral(__FILE__)).absolutePath() + QLatin1String("/onefileproject");
}

QString buildCppCheckErrorLine()
{
    /// Test CPP check output
    QString outputline(QStringLiteral("["));
    outputline.append(projectPath());
    outputline.append("main.cpp:26]: (error) Memory leak: str");
    return outputline;
}

QString buildKrazyErrorLine()
{
    /// Test krazy2 output
    QString outputline(QStringLiteral("\t"));
    outputline.append(projectPath());
    outputline.append("main.cpp: line#22 (1)");
    return outputline;
}

QString buildKrazyErrorLine2()
{
    /// Test krazy2 output
    QString outputline(QStringLiteral("\t"));
    outputline.append(projectPath());
    outputline.append("main.cpp: missing tags: email address line#2  (1)");
    return outputline;
}

QString buildKrazyErrorLine3()
{
    /// Test krazy2 output
    QString outputline(QStringLiteral("\t"));
    outputline.append(projectPath());
    outputline.append("main.cpp: non-const ref iterator line#451 (1)");
    return outputline;
}

QString buildKrazyErrorLineNoLineInfo()
{
    /// Test krazy2 output
    QString outputline(QStringLiteral("\t"));
    outputline.append(projectPath());
    outputline.append("main.cpp: missing license");
    return outputline;
}

QString buildCompilerLine()
{
    /// Test with compiler output
    QString outputline;
    outputline.append(projectPath());
    outputline.append(">make");
    return outputline;
}

QString buildCompilerErrorLine()
{
    QString outputline;
    outputline.append(projectPath());
    outputline.append("main.cpp:5:5: error: ‘RingBuffer’ was not declared in this scope");
    return outputline;
}

QString buildCompilerInformationLine()
{
    QString outputline;
    outputline.append(projectPath());
    outputline.append("main.cpp:6:14: instantiated from here");
    return outputline;
}

QString buildInfileIncludedFromFirstLine()
{
    QString outputline(QStringLiteral("In file included from "));
    outputline.append(projectPath());
    outputline.append("PriorityFactory.h:52:0,");
    return outputline;
}

QString buildInfileIncludedFromSecondLine()
{
    QString outputline(QStringLiteral("    from "));
    outputline.append(projectPath());
    outputline.append("PatchBasedInpainting.hxx:29,");
    return outputline;
}

QString buildCompilerActionLine()
{
    return QStringLiteral("linking testCustombuild (g++)");
}

QString buildCmakeConfigureMultiLine()
{
    QString outputline;
    outputline.append(projectPath());
    outputline.append("CMakeLists.txt:10:");
    return outputline;
}


QString buildLinkerErrorLine()
{
    return QStringLiteral("/path/to/file/Buffer.cpp:66: undefined reference to `Buffer::does_not_exist()'");
}

QString buildPythonErrorLine()
{
    QString outputline(QStringLiteral("File \""));
    outputline.append(projectPath());
    outputline.append("pythonExample.py\", line 10");
    return outputline;
}

QString buildCppCheckInformationLine()
{
    return QStringLiteral("(information) Cppcheck cannot find all the include files. Cpppcheck can check the code without the include\
    files found. But the results will probably be more accurate if all the include files are found. Please check your project's \
    include directories and add all of them as include directories for Cppcheck. To see what files Cppcheck cannot find use --check-config.");
}


}

#endif
