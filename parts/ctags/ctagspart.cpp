/***************************************************************************
 *   Copyright (C) 2001 by Bernd Gehrmann                                  *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ctagspart.h"

#include <qapplication.h>
#include <qfileinfo.h>
#include <qpopupmenu.h>
#include <klocale.h>
#include <kgenericfactory.h>
#include <kaction.h>
#include <kdebug.h>
#include <kmainwindow.h>
#include <kmessagebox.h>
#include <kprocess.h>
#include <kregexp.h>

#include "kdevcore.h"
#include "kdevproject.h"
#include "kdevmainwindow.h"
#include "kdevpartcontroller.h"
#include "ctagskinds.h"
#include "ctagsdlg.h"


typedef KGenericFactory<CTagsPart> CTagsFactory;
K_EXPORT_COMPONENT_FACTORY( libkdevctags, CTagsFactory( "kdevctags" ) );

CTagsPart::CTagsPart( QObject *parent, const char *name, const QStringList & )
    : KDevPlugin("CTags", "ctags", parent, name ? name : "CTagsPart")
{
    setInstance(CTagsFactory::instance());
    setXMLFile("kdevctags.rc");

    KAction *action;
    
    action = new KAction( i18n("CTags..."), 0,
                          this, SLOT(slotSearchTags()),
                          actionCollection(), "tools_ctags" );

    connect( core(), SIGNAL(projectClosed()),
             this, SLOT(projectClosed()) );
    connect( core(), SIGNAL(contextMenu(QPopupMenu *, const Context *)),
             this, SLOT(contextMenu(QPopupMenu *, const Context *)) );

    m_tags = 0;
    m_dialog = 0;
}


CTagsPart::~CTagsPart()
{
    delete m_dialog;
    delete m_tags;
}


void CTagsPart::projectClosed()
{
    delete m_dialog;
    delete m_tags;
    m_dialog = 0;
    m_tags = 0;
}


void CTagsPart::contextMenu(QPopupMenu *popup, const Context *context)
{
    if (!context->hasType("editor"))
        return;
    
    const EditorContext *econtext = static_cast<const EditorContext*>(context);
    QString ident = econtext->currentWord();
    if (ident.isEmpty())
        return;
    
    m_contextString = ident;
    popup->insertItem( i18n("Go to ctags declaration: %1").arg(ident),
                       this, SLOT(slotGotoDeclaration()) );
    popup->insertItem( i18n("Go to ctags definition: %1").arg(ident),
                       this, SLOT(slotGotoDefinition()) );
} 


void CTagsPart::gotoTag(const QString &tag, const QString &kindChars)
{
    if (!ensureTagsLoaded())
        return;
    
    QString fileName, pattern;

    CTagsMapIterator result = m_tags->find(tag);
    if (result != m_tags->end()) {
        CTagsTagInfoListConstIterator it;
        for (it = (*result).begin(); it != (*result).end(); ++it)
            if (kindChars.find((*it).kind) != -1) {
                fileName = (*it).fileName;
                pattern = (*it).pattern;
                break;
            }
    }

    if (fileName.isNull()) {
        KMessageBox::sorry(0, i18n("Tag not found"));
        return;
    }
    
    bool ok;
    int lineNum = pattern.toInt(&ok);
    if (!ok) {
        KMessageBox::sorry(0, i18n("Currently, only tags with line numbers (option -n) are supported"));
        return;
    }
    
    partController()->editDocument(fileName, lineNum-1);
}


void CTagsPart::slotGotoDeclaration()
{
    gotoTag(m_contextString, "Lcegmnpsux");
}


void CTagsPart::slotGotoDefinition()
{
    gotoTag(m_contextString, "Sdftv");
}


void CTagsPart::slotSearchTags()
{
    if (!m_dialog) {
        if( ensureTagsLoaded() )
            m_dialog = new CTagsDialog(this);
    }
    
    if (m_dialog)
        m_dialog->show();
}


bool CTagsPart::ensureTagsLoaded()
{
    if (m_tags)
        return true;
    if (!project())
        return false;

    kdDebug(9022) << "create/load tags" << endl;
    
    QFileInfo fi(project()->projectDirectory() + "/tags");
    if (!fi.exists()) {
        int r = KMessageBox::questionYesNo(mainWindow()->main(), i18n("A ctags file for this project does not exist yet. Create it now?"));
        if (r != KMessageBox::Yes)
            return false;
        if (!createTagsFile()) {
            KMessageBox::sorry(mainWindow()->main(), i18n("Could not create tags file"));
            return false;
        }
    }
    
    kdDebug(9022) << "load tags from " << endl;
    return loadTagsFile();
}


bool CTagsPart::loadTagsFile()
{
    kdDebug(9022) << "load tags file" << endl;

    QFile f(project()->projectDirectory() + "/tags");
    if (!f.open(IO_ReadOnly))
        return false;

    if (m_tags)
        m_tags->clear();
    else
        m_tags = new CTagsMap;
    m_kindStrings.clear();
    
    QTextStream stream(&f);
    KRegExp re("^([^\t]*)\t([^\t]*)\t([^;]*);\"\t(.*)$");
        
    QCString line;
    while (!stream.atEnd()) {
        line = stream.readLine();
        //        kdDebug() << "Line: " << line << endl;
        if (!re.match(line))
            continue;

        
        QString tag = re.group(1);
        QString file = re.group(2);
        QString pattern = re.group(3);
        QString extfield = re.group(4);
        //        kdDebug() <<"Tag " << tag << ", file " << file << ", pattern "
        //                  << pattern << ", extfield " << extfield << endl;
        CTagsMapIterator tiit = m_tags->find(tag);
        if (tiit == m_tags->end())
            tiit = m_tags->insert(tag, CTagsTagInfoList());

        CTagsTagInfo ti;
        ti.fileName = re.group(2);
        ti.pattern = re.group(3);
        ti.kind = re.group(4)[0];
        (*tiit).append(ti);
        
        // Put kind in kind list if not already there
        QString extension;
        if (ti.fileName.right(9) == "/Makefile")
            extension = "mak";
        else {
            int pos = ti.fileName.findRev('.');
            if (pos > 0)
                extension = ti.fileName.mid(pos+1);
        }
        if (extension.isNull())
            continue;
        
        QString kindString = CTagsKinds::findKind(ti.kind, extension);
        if (kindString.isNull())
            continue;

        if (!m_kindStrings.contains(kindString))
            m_kindStrings.append(kindString);
    }
    
    f.close();

    return true;

#if 0
    QDictIterator<CTagsTagInfoList> it(tags);
    for (; it.current(); ++it) {
        kdDebug() << "Id: " << it.currentKey() << endl;
        CTagsTagInfoList *l = it.current();
        QValueList<CTagsTagInfo>::ConstIterator it2;
        for (it2 = l->begin(); it2 != l->end(); ++it2)
            kdDebug() << "at " << (*it2).fileName << "," << (*it2).pattern << endl;
    }
#endif
}


bool CTagsPart::createTagsFile()
{
    kdDebug(9022) << "create tags file" << endl;

    QString cmd = "cd ";
    cmd += KShellProcess::quote(project()->projectDirectory());
    cmd += " && ctags -n --c++-types=+px";

    QStringList l = project()->allFiles();
    QStringList::ConstIterator it;
    for (it = l.begin(); it != l.end(); ++it) {
        cmd += " ";
        cmd += KShellProcess::quote(*it);
    }

    KShellProcess proc("/bin/sh");
    proc << cmd;

    QApplication::setOverrideCursor(Qt::waitCursor);
    bool success = proc.start(KProcess::Block);
    QApplication::restoreOverrideCursor();
    
    return success;
}

#include "ctagspart.moc"
