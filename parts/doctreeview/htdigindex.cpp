/***************************************************************************
 *   Copyright (C) 1999-2001 by Matthias Hoelzer-Kluepfel                  *
 *   hoelzer@kde.org                                                       *
 *   Copyright (C) 2001 by Bernd Gehrmann                                  *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "htdigindex.h"

#include <qapplication.h>
#include <qdir.h>
#include <qfile.h>
#include <qlayout.h>
#include <qtextstream.h>
#include <qtimer.h>
#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kprocess.h>
#include <kdeversion.h>

#define INDEXER
#include "misc.cpp"


ProgressDialog::ProgressDialog(QWidget *parent, const char *name)
    : KDialogBase(KDialogBase::Plain, i18n("Generating Search Index"), Cancel, Cancel,
                  parent, name, false)
{
    proc = 0;

    indexdir = kapp->dirs()->saveLocation("data", "kdevdoctreeview/helpindex");
    QDir d; d.mkdir(indexdir);

    QGridLayout *grid = new QGridLayout(plainPage(), 5,3, spacingHint());

    QLabel *l = new QLabel(i18n("Scanning for files"), plainPage());
    grid->addMultiCellWidget(l, 0, 0, 1, 2);

    filesLabel = new QLabel(plainPage());
    grid->addWidget(filesLabel, 1, 2);
    setFilesScanned(0);

    check1 = new QLabel(plainPage());
    grid->addWidget(check1, 0, 0);

    l = new QLabel(i18n("Extracting search terms"), plainPage());
    grid->addMultiCellWidget(l, 2,2, 1,2);

    bar = new KProgress(plainPage());
    grid->addWidget(bar, 3,2);

    check2 = new QLabel(plainPage());
    grid->addWidget(check2, 2,0);

    l = new QLabel(i18n("Generating index..."), plainPage());
    grid->addMultiCellWidget(l, 4,4, 1,2);

    check3 = new QLabel(plainPage());
    grid->addWidget(check3, 4,0);

    setState(0);

    setMinimumWidth(300);
    connect(this, SIGNAL(cancelClicked()), this, SLOT(cancelClicked()));
    QTimer::singleShot(0, this, SLOT(slotDelayedStart()));
}


ProgressDialog::~ProgressDialog()
{}

void ProgressDialog::slotDelayedStart()
{
    procdone = false;
    scanDirectories();
    if (!createConfig())
    {
      done(1);
      return;
    }
    generateIndex();
}


void ProgressDialog::done(int r)
{
    if (!r && proc)
        proc->kill();
    KDialogBase::done(r);
}


void ProgressDialog::setFilesScanned(int n)
{
    filesLabel->setText(i18n("Files processed: %1").arg(n));
}


void ProgressDialog::setFilesToDig(int n)
{
    bar->setRange(0, n);
}


void ProgressDialog::setFilesDigged(int n)
{
    bar->setValue(n);
}


void ProgressDialog::setState(int n)
{
    QPixmap unchecked = QPixmap(locate("data", "kdevdoctreeview/pics/unchecked.xpm"));
    QPixmap checked = QPixmap(locate("data", "kdevdoctreeview/pics/checked.xpm"));

    check1->setPixmap( n > 0 ? checked : unchecked);
    check2->setPixmap( n > 1 ? checked : unchecked);
    check3->setPixmap( n > 2 ? checked : unchecked);
}


void ProgressDialog::addDir(const QString &dir)
{
    kdDebug(9002) << "Add dir : " << dir << endl;
    QDir d(dir, "*.html", QDir::Name|QDir::IgnoreCase, QDir::Files | QDir::Readable);
    QStringList list = d.entryList();

    QStringList::ConstIterator it;
    for ( it=list.begin(); it!=list.end(); ++it ) {
        files.append(dir + "/" + *it);
        setFilesScanned(++filesScanned);
    }

    QDir d2(dir, QString::null, QDir::Name|QDir::IgnoreCase, QDir::Dirs);
    QStringList dlist = d2.entryList();

    for ( it=dlist.begin(); it != dlist.end(); ++it ) {
        if (*it != "." && *it != "..") {
            addDir(dir + "/" + *it);
            kapp->processEvents();
        }
        if (procdone)
        {
          return;
        }
    }
    kapp->processEvents();
}


void ProgressDialog::addKdocDir(FILE *f)
{
    char buf[1024];
    while (fgets(buf, sizeof buf, f)) {
        QString s = buf;
        if (s.left(11) == "<BASE URL=\"") {
            int pos2 = s.find("\">", 11);
            if (pos2 != -1) {
                addDir(s.mid(11, pos2-11));
                return;
            }
        }
    }
}


void ProgressDialog::addTocFile(QDomDocument &doc)
{
    QStringList candidates;
    QString base;
    QDomElement childEl = doc.documentElement().firstChild().toElement();
    while (!childEl.isNull()) {
        if (childEl.tagName() == "tocsect1") {
            QString url = childEl.attribute("url");
            if (!url.isEmpty()) {
                url.prepend(base);
                kdDebug(9002) << "candidate: " << url << endl;
                candidates.append(url);
            }
            /// @todo Generalize to arbitrary number of levels
            QDomElement grandchildEl = childEl.firstChild().toElement();
            while (!grandchildEl.isNull()) {
                if (grandchildEl.tagName() == "tocsect2") {
                    QString url = grandchildEl.attribute("url");
                    if (!url.isEmpty()) {
                        url.prepend(base);
                        kdDebug(9002) << "candidate: " << url << endl;
                        candidates.append(url);
                    }
                }
                grandchildEl = grandchildEl.nextSibling().toElement();
            }
        } else if (childEl.tagName() == "base") {
            base = childEl.attribute("href");
            if (!base.isEmpty())
                base += "/";
        }
        childEl = childEl.nextSibling().toElement();
    }

    QStringList::ConstIterator it;
    for (it = candidates.begin(); it != candidates.end(); ++it) {
        QString url = *it;
        int pos = url.findRev('#');
        if (pos != -1)
            url.truncate(pos);
        if ((url.startsWith("/") || url.startsWith("file://"))
            && !files.contains(url)) {
            files.append(url);
            kdDebug(9002) << "tocurl: " << url << endl;
            setFilesScanned(++filesScanned);
        }
    }
}


void ProgressDialog::scanDirectories()
{
    KConfig config("kdevdoctreeviewrc", true);
    config.setGroup("Index");
    bool indexKDevelop = config.readBoolEntry("IndexKDevelop");
    bool indexQt = config.readBoolEntry("IndexQt");
    bool indexKdelibs = config.readBoolEntry("IndexKdelibs");
    bool indexBooks = config.readBoolEntry("IndexBooks");
    bool indexBookmarks = config.readBoolEntry("IndexBookmarks");

    bool indexShownLibs = true;
    bool indexHiddenLibs = true;

    filesScanned = 0;

    QStringList itemNames, fileNames, hiddenNames;
    DocTreeViewTool::getAllLibraries(&itemNames, &fileNames);
    DocTreeViewTool::getHiddenLibraries(&hiddenNames);

    QStringList::ConstIterator it1, it2;
    for (it1 = itemNames.begin(), it2 = fileNames.begin();
         it1 != itemNames.end() && it2 != fileNames.end();
         ++it1, ++it2) {
        bool ishidden = hiddenNames.contains(*it2);
        if ( (indexHiddenLibs && ishidden) || (indexShownLibs && !ishidden) ) {
            FILE *f;
            if ((*it2).right(3) != QString::fromLatin1(".gz")) {
                if ( (f = fopen(QFile::encodeName( *it2 ).data(), "r")) != 0) {
                    addKdocDir(f);
                    fclose(f);
                }
            } else {
                QString cmd = "gzip -c -d ";
#if (KDE_VERSION > 305)
                cmd += KProcess::quote(*it2);
#else
                cmd += KShellProcess::quote(*it2);
#endif
                cmd += " 2>/dev/null";
                if ( (f = popen(QFile::encodeName(cmd), "r")) != 0) {
                    addKdocDir(f);
                    pclose(f);
                }
            }
        }
    }

    if (indexKDevelop) {
        /// @todo Problem: they are in index.cache.bz2 :-(
    }

    if (indexQt) {
      QString oldqtdocdir;
      config.setGroup("General Qt");
      QMap<QString, QString> emap = config.entryMap("General Qt");
      QMap<QString, QString>::Iterator it;
      for (it = emap.begin(); it != emap.end(); ++it)
      {
          QString qtdocdir = config.readPathEntry(it.key());
          if (!qtdocdir.isEmpty())
    {
      qtdocdir = qtdocdir.left(qtdocdir.findRev("/") + 1);
      if (qtdocdir != oldqtdocdir)
      {
                  addDir(qtdocdir);
        oldqtdocdir = qtdocdir;
      }
    }
      }
    }

    if (indexKdelibs) {
      config.setGroup("General Doxygen");
      QMap<QString, QString> xmap = config.entryMap("General Doxygen");
      QMap<QString, QString>::Iterator itx;
      for (itx = xmap.begin(); itx != xmap.end(); ++itx)
      {
          QString kdelibsdocdir =  config.readPathEntry(itx.key());
          if (!kdelibsdocdir.isEmpty())
                addDir(kdelibsdocdir);
      }
    }

    if (indexBooks) {
        KStandardDirs *dirs = KGlobal::dirs();
        QStringList tocs = dirs->findAllResources("doctocs", QString::null, false, true);

        QStringList::ConstIterator it4;
        for (it4 = tocs.begin(); it4 != tocs.end(); ++it4) {
            QFile f(*it4);
            if (!f.open(IO_ReadOnly)) {
                kdDebug(9002) << "Could not read doc toc: " << (*it4) << endl;
                continue;
            }
            QDomDocument doc;
            if (!doc.setContent(&f) || doc.doctype().name() != "kdeveloptoc") {
                kdDebug(9002) << "Not a valid kdeveloptoc file: " << (*it4) << endl;
                continue;
            }
            f.close();
            addTocFile(doc);
        }
    }

    if (indexBookmarks) {
        QStringList bookmarksTitle, bookmarksURL;
        DocTreeViewTool::getBookmarks(&bookmarksTitle, &bookmarksURL);
        QStringList::ConstIterator it3;
        for (it3 = bookmarksURL.begin(); it3 != bookmarksURL.end(); ++it3) {
            /// \FIXME Perhaps one should consider indexing the whole directory the file
            // lives in
            files.append(*it3);
            setFilesScanned(++filesScanned);
        }
    }
}


bool ProgressDialog::createConfig()
{
    // locate the common dir
    QString language = KGlobal::locale()->language();
    if (language == "C")
        language = "en";

    QString wrapper = locate("data", QString("kdevdoctreeview/%1/wrapper.html").arg(language));
    if (wrapper.isEmpty())
        wrapper = locate("data", QString("kdevdoctreeview/en/wrapper.html"));
    if (wrapper.isEmpty())
        return false;
    wrapper = wrapper.left(wrapper.length()-12);

    // locate the image dir
    QString images = locate("data", "kdevdoctreeview/pics/star.png");
    if (images.isEmpty())
        return false;
    images = images.left(images.length()-8);

    QFile f(indexdir + "/htdig.conf");
    if (f.open(IO_WriteOnly)) {
        QTextStream ts(&f);

        ts << "database_dir:\t\t" << indexdir << endl;
        ts << "start_url:\t\t`" << indexdir << "/files`" << endl;
        ts << "local_urls:\t\thttp://localhost/=/" << endl;
//        ts << "local_urls:\t\tfile://=" << endl;
        ts << "local_urls_only:\ttrue" << endl;
  ts << "limit_urls_to:\t\tfile://" << endl;
        ts << "maximum_pages:\t\t1" << endl;
        ts << "image_url_prefix:\t" << images << endl;
        ts << "star_image:\t\t" << images << "star.png" << endl;
        ts << "star_blank:\t\t" << images << "star_blank.png" << endl;
        ts << "compression_level:\t6" << endl;
        ts << "max_hop_count:\t\t0" << endl;

        ts << "search_results_wrapper:\t" << wrapper << "wrapper.html" << endl;
        ts << "nothing_found_file:\t" << wrapper << "nomatch.html" << endl;
        ts << "syntax_error_file:\t" << wrapper << "syntax.html" << endl;
        ts << "bad_word_list:\t\t" << wrapper << "bad_words" << endl;

        f.close();
        return true;
    }

    return false;
}


#define CHUNK_SIZE 100

void ProgressDialog::startHtdigProcess(bool initial)
{
  kdDebug(9002) << "htdig started" << endl;
  delete proc;
  proc = new KProcess();
  *proc << exe << "-c" << (indexdir + "/htdig.conf");
  if (initial) {
      *proc << "-i";
  }
  connect(proc, SIGNAL(processExited(KProcess *)),
          this, SLOT(htdigExited(KProcess *)));

  htdigRunning = true;

  // write out file
  QFile f(indexdir+"/files");
  if (!f.open(IO_WriteOnly)) {
      kdDebug(9002) << "Could not open `files` for writing" << endl;
      done(1);
      return;
  }
  QTextStream ts(&f);
  for (int i=0; i<CHUNK_SIZE; ++i, ++count) {
      if (count >= filesToDig) {
          procdone = true;
          break;
      }
//    ts << "file://localhost/" + files[count] << endl;
      ts << "http://localhost/" + files[count] << endl;
  }
  f.close();

  // execute htdig
  proc->start(KProcess::NotifyOnExit, KProcess::Stdout);

}

bool ProgressDialog::generateIndex()
{
    setState(1);
    procdone = false;
    // run htdig
    KConfig config("kdevdoctreeviewrc", true);
    config.setGroup("htdig");
    exe = config.readPathEntry("htdigbin", kapp->dirs()->findExe("htdig"));
    if (exe.isEmpty())
    {
        done(1);
        return true;
    }
    filesToDig = files.count();
    count = 0;
    setFilesToDig(filesToDig);
    filesDigged = 0;

    //    QDir d; d.mkdir(indexdir);
    startHtdigProcess(true);
    return true;
}



void ProgressDialog::htdigStdout(KProcess *, char *buffer, int len)
{
    QString line = QString(buffer).left(len);

    int cnt=0, index=-1;
    while ( (index = line.find("http://", index+1)) > 0)
        cnt++;
    filesDigged += cnt;

    cnt=0, index=-1;
    while ( (index = line.find("not changed", index+1)) > 0)
        cnt++;
    filesDigged -= cnt;

    setFilesDigged(filesDigged);
}


void ProgressDialog::htdigExited(KProcess *proc)
{
    kdDebug(9002) << "htdig terminated" << endl;
    if (!proc->normalExit())
    {
      delete proc;
      proc = 0L;
      done(1);
      return;
    }
    if (proc && proc->exitStatus() != 0) {
                KMessageBox::sorry(0, i18n("Running htdig failed"));
                delete proc;
                proc = 0L;
                done(1);
                return;
            }
    htdigRunning = false;
    filesDigged += CHUNK_SIZE;
    setFilesDigged(filesDigged);
    if (!procdone)
    {
        startHtdigProcess(false);
    } else
    {
      setFilesDigged(filesToDig);
      setState(2);

      KConfig config("kdevdoctreeviewrc", true);
      config.setGroup("htdig");
      // run htmerge -----------------------------------------------------
      exe = config.readPathEntry("htmergebin", kapp->dirs()->findExe("htmerge"));
      if (exe.isEmpty())
      {
          done(1);
          return;
      }
      startHtmergeProcess();
    }
}

void ProgressDialog::startHtmergeProcess()
{
    kdDebug(9002) << "htmerge started" << endl;
    delete proc;
    proc = new KProcess();
    *proc << exe << "-c" << (indexdir + "/htdig.conf");

    kdDebug(9002) << "Running htmerge" << endl;

    connect(proc, SIGNAL(processExited(KProcess *)),
            this, SLOT(htmergeExited(KProcess *)));

    htmergeRunning = true;

    proc->start(KProcess::NotifyOnExit, KProcess::Stdout);
}

void ProgressDialog::htmergeExited(KProcess *proc)
{
    kdDebug(9002) << "htmerge terminated" << endl;
    htmergeRunning = false;
    if (!proc->normalExit())
    {
      delete proc;
      proc = 0L;
      done(1);
      return;
    }
    if (proc && proc->exitStatus() != 0) {
        KMessageBox::sorry(0, i18n("Running htmerge failed"));
        delete proc;
        proc = 0L;
        done(1);
        return;
    }
    setState(3);
    done(0);
}

void ProgressDialog::cancelClicked()
{
  if ((htdigRunning || htmergeRunning) && proc && proc->isRunning())
  {
    kdDebug(9002) << "Killing " << (htdigRunning ? "htdig" : "htmerge") << "daemon with Sig. 9" << endl;
    proc->kill(9);
    htdigRunning = htmergeRunning = false;
  } else
  {
    procdone = true;
    done(2);
  }
}

int main(int argc, char *argv[])
{
#if 0
    static KCmdLineOptions options[] = {
        { "+dirs", I18N_NOOP("The directories to index."), 0 },
        KCmdLineLastOption
    };
#endif

    KAboutData aboutData("kdevdoctreeview", I18N_NOOP("KDevelop"),
                         "0.1", I18N_NOOP("KDE Index generator for help files."));

    KCmdLineArgs::init(argc, argv, &aboutData);
    //    KCmdLineArgs::addCmdLineOptions(options);

    KApplication app;

    KGlobal::dirs()->addResourceType("doctocs", KStandardDirs::kde_default("data") + "kdevdoctreeview/tocs/");
    KGlobal::locale()->setMainCatalogue("kdevelop");

    ProgressDialog *search = new ProgressDialog(0, "progress dialog");
    app.setMainWidget(search);
    search->show();
    app.exec();

    return 0;
}

#include "htdigindex.moc"
