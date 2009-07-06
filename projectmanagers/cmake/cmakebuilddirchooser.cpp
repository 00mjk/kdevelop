/* KDevelop CMake Support
 *
 * Copyright 2007 Aleix Pol <aleixpol@gmail.com>
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

#include "cmakebuilddirchooser.h"
#include <QDir>
#include <KDebug>
#include <KProcess>
#include <KMessageBox>
#include <KStandardDirs>
#include "ui_cmakebuilddirchooser.h"

CMakeBuildDirChooser::CMakeBuildDirChooser(QWidget* parent)
    : KDialog(parent)
{
//     QWidget* w= new QWidget(this);
    m_chooserUi = new Ui::CMakeBuildDirChooser;
    m_chooserUi->setupUi(mainWidget());
    m_chooserUi->buildFolder->setMode(KFile::Directory|KFile::ExistingOnly);
    m_chooserUi->installPrefix->setMode(KFile::Directory|KFile::ExistingOnly);
//     setMainWidget(w);

    QString cmakeBin=KStandardDirs::findExe( "cmake" );
    setCMakeBinary(KUrl(cmakeBin));
    
    connect(m_chooserUi->cmakeBin, SIGNAL(textChanged(const QString &)), this, SLOT(updated()));
    connect(m_chooserUi->buildFolder, SIGNAL(textChanged(const QString &)), this, SLOT(updated()));
    connect(m_chooserUi->buildType, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(updated()));
    updated();
}

void CMakeBuildDirChooser::setSourceFolder( const KUrl& srcFolder )
{
    m_srcFolder = srcFolder;
    KUrl proposedBuildUrl = KUrl( srcFolder.toLocalFile() + "/build" );
    proposedBuildUrl.cleanPath();
    m_chooserUi->buildFolder->setUrl(proposedBuildUrl);
    update();
}

QString CMakeBuildDirChooser::buildDirProject(const KUrl& buildDir)
{
    QFile file(buildDir.toLocalFile()+"/CMakeCache.txt");

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        kWarning(9032) << "Something really strange happened reading CMakeCache.txt";
        return QString();
    }

    QString ret;
    bool correct=false;
    const QString pLine="CMAKE_HOME_DIRECTORY:INTERNAL=";
    while (!correct && !file.atEnd())
    {
        QString line = file.readLine().trimmed();
        if(line.startsWith(pLine))
        {
            correct=true;
            ret=line.mid(pLine.count());
        }
    }
    kDebug(9042) << "The source directory for " << file.fileName() << "is" << ret;
    return ret;
}

void CMakeBuildDirChooser::updated()
{
    bool haveCMake=QFile::exists(m_chooserUi->cmakeBin->url().toLocalFile());
    StatusTypes st;
    if( haveCMake ) st |= HaveCMake;

    m_chooserUi->buildFolder->setEnabled(haveCMake);
    m_chooserUi->installPrefix->setEnabled(haveCMake);
    m_chooserUi->buildType->setEnabled(haveCMake);
//  m_chooserUi->generator->setEnabled(haveCMake);
    if(!haveCMake)
    {
        m_chooserUi->status->setText(i18n("You need to select a cmake binary"));
        button(Ok)->setEnabled(false);
        return;
    }

    bool emptyUrl=m_chooserUi->buildFolder->url().isEmpty();
    if( emptyUrl ) st |= BuildFolderEmpty;

    bool dirEmpty = false, dirExists=false;
    QString srcDir;
    if(!emptyUrl)
    {
        QDir d(m_chooserUi->buildFolder->url().toLocalFile());
        dirExists = d.exists();
        dirEmpty=dirExists && d.count()<=2;
        if(!dirEmpty && dirExists)
        {
            bool hasCache=QFile::exists(m_chooserUi->buildFolder->url().toLocalFile()+"/CMakeCache.txt");
            if(hasCache)
            {
                QString srcfold=m_srcFolder.toLocalFile(KUrl::RemoveTrailingSlash);

                srcDir=buildDirProject(m_chooserUi->buildFolder->url());
                if(!srcDir.isEmpty()) {
                    if(QDir(srcDir).canonicalPath()==QDir(srcfold).canonicalPath())
                    {
                        if(m_alreadyUsed.contains(buildFolder().path(KUrl::AddTrailingSlash)))
                            m_chooserUi->status->setText(i18n("Build directory already configured."));
                        else
                            st |= CorrectBuildDir | BuildDirCreated;
                    }
                } else {
                    kWarning(9042) << "maybe you are trying a damaged CMakeCache.txt file. Proper: ";
                }
            }
        }
    }
    else
    {
        m_chooserUi->status->setText(i18n("You need to specify a build directory"));
        button(Ok)->setEnabled(false);
        return;
    }
    
    
    if(st & (BuildDirCreated | CorrectBuildDir))
    {
        m_chooserUi->status->setText(i18n("Using an already created build directory"));
    }
    else
    {
        bool correct=dirEmpty || !dirExists;
        if(correct)
        {
            st |= CorrectBuildDir;
            m_chooserUi->status->setText(QString());
        }
        else
        {
            //Useful to explain what's going wrong
            if ((st & BuildDirCreated) && !(st & CorrectBuildDir))
                m_chooserUi->status->setText(i18n("This build directory is for %1, "
                        "but the project directory is %2", srcDir, m_srcFolder.toLocalFile()));
            else if(st & BuildDirCreated)
                m_chooserUi->status->setText(i18n("The selected build directory is not empty"));
        }

        m_chooserUi->installPrefix->setEnabled(correct);
        m_chooserUi->buildType->setEnabled(correct);
    }
    button(Ok)->setEnabled(st & CorrectBuildDir);
}

void CMakeBuildDirChooser::setCMakeBinary(const KUrl& url) 
{ 
    m_chooserUi->cmakeBin->setUrl(url); 
    update();
}

void CMakeBuildDirChooser::setInstallPrefix(const KUrl& url) 
{ 
    m_chooserUi->installPrefix->setUrl(url); 
    update();
}

void CMakeBuildDirChooser::setBuildFolder(const KUrl& url) 
{ 
    m_chooserUi->buildFolder->setUrl(url); 
    update();
}

void CMakeBuildDirChooser::setBuildType(const QString& s) 
{ 
    m_chooserUi->buildType->addItem(s); 
    update();
}

KUrl CMakeBuildDirChooser::cmakeBinary() const { return m_chooserUi->cmakeBin->url(); }

KUrl CMakeBuildDirChooser::installPrefix() const { return m_chooserUi->installPrefix->url(); }

KUrl CMakeBuildDirChooser::buildFolder() const { return m_chooserUi->buildFolder->url(); }

QString CMakeBuildDirChooser::buildType() const { return m_chooserUi->buildType->currentText(); }

#include "cmakebuilddirchooser.moc"

