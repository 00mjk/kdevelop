/*
 * This file is part of KDevelop
 *
 * Copyright 2014 Sergey Kalinichev <kalinichev.so.0@gmail.com>
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

#include "test_custommake.h"

#include <QtTest/QtTest>

#include <qtest_kde.h>

#include <QFile>
#include <QTextStream>
#include <KDebug>
#include <KTempDir>

#include <tests/autotestshell.h>
#include <tests/testcore.h>

#include "../makefileresolver.h"

namespace {
void createFile( QFile& file )
{
    file.remove();
    if ( !file.open( QIODevice::ReadWrite ) ) {
        kFatal() << "Cannot create the file " << file.fileName();
    }
}
}

void TestCustomMake::initTestCase() {
    KDevelop::AutoTestShell::init();
    KDevelop::TestCore::initialize(KDevelop::Core::NoUi);
}

void TestCustomMake::cleanupTestCase() {
    KDevelop::TestCore::shutdown();
}

void TestCustomMake::testIncludeDirectories() {
    KTempDir tempDir;
    {
        QFile file( tempDir.name() + "Makefile" );
        createFile( file );
        QFile testfile( tempDir.name() + "testfile.cpp" );
        createFile(testfile);
        QTextStream stream1( &file );
        stream1 << "testfile.o:\n\t g++ testfile.cpp -I/testFile1 -I /testFile2 -isystem /testFile3 --include-dir=/testFile4 -o testfile";
    }

    MakeFileResolver mf;
    auto result = mf.resolveIncludePath(tempDir.name() + "testfile.cpp");
    QVERIFY(result.paths.contains("/testFile1"));
    QVERIFY(result.paths.contains("/testFile2"));
    QVERIFY(result.paths.contains("/testFile3"));
    QVERIFY(result.paths.contains("/testFile4"));
    QCOMPARE(result.paths.size(), 4);
}

QTEST_KDEMAIN(TestCustomMake, NoGUI)