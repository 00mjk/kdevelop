/***************************************************************************
 *   Copyright (C) 2001 by Harald Fernengel                                *
 *   harry@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qlayout.h>
#include <qtextedit.h>

#include <klocale.h>
#include <kservice.h>
#include <ktempfile.h>
 
#include <kparts/componentfactory.h>
#include <kparts/part.h>

#include <kio/jobclasses.h>
#include <kio/job.h>

#include "diffwidget.h"

DiffWidget::DiffWidget( QWidget *parent, const char *name, WFlags f ):
    QWidget( parent, name, f ), tempFile( 0 )
{
  job = 0;
  komparePart = 0;

  loadKomparePart( this );

  te = new QTextEdit( this, "Main Text Edit" );
  te->setReadOnly( true );
  te->setMinimumSize( 300, 200 );

  QVBoxLayout* layout = new QVBoxLayout( this );
  layout->addWidget( te );

  if ( komparePart ) {
    // if compare is installed, we take it instead of the QTextEdit
    te->hide();
    layout->addWidget( komparePart->widget() );
  }
}

DiffWidget::~DiffWidget()
{
}

void DiffWidget::setKompareVisible( bool visible )
{
  if ( !komparePart || !komparePart->widget() ) {
    te->show();
    return;
  }
  if ( visible ) {
    te->hide();
    komparePart->widget()->show();
  } else {
    te->show();
    komparePart->widget()->hide();
  }
}

void DiffWidget::loadKomparePart( QWidget* parent )
{
  if ( komparePart )
    return;

  // ### might be easier to use:
  // createPartInstanceFromQuery( "text/x-diff", QString::null, parent, 0, this, 0 ); (Simon)

  KService::Ptr kompareService = KService::serviceByDesktopName( "komparepart" );
  if ( !kompareService )
    return;

  komparePart = KParts::ComponentFactory::createPartInstanceFromService<KParts::ReadOnlyPart>( kompareService, parent, 0, this, 0 );
}

void DiffWidget::slotClear()
{
  te->clear();
  if ( komparePart )
    komparePart->closeURL();
}

// internally for the TextEdit only!
void DiffWidget::slotAppend( const QString& str )
{
  te->append( str );
}

// internally for the TextEdit only!
void DiffWidget::slotAppend( KIO::Job*, const QByteArray& ba )
{
  slotAppend( QString( ba ) );
}

// internally for the TextEdit only!
void DiffWidget::slotFinished()
{
  // the diff has been loaded so we apply a simple highlighting

  static QColor cAdded( 190, 190, 237);
  static QColor cRemoved( 190, 237, 190 );
  int paragCount = te->paragraphs();

  for ( int i = 0; i < paragCount; ++i ) {
    QString txt = te->text( i );
    if ( txt.length() > 0 ) {
      if ( txt.startsWith( "+" ) || txt.startsWith( ">" ) ) {
        te->setParagraphBackgroundColor( i, cAdded );
      } else if ( txt.startsWith( "-" ) || txt.startsWith( "<" ) ) {
        te->setParagraphBackgroundColor( i, cRemoved );
      }
    }
  }
}

void DiffWidget::setDiff( const QString& diff )
{
  if ( komparePart ) {
    bool ok = false;
    if ( komparePart->openStream( "text/plain", KURL() ) ) {
      komparePart->writeStream( diff.local8Bit() );
      ok = komparePart->closeStream();
    } else {
      // workaround for old kompare versions < KDE 3.2
      delete tempFile;
      tempFile = new KTempFile();
      tempFile->setAutoDelete( true );
      *(tempFile->textStream()) << diff;
      tempFile->close();
      ok = komparePart->openURL( tempFile->name() );
    }
    if ( ok ) {
      setKompareVisible( true );
      return;
    } else {
      setKompareVisible( false );
      te->setText( i18n("*** Error viewing diff with the diff KPart, falling back to plainText ***") );
    }
  }

  slotAppend( diff );
  slotFinished();
}

void DiffWidget::openURL( const KURL& url )
{
  if ( komparePart ) {
    if ( komparePart->openURL( url ) ) {
      setKompareVisible( true );
    } else {
      setKompareVisible( false );
      te->setText( i18n("<b>Error viewing diff with the diff KPart</b>") );
    }
    return;
  }

  if ( job )
    job->kill();

  KIO::TransferJob* job = KIO::get( url );
  if ( !job )
    return;

  connect( job, SIGNAL(data( KIO::Job *, const QByteArray & )),
           this, SLOT(slotAppend( KIO::Job*, const QByteArray& )) );
  connect( job, SIGNAL(result( KIO::Job * )),
           this, SLOT(slotFinished()) );  
}

#include "diffwidget.moc"
