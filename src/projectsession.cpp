/***************************************************************************
                          projectsession.cpp  -  description
                             -------------------
    begin                : 30 Nov 2002
    copyright            : (C) 2002 by Falk Brettschneider
    email                : falk@kdevelop.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qdom.h>
#include <qptrlist.h>
#include <qfile.h>

#include <kparts/part.h>
#include <kurl.h>
#include <kmessagebox.h>
#include <klocale.h>
#include "ktexteditor/viewcursorinterface.h"
#include "ktexteditor/document.h"

#include "api.h"
#include "partcontroller.h"
#include "domutil.h"
#include "documentationpart.h"
#include "toplevel.h"
#include "kdevplugin.h"

#include "projectsession.h"
#include "projectsession.moc"
//---------------------------------------------------------------------------
ProjectSession::ProjectSession()
{
  initXMLTree();
}

//---------------------------------------------------------------------------
ProjectSession::~ProjectSession()
{
}

//---------------------------------------------------------------------------
void ProjectSession::initXMLTree()
{
  // initializes the XML tree on startup of kdevelop and when a project
  // has been closed to ensure that the XML tree exists already including
  // doctype when a project gets opened that doesn't have a kdevses file
  // or a new project gets generated (which doesn't have a kdevses file
  // either as the project has never been closed before opening it).
  domdoc.clear();
  QDomDocument doc("KDevPrjSession");
  domdoc=doc;
  domdoc.appendChild( domdoc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );
  // KDevPrjSession is the root element of the XML file
  QDomElement session = domdoc.documentElement();
  session = domdoc.createElement("KDevPrjSession");
  domdoc.appendChild( session);
}

//---------------------------------------------------------------------------
bool ProjectSession::restoreFromFile(const QString& sessionFileName, const QDict<KDevPlugin>& projectPlugins)
{
  bool bFileOpenOK = true;

  QFile f(sessionFileName);
  if ( f.open(IO_ReadOnly) ) {  // file opened successfully
    bool ok = domdoc.setContent( &f);
    f.close();
    if (!ok) {
      KMessageBox::sorry(0L,
                         i18n("The file %1 does not contain valid XML.\n"
                         "The loading of the session failed.").arg(sessionFileName));
      initXMLTree(); // because it was now broken after failed setContent()
      return false;
    }
  }
  else {
    bFileOpenOK = false;
  }

  // Check for proper document type.
  if (domdoc.doctype().name() != "KDevPrjSession") {
    KMessageBox::sorry(0L,
    i18n("The file %1 does not contain a valid KDevelop project session ('KDevPrjSession').\n").arg(sessionFileName)
    + i18n("The document type seems to be: '%1'.").arg(domdoc.doctype().name()));
    return false;
  }

  QDomElement session = domdoc.documentElement();

  // read the information about the mainframe widget
  if (bFileOpenOK) {
    recreateDocs(session);
  }

  // now also let the project-related plugins load their session stuff
  QDomElement pluginListEl = session.namedItem("pluginList").toElement();
  QDictIterator<KDevPlugin> it(projectPlugins);
  for ( ; it.current(); ++it) {
    KDevPlugin* pPlugin = it.current();
    Q_ASSERT(pPlugin->instance());
    QString pluginName = pPlugin->instance()->instanceName();
    QDomElement pluginEl = pluginListEl.namedItem(pluginName).toElement();
    if (!pluginEl.isNull()) {
      // now plugin, load what you find!
      pPlugin->restorePartialProjectSession(&pluginEl);
    }
  }

  return true;
}

//---------------------------------------------------------------------------
void ProjectSession::recreateDocs(QDomElement& el)
{
////  QDomElement mainframeEl = el.namedItem("Mainframe").toElement();
////  bool bMaxMode =initXMLTree() (bool) mainframeEl.attribute("MaximizeMode", "0").toInt();
////  QextMdiMainFrm* pMainWidget = (QextMdiMainFrm*) qApp->mainWidget();
////  pMainWidget->setEnableMaximizedChildFrmMode(bMaxMode);
////  bool bTaskBarWasOn = pMainWidget->isViewTaskBarOn();
////  pMainWidget->hideViewTaskBar();

  // read the information about the documents
  QDomElement docsAndViewsEl = el.namedItem("DocsAndViews").toElement();
  int nNrOfDocs = docsAndViewsEl.attribute("NumberOfDocuments", "0").toInt();
  // loop over all docs
  int nDoc = 0;
  QDomElement docEl;
  for (docEl = docsAndViewsEl.firstChild().toElement(), nDoc = 0;
       nDoc < nNrOfDocs;
       nDoc++, docEl = docEl.nextSibling().toElement())
  {
    // read the document name and type
    QString docName = docEl.attribute( "URL", "");
    if (!docName.isEmpty() /* && URL::exists(docName)*/) {
      KURL url(docName);
      // create the views of this document, the first view creation will also create the document
      recreateViews(url, docEl);
    }
  }

  if (nNrOfDocs > 0) {
      API::getInstance()->mainWindow()->callCommand("qextmdi-UI: do hack on session loading finished");
  }
////  if (bTaskBarWasOn) {
////     pMainWidget->showViewTaskBar();
////  }
}

//---------------------------------------------------------------------------
void ProjectSession::recreateViews(KURL& url, QDomElement docEl)
{
  // read information about the views
  int nNrOfViews = docEl.attribute( "NumberOfViews", "0").toInt();
  // loop over all views of this document
  int nView = 0;

  QDomElement viewEl;
  QString viewType;
  QString context;
  if (docEl.hasAttribute("context")) {
    context = docEl.attribute("context");
  }

  for (viewEl = docEl.firstChild().toElement(), nView = 0; nView < nNrOfViews; nView++, viewEl = viewEl.nextSibling().toElement()) {

////    // is it the focused view? (XXX well, this only refers to the module instance)
////    if (viewEl.attribute( "Focus", "0").toInt()) {
////      // yes, memorize for later use
////      pFocusedView = pView;
////    }

    if (context.isEmpty()) {
      int line = 0;
      if (viewEl.hasAttribute("line")) {
        line = viewEl.attribute("line", "0").toInt();
      }
      PartController::getInstance()->editDocument(url, line);
    }
    else {
      PartController::getInstance()->showDocument(url, context);
    }
    QDomElement viewPropertiesEl = viewEl.namedItem("AdditionalSettings").toElement();
    if (!viewPropertiesEl.isNull()) {
      emit sig_restoreAdditionalViewProperties(url.url(), &viewPropertiesEl);
    }

  }
////  // restore focus
////  if (pFocusedView != 0L) {
////    if (pFocusedView->parentWidget()->inherits("QextMdiChildView")) {
////        ((QextMdiChildView*)pFocusedView->parentWidget())->activate();
////    }
////    pFocusedView->setFocus();
////  }

}

//---------------------------------------------------------------------------
bool ProjectSession::saveToFile(const QString& sessionFileName, const QDict<KDevPlugin>& projectPlugins)
{

  QString section, keyword;
  QDomElement session = domdoc.documentElement();


  int nDocs = 0;
  QString docIdStr;

////  // read the information about the mainframe widget
////  QDomElement mainframeEl = session.namedItem("Mainframe").toElement();
////  if(mainframeEl.isNull()){
////    mainframeEl=domdoc.createElement("Mainframe");
////    session.appendChild( mainframeEl);
////  }
////  bool bMaxMode = ((QextMdiMainFrm*)m_pDocViewMan->parent())->isInMaximizedChildFrmMode();
////  mainframeEl.setAttribute("MaximizeMode", bMaxMode);


  // read the information about the documents
  QDomElement docsAndViewsEl = session.namedItem("DocsAndViews").toElement();
  if (docsAndViewsEl.isNull()) {
    docsAndViewsEl = domdoc.createElement("DocsAndViews");
    session.appendChild( docsAndViewsEl);
  }
  else {
    // we need to remove the old ones before memorizing the current ones (to avoid merging)
    QDomNode n = docsAndViewsEl.firstChild();
    while ( !n.isNull() ) {
      QDomNode toBeRemoved = n;
      n = n.nextSibling();
      docsAndViewsEl.removeChild(toBeRemoved);
    }
  }

  QPtrListIterator<KParts::Part> it( *PartController::getInstance()->parts() );
  for ( ; it.current(); ++it ) {
////    QString partName = it.current()->name();
////    QMessageBox::information(0L,"",partName);

    KParts::ReadOnlyPart* pReadOnlyPart = dynamic_cast<KParts::ReadOnlyPart*>(it.current());
    if (!pReadOnlyPart)
      continue; // note: read-write parts are also a read-only part, they inherit from it

    DocumentationPart* pDocuPart = dynamic_cast<DocumentationPart*>(pReadOnlyPart);

    /// @todo Save relative path for project sharing?
    QString url = pReadOnlyPart->url().url();

    docIdStr.setNum(nDocs);
    QDomElement docEl = domdoc.createElement("Doc" + docIdStr);
    docEl.setAttribute( "URL", url);
    docsAndViewsEl.appendChild( docEl);
    nDocs++;
////    docEl.setAttribute( "Type", "???");
////    // get the view list
////    QPtrList<KWpEditorPartriteView> viewList = pDoc->viewList();
////    // write the number of views
////    docEl.setAttribute( "NumberOfViews", viewList.count());
    docEl.setAttribute( "NumberOfViews", 1);
    // loop over all views of this document
    int nView = 0;
////    KWriteView* pView = 0L;
    QString viewIdStr;
////    for (viewList.first(), nView = 0; viewList.current() != 0; viewList.next(), nView++) {
////      pView = viewList.current();
////      if (pView != 0L) {
        viewIdStr.setNum( nView);
        QDomElement viewEl = domdoc.createElement( "View"+viewIdStr);
        docEl.appendChild( viewEl);
        // focus?
////        viewEl.setAttribute("Focus", (((CEditWidget*)pView->parentWidget()) == m_pDocViewMan->currentEditView()));
        viewEl.setAttribute("Type", "???");

    QDomElement viewPropertiesEl = domdoc.createElement("AdditionalSettings");
    viewEl.appendChild(viewPropertiesEl);
    emit sig_saveAdditionalViewProperties(url, &viewPropertiesEl);

    if (pReadOnlyPart->inherits("KTextEditor::Document")) {
      KTextEditor::ViewCursorInterface *iface = dynamic_cast<KTextEditor::ViewCursorInterface*>(pReadOnlyPart->widget());
      if (iface) {
        unsigned int line, col;
        iface->cursorPosition(&line, &col);
        viewEl.setAttribute( "line", line );
      }
    }

    if (pDocuPart) {
      docEl.setAttribute( "context", pDocuPart->context() );
    }
  }

  docsAndViewsEl.setAttribute("NumberOfDocuments", nDocs);


  // now also let the project-related plugins save their session stuff
  // read the information about the documents
  QDomElement pluginListEl = session.namedItem("pluginList").toElement();
  if (pluginListEl.isNull()) {
    pluginListEl = domdoc.createElement("pluginList");
    session.appendChild( pluginListEl);
  }
  else {
    // we need to remove the old ones before memorizing the current ones (to avoid merging)
    QDomNode n = pluginListEl.firstChild();
    while ( !n.isNull() ) {
      QDomNode toBeRemoved = n;
      n = n.nextSibling();
      pluginListEl.removeChild(toBeRemoved);
    }
  }

  QDictIterator<KDevPlugin> pluginIter(projectPlugins);
  for ( ; pluginIter.current(); ++pluginIter) {
    KDevPlugin* pPlugin = pluginIter.current();
    Q_ASSERT(pPlugin->instance());
    QString pluginName = pPlugin->instance()->instanceName();
    QDomElement pluginEl = domdoc.createElement(pluginName);
    // now plugin, save what you have!
    pPlugin->savePartialProjectSession(&pluginEl);
    // if the plugin wrote anything, accept it for the session, otherwise forget it
    if (pluginEl.hasChildNodes() || pluginEl.hasAttributes()) {
      pluginListEl.appendChild(pluginEl);
    }
  }

  // Write it out to the session file on disc
  QFile f(sessionFileName);
  if ( f.open(IO_WriteOnly) ) {    // file opened successfully
    QTextStream t( &f );        // use a text stream
    t << domdoc.toCString();
    f.close();
  }
  initXMLTree();  // clear and initialize the tree again

  return true;
}
