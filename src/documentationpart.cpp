#include <stdlib.h>
#include <qfile.h>

#include <kaction.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <kiconloader.h>

#include "partcontroller.h"


#include "documentationpart.h"


DocumentationPart::DocumentationPart()
  : KHTMLPart(0L, 0L, 0L, "DocumentationPart")
{
  connect(browserExtension(), SIGNAL(openURLRequestDelayed(const KURL &,const KParts::URLArgs &)),
          this, SLOT(openURLRequest(const KURL &)) );

  connect(this, SIGNAL(started(KIO::Job *)), this, SLOT(slotStarted(KIO::Job* )));
  connect(this, SIGNAL(completed()), this, SLOT(slotCompleted()));
  connect(this, SIGNAL(canceled(const QString &)), this, SLOT(slotCancelled(const QString &)));

  KActionCollection * actions = new KActionCollection( this );
  reloadAction = new KAction( i18n( "Reload" ), "reload", 0,
    this, SLOT( slotReload() ), actions, "doc_reload" );
  reloadAction->setWhatsThis(i18n("<b>Reload</b><p>Reloads the current document."));
  stopAction = new KAction( i18n( "Stop" ), "stop", 0,
    this, SLOT( slotStop() ), actions, "doc_stop" );
  stopAction->setWhatsThis(i18n("<b>Stop</b><p>Stops the loading of current document."));
  duplicateAction = new KAction( i18n( "Duplicate Window" ), "window_new", 0,
    this, SLOT( slotDuplicate() ), actions, "doc_dup" );
  duplicateAction->setWhatsThis(i18n("<b>Duplicate window</b><p>Opens current document in a new window."));

  connect( this, SIGNAL(popupMenu(const QString &, const QPoint &)), this, SLOT(popup(const QString &, const QPoint &)));
}

void DocumentationPart::popup( const QString & url, const QPoint & p )
{
  KPopupMenu *m_popup = new KPopupMenu( i18n( "Documentation Viewer" ), this->widget() );

  duplicateAction->plug(m_popup);
  int idNewWindow = -2;
  if (!url.isEmpty())
  {
    idNewWindow = m_popup->insertItem(SmallIcon("window_new"),i18n("Open in New Window"));
    m_popup->setWhatsThis(idNewWindow, i18n("<b>Open in new window</b><p>Opens current link in a new window."));
  }
  m_popup->insertSeparator();
  reloadAction->plug(m_popup);
  stopAction->plug(m_popup);
  m_popup->insertSeparator();

/*  if (!url.isEmpty())
  {
    KAction *ac = action("savelinkas");
    if (ac)
    {
        qWarning("savelinkas found");
        ac->plug(m_popup);
    }
    KAction *ac2 = action("copylinklocation");
    if (ac2)
        ac2->plug(m_popup);
    m_popup->insertSeparator();
  }*/

  KAction *ac = action("setEncoding");
  if (ac)
    ac->plug(m_popup);

  int r = m_popup->exec(p);

  if (r == idNewWindow)
  {
    KURL kurl (DocumentationPart::url().upURL());
    kurl.addPath(url);
    if (kurl.isValid())
        PartController::getInstance()->showDocument(kurl, url);
  }
}

void DocumentationPart::setContext(const QString &context)
{
  m_context = context;
}


QString DocumentationPart::context() const
{
  return m_context;
}


// Note: this function is a copy of code in kdecore/kconfigbase.cpp ;)
static bool isUtf8(const char *buf) {
  int i, n;
  register unsigned char c;
  bool gotone = false;

#define F 0   /* character never appears in text */
#define T 1   /* character appears in plain ASCII text */
#define I 2   /* character appears in ISO-8859 text */
#define X 3   /* character appears in non-ISO extended ASCII (Mac, IBM PC) */

  static const unsigned char text_chars[256] = {
  /*                  BEL BS HT LF    FF CR    */
        F, F, F, F, F, F, F, T, T, T, T, F, T, T, F, F,  /* 0x0X */
        /*                              ESC          */
        F, F, F, F, F, F, F, F, F, F, F, T, F, F, F, F,  /* 0x1X */
        T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x2X */
        T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x3X */
        T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x4X */
        T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x5X */
        T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x6X */
        T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, F,  /* 0x7X */
        /*            NEL                            */
        X, X, X, X, X, T, X, X, X, X, X, X, X, X, X, X,  /* 0x8X */
        X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,  /* 0x9X */
        I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xaX */
        I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xbX */
        I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xcX */
        I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xdX */
        I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xeX */
        I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I   /* 0xfX */
  };

  /* *ulen = 0; */
  for (i = 0; (c = buf[i]); i++) {
    if ((c & 0x80) == 0) {        /* 0xxxxxxx is plain ASCII */
      /*
       * Even if the whole file is valid UTF-8 sequences,
       * still reject it if it uses weird control characters.
       */

      if (text_chars[c] != T)
        return false;

    } else if ((c & 0x40) == 0) { /* 10xxxxxx never 1st byte */
      return false;
    } else {                           /* 11xxxxxx begins UTF-8 */
      int following;

    if ((c & 0x20) == 0) {             /* 110xxxxx */
      following = 1;
    } else if ((c & 0x10) == 0) {      /* 1110xxxx */
      following = 2;
    } else if ((c & 0x08) == 0) {      /* 11110xxx */
      following = 3;
    } else if ((c & 0x04) == 0) {      /* 111110xx */
      following = 4;
    } else if ((c & 0x02) == 0) {      /* 1111110x */
      following = 5;
    } else
      return false;

      for (n = 0; n < following; n++) {
        i++;
        if (!(c = buf[i]))
          goto done;

        if ((c & 0x80) == 0 || (c & 0x40))
          return false;
      }
      gotone = true;
    }
  }
done:
  return gotone;   /* don't claim it's UTF-8 if it's all 7-bit */
}
#undef F
#undef T
#undef I
#undef X

QString DocumentationPart::resolveEnvVarsInURL(const QString& url)
{
  // check for environment variables and make necessary translations
  QString path = url;
  int nDollarPos = path.find( '$' );

  // Note: the while loop below is a copy of code in kdecore/kconfigbase.cpp ;)
  while( nDollarPos != -1 && nDollarPos+1 < static_cast<int>(path.length())) {
    // there is at least one $
    if( (path)[nDollarPos+1] == '(' ) {
      uint nEndPos = nDollarPos+1;
      // the next character is no $
      while ( (nEndPos <= path.length()) && (path[nEndPos]!=')') )
          nEndPos++;
      nEndPos++;
      QString cmd = path.mid( nDollarPos+2, nEndPos-nDollarPos-3 );

      QString result;
      FILE *fs = popen(QFile::encodeName(cmd).data(), "r");
      if (fs)
      {
         QTextStream ts(fs, IO_ReadOnly);
         result = ts.read().stripWhiteSpace();
         pclose(fs);
      }
      path.replace( nDollarPos, nEndPos-nDollarPos, result );
    } else if( (path)[nDollarPos+1] != '$' ) {
      uint nEndPos = nDollarPos+1;
      // the next character is no $
      QString aVarName;
      if (path[nEndPos]=='{')
      {
        while ( (nEndPos <= path.length()) && (path[nEndPos]!='}') )
            nEndPos++;
        nEndPos++;
        aVarName = path.mid( nDollarPos+2, nEndPos-nDollarPos-3 );
      }
      else
      {
        while ( nEndPos <= path.length() && (path[nEndPos].isNumber()
                || path[nEndPos].isLetter() || path[nEndPos]=='_' )  )
            nEndPos++;
        aVarName = path.mid( nDollarPos+1, nEndPos-nDollarPos-1 );
      }
      const char* pEnv = 0;
      if (!aVarName.isEmpty())
           pEnv = getenv( aVarName.ascii() );
      if( pEnv ) {
        // !!! Sergey A. Sukiyazov <corwin@micom.don.ru> !!!
        // A environment variables may contain values in 8bit
        // locale cpecified encoding or in UTF8 encoding.
        if (isUtf8( pEnv ))
            path.replace( nDollarPos, nEndPos-nDollarPos, QString::fromUtf8(pEnv) );
        else
            path.replace( nDollarPos, nEndPos-nDollarPos, QString::fromLocal8Bit(pEnv) );
      } else
      path.remove( nDollarPos, nEndPos-nDollarPos );
    } else {
      // remove one of the dollar signs
      path.remove( nDollarPos, 1 );
      nDollarPos++;
    }
    nDollarPos = path.find( '$', nDollarPos );
  }

  return path;
}

bool DocumentationPart::openURL(const KURL &url)
{
  QString path = resolveEnvVarsInURL(url.url());
  KURL newUrl(path);
  
  bool retval = KHTMLPart::openURL(newUrl);
  if ( retval )
    emit fileNameChanged();

  return retval;
}

void DocumentationPart::openURLRequest(const KURL &url)
{
  PartController::getInstance()->showDocument(url, context());
}

void DocumentationPart::slotReload( )
{
    PartController::getInstance()->showDocument(url(), m_context);
}

void DocumentationPart::slotStop( )
{
    closeURL();
}

void DocumentationPart::slotStarted( KIO::Job * )
{
    stopAction->setEnabled(true);
}

void DocumentationPart::slotCompleted( )
{
    stopAction->setEnabled(false);
}

void DocumentationPart::slotCancelled( const QString & /*errMsg*/ )
{
    stopAction->setEnabled(false);
}

void DocumentationPart::slotDuplicate( )
{
    PartController::getInstance()->showDocument(url(), m_context + "dup");
}

#include "documentationpart.moc"
