/***************************************************************************
  docviewman.cpp - MDI manager for
                   document classes of KDevelop (KWriteDocs, CDocBrowser)
                   and view classes of KDevelop (CEditWidget, KHTMLView)
                             -------------------

    begin                : 03 Mar 2001
    copyright            : (C) 2001 by Falk Brettschneider
    email                : falk.brettschneider@gmx.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qfile.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qlayout.h>
#include <qobjectlist.h>
#include <qprogressbar.h>
#include <qwhatsthis.h>

#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>

#include "ckdevelop.h"
#include "kwdoc.h"
#include "cdocbrowser.h"
#include "doctreeview.h"
#include "khtmlview.h"
#include "ceditwidget.h"
#include "docviewman.h"
#include "./dbg/brkptmanager.h"
#include "./dbg/vartree.h"
#include "./ctags/ctagsdialog_impl.h"
#include "ckdevaccel.h"

//==============================================================================
// class implementation
//------------------------------------------------------------------------------
DocViewMan::DocViewMan( CKDevelop* parent)
: QObject( parent)
  ,m_pParent(parent)
  ,m_pDocBookmarksMenu(0L)
  ,m_pCurEditDoc(0L)
  ,m_pCurEditView(0L)
  ,m_pCurBrowserDoc(0L)
  ,m_pCurBrowserView(0L)
  ,m_curIsBrowser(true)
{
  debug("create docviewman !\n");

  m_MDICoverList.setAutoDelete(true);

  m_docBookmarksList.setAutoDelete(TRUE);
  m_docBookmarksTitleList.setAutoDelete(TRUE);
	
  connect( this, SIGNAL(sig_viewGotFocus(QWidget*)), m_pParent, SLOT(slotViewSelected(QWidget*)) );
}

//------------------------------------------------------------------------------
DocViewMan::~DocViewMan()
{
  debug("delete docviewman !\n");
}

void DocViewMan::doSelectURL(const QString& url)
{
  debug("DocViewMan::doSelectURL !\n");

  // use latest focused browser document or create new browser document and view
  CDocBrowser* pCurBrowserDoc = currentBrowserDoc();
  KHTMLView* pBrowserView = currentBrowserView();
  if (pCurBrowserDoc) {
    if(url.contains("kdevelop/search_result.html") != 0){
      pCurBrowserDoc->showURL(url, true); // with reload if equal
    }
    else{
      pCurBrowserDoc->showURL(url); // without reload if equal
    }
  }
  else {
    CDocBrowser* pDoc = createCDocBrowser(url);
    if (pDoc == 0) return; // failed
    pBrowserView = createBrowserView(pDoc, true);
  }

  pBrowserView->parentWidget()->setFocus();
}

void DocViewMan::doSwitchToFile(QString filename, int line, int col, bool bForceReload, bool bShowModifiedBox)
{
  debug("DocViewMan::doSwitchToFile : '%s' !\n", filename.data());

  debug("show modified box : %d !\n", bShowModifiedBox);

  CEditWidget* pCurEditWidget = currentEditView();
  KWriteDoc* pCurEditDoc = currentEditDoc();

  QString editWidgetName;
  if (pCurEditWidget) {
    debug("getting edit widget name !");
    editWidgetName = pCurEditWidget->getName();
    debug("editWidgetName : '%s' !", editWidgetName.data());
  }
  else {
    debug("editWidgetName : '' !");
  }

  debug("looking if file already open !\n");

  // Make sure that we found the file in the editor_widget in our list
  if (pCurEditDoc) {
    // handle file if it was modified on disk by another editor/cvs
    QFileInfo file_info(editWidgetName);
    if ((file_info.lastModified() != pCurEditDoc->getLastFileModifDate()) && bShowModifiedBox) {
      debug(" KMessageBox !\n");
      if(KMessageBox::questionYesNo(m_pParent,
                                    i18n("The file %1 was modified outside this editor.\n"
                                         "Open the file from disk and delete the current Buffer?")
                                    .arg(editWidgetName),
                                    i18n("File modified"))==KMessageBox::Yes) {
        bForceReload = true;
        pCurEditDoc->setLastFileModifDate(file_info.lastModified());
      }
    }

    debug(" getting lastModified !\n");

    if (!bShowModifiedBox) {
      pCurEditDoc->setLastFileModifDate(file_info.lastModified());
    }

    debug(" before setCursorPosition !\n");

    if (!bForceReload && filename == editWidgetName) {
      if (pCurEditWidget && (line != -1))
        pCurEditWidget->setCursorPosition(line, col);

      //    cerr << endl <<endl << "Filename:" << filename
      // << "EDITNAME:" << pCurEditWidget->getName() <<"no action---:" << endl;
      QextMdiChildView* pMDICover = (QextMdiChildView*) pCurEditWidget->parentWidget();
      pMDICover->activate();
      return;
    }
  }

  // See if we already have the file wanted.
  KWriteDoc* pDoc = findKWriteDoc(filename);

  // bool found = (pDoc != 0);

  debug("getting document type !\n");

  // Not found or needing a reload causes the file to be read from disk
  if ((!pDoc) || bForceReload) {
    QFileInfo fileinfo(filename);
    if (!pDoc) {
      debug("Create a new doc !\n");
      pDoc = createKWriteDoc(filename);
      if (pDoc) {
        // Set the last modify date
        pDoc->setLastFileModifDate(fileinfo.lastModified());

        qDebug("createView for a new created doc");
        pCurEditWidget = createEditView(pDoc, true);
      }
    }
    else {
      // a view for this doc exists, already;
      // use the first view we found of this doc to show the text
      pCurEditWidget = getFirstEditView(pDoc);
      qDebug("found view in list of doc");
    }
    loadKWriteDoc(pDoc , filename, 1);

    qDebug("and loadDoc");
  }
  else {
    debug(" document type found !\n");

    KWriteDoc* pDoc = findKWriteDoc(filename);
    pCurEditWidget = getFirstEditView(pDoc);

    debug(" focus view !\n");

    // Don't use the saved text because it is useless
    // and removes the bookmarks
    // pCurEditWidget->setText(info->text);

    qDebug("doc (and at least 1 view) did exist, raise it");
  }

  if (!pCurEditWidget)
    return;

  // debug(" toggle modify cur edit widget !\n");
  // pCurEditWidget->toggleModified(info->modified);

  // If the caller wanted to be positioned at a particular place in the file
  // then they have supplied the line and col. Otherwise we use the
  // current info values (0 if new) for the placement.
  if (line != -1)
    pCurEditWidget->setCursorPosition(line, col);
  // else
  //  pCurEditWidget->setCursorPosition(info->cursor_line,info->cursor_col);

  pCurEditWidget->setName(filename);
  // info->text = pCurEditWidget->text();

  debug(" set focus on view (this will raise and activate the MDI view!\n");
  QextMdiChildView* pMDICover = (QextMdiChildView*) pCurEditWidget->parentWidget();
  pMDICover->activate();
}


void DocViewMan::doOptionsEditor()
{
  debug("DocViewMan::doOptionsEditor !\n");

  if(currentEditView())
  {
    currentEditView()->optDlg();
    doTakeOverOfEditorOptions();
  }
}

void DocViewMan::doOptionsEditorColors()
{
  debug("DocViewMan::doOptionsEditorColors !\n");

  if(currentEditView())
  {
    currentEditView()->colDlg();
    doTakeOverOfEditorOptions();
  }
}


void DocViewMan::doOptionsSyntaxHighlightingDefaults()
{
  debug("DocViewMan::doOptionsSyntaxHighlightingDefaults !\n");

  if(currentEditView())
  {
    currentEditView()->hlDef();
    doTakeOverOfEditorOptions();
  }
}

void DocViewMan::doOptionsSyntaxHighlighting()
{
  debug("DocViewMan::doOptionsSyntaxHighlighting !\n");

  if(currentEditView())
  {
    currentEditView()->hlDlg();
    doTakeOverOfEditorOptions();
  }
}

/** shared helper function for the 4 slots 
  * doOptionsEditor, doOptionsEditorColors,
  * doOptionsSyntaxHighlightingDefaults and doOptionsSyntaxHighlighting
  */
void DocViewMan::doTakeOverOfEditorOptions()
{
  debug("DocViewMan::doTakeOverOfEditorOptions !\n");

  KConfig* config = m_pParent->getConfig();
  if (config) {
    config->setGroup("KWrite Options");
    currentEditView()->writeConfig(config);
    currentEditView()->doc()->writeConfig(config);

    QListIterator<QObject> itDoc(m_documentList);
    for (itDoc.toFirst(); itDoc.current() != 0; ++itDoc) {
      KWriteDoc* pDoc = dynamic_cast<KWriteDoc*> (itDoc.current());
      if (pDoc) {
        CEditWidget* pCurEW = getFirstEditView(pDoc);
        pCurEW->copySettings(currentEditView());
        config->setGroup("KWrite Options");
        pCurEW->readConfig(config);
        pDoc->readConfig(config);
      }
    }
  }
}

/** */
void DocViewMan::doCopy()
{
  debug("DocViewMan::doCopy !\n");

  if (currentEditView()) {
    if (curDocIsBrowser())
      currentBrowserDoc()->slotCopyText();
    else
      currentEditView()->copyText();
  }
}

/** */
void DocViewMan::doSearch()
{
  debug("DocViewMan::doSearch !\n");

  if (curDocIsBrowser()) {
    if (currentBrowserDoc()) {
      currentBrowserDoc()->doSearchDialog();
    }
  }
  else {
    if (currentEditView()) {
      currentEditView()->search();
    }
  }
}

/** */
void DocViewMan::doRepeatSearch(QString &search_text, int back)
{
  debug("DocViewMan::doRepeatSearch !\n");

  if (currentEditView()) {
    if (curDocIsBrowser())
      currentBrowserDoc()->findTextNext(QRegExp(search_text),true);
    else
      currentEditView()->searchAgain(back==1);
  }
}

/** */
void DocViewMan::doSearchText(QString &text)
{
  debug("DocViewMan::doSearchText !\n");

  if (currentEditView()) {
    if (curDocIsBrowser())
      text = currentBrowserDoc()->selectedText();
    else {
      text = currentEditView()->markedText();
      if(text == "") {
        text = currentEditView()->currentWord();
      }
    }
  }
}

/** */
void DocViewMan::doClearBookmarks()
{
  debug("DocViewMan::doClearBookmarks !\n");

  QListIterator<QObject> itDoc(m_documentList);
  for (itDoc.toFirst(); itDoc.current() != 0; ++itDoc) {
    KWriteDoc* pDoc = dynamic_cast<KWriteDoc*> (itDoc.current());
    if (pDoc) {
      pDoc->clearBookmarks();
    }
  }
}

void DocViewMan::doCreateNewView()
{
  debug("DocViewMan::doCreateNewView !\n");

  QWidget* pNewView = 0L;

  if(curDocIsBrowser()) {
    CDocBrowser* pBrowserDoc = createCDocBrowser(DocTreeKDevelopBook::locatehtml("about/intro.html"));
    pNewView = createBrowserView(pBrowserDoc, true);
  }
  else {
    pNewView = createEditView(currentEditDoc(), true);
  }

  // raise and activate
  if (pNewView)
    pNewView->parentWidget()->setFocus();
}

/** */
bool DocViewMan::curDocIsHeaderFile()
{
  debug("DocViewMan::curDocIsHeaderFile !\n");

  return (!curDocIsBrowser()
    && m_pCurEditDoc
    && (getKWriteDocType(m_pCurEditDoc) == CPP_HEADER));
}

/** */
bool DocViewMan::curDocIsCppFile()
{
  debug("DocViewMan::curDocIsCppFile !\n");

  return (!curDocIsBrowser() 
	  && m_pCurEditDoc
	  && (getKWriteDocType(m_pCurEditDoc) == CPP_SOURCE));
}


ProjectFileType DocViewMan::getKWriteDocType(KWriteDoc* pDoc)
{
  debug("DocViewMan::getKWriteDocType !\n");

  return CProject::getType(pDoc->fileName());
}

KWriteDoc* DocViewMan::createKWriteDoc(const QString& strFileName)
{
  debug("creating KWriteDoc ");

  KWriteDoc* pDoc = new KWriteDoc(&m_highlightManager, strFileName);
  if (!pDoc)
    return 0L;

	// Check if we must set the last modif date
  QFileInfo file_info(strFileName);
	if(file_info.exists()) {
		pDoc->setLastFileModifDate(file_info.lastModified());
	}
    
  // Add the new doc to the list
  m_documentList.append(pDoc);

  KConfig* config = m_pParent->getConfig();
  if(config) {
    config->setGroup("KWrite Options");
    pDoc->readConfig(config);
  }

  pDoc->setFileName(strFileName);

  // Return the new document
  return pDoc; 
}

CDocBrowser* DocViewMan::createCDocBrowser(const QString& url)
{
  debug("DocViewMan::createCDocBrowser !\n");

  CDocBrowser* pDocBr = new CDocBrowser(0L, "browser");

  if(pDocBr) {
    // some signal-slot connections
    connect(pDocBr, SIGNAL(completed()),m_pParent, SLOT(slotDocumentDone()));
    connect(pDocBr, SIGNAL(signalURLBack()),m_pParent,SLOT(slotHelpBack()));
    connect(pDocBr, SIGNAL(signalURLForward()),m_pParent,SLOT(slotHelpForward()));
    connect(pDocBr, SIGNAL(signalBookmarkToggle()),m_pParent,SLOT(slotBookmarksToggle()));
    connect(pDocBr, SIGNAL(onURL(const QString&)),m_pParent,SLOT(slotURLonURL(const QString&)));
    connect(pDocBr, SIGNAL(signalSearchText()),m_pParent,SLOT(slotHelpSearchText()));
    //  connect(pDocBr, SIGNAL(goRight()), m_pParent, SLOT(slotHelpForward()));
    //  connect(pDocBr, SIGNAL(goLeft()), m_pParent, SLOT(slotHelpBack()));
    connect(pDocBr, SIGNAL(enableStop(int)), m_pParent, SLOT(enableCommand(int)));	
    connect(pDocBr->popup(), SIGNAL(highlighted(int)), m_pParent, SLOT(statusCallback(int)));
    connect(pDocBr, SIGNAL(signalGrepText(QString)), m_pParent, SLOT(slotEditSearchInFiles(QString)));
    //  connect(pDocBr, SIGNAL(textSelected(KHTMLPart *, bool)),m_pParent,SLOT(slotBROWSERMarkStatus(KHTMLView *, bool)));
    
    // init browser and assign URL
    pDocBr->setDocBrowserOptions();
    pDocBr->showURL(url, true); // with reload if equal

    // Add the new doc to the list
    m_documentList.append(pDocBr);
  }

  debug("End DocViewMan::createCDocBrowser !\n");

  // Return the new document
  return pDocBr; 
}

//-----------------------------------------------------------------------------
// load edit document from file
//-----------------------------------------------------------------------------
void DocViewMan::loadKWriteDoc(KWriteDoc* pDoc, 
                               const QString& strFileName, 
                               int /*mode*/)
{
  debug("DocViewMan::loadKWriteDoc !\n");

  if(QFile::exists(strFileName)) {
    QFile f(strFileName);
    if (f.open(IO_ReadOnly)) {
      pDoc->loadFile(f);
      f.close();
    }
  }
}

//-----------------------------------------------------------------------------
// save document to file
//-----------------------------------------------------------------------------
bool DocViewMan::saveKWriteDoc(KWriteDoc* pDoc, const QString& strFileName)
{
  debug("DocViewMan::saveKWriteDoc !\n");

  QFileInfo info(strFileName);
  if(info.exists() && !info.isWritable()) {
    KMessageBox::sorry(0L, i18n("You do not have write permission to this file:\n" + strFileName));
    return false;
  }
  
  QFile f(strFileName);
  if (f.open(IO_WriteOnly | IO_Truncate)) {
    pDoc->writeFile(f);
    pDoc->updateViews();
    f.close();
    return true;//kWriteDoc->setFileName(name);
  }
  KMessageBox::sorry(0L,  i18n("An Error occured while trying to save this Document"));
  return false;
}

//-----------------------------------------------------------------------------
// Find if there is another KWriteDoc in the doc list 
//-----------------------------------------------------------------------------
KWriteDoc* DocViewMan::findKWriteDoc()
{
  debug("DocViewMan::findKWriteDoc !\n");

  QListIterator<QObject> itDoc(m_documentList);
  for (itDoc.toFirst(); itDoc.current() != 0; ++itDoc) {
    KWriteDoc* pDoc = dynamic_cast<KWriteDoc*> (itDoc.current());
    if(pDoc) {
      return pDoc;
    }
  }
  return 0L;
}

//-----------------------------------------------------------------------------
void DocViewMan::slotRemoveFileFromEditlist(const QString &absFilename)
{
  KWriteDoc* pDoc = findKWriteDoc( absFilename);
  closeKWriteDoc( pDoc);
  m_pParent->setMainCaption();
}


//-----------------------------------------------------------------------------
// close an edit document, causes all views to be closed
//-----------------------------------------------------------------------------
void DocViewMan::closeKWriteDoc(KWriteDoc* pDoc)
{
  debug("DocViewMan::closeKWriteDoc !\n");
  if (!pDoc) return;

  QList<KWriteView> views = pDoc->viewList();
  QListIterator<KWriteView>  itViews(views);
  for (; itViews.current() != 0; ++itViews) {
    CEditWidget* pView = (CEditWidget*) itViews.current()->parentWidget();
    if (!pView) continue;
    disconnect(pView, SIGNAL(gotFocus(CEditWidget*)),
               this, SLOT(slot_gotFocus(CEditWidget*)));
    // remove the view from MDI and delete the view
    QextMdiChildView* pMDICover = (QextMdiChildView*) pView->parentWidget();
    m_pParent->removeWindowFromMdi( pMDICover);
    m_MDICoverList.remove( pMDICover);
  }

  if(pDoc) {
    // Remove the document from the list
    m_documentList.removeRef(pDoc);
    // now finally, delete the document
    delete pDoc;
  }

  // check if there's still a m_pCurBrowserDoc, m_pCurBrowserView, 
  // m_pCurEditDoc, m_pCurEditView
  KWriteDoc* pNewDoc = findKWriteDoc();

  if (pNewDoc == 0) {
    m_pCurEditDoc = 0L;
    m_pCurEditView = 0L;
  }

  //   emit an according signal if we closed the last doc
  if (m_documentList.count() == 0) {
    emit sig_lastViewClosed();
    emit sig_lastDocClosed();
  }
}

//-----------------------------------------------------------------------------
// Find if there is another CDocBrowser in the doc list 
//-----------------------------------------------------------------------------
CDocBrowser* DocViewMan::findCDocBrowser()
{
  debug("DocViewMan::findCDocBrowser !\n");

  QListIterator<QObject> itDoc(m_documentList);
  for (itDoc.toFirst(); itDoc.current() != 0; ++itDoc) {
    CDocBrowser* pDoc = dynamic_cast<CDocBrowser*> (itDoc.current());
    if(pDoc) {
      return pDoc;
    }
  }
  return 0L;
}

//-----------------------------------------------------------------------------
// close a browser document, causes all views to be closed
//-----------------------------------------------------------------------------
void DocViewMan::closeCDocBrowser(CDocBrowser* pDoc)
{
  debug("DocViewMan::closeCDocBrowser : %d !\n", (int)pDoc);

  if(pDoc) {
    debug("getting pView !\n");
    KHTMLView* pView = pDoc->view();
    debug("pView : %d !\n", (int)pView);
    if(pView) {
      debug("getting pMDICover !\n");
      QextMdiChildView* pMDICover = (QextMdiChildView*) pView->parentWidget();
      if(pMDICover) {
        debug("pMDICover : %d !\n", (int)pMDICover);
        debug("hiding pMDICover !\n");
        pMDICover->hide();
        debug("reparent pView !\n");
        pView->reparent(0L, 0, QPoint(0,0));
        QApplication::sendPostedEvents();
        m_pParent->removeWindowFromMdi( pMDICover);
        m_MDICoverList.remove( pMDICover);
      }
    }
    debug("deleting pDoc !\n");
    // Remove the document from the list
    m_documentList.removeRef(pDoc);
    // now finally, delete the document (which inclusively deletes the view)
    delete pDoc;
  }

  debug("finding new doc !\n");
  CDocBrowser* pNewDoc = findCDocBrowser();
  if (pNewDoc == 0) {
    m_pCurBrowserDoc = 0L;
    m_pCurBrowserView = 0L;
  }
  
  //   emit an according signal if we closed the last doc
  debug("counting documents !\n");
  if (m_documentList.count() == 0) {
    emit sig_lastViewClosed();
    emit sig_lastDocClosed();
  }
}

//-----------------------------------------------------------------------------
// retrieve the document pointer
//-----------------------------------------------------------------------------
/*
QObject* DocViewMan::docPointer(int docId) const
{
  // find document info
  QListIterator<DocViewNode>  itDoc(m_docsAndViews);
  for (; (itDoc.current() != 0) && (itDoc.current()->docId != docId); ++itDoc) {}

  if (itDoc.current() != 0) {
    return itDoc.current()->pDoc;
  }
  else {
    return 0;
  }
}
*/
//-----------------------------------------------------------------------------
// retrieve the document pointer and make sure it is a KWriteDoc
//-----------------------------------------------------------------------------
/*
KWriteDoc* DocViewMan::kwDocPointer(int docId) const
{
  return (dynamic_cast<KWriteDoc*> (docPointer(docId)));
}
*/

//-----------------------------------------------------------------------------
// cover a newly created view with a QextMDI childview 
//-----------------------------------------------------------------------------
void DocViewMan::addQExtMDIFrame(QWidget* pNewView, bool bShow, const QPixmap& icon)
{
  debug("DocViewMan::addQExtMDIFrame !\n");

  if (!pNewView) return;  // failed, could not create view

  // cover it by a QextMDI childview and add that MDI system
  QextMdiChildView* pMDICover = new QextMdiChildView( pNewView->caption());
  pMDICover->setIcon(icon);
//  pMDICover->setIcon(kapp->miniIcon());
  m_MDICoverList.append( pMDICover);
  QBoxLayout* pLayout = new QHBoxLayout( pMDICover, 0, -1, "layout");
  pNewView->reparent( pMDICover, QPoint(0,0));
//  QApplication::sendPostedEvents();
  pLayout->addWidget( pNewView);
  pMDICover->setName( pNewView->name());
  // captions
  QString shortName = pNewView->caption();
  int length = shortName.length();
  shortName = shortName.right(length - (shortName.findRev('/') +1));
  pMDICover->setTabCaption( shortName);
  connect(pMDICover, SIGNAL(gotFocus(QextMdiChildView*)),
          this, SLOT(slot_gotFocus(QextMdiChildView*)));

  // fake a gotFocus to update the currentEditView/currentBrowserView pointers _before_ adding to MDI control
  slot_gotFocus( pMDICover);

  // take it under MDI mainframe control (note: this triggers also a setFocus())
  int flags;
  if (bShow) {
  	flags = QextMdi::StandardAdd;
  }
  else {
    flags = QextMdi::Hide;
  }
  m_pParent->addWindow( pMDICover, flags);
  // correct the default settings of QextMDI ('cause we haven't a tab order for subwidget focuses)
  pMDICover->setFirstFocusableChildWidget(0L);
  pMDICover->setLastFocusableChildWidget(0L);

  // set the accelerators for Toplevel MDI mode (each toplevel window needs its own accels
  connect( m_pParent, SIGNAL(childViewIsDetachedNow(QWidget*)), this, SLOT(initKeyAccel(QWidget*)) );
}

//-----------------------------------------------------------------------------
// create a new view for an edit document
//-----------------------------------------------------------------------------
CEditWidget* DocViewMan::createEditView(KWriteDoc* pDoc, bool bShow)
{
  debug("DocViewMan::createEditView !\n");

  // create the view and add to MDI
  CEditWidget* pEW = new CEditWidget(0L, "autocreatedview", pDoc);
  if(!pEW) return 0L;
  pEW->setCaption(pDoc->fileName());

  // connect tag related functionality with searchTagsDialogImpl
  searchTagsDialogImpl* ctagsDlg = m_pParent->getCTagsDialog();
  connect( pEW, SIGNAL(tagSwitchTo()), m_pParent, SLOT(slotTagSwitchTo()));
  connect( pEW, SIGNAL(tagOpenFile(QString)), ctagsDlg, SLOT(slotGotoFile(QString)));
  connect( pEW, SIGNAL(tagDefinition(QString)), ctagsDlg, SLOT(slotGotoDefinition(QString)));
  connect( pEW, SIGNAL(tagDeclaration(QString)), ctagsDlg, SLOT(slotGotoDeclaration(QString)));

  //connect the editor lookup function with slotHelpSText
  connect( pEW, SIGNAL(manpage(QString)),m_pParent, SLOT(slotHelpManpage(QString)));
  connect( pEW, SIGNAL(lookUp(QString)),m_pParent, SLOT(slotHelpSearchText(QString)));
  connect( pEW, SIGNAL(newCurPos()), m_pParent, SLOT(slotNewLineColumn()));
  connect( pEW, SIGNAL(newStatus()),m_pParent, SLOT(slotNewStatus()));
  connect( pEW, SIGNAL(clipboardStatus(KWriteView *, bool)), m_pParent, SLOT(slotClipboardChanged(KWriteView *, bool)));
  connect( pEW, SIGNAL(newUndo()),m_pParent, SLOT(slotNewUndo()));
  connect( pEW, SIGNAL(bufferMenu(const QPoint&)),m_pParent, SLOT(slotBufferMenu(const QPoint&)));
  connect( pEW, SIGNAL(grepText(QString)), m_pParent, SLOT(slotEditSearchInFiles(QString)));
  connect( pEW->popup(), SIGNAL(highlighted(int)), m_pParent, SLOT(statusCallback(int)));
  // Connect the breakpoint manager to monitor the bp setting - even when the debugging isn't running
  connect( pEW, SIGNAL(editBreakpoint(const QString&,int)), m_pParent->getBrkptManager(), SLOT(slotEditBreakpoint(const QString&,int)));
  connect( pEW, SIGNAL(toggleBPEnabled(const QString&,int)), m_pParent->getBrkptManager(), SLOT(slotToggleBPEnabled(const QString&,int)));
  connect( pEW, SIGNAL(toggleBreakpoint(const QString&,int)), m_pParent->getBrkptManager(), SLOT(slotToggleStdBreakpoint(const QString&,int)));
  connect( pEW, SIGNAL(clearAllBreakpoints()), m_pParent->getBrkptManager(),   SLOT(slotClearAllBreakpoints()));
  connect( pEW, SIGNAL(runToCursor(const QString&, int)), m_pParent, SLOT(slotDebugRunUntil(const QString&, int)));

  // connect adding watch variable from the rmb in the editors
  connect( pEW, SIGNAL(addWatchVariable(const QString&)), m_pParent->getVarViewer()->varTree(), SLOT(slotAddWatchVariable(const QString&)));

  if (getKWriteDocType(pDoc) == CPP_SOURCE) {
    connect( pEW, SIGNAL(markStatus(KWriteView *, bool)), m_pParent, SLOT(slotCPPMarkStatus(KWriteView *, bool)));
    QIconSet iconSet(SmallIcon("source_cpp"));
    // Cover it by a QextMDI childview and add that MDI system
    addQExtMDIFrame(pEW, bShow, iconSet.pixmap());
  } else {
    connect( pEW, SIGNAL(markStatus(KWriteView *, bool)), m_pParent, SLOT(slotHEADERMarkStatus(KWriteView *, bool)));
    QIconSet iconSet(SmallIcon("source_h"));
    // Cover it by a QextMDI childview and add that MDI system
    addQExtMDIFrame(pEW, bShow, iconSet.pixmap());
  }
  
  // some additional settings
  pEW->setFocusPolicy(QWidget::StrongFocus);
  pEW->setFont(KGlobalSettings::fixedFont());
  KConfig* config = m_pParent->getConfig();
  if(config) {
    config->setGroup("KWrite Options");
    pEW->readConfig(config);
  }

  return pEW;
}

//-----------------------------------------------------------------------------
// create a new view for a browser document
//-----------------------------------------------------------------------------
KHTMLView* DocViewMan::createBrowserView(CDocBrowser* pDoc, bool bShow)
{
  debug("DocViewMan::createBrowserView !\n");

  KHTMLView* pNewView = pDoc->view();
  pNewView->setCaption( pDoc->currentTitle());
  // add "what's this" entry
  m_pParent->getWhatsThis()->add(pNewView, i18n("Documentation Browser\n\n"
            "The documentation browser window shows the online-"
            "documentation provided with kdevelop as well as "
            "library class documentation created. Use the documentation "
            "tree to switch between various parts of the documentation."));
  pDoc->showURL(pDoc->currentURL(), true); // with reload if equal

  // Cover it by a QextMDI childview and add that MDI system
  addQExtMDIFrame(pNewView, bShow, SmallIcon("contents"));

  return pNewView;
}

//-----------------------------------------------------------------------------
// close a view
//-----------------------------------------------------------------------------
bool DocViewMan::closeView(QWidget* pWnd)
{
  debug("DocViewMan::closeView !\n");

  // get the embedded view
  QObjectList* pL = (QObjectList*) (pWnd->children());
  QWidget* pView = 0L;
  QObject* pChild;
  // the first object we'll find is the layout,
  // the second test will be successful for the one and only embedded widget
  for ( pChild = pL->first(); pChild && !pView; pChild = pL->next()) {
    if (pChild->inherits("QWidget")) {
      pView = (QWidget*) pChild;
    }
  }

  if (CEditWidget* pEditView = dynamic_cast<CEditWidget*> (pView)) {
    if (!pEditView)
      return true;

    if (checkAndSaveFileOfCurrentEditView(true) != KMessageBox::Cancel)
      closeEditView(pEditView);
    else
      return false;
  }
  else if (KHTMLView* pHTMLView = dynamic_cast<KHTMLView*> (pView)) {
    if(pHTMLView) {
      closeBrowserView(pHTMLView);
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
// close an edit view
//-----------------------------------------------------------------------------
void DocViewMan::closeEditView(CEditWidget* pView)
{
  debug("DocViewMan::closeEditView !\n");
  if (!pView) return;

  // Get the document
  KWriteDoc* pDoc = pView->doc();

  QextMdiChildView* pMDICover = (QextMdiChildView*) pView->parentWidget();
  pMDICover->hide();
  
  // disconnect the focus signals
  disconnect(pMDICover, SIGNAL(gotFocus(QextMdiChildView*)), this, SLOT(slot_gotFocus(QextMdiChildView*)));

  // remove the view from MDI and delete the view
  m_pParent->removeWindowFromMdi( pMDICover);
  m_MDICoverList.remove( pMDICover);

  if (pDoc->viewCount() == 0) {
    closeKWriteDoc(pDoc);
  }
}

//-----------------------------------------------------------------------------
// close a browser view
//-----------------------------------------------------------------------------
void DocViewMan::closeBrowserView(KHTMLView* pView)
{
  debug("DocViewMan::closeBrowserView !\n");

  CDocBrowser* pDoc = (CDocBrowser*) pView->part();

  QextMdiChildView* pMDICover = (QextMdiChildView*) pView->parentWidget();
  pMDICover->hide();
  
  // disconnect the focus signals
  disconnect(pMDICover, SIGNAL(gotFocus(QextMdiChildView*)), this, SLOT(slot_gotFocus(QextMdiChildView*)));
  
  // get a KHTMLView out of the parent to avoid a delete, 
  // it will be deleted later in the CDocBrowser destructor
  pView->reparent(0L,0,QPoint(0,0));
  QApplication::sendPostedEvents();
  
  // remove the view from MDI and delete the view
  m_pParent->removeWindowFromMdi( pMDICover);
  m_MDICoverList.remove( pMDICover);

  closeCDocBrowser(pDoc);
}

//-----------------------------------------------------------------------------
// get the first edit view for a document
//-----------------------------------------------------------------------------
CEditWidget* DocViewMan::getFirstEditView(KWriteDoc* pDoc) const
{
  debug("DocViewMan::getFirstEditView !\n");

  return (dynamic_cast<CEditWidget*> (pDoc->getKWrite()));
}

//-----------------------------------------------------------------------------
// Connected to the focus in event occures signal of CEditWidget.
// Moves the focused view to the end of the list of focused views or
// adds it. Emits the sig_viewGotFocus signal
//-----------------------------------------------------------------------------
void DocViewMan::slot_gotFocus(QextMdiChildView* pMDICover)
{
  debug("DocViewMan::slot_gotFocus !\n");

  // set current view, distinguish between edit widget and browser widget
  QObjectList* pL = (QObjectList*) pMDICover->children();
  QWidget* pView = 0L;
  QObject* pChild;
  for ( pChild = pL->first(); pChild && !pView; pChild = pL->next()) {
    if (pChild->inherits("QWidget")) {
      pView = (QWidget*) pChild;
    }
  }

  if (!pView) return;

  if (pView->inherits("CEditWidget")) {
    m_pCurEditView = (CEditWidget*) pView;
    m_pCurEditDoc = m_pCurEditView->doc();
    m_curIsBrowser = false;

    // check if the file has been modified outside
    // --------
    // Get the current file name
    QString filename = m_pCurEditView->getName();
    if (m_pCurEditDoc == 0)
      return; //oops :-(
    // check if it modified inside KDevelop
    bool bModifiedInside = false;
    if (m_pCurEditDoc->isModified()) {
      bModifiedInside = true;
    }
    // check if it is modified outside KDevelop
    bool bModifiedOutside = false;
    QFileInfo file_info(filename);
    if ((file_info.lastModified() != m_pCurEditDoc->getLastFileModifDate())) {
      bModifiedOutside = true;
    }
    if (bModifiedInside && bModifiedOutside) {
      if (KMessageBox::warningYesNo(m_pParent
               ,i18n("This file %1 was modified inside but also outside this editor.\n"
                     "Do you want to keep your changes or reload it from disk?")
               .arg(filename), i18n("File modified")
               ,i18n("&Keep"), i18n("&Reload")) == KMessageBox::No) {
        loadKWriteDoc(m_pCurEditDoc, filename, 1);
      }
      else {
        m_pCurEditDoc->setLastFileModifDate(file_info.lastModified());
      }
    }
    else if (bModifiedOutside) {
      if (KMessageBox::questionYesNo(m_pParent
               ,i18n("This file %1 was modified outside this editor.\n"
                    "Do you want reload it from disk?")
               .arg(filename), i18n("File modified")
               ,i18n("&Yes"), i18n("&No")) == KMessageBox::Yes) {
        loadKWriteDoc(m_pCurEditDoc, filename, 1);
      }
      else {
        m_pCurEditDoc->setLastFileModifDate(file_info.lastModified());
      }
    }
  }
  else {
    m_pCurBrowserView = (KHTMLView*) pView;
    m_pCurBrowserDoc = (CDocBrowser*) m_pCurBrowserView->part();
    m_curIsBrowser = true;
  }

  debug("pView : %d !", (int)pView);
  
  // emit the got focus signal 
  // (connected to CKDevelop but could also be caught by other ones)
  emit sig_viewGotFocus(pView);

  debug("emit the got focus signal !\n");
}

//-----------------------------------------------------------------------------
// Returns the number of documents
//-----------------------------------------------------------------------------
int DocViewMan::docCount() const
{
  debug("DocViewMan::docCount !\n");

  return m_documentList.count();
}

//-----------------------------------------------------------------------------
// Retrieves the KWriteDoc found by its filename
//-----------------------------------------------------------------------------
KWriteDoc* DocViewMan::findKWriteDoc(const QString& strFileName) const
{
  debug("DocViewMan::findKWriteDoc !\n");

  QListIterator<QObject> itDoc(m_documentList);
  for (itDoc.toFirst(); itDoc.current() != 0; ++itDoc) {
    KWriteDoc* pDoc = dynamic_cast<KWriteDoc*> (itDoc.current());
    if(pDoc && (pDoc->fileName() == strFileName)) {
      return pDoc;
    }
  }
  return 0L;
}

//-----------------------------------------------------------------------------
// Retrieves any Document found by its filename
//-----------------------------------------------------------------------------
QObject* DocViewMan::findDocFromFilename(const QString& strFileName) const
{
  debug("DocViewMan::findDocFromFilename !\n");

  QListIterator<QObject> itDoc(m_documentList);

  for (; itDoc.current() != 0; ++itDoc) {
    QObject* doc = itDoc.current();
    if (doc  && (docName(doc) == strFileName)) {
      return doc;
    }
  }

  return 0L;
}

//-----------------------------------------------------------------------------
// Find if no documents have been modified 
//-----------------------------------------------------------------------------
bool DocViewMan::noDocModified()
{
  debug("DocViewMan::noDocModified !\n");

  QListIterator<QObject> itDoc(m_documentList);
  for (itDoc.toFirst(); itDoc.current() != 0; ++itDoc) {
    KWriteDoc* pDoc = dynamic_cast<KWriteDoc*> (itDoc.current());
    if(pDoc && pDoc->isModified()) {
      return false;
    }
  }
  return true;
}

bool DocViewMan::doFileClose()
{
  // we want to close a whole document,
  // this means to save and close all views!

  // get the current view (one of possibly many view of the document)
  CEditWidget* pCurEditView = currentEditView();
  if (!pCurEditView) return true;
  QString filename = pCurEditView->getName();

  // close the current view of the document (this will ask the user in case of being modified)
  bool bClosed = closeView(pCurEditView);
  if (!bClosed) // action was cancelled
    return false;

  // if there was only one view of a document, the call of closeView() has closed the document as well.
  // That's why for closing of possible other views we must check if the document is still alive
  KWriteDoc* pDoc = findKWriteDoc(filename);
  if (pDoc) {
     // now close all other open views of this document
     closeKWriteDoc( pDoc);
  }
  return true;
}

// closes all KWrite documents and their views
// but not the document browser views
void DocViewMan::doFileCloseAll()
{
  debug("DocViewMan::doFileCloseAll !\n");

  QStrList handledNames;
  bool cont=true;

  QListIterator<QObject> itDoc(m_documentList);
  itDoc.toFirst();
  QObject* itDocObject;
  while ( (itDocObject = itDoc.current()) )
  {
    KWriteDoc* pDoc = dynamic_cast<KWriteDoc*> (itDocObject);
    if (pDoc
        && pDoc->isModified()
        && handledNames.contains(pDoc->fileName())<1)
    {
      SaveAllDialog::SaveAllResult result = m_pParent->doProjectSaveAllDialog(pDoc->fileName());
      switch (result)
      {
        case SaveAllDialog::Yes:
        { // Yes- only save the current file
          // save file as if Untitled and close file
          if(m_pParent->isUntitled(pDoc->fileName()))
          {
            m_pParent->switchToFile(pDoc->fileName());
            handledNames.append(pDoc->fileName());
            cont = m_pParent->fileSaveAs();
          }
          else
          { // Save file and close it
            m_pParent->switchToFile(pDoc->fileName());
            handledNames.append(pDoc->fileName());
            m_pParent->slotFileSave();
            cont = !currentEditView()->isModified(); //something went wrong
          }
          break;
        }

        case SaveAllDialog::No:
        {
          // No - no save but close
          handledNames.append(pDoc->fileName());
          pDoc->setModified(false);
          break;
        }

        case SaveAllDialog::SaveAll:
        {
          // Save all
          m_pParent->slotFileSaveAll();
          break;
        }

        case SaveAllDialog::Cancel:
        {
          cont=false;     // We are not going to continue
          break;
        }
      }
    }  // end file close/save

    if (cont)
      // We want to close the document and views
      closeKWriteDoc(pDoc);
    else
      break;    // user decided not to continue because of unsaved files

    // If the doc has been deleted then the iterator will be moved to the next
    // doc already - so we only have to move on if a doc wasn't deleted.
    if (itDocObject == itDoc.current())
      ++itDoc;
  } // end while-loop
}

bool DocViewMan::doProjectClose()
{
  debug("DocViewMan::doProjectClose !\n");

  QStrList handledNames;
  bool cont = true;

  // synchronizeDocAndInfo();

  QListIterator<QObject> itDoc(m_documentList);
  for (itDoc.toFirst(); cont && itDoc.current() != 0; ++itDoc) {
    KWriteDoc* pDoc = dynamic_cast<KWriteDoc*> (itDoc.current());
    if (pDoc
        && pDoc->isModified()
        && handledNames.contains(pDoc->fileName())<1) {

      SaveAllDialog::SaveAllResult result = m_pParent->doProjectSaveAllDialog(pDoc->fileName());

        // what to do
      if(result==SaveAllDialog::Yes) {  // Yes- only save the actual file
        // save file as if Untitled and close file
        if(m_pParent->isUntitled(pDoc->fileName())) {
          m_pParent->switchToFile(pDoc->fileName());
          handledNames.append(pDoc->fileName());
          cont = m_pParent->fileSaveAs();
          // start again... 'cause we deleted an entry
          itDoc.toFirst();
        }
        // Save file and close it
        else {
          m_pParent->switchToFile(pDoc->fileName());
          handledNames.append(pDoc->fileName());
          m_pParent->slotFileSave();
          cont = ! pDoc->isModified(); //something went wrong
        }
      }

      if(result==SaveAllDialog::No) {   // No - no save but close
        handledNames.append(pDoc->fileName());
        pDoc->setModified(false);
        // start again... 'cause we deleted an entry
        itDoc.toFirst();
      }

      if(result==SaveAllDialog::SaveAll) {  // Save all
        m_pParent->slotFileSaveAll();
        break;
      }

      if (result==SaveAllDialog::Cancel) { // Cancel
        cont=false;
        break;
      }
    }  // end actual file close
  } // end for-loop

  // check if something went wrong with saving
  if (cont) {
    cont = noDocModified();
  }

  return cont;
}

/** This closes all the edit and browser documents */
void DocViewMan::doCloseAllDocs()
{
  debug("DocViewMan::doCloseAllDocs !\n");

  QListIterator<QObject> itDoc(m_documentList);
  while (itDoc.current() != 0L) {
    KWriteDoc* pEditDoc = dynamic_cast<KWriteDoc*> (itDoc.current());
    if (pEditDoc) {
      closeKWriteDoc(pEditDoc);
    }
    else {
      CDocBrowser* pBrDoc = dynamic_cast<CDocBrowser*> (itDoc.current());
      if (pBrDoc) {
        closeCDocBrowser( pBrDoc);
      }
    }

    // go to the begin of the list
    // we cannot go through the list because closing the document changes the list itself
    // and the iterator doesn't work any more
    itDoc.toFirst();
  }
}

int DocViewMan::checkAndSaveFileOfCurrentEditView(bool bDoModifiedInsideCheck)
{
  debug("DocViewMan::saveFileFromTheCurrentEditWidget !\n");

  // Get the current file name
  QString filename = currentEditView()->getName();
  KWriteDoc* pCurEditDoc = currentEditDoc();

  if (pCurEditDoc == 0)
    return KMessageBox::Cancel; //oops :-(

  // check if it modified inside KDevelop
  bool bModifiedInside = false;
  if (pCurEditDoc->isModified()) {
    bModifiedInside = true;
  }
  // check if it is modified outside KDevelop
  bool bModifiedOutside = false;
  QFileInfo file_info(filename);
  if ((file_info.lastModified() < pCurEditDoc->getLastFileModifDate())) {
    bModifiedOutside = true;
  }

  if (!bModifiedInside && !bModifiedOutside)
     return KMessageBox::No;  // nothing to save

  int button=KMessageBox::Yes;
  if (bModifiedInside && bModifiedOutside) {
    button = KMessageBox::warningYesNoCancel(m_pParent
             ,i18n("This file %1 was modified inside but also outside this editor.\n"
                   "Do you want to reject your changes or overwrite the changes that happened outside?")
             .arg(filename), i18n("File modified")
             ,i18n("&Overwrite"), i18n("&Reject"));
  }
  else if (bDoModifiedInsideCheck && bModifiedInside) {
    button = KMessageBox::warningYesNoCancel(m_pParent
             ,i18n("The file %1 was modified.\n"
                  "Do you want to save your changes?")
             .arg(filename), i18n("File modified")
             ,i18n("&Yes"), i18n("&No"));
  }

  switch (button) {
  case KMessageBox::No:
  case KMessageBox::Cancel:
    return button;
  default:
    break;
  }

  // Really save it
  currentEditView()->doSave();
  QFileInfo file_info2(filename);
  pCurEditDoc->setLastFileModifDate(file_info2.lastModified());

  // refresh class tree-view
  QStrList lSavedFile;
  lSavedFile.append(filename);
#ifdef WITH_CPP_REPARSE
  if (m_pParent->hasProject())
#else
    if (m_pParent->hasProject() && getKWriteDocType(m_pCurEditDoc) == CPP_HEADER)
#endif
       m_pParent->refreshClassViewByFileList(&lSavedFile);
  return KMessageBox::Yes;
}

void DocViewMan::saveModifiedFiles()
{
  debug("DocViewMan::saveModifiedFiles ! \n");

  QProgressBar* pProgressBar = m_pParent->getProgressBar();
  ASSERT(pProgressBar);

  pProgressBar->setTotalSteps(m_documentList.count());
  pProgressBar->show();

  QStrList iFileList(false);
  bool mod = false;

  QStrList handledNames;

  // check all edit_infos if they are modified outside; if yes, ask for saving
  QListIterator<QObject> itDoc(m_documentList);
  for (itDoc.toFirst(); itDoc.current() != 0; ++itDoc) {
    KWriteDoc* pDoc = dynamic_cast<KWriteDoc*> (itDoc.current());
    if (pDoc) {
      int i = 0;
      pProgressBar->setProgress(++i);

      kdDebug() << "checking: " << pDoc->fileName() << "\n";
      kdDebug() << " " << ((pDoc->isModified()) ?
      "modified" : "not modified") << "\n";

      if (!m_pParent->isUntitled(pDoc->fileName())
          && pDoc->isModified()
          && handledNames.contains(pDoc->fileName()) < 1) {
        int qYesNo = KMessageBox::Yes;
        handledNames.append(pDoc->fileName());


        kdDebug() << " file info" << "\n";
        QFileInfo file_info(pDoc->fileName());
        if (file_info.lastModified() != pDoc->getLastFileModifDate()) {
          qYesNo = KMessageBox::questionYesNo(m_pParent,
                                              i18n("The file %1 was modified outside\n"
                                                   "this editor. Save anyway?")
                                              .arg(pDoc->fileName()),
                                              i18n("File modified"));
        }


        if (qYesNo == KMessageBox::Yes) {
          kdDebug() << " KMessageBox::Yes" << "\n";
          bool isModified=true;
          if (CEditWidget* pEditView=getFirstEditView(pDoc)) {
            pEditView->doSave();
            // kind of awkward way to find out whether the save succeeded
            isModified = pEditView->isModified();
            kdDebug() << "save document: "
                      << pEditView->getName() << ", "
                      << ((!isModified) ? "succeeded" : "failed") << "\n";
          }

          if (!isModified) {
#ifdef WITH_CPP_REPARSE
            mod = true;
#else
            mod |= (pDoc->fileName().right(2)==".h" || pDoc->fileName().right(4)==".hxx");
#endif
            iFileList.append(pDoc->fileName());

            // file_info.lastModified has not recognized here the file modif time has changed
            // maybe a sync problem, maybe a Qt bug?
            // anyway, we have to create another file info, this works then.
            QFileInfo fileInfoSavedFile(pDoc->fileName());
            pDoc->setLastFileModifDate(fileInfoSavedFile.lastModified());
          }
        }
      }
    }
  }

  debug("end handledNames !\n");
  debug("end edit widget !\n");
  debug("stat prog ! \n");

  pProgressBar->reset();

  debug("refreshClassViewByFileList ! \n");
  if (m_pParent->hasProject() && !iFileList.isEmpty() && mod)
    m_pParent->refreshClassViewByFileList(&iFileList);

}
  
void DocViewMan::reloadModifiedFiles()
{
  debug("DocViewMan::reloadModifiedFiles !\n");

  QListIterator<QObject> itDoc(m_documentList);
  for (itDoc.toFirst(); itDoc.current() != 0; ++itDoc) {
    KWriteDoc* pDoc = dynamic_cast<KWriteDoc*> (itDoc.current());
    if (pDoc) {
      QFileInfo file_info(pDoc->fileName());
    	
      // Reload only changed files
      if(pDoc->getLastFileModifDate() != file_info.lastModified()) {
        // Force reload, no modified on disc messagebox
        m_pParent->switchToFile(pDoc->fileName(),-1,-1,true,false);
      }
    }
  }
}


QList<KWriteDoc> DocViewMan::getKWriteDocList() const
{
  QListIterator<QObject> itDoc(m_documentList);
  QList<KWriteDoc> resultList;

  for (; itDoc.current() != 0; ++itDoc) {
    KWriteDoc* doc = dynamic_cast<KWriteDoc*> (itDoc.current());
    if (doc) {
      resultList.append(doc);
    }
  }
  return resultList;
}

QList<CDocBrowser> DocViewMan::getDocBrowserList() const
{
  QListIterator<QObject> itDoc(m_documentList);
  QList<CDocBrowser> resultList;

  for (; itDoc.current() != 0; ++itDoc) {
    CDocBrowser* doc = dynamic_cast<CDocBrowser*> (itDoc.current());
    if (doc) {
      resultList.append(doc);
    }
  }
  return resultList;
}

QString DocViewMan::docName(QObject* pDoc) const
{
  debug("DocViewMan::docName !\n");

  KWriteDoc* kwDoc = (dynamic_cast<KWriteDoc*> (pDoc));
  if (kwDoc) {
    return (kwDoc->fileName());
  }

  CDocBrowser* brDoc = (dynamic_cast<CDocBrowser*> (pDoc));
  if (brDoc) {
    return (QString(brDoc->name()));
  }

  return QString("");
}

//-----------------------------------------------------------------------------
// Connect to the signals of the bookmark popup menus
//-----------------------------------------------------------------------------
void DocViewMan::installBMPopup(QPopupMenu * bm_menu)
{
  debug("DocViewMan::installBMPopup");

	// Install editor bookmark popup menu
  QPopupMenu* code_bookmarks = new QPopupMenu();

  connect(code_bookmarks,SIGNAL(aboutToShow()),
          this,SLOT(updateCodeBMPopup()));
  connect(code_bookmarks,SIGNAL(activated(int)),
          this,SLOT(gotoCodeBookmark(int)));

	
  bm_menu->insertItem(SmallIconSet("bookmark_folder"),
                      i18n("Code &Window"),code_bookmarks,31000);

	// Install browser bookmark popup menu
  m_pDocBookmarksMenu = new QPopupMenu();

  connect(m_pDocBookmarksMenu,SIGNAL(activated(int)),
          this,SLOT(gotoDocBookmark(int)));

  bm_menu->insertItem(SmallIconSet("bookmark_folder"),
                      i18n("&Browser Window"), m_pDocBookmarksMenu,31010);

}

//-----------------------------------------------------------------------------
// Updates the bookmarks for each editor document
//-----------------------------------------------------------------------------
void DocViewMan::updateCodeBMPopup()
{
  debug("DocViewMan::updateCodeBMPopup !\n");

  QPopupMenu* popup = (QPopupMenu *) sender();

  // Remove all menu items
  popup->clear();

  // Insert separator
  popup->insertSeparator();

  // Update bookmarks for each document
  QListIterator<QObject> itDoc(m_documentList);
  for (itDoc.toFirst(); itDoc.current() != 0; ++itDoc) {
    KWriteDoc* pDoc = dynamic_cast<KWriteDoc*> (itDoc.current());
    if(pDoc) {
      pDoc->updateBMPopup(popup);
    }
  }
}

//-----------------------------------------------------------------------------
// Shows the desired editor bookmark 
// (eventually, switches to file and activates it)
//-----------------------------------------------------------------------------
void DocViewMan::gotoCodeBookmark(int n) {

  debug("DocViewMan::gotoCodeBookmark : %d !\n", n);

  QPopupMenu* popup = (QPopupMenu *) sender();

  QString text = popup->text(n);

  // Find the KWriteDoc for this bookmark
  QListIterator<QObject> itDoc(m_documentList);
  for (itDoc.toFirst(); itDoc.current() != 0; ++itDoc) {
    KWriteDoc* pDoc = dynamic_cast<KWriteDoc*> (itDoc.current());
    if(pDoc) {
      if(text.contains(pDoc->fileName() + ";")) {
        m_pParent->switchToFile(pDoc->fileName());
        pDoc->gotoBookmark(text);
        return;
      }
    }
  }
}

//-----------------------------------------------------------------------------
// Shows the desired browser bookmark 
// (eventually, switches to file and activates it)
//-----------------------------------------------------------------------------
void DocViewMan::gotoDocBookmark(int id_)
{
  debug("DocViewMan::gotoDocBookmark : id : %d !\n", id_);

	int id_index = m_pDocBookmarksMenu->indexOf(id_);

  m_pParent->openBrowserBookmark(m_docBookmarksList.at(id_index));
}

//-----------------------------------------------------------------------------
// Toggles a bookmark in the current document
//-----------------------------------------------------------------------------
void DocViewMan::doBookmarksToggle()
{
  debug("DocViewMan::doBookmarksToggle !\n");

  if (curDocIsBrowser())
  {
    m_pDocBookmarksMenu->clear();

    // Check if the current URL is bookmarked
    int pos = m_docBookmarksList.find(currentBrowserDoc()->currentURL());
    if(pos > -1)
    {
      // The current URL is bookmarked, let's remove the bookmark
      m_docBookmarksList.remove(pos);
      m_docBookmarksTitleList.remove(pos);
    }
    else
    {
      CDocBrowser* pCurBrowserDoc = currentBrowserDoc();
      // The current URL is not bookmark, let's bookmark it
      m_docBookmarksList.append(pCurBrowserDoc->currentURL());
      m_docBookmarksTitleList.append(pCurBrowserDoc->currentTitle());
    }

    // Recreate thepopup menu
    for (uint i = 0 ; i < m_docBookmarksList.count(); i++){
      m_pDocBookmarksMenu->insertItem(SmallIconSet("html"), 
                                      getBrowserMenuItem(i));
    }
  }
  else
  {
    if (currentEditView())
      currentEditView()->toggleBookmark();
  }

}

//-----------------------------------------------------------------------------
// Clears bookmarks in the current document
//-----------------------------------------------------------------------------
void DocViewMan::doBookmarksClear()
{
  if (curDocIsBrowser())
    {
      m_docBookmarksList.clear();
      m_docBookmarksTitleList.clear();
      m_pDocBookmarksMenu->clear();
    }    
  else
    {
      doClearBookmarks();
    }
}

//-----------------------------------------------------------------------------
// Goes to the next bookmark in the current document
//-----------------------------------------------------------------------------
void DocViewMan::doBookmarksNext()
{
  if (curDocIsBrowser())
  {
    if(m_docBookmarksList.count() > 0)
    {
      QString file = m_docBookmarksList.next();
      if (file.isEmpty())
        file = m_docBookmarksList.first();
      m_pParent->openBrowserBookmark(file);  
    }
  }
  else
  {
    if (currentEditView())
      currentEditView()->nextBookmark();
  }
}

//-----------------------------------------------------------------------------
// Goes to the previous bookmark in the current document
//-----------------------------------------------------------------------------
void DocViewMan::doBookmarksPrevious()
{
  if (curDocIsBrowser())
  {
    if(m_docBookmarksList.count() > 0)
    {
      QString file = m_docBookmarksList.prev();
      if(file.isEmpty())
        file = m_docBookmarksList.last();
      m_pParent->openBrowserBookmark(file);  
    }
  }
  else
  {
    if (currentEditView())
      currentEditView()->previousBookmark();
  }
}

//-----------------------------------------------------------------------------
// Get browser bookmark popup menu item
//-----------------------------------------------------------------------------
QString DocViewMan::getBrowserMenuItem(int index)
{
  QString bmTitle;
  if (index < 9) {
    bmTitle = QString ("&%1 %2").arg(index + 1)
      .arg(m_docBookmarksTitleList.at(index));
  } else {
    bmTitle = m_docBookmarksTitleList.at(index);
  }
  return bmTitle;
} 

//-----------------------------------------------------------------------------
// Reads bookmarks from the config
//-----------------------------------------------------------------------------
void DocViewMan::readBookmarkConfig(KConfig* theConfig)
{
  theConfig->readListEntry("doc_bookmarks",m_docBookmarksList);
  theConfig->readListEntry("doc_bookmarks_title",m_docBookmarksTitleList);
  for ( uint i =0 ; i < m_docBookmarksTitleList.count(); i++)
    {
      m_pDocBookmarksMenu->insertItem(SmallIconSet("html"),
                                      getBrowserMenuItem(i));
    }
}

//-----------------------------------------------------------------------------
// Writes bookmarks from the config
//-----------------------------------------------------------------------------
void DocViewMan::writeBookmarkConfig(KConfig* theConfig)
{
  theConfig->writeEntry("doc_bookmarks", m_docBookmarksList);
  theConfig->writeEntry("doc_bookmarks_title", m_docBookmarksTitleList);
}

/**
* Helper method for the method below (slot entry point for QextMdi signals)
*/
void DocViewMan::initKeyAccel( QWidget* pTopLevelWidget)
{
  initKeyAccel(0L, pTopLevelWidget);
}

/**
* This method can be called from various places.
* CKDevelop calls it in its constructor, DocViewMan calls it from addQExtMDIFrame(..).
* Anyway, it sets the application accelerators for any _toplevel_ window of KDevelop.
*/
void DocViewMan::initKeyAccel( CKDevAccel* accel, QWidget* pTopLevelWidget)
{
  if (accel == 0L) {
    accel = new CKDevAccel( pTopLevelWidget );
  }

  accel->insertItem( i18n("Goto Previous Window"), "GotoPrevWin", IDK_GOTO_PREVWIN);
  accel->connectItem("GotoPrevWin", m_pParent, SLOT(activatePrevWin()), true, 0);
  accel->insertItem( i18n("Goto Next Window"), "GotoNextWin", IDK_GOTO_NEXTWIN);
  accel->connectItem("GotoNextWin", m_pParent, SLOT(activateNextWin()), true, 0);

  //file menu
  accel->connectItem( KStdAccel::New, m_pParent, SLOT(slotFileNew()), true, ID_FILE_NEW );
  accel->connectItem( KStdAccel::Open , m_pParent, SLOT(slotFileOpen()), true, ID_FILE_OPEN );
  accel->connectItem( KStdAccel::Close , m_pParent, SLOT(slotFileClose()), true, ID_FILE_CLOSE );

  accel->connectItem( KStdAccel::Save , m_pParent, SLOT(slotFileSave()), true, ID_FILE_SAVE );

  accel->insertItem(i18n("Save As"), "SaveAs", (unsigned int) 0);
  accel->connectItem( "SaveAs", m_pParent, SLOT(slotFileSaveAs()), true, ID_FILE_SAVE_AS);

  accel->insertItem(i18n("Save All"), "SaveAll", (unsigned int) 0);
  accel->connectItem( "SaveAll", m_pParent, SLOT(slotFileSaveAll()), true, ID_FILE_SAVE_ALL);

  accel->connectItem( KStdAccel::Print , m_pParent, SLOT(slotFilePrint()), true, ID_FILE_PRINT );
  accel->connectItem( KStdAccel::Quit, m_pParent, SLOT(slotFileQuit()), true, ID_FILE_QUIT );

  //edit menu
  accel->connectItem( KStdAccel::Undo , m_pParent, SLOT(slotEditUndo()), true, ID_EDIT_UNDO );

  accel->insertItem( i18n("Redo"), "Redo",IDK_EDIT_REDO );
  accel->connectItem( "Redo" , m_pParent, SLOT(slotEditRedo()), true, ID_EDIT_REDO  );

  accel->connectItem( KStdAccel::Cut , m_pParent, SLOT(slotEditCut()), true, ID_EDIT_CUT );
  accel->connectItem( KStdAccel::Copy , m_pParent, SLOT(slotEditCopy()), true, ID_EDIT_COPY );
  accel->connectItem( KStdAccel::Paste , m_pParent, SLOT(slotEditPaste()), true, ID_EDIT_PASTE );

  accel->insertItem( i18n("Indent"), "Indent",IDK_EDIT_INDENT );
  accel->connectItem( "Indent", m_pParent, SLOT(slotEditIndent() ), true, ID_EDIT_INDENT );

  accel->insertItem( i18n("Unindent"), "Unindent",IDK_EDIT_UNINDENT );
  accel->connectItem( "Unindent", m_pParent, SLOT(slotEditUnindent() ), true, ID_EDIT_UNINDENT );

  accel->insertItem( i18n("Comment"), "Comment",IDK_EDIT_COMMENT );
  accel->connectItem( "Comment", m_pParent, SLOT(slotEditComment() ), true, ID_EDIT_COMMENT );

  accel->insertItem( i18n("Uncomment"), "Uncomment",IDK_EDIT_UNCOMMENT );
  accel->connectItem( "Uncomment", m_pParent, SLOT(slotEditUncomment() ), true, ID_EDIT_UNCOMMENT );

  accel->insertItem( i18n("Insert File"), "InsertFile", (unsigned int) 0);
  accel->connectItem( "InsertFile", m_pParent, SLOT(slotEditInsertFile()), true, ID_EDIT_INSERT_FILE );

  accel->connectItem( KStdAccel::Find, m_pParent, SLOT(slotEditSearch() ), true, ID_EDIT_SEARCH );

  accel->insertItem( i18n("Repeat Search"), "RepeatSearch",IDK_EDIT_REPEAT_SEARCH );
  accel->connectItem( "RepeatSearch", m_pParent, SLOT(slotEditRepeatSearch(int) ), true, ID_EDIT_REPEAT_SEARCH );

  accel->insertItem( i18n("Repeat Search Back"), "RepeatSearchBack",IDK_EDIT_REPEAT_SEARCH_BACK );
  accel->connectItem( "RepeatSearchBack", m_pParent, SLOT(slotEditRepeatSearchBack() ), true, ID_EDIT_REPEAT_SEARCH_BACK );
  accel->connectItem( KStdAccel::Replace, m_pParent, SLOT(slotEditReplace() ), true, ID_EDIT_REPLACE );

  accel->insertItem( i18n("Search in Files"), "Grep", IDK_EDIT_GREP_IN_FILES );
  accel->connectItem( "Grep", m_pParent, SLOT(slotEditSearchInFiles() ), true, ID_EDIT_SEARCH_IN_FILES );

  accel->insertItem( i18n("Search selection in Files"), "GrepSearch", IDK_EDIT_SEARCH_GREP_IN_FILES );
  accel->connectItem( i18n("GrepSearch"), m_pParent, SLOT(slotEditSearchText() ) );

  accel->insertItem( i18n("Search CTags Database"), "CTagsSearch", IDK_EDIT_TAGS_SEARCH );
  accel->connectItem( "CTagsSearch", m_pParent, SLOT(slotTagSearch() ), true, ID_EDIT_TAGS_SEARCH );

  accel->insertItem( i18n("Switch To Header/Source"), "CTagsSwitch", IDK_EDIT_TAGS_SWITCH );
  accel->connectItem( "CTagsSwitch", m_pParent, SLOT(slotTagSwitchTo() ), true, ID_EDIT_TAGS_SWITCH );

  accel->insertItem( i18n("Select All"), "SelectAll", IDK_EDIT_SELECT_ALL);
  accel->connectItem("SelectAll", m_pParent, SLOT(slotEditSelectAll() ), true, ID_EDIT_SELECT_ALL );

  accel->insertItem(i18n("Deselect All"), "DeselectAll", (unsigned int) 0);
  accel->connectItem("DeselectAll", m_pParent, SLOT(slotEditDeselectAll()), true, ID_EDIT_DESELECT_ALL);

  accel->insertItem(i18n("Invert Selection"), "Invert Selection", (unsigned int) 0);
  accel->connectItem("Invert Selection", m_pParent, SLOT(slotEditInvertSelection()), true, ID_EDIT_INVERT_SELECTION);

  //view menu
  accel->insertItem( i18n("Goto Line"), "GotoLine",IDK_VIEW_GOTO_LINE);
  accel->connectItem( "GotoLine", m_pParent, SLOT( slotViewGotoLine()), true, ID_VIEW_GOTO_LINE );

  accel->insertItem( i18n("Next Error"), "NextError",IDK_VIEW_NEXT_ERROR);
  accel->connectItem( "NextError", m_pParent, SLOT( slotViewNextError()), true, ID_VIEW_NEXT_ERROR );

  accel->insertItem( i18n("Previous Error"), "PreviousError",IDK_VIEW_PREVIOUS_ERROR);
  accel->connectItem( "PreviousError", m_pParent, SLOT( slotViewPreviousError()), true, ID_VIEW_PREVIOUS_ERROR  );

  accel->insertItem( i18n("Dialog Editor"), "Dialog Editor", (unsigned int) 0);
  accel->connectItem("Dialog Editor", m_pParent, SLOT(startDesigner()), true, ID_TOOLS_DESIGNER );

  accel->insertItem( i18n("Toogle Tree-View"), "Tree-View",IDK_VIEW_TREEVIEW);
  accel->connectItem( "Tree-View", m_pParent, SLOT(slotViewTTreeView()), true, ID_VIEW_TREEVIEW );

  accel->insertItem( i18n("Toogle Output-View"), "Output-View",IDK_VIEW_OUTPUTVIEW);
  accel->connectItem( "Output-View", m_pParent, SLOT(slotViewTOutputView()), true, ID_VIEW_OUTPUTVIEW );

  accel->insertItem( i18n("Toolbar"), "Toolbar", (unsigned int) 0);
  accel->connectItem( "Toolbar", m_pParent, SLOT(slotViewTStdToolbar()), true, ID_VIEW_TOOLBAR );

  accel->insertItem( i18n("Browser-Toolbar"), "Browser-Toolbar", (unsigned int) 0);
  accel->connectItem( "Browser-Toolbar", m_pParent, SLOT(slotViewTBrowserToolbar()), true, ID_VIEW_BROWSER_TOOLBAR );
	
  accel->insertItem( i18n("Statusbar"), "Statusbar", (unsigned int) 0);
  accel->connectItem( "Statusbar", m_pParent, SLOT(slotViewTStatusbar()), true, ID_VIEW_STATUSBAR );

  accel->insertItem( i18n("MDI-View-Taskbar"), "MDI-View-Taskbar", (unsigned int) 0);
  accel->connectItem( "MDI-View-Taskbar", m_pParent, SLOT(slotViewMdiViewTaskbar()), true, ID_VIEW_MDIVIEWTASKBAR );


  accel->insertItem( i18n("Preview dialog"), "Preview dialog",IDK_VIEW_PREVIEW);

  accel->insertItem( i18n("Refresh"), "Refresh", (unsigned int) 0);
  accel->connectItem( "Refresh", m_pParent, SLOT(slotViewRefresh()), true, ID_VIEW_REFRESH);

  accel->insertItem( i18n("Goto Declaration"), "CVGotoDeclaration", (unsigned int) 0);
  accel->connectItem( "CVGotoDeclaration", m_pParent,SLOT(slotClassbrowserViewDeclaration()),true, ID_CV_VIEW_DECLARATION); // project menu

  accel->insertItem( i18n("Goto Definition"), "CVGotoDefinition", (unsigned int) 0);
  accel->connectItem( "CVGotoDefinition", m_pParent, SLOT(slotClassbrowserViewDefinition()), true,ID_CV_VIEW_DEFINITION );

  accel->insertItem( i18n("Class Declaration"), "CVGotoClass", (unsigned int) 0);
  accel->connectItem( "CVGotoClass", m_pParent,SLOT(slotClassbrowserViewClass()),true, ID_CV_VIEW_CLASS_DECLARATION);

  accel->insertItem( i18n("Graphical Classview"), "CVViewTree", (unsigned int) 0);
  accel->connectItem( "CVViewTree", m_pParent, SLOT(slotClassbrowserViewTree()), true, ID_CV_GRAPHICAL_VIEW );


  // projectmenu
  accel->insertItem( i18n("New Project"), "NewProject",(unsigned int) 0);
  accel->connectItem( "NewProject", m_pParent, SLOT(slotProjectNewAppl()), true, ID_PROJECT_KAPPWIZARD );

  accel->insertItem( i18n("Open Project"), "OpenProject", (unsigned int) 0);
  accel->connectItem( "OpenProject", m_pParent, SLOT(slotProjectOpen()), true, ID_PROJECT_OPEN );

  accel->insertItem( i18n("Close Project"), "CloseProject", (unsigned int) 0);
  accel->connectItem("CloseProject", m_pParent, SLOT(slotProjectClose()), true, ID_PROJECT_CLOSE );

  accel->insertItem(i18n("New Class"), "NewClass", (unsigned int) 0);
  accel->connectItem("NewClass", m_pParent, SLOT(slotProjectNewClass()), true, ID_PROJECT_NEW_CLASS );

  accel->insertItem(i18n("Add existing File(s)"), "AddExistingFiles", (unsigned int) 0);
  accel->connectItem("AddExistingFiles",m_pParent, SLOT(slotProjectAddExistingFiles()), true, ID_PROJECT_ADD_FILE_EXIST );

  accel->insertItem(i18n("Add new Translation File"),"Add new Translation File", (unsigned int) 0);
  accel->connectItem("Add new Translation File", m_pParent, SLOT(slotProjectAddNewTranslationFile()), true, ID_PROJECT_ADD_NEW_TRANSLATION_FILE );

  accel->insertItem(i18n("File Properties"), "FileProperties", IDK_PROJECT_FILE_PROPERTIES);
  accel->connectItem("FileProperties", m_pParent, SLOT(slotProjectFileProperties() ), true, ID_PROJECT_FILE_PROPERTIES );

  accel->insertItem(i18n("Make messages and merge"), "MakeMessages", (unsigned int) 0);
  accel->connectItem("MakeMessages", m_pParent, SLOT(slotProjectMessages()), true, ID_PROJECT_MESSAGES  );

  accel->insertItem(i18n("Make API-Doc"), "ProjectAPI", (unsigned int) 0);
  accel->connectItem("ProjectAPI", m_pParent, SLOT(slotProjectAPI()), true, ID_PROJECT_MAKE_PROJECT_API );

  accel->insertItem(i18n("Make User-Manual..."), "ProjectManual", (unsigned int) 0);
  accel->connectItem("ProjectManual", m_pParent, SLOT(slotProjectManual()), true, ID_PROJECT_MAKE_USER_MANUAL);

  accel->insertItem(i18n("Make Source-tgz"), "Source-tgz", (unsigned int) 0);
  accel->connectItem("Source-tgz", m_pParent, SLOT(slotProjectMakeDistSourceTgz()), true, ID_PROJECT_MAKE_DISTRIBUTION_SOURCE_TGZ );
 	
  accel->insertItem(i18n("Project options"), "ProjectOptions", IDK_PROJECT_OPTIONS);
  accel->connectItem("ProjectOptions", m_pParent, SLOT(slotProjectOptions() ), true, ID_PROJECT_OPTIONS );

  accel->insertItem(i18n("Make tags file"), "MakeTagsfile", ID_PROJECT_MAKE_TAGS);
  accel->connectItem("MakeTagsfile", m_pParent, SLOT(slotProjectMakeTags() ), true, ID_PROJECT_MAKE_TAGS );

  accel->insertItem(i18n("Load tags file"), "LoadTagsfile", ID_PROJECT_LOAD_TAGS);
  accel->connectItem("LoadTagsfile", m_pParent, SLOT(slotProjectLoadTags() ), true, ID_PROJECT_LOAD_TAGS );

  //build menu
  accel->insertItem( i18n("Compile File"), "CompileFile", IDK_BUILD_COMPILE_FILE );
  accel->connectItem( "CompileFile", m_pParent, SLOT( slotBuildCompileFile()), true, ID_BUILD_COMPILE_FILE );

  accel->insertItem( i18n("Make"), "Make", IDK_BUILD_MAKE );
  accel->connectItem( "Make", m_pParent, SLOT(slotBuildMake() ), true, ID_BUILD_MAKE );

  accel->insertItem( i18n("Rebuild All"), "RebuildAll", (unsigned int) 0);
  accel->connectItem( "RebuildAll", m_pParent, SLOT(slotBuildRebuildAll()), true, ID_BUILD_REBUILD_ALL );

  accel->insertItem( i18n("Clean/Rebuild all"), "CleanRebuildAll", (unsigned int) 0);
  accel->connectItem( "CleanRebuildAll", m_pParent, SLOT(slotBuildCleanRebuildAll()), true, ID_BUILD_CLEAN_REBUILD_ALL );

  accel->insertItem( i18n("Stop process"), "Stop_proc", IDK_BUILD_STOP);
  accel->connectItem( "Stop_proc", m_pParent, SLOT(slotBuildStop() ), true, ID_BUILD_STOP );

  accel->insertItem( i18n("Execute"), "Run", IDK_BUILD_RUN);
  accel->connectItem( "Run", m_pParent, SLOT(slotBuildRun() ), true, ID_BUILD_RUN );

  accel->insertItem( i18n("Execute with arguments"), "Run_with_args", IDK_BUILD_RUN_WITH_ARGS);
  accel->connectItem( "Run_with_args", m_pParent, SLOT(slotBuildRunWithArgs() ), true, ID_BUILD_RUN_WITH_ARGS );

  accel->insertItem( i18n("DistClean"), "BuildDistClean", (unsigned int) 0);
  accel->connectItem("BuildDistClean",m_pParent, SLOT(slotBuildDistClean()), true, ID_BUILD_DISTCLEAN );

  accel->insertItem( i18n("Make Clean"), "BuildMakeClean", (unsigned int) 0);
  accel->connectItem("BuildMakeClean",m_pParent, SLOT(slotBuildMakeClean()), true, ID_BUILD_MAKECLEAN );

  accel->insertItem( i18n("Autoconf and automake"), "BuildAutoconf", (unsigned int) 0);
  accel->connectItem("BuildAutoconf", m_pParent,SLOT(slotBuildAutoconf()), true, ID_BUILD_AUTOCONF );

  accel->insertItem( i18n("Configure..."), "BuildConfigure", (unsigned int) 0);
  accel->connectItem( "BuildConfigure", m_pParent, SLOT(slotBuildConfigure()), true, ID_BUILD_CONFIGURE );

  // Bookmarks-menu
  accel->insertItem( i18n("Toggle Bookmark"), "Toggle_Bookmarks", IDK_BOOKMARKS_TOGGLE);
  accel->connectItem( "Toggle_Bookmarks", m_pParent, SLOT(slotBookmarksToggle() ), true, ID_BOOKMARKS_TOGGLE );

  accel->insertItem( i18n("Next Bookmark"), "Next_Bookmarks", IDK_BOOKMARKS_NEXT);
  accel->connectItem( "Next_Bookmarks", m_pParent, SLOT(slotBookmarksNext() ), true, ID_BOOKMARKS_NEXT );

  accel->insertItem( i18n("Previous Bookmark"), "Previous_Bookmarks", IDK_BOOKMARKS_PREVIOUS);
  accel->connectItem( "Previous_Bookmarks", m_pParent, SLOT(slotBookmarksPrevious() ), true, ID_BOOKMARKS_PREVIOUS );

  accel->insertItem( i18n("Clear Bookmarks"), "Clear_Bookmarks", IDK_BOOKMARKS_CLEAR);
  accel->connectItem( "Clear_Bookmarks", m_pParent, SLOT(slotBookmarksClear() ), true, ID_BOOKMARKS_CLEAR );

  //Help menu
  accel->connectItem( KStdAccel::Help , m_pParent, SLOT(slotHelpContents()), true, ID_HELP_CONTENTS );

  accel->insertItem( i18n("Search Marked Text"), "SearchMarkedText",IDK_HELP_SEARCH_TEXT);
  accel->connectItem( "SearchMarkedText", m_pParent, SLOT(slotHelpSearchText() ), true, ID_HELP_SEARCH_TEXT );

  accel->insertItem( i18n("Search for Help on"), "HelpSearch", (unsigned int) 0);
  accel->connectItem( "HelpSearch", m_pParent, SLOT(slotHelpSearch()), true, ID_HELP_SEARCH );

  accel->insertItem( i18n("View Project API-Doc"), "HelpProjectAPI", (unsigned int) 0);
  accel->connectItem("HelpProjectAPI", m_pParent, SLOT(slotHelpAPI()), true, ID_HELP_PROJECT_API);

  accel->insertItem( i18n("View Project User-Manual"), "HelpProjectManual", (unsigned int) 0);
  accel->connectItem( "HelpProjectManual", m_pParent, SLOT(slotHelpManual()), true, ID_HELP_USER_MANUAL);   // Tab-Switch

  // Debugger startups
  accel->insertItem( i18n("Debug start"), "DebugStart", (unsigned int) 0);
  accel->connectItem( "DebugStart", m_pParent, SLOT(slotBuildDebugStart()), true, ID_DEBUG_START);

  accel->insertItem( i18n("Debug start other"), "DebugStartOther", (unsigned int) 0);
  accel->connectItem( "DebugStartOther", m_pParent, SLOT(slotDebugNamedFile()), true, ID_DEBUG_START_OTHER);

  accel->insertItem( i18n("Debug start with args"), "DebugRunWithArgs", (unsigned int) 0);
  accel->connectItem( "DebugRunWithArgs", m_pParent, SLOT(slotDebugRunWithArgs()), true, ID_DEBUG_SET_ARGS);

  accel->insertItem( i18n("Debug examine core"), "DebugExamineCore", (unsigned int) 0);
  accel->connectItem( "DebugExamineCore", m_pParent, SLOT(slotDebugExamineCore()), true, ID_DEBUG_CORE);

  accel->insertItem( i18n("Debug other executable"), "DebugOtherExec", (unsigned int) 0);
  accel->connectItem( "DebugOtherExec", m_pParent, SLOT(slotDebugNamedFile()), true, ID_DEBUG_NAMED_FILE);

  accel->insertItem( i18n("Debug attach"), "DebugAttach", (unsigned int) 0);
  accel->connectItem( "DebugAttach", m_pParent, SLOT(slotDebugAttach()), true, ID_DEBUG_ATTACH);

  // Debugger actions
  accel->insertItem( i18n("Debug run"), "DebugRun", (unsigned int) 0);
  accel->connectItem( "DebugRun", m_pParent, SLOT(slotDebugRun()), true, ID_DEBUG_RUN );

  accel->insertItem( i18n("Debug run to cursor"), "DebugRunCursor", (unsigned int) 0);
  accel->connectItem( "DebugRunCursor", m_pParent, SLOT(slotDebugRunToCursor()), true, ID_DEBUG_RUN_CURSOR );

  accel->insertItem( i18n("Debug stop"), "DebugStop", (unsigned int) 0);
  accel->connectItem( "DebugStop", m_pParent, SLOT(slotDebugStop()), true, ID_DEBUG_STOP);

  accel->insertItem( i18n("Debug step into"), "DebugStepInto", (unsigned int) 0);
  accel->connectItem( "DebugStepInto", m_pParent, SLOT(slotDebugStepInto()), true, ID_DEBUG_STEP);

  accel->insertItem( i18n("Debug step into instr"), "DebugStepIntoInstr", (unsigned int) 0);
  accel->connectItem( "DebugStepIntoInstr", m_pParent, SLOT(slotDebugStepIntoIns()), true, ID_DEBUG_STEP_INST);

  accel->insertItem( i18n("Debug step over"), "DebugStepOver", (unsigned int) 0);
  accel->connectItem( "DebugStepOver", m_pParent, SLOT(slotDebugStepOver()), true, ID_DEBUG_NEXT);

  accel->insertItem( i18n("Debug step over instr"), "DebugStepOverInstr", (unsigned int) 0);
  accel->connectItem( "DebugStepOverInstr", m_pParent, SLOT(slotDebugStepOverIns()), true, ID_DEBUG_NEXT_INST);

  accel->insertItem( i18n("Debug step out"), "DebugStepOut", (unsigned int) 0);
  accel->connectItem( "DebugStepOut", m_pParent, SLOT(slotDebugStepOutOff()), true, ID_DEBUG_FINISH);

  accel->insertItem( i18n("Debug viewers"), "DebugViewer", (unsigned int) 0);
  accel->connectItem( "DebugViewer", m_pParent, SLOT(slotDebugMemoryView()), true, ID_DEBUG_MEMVIEW);

  accel->insertItem( i18n("Debug interrupt"), "DebugInterrupt", (unsigned int) 0);
  accel->connectItem( "DebugInterrupt", m_pParent, SLOT(slotDebugInterrupt()), true, ID_DEBUG_BREAK_INTO);

  accel->insertItem( i18n("Debug toggle breakpoint"), "DebugToggleBreakpoint", (unsigned int) 0);
  accel->connectItem( "DebugToggleBreakpoint", m_pParent, SLOT(slotDebugToggleBreakpoint()), true, ID_DEBUG_TOGGLE_BP);

  accel->readSettings(0, false);
}

#include "docviewman.moc"
