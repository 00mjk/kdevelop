/***************************************************************************
 *   Copyright 2007 Dukju Ahn <dukjuahn@gmail.com>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "svnssldialog.h"

#include <QPushButton>
#include <QTreeWidgetItem>

#include <KLocalizedString>

#include "ui_ssltrustdialog.h"

class SvnSSLTrustDialogPrivate
{
public:
    Ui::SvnSSLTrustDialog ui;
    bool temporarily;
};

SvnSSLTrustDialog::SvnSSLTrustDialog( QWidget *parent )
    : QDialog( parent ), d( new SvnSSLTrustDialogPrivate )
{
    d->ui.setupUi( this );
    d->temporarily = true;
    setWindowTitle( i18n( "Ssl Server Certificate" ) );
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
    buttonBox->addButton(i18n("Trust Permanently"), QDialogButtonBox::YesRole);
    buttonBox->addButton(i18n("Trust Temporarily"), QDialogButtonBox::AcceptRole)->setDefault(true);
    auto layout = new QVBoxLayout();
    setLayout(layout);
    layout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::clicked, this, &SvnSSLTrustDialog::buttonClicked);
}
SvnSSLTrustDialog::~SvnSSLTrustDialog()
{
    delete d;
}

void SvnSSLTrustDialog::setCertInfos( const QString& hostname,
                                      const QString& fingerPrint,
                                      const QString& validfrom,
                                      const QString& validuntil,
                                      const QString& issuerName,
                                      const QString& realm,
                                      const QStringList& failures )
{
    QString txt = "<ul>";
    foreach( const QString &fail, failures )
    {
        txt += "<li>"+fail+"</li>";
    }
    d->ui.reasons->setHtml( txt );

    d->ui.hostname->setText( hostname );
    d->ui.fingerprint->setText( fingerPrint );
    d->ui.validUntil->setText( validuntil );
    d->ui.validFrom->setText( validfrom );
    d->ui.issuer->setText( issuerName );
    setWindowTitle( i18n( "Ssl Server Certificate: %1", realm ) );

}

bool SvnSSLTrustDialog::useTemporarily()
{
    return d->temporarily;
}

void SvnSSLTrustDialog::buttonClicked(QAbstractButton *button)
{
    if (buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole)
    {
        d->temporarily = true;
    } else
    {
        d->temporarily = false;
    }
    accept();
}
